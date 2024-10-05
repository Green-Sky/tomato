#include "./tox_av_voip_model.hpp"

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/tox_contacts/components.hpp>

#include "./frame_streams/stream_manager.hpp"
#include "./frame_streams/audio_stream2.hpp"
#include "./frame_streams/locked_frame_stream.hpp"
#include "./frame_streams/multi_source.hpp"
#include "./frame_streams/audio_stream_pop_reframer.hpp"

#include "./frame_streams/sdl/video.hpp"
#include "./frame_streams/sdl/video_push_converter.hpp"

#include <cstring>

#include <iostream>

// fwd
namespace Message {
uint64_t getTimeMS(void);
} // Message

namespace Components {
	struct ToxAVIncomingAV {
		bool incoming_audio {false};
		bool incoming_video {false};
	};

	struct ToxAVAudioSink {
		ObjectHandle o;
		// ptr?
	};
	struct ToxAVVideoSink {
		ObjectHandle o;
	};
	struct ToxAVAudioSource {
		ObjectHandle o;
	};
	struct ToxAVVideoSource {
		ObjectHandle o;
	};
} // Components

struct ToxAVCallAudioSink : public FrameStream2SinkI<AudioFrame2> {
	ToxAVI& _toxav;

	// bitrate for enabled state
	uint32_t _audio_bitrate {32};

	uint32_t _fid;
	std::shared_ptr<AudioStreamPopReFramer<LockedFrameStream2<AudioFrame2>>> _writer;

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

		// 20ms for now, 10ms would work too, further investigate stutters at 5ms (probably too slow interval rate)
		_writer = std::make_shared<AudioStreamPopReFramer<LockedFrameStream2<AudioFrame2>>>(20);

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

// exlusive
struct ToxAVCallVideoSink : public FrameStream2SinkI<SDLVideoFrame> {
	using stream_type = PushConversionVideoStream<LockedFrameStream2<SDLVideoFrame>>;
	ToxAVI& _toxav;

	// bitrate for enabled state
	uint32_t _video_bitrate {2}; // HACK: hardcode to 2mbits (toxap wrongly multiplies internally by 1000)

	uint32_t _fid;
	std::shared_ptr<stream_type> _writer;

	ToxAVCallVideoSink(ToxAVI& toxav, uint32_t fid) : _toxav(toxav), _fid(fid) {}
	~ToxAVCallVideoSink(void) {
		if (_writer) {
			_writer = nullptr;
			_toxav.toxavVideoSetBitRate(_fid, 0);
		}
	}

