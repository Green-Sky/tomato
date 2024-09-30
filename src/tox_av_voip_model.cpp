#include "./tox_av_voip_model.hpp"

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/tox_contacts/components.hpp>

#include "./frame_streams/stream_manager.hpp"
#include "./frame_streams/audio_stream2.hpp"
#include "./frame_streams/locked_frame_stream.hpp"
#include "./frame_streams/multi_source.hpp"

#include <iostream>

namespace Contact::Components {
	// session instead???
	struct ToxAVCall {
		bool incoming_audio {false};
		bool incoming_video {false};
	};
}

namespace Components {
	struct ToxAVAudioSink {
		ObjectHandle o;
		// ptr?
	};
	// vid
	struct ToxAVAudioSource {
		ObjectHandle o;
		// ptr?
	};
	// vid
} // Components

// TODO: make proper adapter
struct AudioStreamReFramer {
	FrameStream2I<AudioFrame2>* _stream {nullptr};
	uint32_t frame_length_ms {20};

	// dequeue?
	std::vector<int16_t> buffer;

	uint32_t sample_rate {48'000};
	size_t channels {0};

	std::optional<AudioFrame2> pop(void) {
		assert(_stream != nullptr);

		auto new_in = _stream->pop();
		if (new_in.has_value()) {
			auto& new_value = new_in.value();

			// changed
			if (sample_rate != new_value.sample_rate || channels != new_value.channels) {
				if (channels != 0) {
					std::cerr << "ReFramer warning: reconfiguring, dropping buffer\n";
				}
				sample_rate = new_value.sample_rate;
				channels = new_value.channels;

				// buffer does not exist or config changed and we discard
				buffer = {};
			}


			//std::cout << "new incoming frame is " << new_value.getSpan().size/new_value.channels*1000/new_value.sample_rate << "ms\n";

			auto new_span = new_value.getSpan();

			if (buffer.empty()) {
				buffer = {new_span.cbegin(), new_span.cend()};
			} else {
				buffer.insert(buffer.cend(), new_span.cbegin(), new_span.cend());
			}
		} else if (buffer.empty()) {
			// first pop might result in invalid state
			return std::nullopt;
		}

		const size_t desired_size {frame_length_ms * sample_rate * channels / 1000};

		// > threshold?
		if (buffer.size() < desired_size) {
			return std::nullopt;
		}

		// copy data
		std::vector<int16_t> return_buffer(buffer.cbegin(), buffer.cbegin()+desired_size);

		// now crop buffer (meh)
		// move data from back to front
		buffer.erase(buffer.cbegin(), buffer.cbegin() + desired_size);

		return AudioFrame2{
			sample_rate,
			channels,
			std::move(return_buffer),
		};
	}
};

struct ToxAVCallAudioSink : public FrameStream2SinkI<AudioFrame2> {
	ToxAVI& _toxav;

	// bitrate for enabled state
	uint32_t _audio_bitrate {32};

	uint32_t _fid;
	std::shared_ptr<LockedFrameStream2<AudioFrame2>> _writer;

	ToxAVCallAudioSink(ToxAVI& toxav, uint32_t fid) : _toxav(toxav), _fid(fid) {}
	~ToxAVCallAudioSink(void) {
		if (_writer) {
			_writer = nullptr;
			_toxav.toxavAudioSetBitRate(_fid, 0);
		}
	}

