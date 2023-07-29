#pragma once

#include <solanaceae/toxcore/tox_event_interface.hpp>

// fwd
class ToxClient;

// sets ToxClient dirty on some events
class AutoDirty : public ToxEventI {
	ToxClient& _tc;

	void subscribe(void); // private

	public:
		AutoDirty(ToxClient& tc);

	protected: // tox events
		bool onToxEvent(const Tox_Event_Self_Connection_Status* e) override;
		bool onToxEvent(const Tox_Event_Friend_Connection_Status* e) override;
		bool onToxEvent(const Tox_Event_Friend_Request* e) override;
		bool onToxEvent(const Tox_Event_Group_Invite* e) override;
		bool onToxEvent(const Tox_Event_Group_Self_Join* e) override;
		bool onToxEvent(const Tox_Event_Group_Peer_Join* e) override;
		bool onToxEvent(const Tox_Event_Group_Peer_Exit* e) override;
		bool onToxEvent(const Tox_Event_Conference_Invite* e) override;
};

