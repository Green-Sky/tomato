#include "./tox_avatar_sender.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/contact/contact_store_events.hpp>
#include <solanaceae/tox_contacts/components.hpp>
#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/object_store/meta_components.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>
#include <solanaceae/tox_messages/obj_components.hpp>

#include <iostream>

namespace Components {
	struct TagAvatarOffered {};
} // Components

bool ToxAvatarSender::checkContact(ContactHandle4 c) {
	// check if tox friend and not sent
	if (!c.all_of<
		Contact::Components::ToxFriendEphemeral,
		Contact::Components::ConnectionState
	>()) {
		return false;
	}
	if (c.any_of<Components::TagAvatarOffered>()) {
		return false;
	}

	// is (not) self
	if (c.all_of<Contact::Components::TagSelfStrong>()) {
		return false;
	}

	if (!c.all_of<Contact::Components::Self>()) {
		std::cout << "TAS warning: contact has no self\n";
		return false;
	}

	auto self_c = _cs.contactHandle(c.get<Contact::Components::Self>().self);
	if (!static_cast<bool>(self_c)) {
		std::cerr << "TAS error: invalid self\n";
		assert(false);
		return false;
	}

	// self has avatar (obj)
	if (!self_c.all_of<Contact::Components::AvatarObj>()) {
		return false;
	}

	return true;
}

bool ToxAvatarSender::inQueue(ContactHandle4 c) const {
	return std::find_if(_queue.cbegin(), _queue.cend(), [&c](const Entry& e) { return e.c == c; }) != _queue.cend();
}

void ToxAvatarSender::addToQueue(ContactHandle4 c) {
	assert(!inQueue(c));
	_queue.push_back({
		c,
		c.get<Contact::Components::ConnectionState>().state
	});
}

void ToxAvatarSender::sendAvatar(ContactHandle4 c) {
	std::cout << "TAS: sending self avatar to " << entt::to_integral(c.entity()) << "\n";

	if (!c.all_of<Contact::Components::Self>()) {
		std::cout << "TAS warning: contact has no self\n";
		return;
	}

	auto self_c = _cs.contactHandle(c.get<Contact::Components::Self>().self);
	if (!static_cast<bool>(self_c)) {
		std::cerr << "TAS error: invalid self\n";
		assert(false);
		return;
	}

	if (!self_c.all_of<Contact::Components::AvatarObj>()) {
		return;
	}

	const auto ao = self_c.get<Contact::Components::AvatarObj>().obj;

	// .... ?
	// duplicate object?
	// a single object can only ever be one transfer (meh)
	// TODO: make object multi transfer-able (enforce)
	auto self_o = _os.objectHandle(ao);
	if (!static_cast<bool>(self_o)) {
		std::cerr << "TAS error: self avatar obj not real?\n";
		return;
	}
	// TODO: move to tox avatar specific backend
	// HACK: manual clone
	// using self_o meta backend newObject() should emplace the file backend too
	auto new_o = self_o.get<ObjComp::Ephemeral::BackendMeta>().ptr->newObject(ByteSpan{}, false);
	assert(new_o);

	if (self_o.all_of<ObjComp::ID>()) {
		new_o.emplace_or_replace<ObjComp::ID>(self_o.get<ObjComp::ID>());
	}
	if (self_o.all_of<ObjComp::F::SingleInfo>()) {
		new_o.emplace_or_replace<ObjComp::F::SingleInfo>(self_o.get<ObjComp::F::SingleInfo>());
	}
	if (self_o.all_of<ObjComp::F::SingleInfoLocal>()) {
		new_o.emplace_or_replace<ObjComp::F::SingleInfoLocal>(self_o.get<ObjComp::F::SingleInfoLocal>());
	}
	if (self_o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		new_o.emplace_or_replace<ObjComp::F::TagLocalHaveAll>();
	}

	// tox avatar
	new_o.emplace_or_replace<ObjComp::Tox::FileKind>(uint64_t(1));

	_os.throwEventConstruct(new_o); // ?

	if (!_rmm.sendFileObj(c, new_o)) {
		std::cerr << "TAS error: failed to send avatar file obj\n";
		_os.throwEventDestroy(new_o);
		new_o.destroy();
	}

	c.emplace_or_replace<Components::TagAvatarOffered>();
}

ToxAvatarSender::ToxAvatarSender(ObjectStore2& os, ContactStore4I& cs, RegistryMessageModelI& rmm) :
	_os(os),
	_cs(cs),
	_cs_sr(cs.newSubRef(this)),
	_rmm(rmm),
	_rmm_sr(_rmm.newSubRef(this))
{
	_cs_sr
		.subscribe(ContactStore4_Event::contact_construct)
		.subscribe(ContactStore4_Event::contact_update)
		.subscribe(ContactStore4_Event::contact_destroy)
	;

	_rmm_sr
		.subscribe(RegistryMessageModel_Event::send_file_obj)
	;
}

void ToxAvatarSender::iterate(float delta) {
	for (auto it = _queue.begin(); it != _queue.end();) {
		it->timer -= delta;
		if (it->timer <= 0.f) {
			sendAvatar(it->c);
			it = _queue.erase(it);
		} else {
			it++;
		}
	}
}

bool ToxAvatarSender::onEvent(const ContactStore::Events::Contact4Construct& e) {
	if (!static_cast<bool>(e.e)) {
		assert(false);
		return false;
	}

	if (!checkContact(e.e)) {
		return false;
	}

	if (e.e.get<Contact::Components::ConnectionState>().state == Contact::Components::ConnectionState::disconnected) {
		return false;
	}

	addToQueue(e.e);

	return false;
}

bool ToxAvatarSender::onEvent(const ContactStore::Events::Contact4Update& e) {
	if (!static_cast<bool>(e.e)) {
		assert(false);
		return false;
	}

	if (!checkContact(e.e)) {
		return false;
	}

	auto it = std::find_if(_queue.begin(), _queue.end(), [c = e.e](const Entry& en) { return en.c == c; });
	if (it != _queue.end()) {
		// check if connections state changed and reset timer
		const auto current_state = e.e.get<Contact::Components::ConnectionState>().state;
		if (current_state == Contact::Components::ConnectionState::disconnected) {
			_queue.erase(it);
		} else if (it->ls != current_state) {
			// state changed, reset timer
			it->timer = 61.33f;
		}
	} else if (e.e.get<Contact::Components::ConnectionState>().state != Contact::Components::ConnectionState::disconnected) {
		addToQueue(e.e);
	}

	return false;
}

bool ToxAvatarSender::onEvent(const ContactStore::Events::Contact4Destory& e) {
	// remove from queue
	// queue is assumed to be short
	auto it = std::find_if(_queue.begin(), _queue.end(), [c = e.e](const Entry& en) { return en.c == c; });
	if (it != _queue.end()) {
		_queue.erase(it);
	}

	return false;
}

