#include "./auto_dirty.hpp"

#include "./tox_client.hpp"

AutoDirty::AutoDirty(ToxClient& tc) : _tc(tc), _tep_sr(_tc.newSubRef(this)) {
	// TODO: add more events
	_tep_sr
		.subscribe(Tox_Event_Type::TOX_EVENT_SELF_CONNECTION_STATUS)
		.subscribe(Tox_Event_Type::TOX_EVENT_FRIEND_CONNECTION_STATUS)
		.subscribe(Tox_Event_Type::TOX_EVENT_FRIEND_REQUEST)
		.subscribe(Tox_Event_Type::TOX_EVENT_GROUP_INVITE)
		.subscribe(Tox_Event_Type::TOX_EVENT_GROUP_SELF_JOIN)
		.subscribe(Tox_Event_Type::TOX_EVENT_GROUP_PEER_JOIN)
		.subscribe(Tox_Event_Type::TOX_EVENT_GROUP_PEER_EXIT)
		.subscribe(Tox_Event_Type::TOX_EVENT_CONFERENCE_INVITE)
	;
}

bool AutoDirty::onToxEvent(const Tox_Event_Self_Connection_Status*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Friend_Connection_Status*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Friend_Request*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Group_Invite*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Group_Self_Join*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Group_Peer_Join*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Group_Peer_Exit*) {
	_tc.setDirty();
	return false;
}

bool AutoDirty::onToxEvent(const Tox_Event_Conference_Invite*) {
	_tc.setDirty();
	return false;
}


