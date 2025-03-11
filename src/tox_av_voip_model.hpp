#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/contact/fwd.hpp>
#include <solanaceae/tox_contacts/tox_contact_model2.hpp>
#include "./frame_streams/voip_model.hpp"
#include "./tox_av.hpp"

#include <unordered_map>
#include <variant>
#include <deque>
#include <mutex>
#include <atomic>

// fwd
struct ToxAVCallAudioSink;
struct ToxAVCallVideoSink;

class ToxAVVoIPModel : protected ToxAVEventI, public VoIPModelI {
	ObjectStore2& _os;
	ToxAVI& _av;
	ToxAVI::SubscriptionReference _av_sr;
	ContactStore4I& _cs;
	ToxContactModel2& _tcm;

	uint64_t _pad0;
	// these events need to be worked on the main thread instead
	// TODO: replace with lockless queue
	std::deque<
	std::variant<
		Events::FriendCall,
		Events::FriendCallState
		// bitrates
	>> _e_queue;
	std::mutex _e_queue_mutex;
	uint64_t _pad1;

	std::vector<ToxAVCallAudioSink*> _audio_sinks;
	std::mutex _audio_sinks_mutex;
	uint64_t _pad2;

	std::vector<ToxAVCallVideoSink*> _video_sinks;
	std::mutex _video_sinks_mutex;
	uint64_t _pad3;

	// filled with min() in video_thread_tick()
	std::atomic<uint64_t> _video_send_time_until_next_frame{2'000};
	uint64_t _pad4;

	// for faster lookup
	std::unordered_map<uint32_t, ObjectHandle> _audio_sources;
	std::unordered_map<uint32_t, ObjectHandle> _video_sources;

	// TODO: virtual? strategy? protected?
	virtual void addAudioSource(ObjectHandle session, uint32_t friend_number);
	virtual void addAudioSink(ObjectHandle session, uint32_t friend_number);
	virtual void addVideoSource(ObjectHandle session, uint32_t friend_number);
	virtual void addVideoSink(ObjectHandle session, uint32_t friend_number);

	void destroySession(ObjectHandle session);

	// we use "events" as pre/post audio/video iterate...
	void audio_thread_tick(void);
	void video_thread_tick(void);

	void handleEvent(const Events::FriendCall&);
	void handleEvent(const Events::FriendCallState&);

	public:
		ToxAVVoIPModel(ObjectStore2& os, ToxAVI& av, ContactStore4I& cs, ToxContactModel2& tcm);
		~ToxAVVoIPModel(void);

		// handle events coming from toxav thread(s)
		float tick(void);

	public: // voip model
		ObjectHandle enter(const Contact4 c, const Components::VoIP::DefaultConfig& defaults) override;
		bool accept(ObjectHandle session, const Components::VoIP::DefaultConfig& defaults) override;
		bool leave(ObjectHandle session) override;

	protected: // toxav events
		bool onEvent(const Events::FriendCall&) override;
		bool onEvent(const Events::FriendCallState&) override;
		bool onEvent(const Events::FriendAudioBitrate&) override;
		bool onEvent(const Events::FriendVideoBitrate&) override;
		bool onEvent(const Events::FriendAudioFrame&) override;
		bool onEvent(const Events::FriendVideoFrame&) override;
		bool onEvent(const Events::IterateAudio&) override;
		bool onEvent(const Events::IterateVideo&) override;
};