	// sink
	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override {
		if (_writer) {
			// max 1 (exclusive, composite video somewhere else)
			return nullptr;
		}

		auto err = _toxav.toxavVideoSetBitRate(_fid, _video_bitrate);
		if (err != TOXAV_ERR_BIT_RATE_SET_OK) {
			return nullptr;
		}

		// toxav needs I420
		_writer = std::make_shared<stream_type>(SDL_PIXELFORMAT_IYUV);

		return _writer;
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override {
		if (!sub || !_writer) {
			// nah
			return false;
		}

		if (sub == _writer) {
			_writer = nullptr;

			/*auto err = */_toxav.toxavVideoSetBitRate(_fid, 0);
			// print warning? on error?

			return true;
		}

		// what
		return false;
	}
};

void ToxAVVoIPModel::addAudioSource(ObjectHandle session, uint32_t friend_number) {
	auto& stream_source = session.get_or_emplace<Components::VoIP::StreamSources>().streams;

	ObjectHandle incoming_audio {_os.registry(), _os.registry().create()};

	auto new_asrc = std::make_unique<FrameStream2MultiSource<AudioFrame2>>();
	incoming_audio.emplace<FrameStream2MultiSource<AudioFrame2>*>(new_asrc.get());
	incoming_audio.emplace<Components::FrameStream2Source<AudioFrame2>>(std::move(new_asrc));
	incoming_audio.emplace<Components::StreamSource>(Components::StreamSource::create<AudioFrame2>("ToxAV Friend Call Incoming Audio"));

	if (
		const auto* defaults = session.try_get<Components::VoIP::DefaultConfig>();
		defaults != nullptr && defaults->incoming_audio
	) {
		incoming_audio.emplace<Components::TagConnectToDefault>(); // depends on what was specified in enter()
	}

	stream_source.push_back(incoming_audio);
	session.emplace<Components::ToxAVAudioSource>(incoming_audio);
	// TODO: tie session to stream

	_audio_sources[friend_number] = incoming_audio;

	_os.throwEventConstruct(incoming_audio);
}

void ToxAVVoIPModel::addAudioSink(ObjectHandle session, uint32_t friend_number) {
	auto& stream_sinks = session.get_or_emplace<Components::VoIP::StreamSinks>().streams;
	ObjectHandle outgoing_audio {_os.registry(), _os.registry().create()};

	auto new_asink = std::make_unique<ToxAVCallAudioSink>(_av, friend_number);
	auto* new_asink_ptr = new_asink.get();
	outgoing_audio.emplace<ToxAVCallAudioSink*>(new_asink_ptr);
	outgoing_audio.emplace<Components::FrameStream2Sink<AudioFrame2>>(std::move(new_asink));
	outgoing_audio.emplace<Components::StreamSink>(Components::StreamSink::create<AudioFrame2>("ToxAV Friend Call Outgoing Audio"));

	if (
		const auto* defaults = session.try_get<Components::VoIP::DefaultConfig>();
		defaults != nullptr && defaults->outgoing_audio
	) {
		outgoing_audio.emplace<Components::TagConnectToDefault>(); // depends on what was specified in enter()
	}

	stream_sinks.push_back(outgoing_audio);
	session.emplace<Components::ToxAVAudioSink>(outgoing_audio);
	// TODO: tie session to stream

	_os.throwEventConstruct(outgoing_audio);

	std::lock_guard lg{_audio_sinks_mutex};
	_audio_sinks.push_back(new_asink_ptr);
}

void ToxAVVoIPModel::addVideoSource(ObjectHandle session, uint32_t friend_number) {
	auto& stream_source = session.get_or_emplace<Components::VoIP::StreamSources>().streams;

	ObjectHandle incoming_video {_os.registry(), _os.registry().create()};

	auto new_vsrc = std::make_unique<FrameStream2MultiSource<SDLVideoFrame>>();
	incoming_video.emplace<FrameStream2MultiSource<SDLVideoFrame>*>(new_vsrc.get());
	incoming_video.emplace<Components::FrameStream2Source<SDLVideoFrame>>(std::move(new_vsrc));
	incoming_video.emplace<Components::StreamSource>(Components::StreamSource::create<SDLVideoFrame>("ToxAV Friend Call Incoming Video"));

	if (
		const auto* defaults = session.try_get<Components::VoIP::DefaultConfig>();
		defaults != nullptr && defaults->incoming_video
	) {
		incoming_video.emplace<Components::TagConnectToDefault>(); // depends on what was specified in enter()
	}

	stream_source.push_back(incoming_video);
	session.emplace<Components::ToxAVVideoSource>(incoming_video);
	// TODO: tie session to stream

	_video_sources[friend_number] = incoming_video;

	_os.throwEventConstruct(incoming_video);
}

void ToxAVVoIPModel::addVideoSink(ObjectHandle session, uint32_t friend_number) {
	auto& stream_sinks = session.get_or_emplace<Components::VoIP::StreamSinks>().streams;
	ObjectHandle outgoing_video {_os.registry(), _os.registry().create()};

	auto new_vsink = std::make_unique<ToxAVCallVideoSink>(_av, friend_number);
	auto* new_vsink_ptr = new_vsink.get();
	outgoing_video.emplace<ToxAVCallVideoSink*>(new_vsink_ptr);
	outgoing_video.emplace<Components::FrameStream2Sink<SDLVideoFrame>>(std::move(new_vsink));
	outgoing_video.emplace<Components::StreamSink>(Components::StreamSink::create<SDLVideoFrame>("ToxAV Friend Call Outgoing Video"));

	if (
		const auto* defaults = session.try_get<Components::VoIP::DefaultConfig>();
		defaults != nullptr && defaults->outgoing_video
	) {
		outgoing_video.emplace<Components::TagConnectToDefault>(); // depends on what was specified in enter()
	}

	stream_sinks.push_back(outgoing_video);
	session.emplace<Components::ToxAVVideoSink>(outgoing_video);
	// TODO: tie session to stream

	_os.throwEventConstruct(outgoing_video);

	std::lock_guard lg{_video_sinks_mutex};
	_video_sinks.push_back(new_vsink_ptr);
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
	if (session.all_of<Components::ToxAVVideoSource>()) {
		auto it_vsrc = std::find_if(
			_video_sources.cbegin(), _video_sources.cend(),
			[o = session.get<Components::ToxAVVideoSource>().o](const auto& it) {
				return it.second == o;
			}
		);

		if (it_vsrc != _video_sources.cend()) {
			_video_sources.erase(it_vsrc);
		}
	}
	if (session.all_of<ToxAVCallAudioSink*>()) {
		std::lock_guard lg{_audio_sinks_mutex};
		auto it = std::find(_audio_sinks.cbegin(), _audio_sinks.cend(), session.get<ToxAVCallAudioSink*>());
		if (it != _audio_sinks.cend()) {
			_audio_sinks.erase(it);
		}
	}
	if (session.all_of<ToxAVCallVideoSink*>()) {
		std::lock_guard lg{_video_sinks_mutex};
		auto it = std::find(_video_sinks.cbegin(), _video_sinks.cend(), session.get<ToxAVCallVideoSink*>());
		if (it != _video_sinks.cend()) {
			_video_sinks.erase(it);
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

void ToxAVVoIPModel::audio_thread_tick(void) {
	//for (const auto& [oc, asink] : _os.registry().view<ToxAVCallAudioSink*>().each()) {
	std::lock_guard lg{_audio_sinks_mutex};
	for (const auto& asink : _audio_sinks) {
		if (!asink->_writer) {
			continue;
		}

		for (size_t i = 0; i < 100; i++) {
			auto new_frame_opt = asink->_writer->pop();
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

void ToxAVVoIPModel::video_thread_tick(void) {
	//for (const auto& [oc, vsink] : _os.registry().view<ToxAVCallVideoSink*>().each()) {
	std::lock_guard lg{_video_sinks_mutex};
	for (const auto& vsink : _video_sinks) {
		if (!vsink->_writer) {
			continue;
		}

		for (size_t i = 0; i < 10; i++) {
			auto new_frame_opt = vsink->_writer->pop();
			if (!new_frame_opt.has_value()) {
				break;
			}
			const auto& new_frame = new_frame_opt.value();

			if (!new_frame.surface) {
				// wtf?
				continue;
			}

			// conversion is done in the sink's stream
			SDL_Surface* surf = new_frame.surface.get();
			assert(surf != nullptr);

			SDL_LockSurface(surf);
			_av.toxavVideoSendFrame(
				vsink->_fid,
				surf->w, surf->h,
				static_cast<const uint8_t*>(surf->pixels),
				static_cast<const uint8_t*>(surf->pixels) + surf->w * surf->h,
				static_cast<const uint8_t*>(surf->pixels) + surf->w * surf->h + (surf->w/2) * (surf->h/2)
			);
			SDL_UnlockSurface(surf);
		}
	}
}

void ToxAVVoIPModel::handleEvent(const Events::FriendCall& e) {
	// new incoming call, create voip session, ready to be accepted
	// (or rejected...)

	const auto session_contact = _tcm.getContactFriend(e.friend_number);
	if (!_cr.valid(session_contact)) {
		return;
	}

	ObjectHandle new_session {_os.registry(), _os.registry().create()};

	new_session.emplace<VoIPModelI*>(this);
	new_session.emplace<Components::VoIP::TagVoIPSession>(); // ??
	new_session.emplace<Components::VoIP::Incoming>(session_contact); // in 1on1 its always the same contact, might leave blank
	new_session.emplace<Components::VoIP::SessionContact>(session_contact);
	new_session.emplace<Components::VoIP::SessionState>().state = Components::VoIP::SessionState::State::RINGING;
	new_session.emplace<Components::ToxAVIncomingAV>(e.audio_enabled, e.video_enabled);

	_os.throwEventConstruct(new_session);
}

void ToxAVVoIPModel::handleEvent(const Events::FriendCallState& e) {
	const auto session_contact = _tcm.getContactFriend(e.friend_number);
	if (!_cr.valid(session_contact)) {
		return;
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

				if (s.is_accepting_a() && !o.all_of<Components::ToxAVAudioSink>()) {
					addAudioSink(o, e.friend_number);
				} else if (!s.is_accepting_a() && o.all_of<Components::ToxAVAudioSink>()) {
					// remove asink?
				}

				// video
				if (s.is_accepting_v() && !o.all_of<Components::ToxAVVideoSink>()) {
					addVideoSink(o, e.friend_number);
				} else if (!s.is_accepting_v() && o.all_of<Components::ToxAVVideoSink>()) {
					// remove vsink?
				}

				// add/update sources
				// audio
				if (s.is_sending_a() && !o.all_of<Components::ToxAVAudioSource>()) {
					addAudioSource(o, e.friend_number);
				} else if (!s.is_sending_a() && o.all_of<Components::ToxAVAudioSource>()) {
					// remove asrc?
				}

				// video
				if (s.is_sending_v() && !o.all_of<Components::ToxAVVideoSource>()) {
					addVideoSource(o, e.friend_number);
				} else if (!s.is_sending_v() && o.all_of<Components::ToxAVVideoSource>()) {
					// remove vsrc?
				}
			}
		}
	}
}

ToxAVVoIPModel::ToxAVVoIPModel(ObjectStore2& os, ToxAVI& av, Contact3Registry& cr, ToxContactModel2& tcm) :
	_os(os), _av(av), _cr(cr), _tcm(tcm)
{
	_av.subscribe(this, ToxAV_Event::friend_call);
	_av.subscribe(this, ToxAV_Event::friend_call_state);
	_av.subscribe(this, ToxAV_Event::friend_audio_bitrate);
	_av.subscribe(this, ToxAV_Event::friend_video_bitrate);
	_av.subscribe(this, ToxAV_Event::friend_audio_frame);
	_av.subscribe(this, ToxAV_Event::friend_video_frame);
	_av.subscribe(this, ToxAV_Event::iterate_audio);
	_av.subscribe(this, ToxAV_Event::iterate_video);

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

void ToxAVVoIPModel::tick(void) {
	std::lock_guard lg{_e_queue_mutex};
	while (!_e_queue.empty()) {
		const auto& e_var = _e_queue.front();

		if (std::holds_alternative<Events::FriendCall>(e_var)) {
			const auto& e = std::get<Events::FriendCall>(e_var);
			handleEvent(e);
		} else if (std::holds_alternative<Events::FriendCallState>(e_var)) {
			const auto& e = std::get<Events::FriendCallState>(e_var);
			handleEvent(e);
		} else {
			assert(false && "unk event");
		}

		_e_queue.pop_front();
	}
}

ObjectHandle ToxAVVoIPModel::enter(const Contact3 c, const Components::VoIP::DefaultConfig& defaults) {
	if (!_cr.all_of<Contact::Components::ToxFriendEphemeral>(c)) {
		return {};
	}

	const auto friend_number = _cr.get<Contact::Components::ToxFriendEphemeral>(c).friend_number;

	const auto err = _av.toxavCall(friend_number, 0, 0);
	if (err != TOXAV_ERR_CALL_OK) {
		std::cerr << "TAVVOIP error: failed to start call: " << err << "\n";
		return {};
	}

	ObjectHandle new_session {_os.registry(), _os.registry().create()};

	new_session.emplace<VoIPModelI*>(this);
	new_session.emplace<Components::VoIP::TagVoIPSession>(); // ??
	new_session.emplace<Components::VoIP::SessionContact>(c);
	new_session.emplace<Components::VoIP::SessionState>().state = Components::VoIP::SessionState::State::RINGING;
	new_session.emplace<Components::VoIP::DefaultConfig>(defaults);

	_os.throwEventConstruct(new_session);
	return new_session;
}

bool ToxAVVoIPModel::accept(ObjectHandle session, const Components::VoIP::DefaultConfig& defaults) {
	if (!static_cast<bool>(session)) {
		return false;
	}

	if (!session.all_of<
		Components::VoIP::TagVoIPSession,
		VoIPModelI*,
		Components::VoIP::SessionContact,
		Components::VoIP::Incoming
	>()) {
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
	auto err = _av.toxavAnswer(friend_number, 0, 0);
	if (err != TOXAV_ERR_ANSWER_OK) {
		std::cerr << "TOXAVVOIP error: ansering call failed: " << err << "\n";
		// we simply let it be for now, it apears we can try ansering later again
		// we also get an error here when the call is already in progress (:
		return false;
	}

	session.emplace<Components::VoIP::DefaultConfig>(defaults);

	// answer defaults to enabled receiving audio and video
	// TODO: think about how we should handle this
	// set to disabled? and enable on src connection?
	// we already default disabled send and enabled on sink connection
	//_av.toxavCallControl(friend_number, TOXAV_CALL_CONTROL_HIDE_VIDEO);
	//_av.toxavCallControl(friend_number, TOXAV_CALL_CONTROL_MUTE_AUDIO);


	// how do we know the other side is accepting audio
	// bitrate cb or what?
	assert(!session.all_of<Components::ToxAVAudioSink>());
	addAudioSink(session, friend_number);
	assert(!session.all_of<Components::ToxAVVideoSink>());
	addVideoSink(session, friend_number);

	if (const auto* i_av = session.try_get<Components::ToxAVIncomingAV>(); i_av != nullptr) {
		// create audio src
		if (i_av->incoming_audio) {
			assert(!session.all_of<Components::ToxAVAudioSource>());
			addAudioSource(session, friend_number);
		}

		// create video src
		if (i_av->incoming_video) {
			assert(!session.all_of<Components::ToxAVVideoSource>());
			addVideoSource(session, friend_number);
		}
	}

	session.get_or_emplace<Components::VoIP::SessionState>().state = Components::VoIP::SessionState::State::CONNECTED;
	_os.throwEventUpdate(session);
	return true;
}

bool ToxAVVoIPModel::leave(ObjectHandle session) {
	// rename to end?

	if (!static_cast<bool>(session)) {
		return false;
	}

	if (!session.all_of<
		Components::VoIP::TagVoIPSession,
		VoIPModelI*,
		Components::VoIP::SessionContact
	>()) {
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
	std::lock_guard lg{_e_queue_mutex};
	_e_queue.push_back(e);
	return true; // false?
}

bool ToxAVVoIPModel::onEvent(const Events::FriendCallState& e) {
	std::lock_guard lg{_e_queue_mutex};
	_e_queue.push_back(e);
	return true; // false?
}

bool ToxAVVoIPModel::onEvent(const Events::FriendAudioBitrate&) {
	// TODO: use this info
	return false;
}

bool ToxAVVoIPModel::onEvent(const Events::FriendVideoBitrate&) {
	// TODO: use this info
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

bool ToxAVVoIPModel::onEvent(const Events::FriendVideoFrame& e) {
	auto vsrc_it = _video_sources.find(e.friend_number);
	if (vsrc_it == _video_sources.cend()) {
		// missing src from lookup table
		return false;
	}

	auto vsrc = vsrc_it->second;

	if (!static_cast<bool>(vsrc)) {
		// missing src to put frame into ??
		return false;
	}

	assert(vsrc.all_of<FrameStream2MultiSource<SDLVideoFrame>*>());
	assert(vsrc.all_of<Components::FrameStream2Source<SDLVideoFrame>>());

	auto* new_surf = SDL_CreateSurface(e.width, e.height, SDL_PIXELFORMAT_IYUV);
	assert(new_surf);
	if (SDL_LockSurface(new_surf)) {
		// copy the data
		// we know how the implementation works, its y u v consecutivlely
		// y
		for (size_t y = 0; y < e.height; y++) {
			std::memcpy(
				//static_cast<uint8_t*>(new_surf->pixels) + new_surf->pitch*y,
				static_cast<uint8_t*>(new_surf->pixels) + e.width*y,
				e.y.ptr + e.ystride*y,
				e.width
			);
		}

		// u
		for (size_t y = 0; y < e.height/2; y++) {
			std::memcpy(
				static_cast<uint8_t*>(new_surf->pixels) + (e.width*e.height) + (e.width/2)*y,
				e.u.ptr + e.ustride*y,
				e.width/2
			);
		}

		// v
		for (size_t y = 0; y < e.height/2; y++) {
			std::memcpy(
				static_cast<uint8_t*>(new_surf->pixels) + (e.width*e.height) + ((e.width/2)*(e.height/2)) + (e.width/2)*y,
				e.v.ptr + e.vstride*y,
				e.width/2
			);
		}

		SDL_UnlockSurface(new_surf);
	}

	vsrc.get<FrameStream2MultiSource<SDLVideoFrame>*>()->push({
		// ms -> us
		// would be nice if we had been giving this from toxcore
		// TODO: make more precise
		Message::getTimeMS() * 1000,
		new_surf
	});

	SDL_DestroySurface(new_surf);
	return true;
}

bool ToxAVVoIPModel::onEvent(const Events::IterateAudio&) {
	audio_thread_tick();
	return false;
}

bool ToxAVVoIPModel::onEvent(const Events::IterateVideo&) {
	video_thread_tick();
	return false;
}

