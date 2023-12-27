#include "./auto_dirty.hpp"

#include "./tox_client.hpp"

// TODO: add more events

void AutoDirty::subscribe(void) {
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_SELF_CONNECTION_STATUS);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_FRIEND_CONNECTION_STATUS);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_FRIEND_REQUEST);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_GROUP_INVITE);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_GROUP_SELF_JOIN);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_GROUP_PEER_JOIN);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_GROUP_PEER_EXIT);
	_tc.subscribe(this, Tox_Event_Type::TOX_EVENT_CONFERENCE_INVITE);
}

AutoDirty::AutoDirty(ToxClient& tc) : _tc(tc) {
	subscribe();
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


