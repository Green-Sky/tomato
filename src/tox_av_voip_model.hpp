#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/tox_contacts/tox_contact_model2.hpp>
#include "./frame_streams/voip_model.hpp"
#include "./tox_av.hpp"

#include <unordered_map>

class ToxAVVoIPModel : protected ToxAVEventI, public VoIPModelI {
	ObjectStore2& _os;
	ToxAVI& _av;
	Contact3Registry& _cr;
	ToxContactModel2& _tcm;

	// for faster lookup
	std::unordered_map<uint32_t, ObjectHandle> _audio_sources;
	std::unordered_map<uint32_t, ObjectHandle> _video_sources;

	// TODO: virtual? strategy? protected?
	virtual void addAudioSource(ObjectHandle session, uint32_t friend_number);
	virtual void addAudioSink(ObjectHandle session, uint32_t friend_number);
	virtual void addVideoSource(ObjectHandle session, uint32_t friend_number);
	virtual void addVideoSink(ObjectHandle session, uint32_t friend_number);

	void destroySession(ObjectHandle session);

	public:
		ToxAVVoIPModel(ObjectStore2& os, ToxAVI& av, Contact3Registry& cr, ToxContactModel2& tcm);
		~ToxAVVoIPModel(void);

		void tick(void);

	public: // voip model
		ObjectHandle enter(const Contact3 c, const Components::VoIP::DefaultConfig& defaults) override;
		bool accept(ObjectHandle session, const Components::VoIP::DefaultConfig& defaults) override;
		bool leave(ObjectHandle session) override;

	protected: // toxav events
		bool onEvent(const Events::FriendCall&) override;
		bool onEvent(const Events::FriendCallState&) override;
		bool onEvent(const Events::FriendAudioBitrate&) override;
		bool onEvent(const Events::FriendVideoBitrate&) override;
		bool onEvent(const Events::FriendAudioFrame&) override;
		bool onEvent(const Events::FriendVideoFrame&) override;
};