	// sink
	std::shared_ptr<FrameStream2I<AudioFrame2>> subscribe(void) override {
		if (_writer) {
			// max 1 (exclusive for now)
			return nullptr;
		}

		auto err = _toxav.toxavAudioSetBitRate(_fid, _audio_bitrate);
		if (err != TOXAV_ERR_BIT_RATE_SET_OK) {
			return nullptr;
		}

		_writer = std::make_shared<LockedFrameStream2<AudioFrame2>>();

		return _writer;
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame2>>& sub) override {
		if (!sub || !_writer) {
			// nah
			return false;
		}

		if (sub == _writer) {
			_writer = nullptr;

			/*auto err = */_toxav.toxavAudioSetBitRate(_fid, 0);
			// print warning? on error?

			return true;
		}

		// what
		return false;
	}
};

ToxAVVoIPModel::ToxAVVoIPModel(ObjectStore2& os, ToxAVI& av, Contact3Registry& cr, ToxContactModel2& tcm) :
	_os(os), _av(av), _cr(cr), _tcm(tcm)
{
	_av.subscribe(this, ToxAV_Event::friend_call);
	_av.subscribe(this, ToxAV_Event::friend_call_state);
	_av.subscribe(this, ToxAV_Event::friend_audio_bitrate);
	_av.subscribe(this, ToxAV_Event::friend_video_bitrate);
	_av.subscribe(this, ToxAV_Event::friend_audio_frame);
	_av.subscribe(this, ToxAV_Event::friend_video_frame);

	// attach to all tox friend contacts

	for (const auto& [cv, _] : _cr.view<Contact::Components::ToxFriendPersistent>().each()) {
		_cr.emplace<VoIPModelI*>(cv, this);
	}
	// TODO: events
}

ToxAVVoIPModel::~ToxAVVoIPModel(void) {
	for (const auto& [ov, voipmodel] : _os.registry().view<VoIPModelI*>().each()) {
		if (voipmodel == this) {
			destroySession(_os.objectHandle(ov));
		}
	}
}

void ToxAVVoIPModel::destroySession(ObjectHandle session) {
	if (!static_cast<bool>(session)) {
		return;
	}

	// remove lookup
	if (session.all_of<Components::ToxAVAudioSource>()) {
		auto it_asrc = std::find_if(
			_audio_sources.cbegin(), _audio_sources.cend(),
			[o = session.get<Components::ToxAVAudioSource>().o](const auto& it) {
				return it.second == o;
			}
		);

		if (it_asrc != _audio_sources.cend()) {
			_audio_sources.erase(it_asrc);
		}
	}

	// destory sources
	if (auto* ss = session.try_get<Components::VoIP::StreamSources>(); ss != nullptr) {
		for (const auto ssov : ss->streams) {

			_os.throwEventDestroy(ssov);
			_os.registry().destroy(ssov);
		}
	}

	// destory sinks
	if (auto* ss = session.try_get<Components::VoIP::StreamSinks>(); ss != nullptr) {
		for (const auto ssov : ss->streams) {

			_os.throwEventDestroy(ssov);
			_os.registry().destroy(ssov);
		}
	}

	// destory session
	_os.throwEventDestroy(session);
	_os.registry().destroy(session);
}

void ToxAVVoIPModel::tick(void) {
	for (const auto& [oc, asink, asrf] : _os.registry().view<ToxAVCallAudioSink*, AudioStreamReFramer>().each()) {
	//for (const auto& [oc, asink] : _os.registry().view<ToxAVCallAudioSink*>().each()) {
		if (!asink->_writer) {
			continue;
		}

		asrf._stream = asink->_writer.get();

		for (size_t i = 0; i < 100; i++) {
			//auto new_frame_opt = asink->_writer->pop();
			auto new_frame_opt = asrf.pop();
			if (!new_frame_opt.has_value()) {
				break;
			}
			const auto& new_frame = new_frame_opt.value();

			//* @param sample_count Number of samples in this frame. Valid numbers here are
			//*   `((sample rate) * (audio length) / 1000)`, where audio length can be
			//*   2.5, 5, 10, 20, 40 or 60 milliseconds.

			// we likely needs to subdivide/repackage
			// frame size should be an option exposed to the user
			// with 10ms as a default ?
			// the larger the frame size, the less overhead but the more delay

			auto err = _av.toxavAudioSendFrame(
				asink->_fid,
				new_frame.getSpan().ptr,
				new_frame.getSpan().size / new_frame.channels,
				new_frame.channels,
				new_frame.sample_rate
			);
			if (err != TOXAV_ERR_SEND_FRAME_OK) {
				std::cerr << "DTC: failed to send audio frame " << err << "\n";
			}
		}
	}
}

ObjectHandle ToxAVVoIPModel::enter(const Contact3 c, const Components::VoIP::DefaultConfig& defaults) {
	if (!_cr.all_of<Contact::Components::ToxFriendEphemeral>(c)) {
		return {};
	}

	const auto friend_number = _cr.get<Contact::Components::ToxFriendEphemeral>(c).friend_number;

	std::cerr << "TAVVOIP: lol\n";
	const auto err = _av.toxavCall(friend_number, 0, 0);
	if (err != TOXAV_ERR_CALL_OK) {
		std::cerr << "TAVVOIP error: failed to start call: " << err << "\n";
		return {};
	}

	ObjectHandle new_session {_os.registry(), _os.registry().create()};

	new_session.emplace<Components::VoIP::SessionContact>(c);
	new_session.emplace<Components::VoIP::TagVoIPSession>(); // ??
	new_session.emplace<Components::VoIP::SessionState>().state = Components::VoIP::SessionState::State::RINGING;
	new_session.emplace<VoIPModelI*>(this);
	new_session.emplace<Components::VoIP::DefaultConfig>(defaults);

	_os.throwEventConstruct(new_session);
	return new_session;
}

bool ToxAVVoIPModel::accept(ObjectHandle session) {
	//_av.toxavAnswer(, 0, 0);
	return false;
}

bool ToxAVVoIPModel::leave(ObjectHandle session) {
	// rename to end?

	if (!static_cast<bool>(session)) {
		return false;
	}

	if (!session.all_of<Components::VoIP::TagVoIPSession, VoIPModelI*, Components::VoIP::SessionContact>()) {
		return false;
	}

	// check if self
	if (session.get<VoIPModelI*>() != this) {
		return false;
	}

	const auto session_contact = session.get<Components::VoIP::SessionContact>().c;
	if (!_cr.all_of<Contact::Components::ToxFriendEphemeral>(session_contact)) {
		return false;
	}

	const auto friend_number = _cr.get<Contact::Components::ToxFriendEphemeral>(session_contact).friend_number;
	// check error? (we delete anyway)
	_av.toxavCallControl(friend_number, Toxav_Call_Control::TOXAV_CALL_CONTROL_CANCEL);

	destroySession(session);
	return true;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendCall& e) {
	// new incoming call, create voip session, ready to be accepted
	// (or rejected...)

	const auto session_contact = _tcm.getContactFriend(e.friend_number);
	if (!_cr.valid(session_contact)) {
		return false;
	}

	ObjectHandle new_session {_os.registry(), _os.registry().create()};

	new_session.emplace<Components::VoIP::SessionContact>(session_contact);
	new_session.emplace<Components::VoIP::TagVoIPSession>(); // ??
	new_session.emplace<Components::VoIP::SessionState>().state = Components::VoIP::SessionState::State::RINGING;
	new_session.emplace<VoIPModelI*>(this);

	_os.throwEventConstruct(new_session);
	return true;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendCallState& e) {
	const auto session_contact = _tcm.getContactFriend(e.friend_number);
	if (!_cr.valid(session_contact)) {
		return false;
	}

	ToxAVFriendCallState s{e.state};

	// find session(s?)
	// TODO: keep lookup table
	for (const auto& [ov, voipmodel] : _os.registry().view<VoIPModelI*>().each()) {
		if (voipmodel == this) {
			auto o = _os.objectHandle(ov);

			if (!o.all_of<Components::VoIP::SessionContact>()) {
				continue;
			}
			if (session_contact != o.get<Components::VoIP::SessionContact>().c) {
				continue;
			}

			if (s.is_error() || s.is_finished()) {
				// destroy call
				destroySession(o);
			} else {
				// remote accepted our call, or av send/recv conditions changed?
				o.get<Components::VoIP::SessionState>().state = Components::VoIP::SessionState::State::CONNECTED; // set to in call ??

				auto& stream_sinks = o.get_or_emplace<Components::VoIP::StreamSinks>().streams;
				if (s.is_accepting_a() && !o.all_of<Components::ToxAVAudioSink>()) {
					ObjectHandle outgoing_audio {_os.registry(), _os.registry().create()};

					auto new_asink = std::make_unique<ToxAVCallAudioSink>(_av, e.friend_number);
					outgoing_audio.emplace<ToxAVCallAudioSink*>(new_asink.get());
					outgoing_audio.emplace<AudioStreamReFramer>();
					outgoing_audio.emplace<Components::FrameStream2Sink<AudioFrame2>>(std::move(new_asink));
					outgoing_audio.emplace<Components::StreamSink>(Components::StreamSink::create<AudioFrame2>("ToxAV Friend Call Outgoing Audio"));

					if (
						const auto* defaults = o.try_get<Components::VoIP::DefaultConfig>();
						defaults != nullptr && defaults->outgoing_audio
					) {
						outgoing_audio.emplace<Components::TagConnectToDefault>(); // depends on what was specified in enter()
					}

					stream_sinks.push_back(outgoing_audio);
					o.emplace<Components::ToxAVAudioSink>(outgoing_audio);
					// TODO: tie session to stream

					_os.throwEventConstruct(outgoing_audio);
				} else if (!s.is_accepting_a() && o.all_of<Components::ToxAVAudioSink>()) {
					// remove asink?
				}

				// video

				// add/update sources
				auto& stream_source = o.get_or_emplace<Components::VoIP::StreamSources>().streams;

				// audio
				if (s.is_sending_a() && !o.all_of<Components::ToxAVAudioSource>()) {
					ObjectHandle incoming_audio {_os.registry(), _os.registry().create()};

					auto new_asrc = std::make_unique<FrameStream2MultiSource<AudioFrame2>>();
					incoming_audio.emplace<FrameStream2MultiSource<AudioFrame2>*>(new_asrc.get());
					incoming_audio.emplace<Components::FrameStream2Source<AudioFrame2>>(std::move(new_asrc));
					incoming_audio.emplace<Components::StreamSource>(Components::StreamSource::create<AudioFrame2>("ToxAV Friend Call Incoming Audio"));

					std::cout << "new incoming audio\n";
					if (
						const auto* defaults = o.try_get<Components::VoIP::DefaultConfig>();
						defaults != nullptr && defaults->incoming_audio
					) {
						incoming_audio.emplace<Components::TagConnectToDefault>(); // depends on what was specified in enter()
						std::cout << "with default\n";
					}

					stream_source.push_back(incoming_audio);
					o.emplace<Components::ToxAVAudioSource>(incoming_audio);
					// TODO: tie session to stream

					_audio_sources[e.friend_number] = incoming_audio;

					_os.throwEventConstruct(incoming_audio);
				} else if (!s.is_sending_a() && o.all_of<Components::ToxAVAudioSource>()) {
					// remove asrc?
				}

				// video
			}
		}
	}

	return true;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendAudioBitrate&) {
	return false;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendVideoBitrate&) {
	return false;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendAudioFrame& e) {
	auto asrc_it = _audio_sources.find(e.friend_number);
	if (asrc_it == _audio_sources.cend()) {
		// missing src from lookup table
		return false;
	}

	auto asrc = asrc_it->second;

	if (!static_cast<bool>(asrc)) {
		// missing src to put frame into ??
		return false;
	}

	assert(asrc.all_of<FrameStream2MultiSource<AudioFrame2>*>());
	assert(asrc.all_of<Components::FrameStream2Source<AudioFrame2>>());

	asrc.get<FrameStream2MultiSource<AudioFrame2>*>()->push(AudioFrame2{
		e.sampling_rate,
		e.channels,
		std::vector<int16_t>(e.pcm.begin(), e.pcm.end()) // copy
	});

	return true;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendVideoFrame&) {
	return false;
}

