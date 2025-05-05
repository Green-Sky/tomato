#include "./tox_avatar_manager.hpp"

// TODO: this whole thing needs a rewrite, seperating tcs and rng uid os

#include <solanaceae/util/config_model.hpp>

#include <solanaceae/contact/contact_store_i.hpp>

//#include <solanaceae/message3/components.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>
// for comp transfer tox filekind (TODO: generalize -> content system?)
#include <solanaceae/tox_messages/obj_components.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/tox_contacts/components.hpp>

#include <solanaceae/util/utils.hpp>

#include <filesystem>
#include <string_view>

#include <iostream>

// see https://github.com/Tox/Tox-Client-Standard/blob/master/user_identification/avatar.md
// see (old) https://github.com/Tox-Archive/Tox-STS/blob/master/STS.md#avatars

// https://youtu.be/_uuCLRqc9QA

namespace Components {
	struct TagAvatarImageHandled {};
}

ToxAvatarManager::ToxAvatarManager(
	ObjectStore2& os,
	ContactStore4I& cs,
	ConfigModelI& conf
) : _os(os), _os_sr(_os.newSubRef(this)), _cs(cs), _conf(conf), _sb_tcs(os) {
	_os_sr
		.subscribe(ObjectStore_Event::object_construct)
		.subscribe(ObjectStore_Event::object_update)
		.subscribe(ObjectStore_Event::object_destroy)
	;

	if (!_conf.has_string("ToxAvatarManager", "save_path")) {
		// or on linux: $HOME/.config/tox/avatars/
		_conf.set("ToxAvatarManager", "save_path", std::string_view{"tmp_avatar_dir"});
	}

	const std::string_view avatar_save_path {_conf.get_string("ToxAvatarManager", "save_path").value()};
	// make sure it exists
	std::filesystem::create_directories(avatar_save_path);

	// TODO: instead listen for new contacts, and attach
	{ // scan tox contacts for cached avatars
		// old sts says pubkey.png

#if 0
		_cs.registry().view<Contact::Components::ToxFriendPersistent>().each([this](auto c, const Contact::Components::ToxFriendPersistent& tox_pers) {
			// try
			addAvatarFileToContact(c, tox_pers.key);
		});

		_cs.registry().view<Contact::Components::ToxGroupPersistent>().each([this](auto c, const Contact::Components::ToxGroupPersistent& tox_pers) {
			// try
			addAvatarFileToContact(c, tox_pers.chat_id);
		});
#else
		// HACK: assumed id is pubkey
		_cs.registry().view<Contact::Components::ID>().each([this](auto c, const Contact::Components::ID& id) {
			// try
			addAvatarFileToContact(c, id.data);
		});
#endif

		// TODO: also for group peers?
		// TODO: conf?
	}
}

void ToxAvatarManager::iterate(void) {
	// cancel queue

	// accept queue
	for (auto& [o, path] : _accept_queue) {
		if (o.any_of<ObjComp::Ephemeral::File::ActionTransferAccept, ObjComp::F::TagLocalHaveAll>()) {
			continue; // already accepted / done
		}

		o.emplace<ObjComp::Ephemeral::File::ActionTransferAccept>(path, true);
		std::cout << "TAM: auto accepted transfer\n";

		_os.throwEventUpdate(o);
	}
	_accept_queue.clear();

	//  - add message/content/transfer listener for onComplete
}

std::string ToxAvatarManager::getAvatarPath(const ToxKey& key) const {
	const std::string_view avatar_save_path {_conf.get_string("ToxAvatarManager", "save_path").value()};
	const auto file_path = std::filesystem::path(avatar_save_path) / getAvatarFileName(key);
	return file_path.generic_u8string();
}

std::string ToxAvatarManager::getAvatarFileName(const ToxKey& key) const {
	const auto pub_key_string = bin2hex({key.data.cbegin(), key.data.cend()});
	return pub_key_string + ".png"; // TODO: remove png?
}

void ToxAvatarManager::addAvatarFileToContact(const Contact4 c, const ToxKey& key) {
	const auto file_path = getAvatarPath(key);
	if (!std::filesystem::is_regular_file(file_path)) {
		return;
	}

	//std::cout << "TAM: found '" << file_path << "'\n";

	// TODO: use guid instead
	auto o = _sb_tcs.newObject(ByteSpan{key.data}, false);
	o.emplace_or_replace<ObjComp::F::SingleInfoLocal>(file_path);
	o.emplace_or_replace<ObjComp::F::TagLocalHaveAll>();

	// for file size
	o.emplace_or_replace<ObjComp::F::SingleInfo>(
		getAvatarFileName(key),
		std::filesystem::file_size(file_path)
	);

	_os.throwEventConstruct(o);

	// avatar file "png" exists
	//_cs.registry().emplace_or_replace<Contact::Components::AvatarFile>(c, file_path);
	_cs.registry().emplace_or_replace<Contact::Components::AvatarObj>(c, o);
	_cs.registry().emplace_or_replace<Contact::Components::TagAvatarInvalidate>(c);

	_cs.throwEventUpdate(c);
}

void ToxAvatarManager::clearAvatarFromContact(const Contact4 c) {
	auto& cr = _cs.registry();
	if (cr.any_of<Contact::Components::AvatarFile, Contact::Components::AvatarObj>(c)) {
		if (cr.all_of<Contact::Components::AvatarFile>(c)) {
			std::filesystem::remove(cr.get<Contact::Components::AvatarFile>(c).file_path);
		} else if (cr.all_of<Contact::Components::AvatarObj>(c)) {
			auto o = _os.objectHandle(cr.get<Contact::Components::AvatarObj>(c).obj);
			if (o) {
				if (o.all_of<ObjComp::F::SingleInfoLocal>()) {
					std::filesystem::remove(o.get<ObjComp::F::SingleInfoLocal>().file_path);
				}
				// TODO: make destruction more ergonomic
				//_sb_tcs.destroy() ??
				_os.throwEventDestroy(o);
				o.destroy();
			}
		}
		cr.remove<
			Contact::Components::AvatarFile,
			Contact::Components::AvatarObj
		>(c);
		cr.emplace_or_replace<Contact::Components::TagAvatarInvalidate>(c);

		_cs.throwEventUpdate(c);

		std::cout << "TAM: cleared avatar from " << entt::to_integral(c) << "\n";
	}
}

void ToxAvatarManager::checkObj(ObjectHandle o) {
	if (o.any_of<
		ObjComp::Ephemeral::File::ActionTransferAccept,
		Components::TagAvatarImageHandled
	>()) {
		return; // already accepted or handled
	}

	if (!o.any_of<
		ObjComp::Ephemeral::File::TagTransferPaused,
		ObjComp::F::TagLocalHaveAll
	>()) {
		// we only handle unaccepted or finished
		return;
	}

	// TODO: non tox code path?
	if (!o.all_of<
		ObjComp::Tox::TagIncomming,
		ObjComp::F::SingleInfo,
		ObjComp::Tox::FileKind,
		ObjComp::Ephemeral::ToxContact
	>()) {
		return;
	}

	// TCS-2.2.11 (big list, should have been sub points ...)

	if (o.get<ObjComp::Tox::FileKind>().kind != 1) {
		// not an avatar
		return;
	}

	const auto& file_info = o.get<ObjComp::F::SingleInfo>();

	//const auto contact = h.get<Message::Components::ContactFrom>().c;

	// TCS-2.2.4
	if (file_info.file_size > 65536ul) {
		// TODO: mark handled?
		return; // too large
	}

	auto contact = o.get<ObjComp::Ephemeral::ToxContact>().c;
	if (!static_cast<bool>(contact)) {
		std::cerr << "TAM error: failed to attribute object to contact\n";
	}

	// TCS-2.2.10
	if (file_info.file_name.empty() || file_info.file_size == 0) {
		std::cout << "TAM: no filename or filesize, deleting avatar\n";
		// reset
		clearAvatarFromContact(contact);
		// TODO: cancel
		return;
	}

	if (!o.all_of<
		ObjComp::Tox::FileID
	>()) {
		return;
	}

	if (o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		std::cout << "TAM: full avatar received\n";

		// old code no longer works, we already have the object (right here lol)

		contact.emplace_or_replace<Contact::Components::AvatarObj>(o);
		contact.emplace_or_replace<Contact::Components::TagAvatarInvalidate>();

		o.emplace_or_replace<Components::TagAvatarImageHandled>();
	} else if (!o.all_of<
		ObjComp::Ephemeral::BackendMeta, // hmm
		ObjComp::Ephemeral::BackendFile2,
		ObjComp::Ephemeral::BackendAtomic // to be safe
	>()) {
		std::string file_path;
		if (contact.all_of<Contact::Components::ToxFriendPersistent>()) {
			file_path = getAvatarPath(contact.get<Contact::Components::ToxFriendPersistent>().key);
		} else if (contact.all_of<Contact::Components::ToxGroupPersistent>()) {
			file_path = getAvatarPath(contact.get<Contact::Components::ToxGroupPersistent>().chat_id);
		} else {
			std::cerr << "TAM error: cant get toxkey for contact\n";
			// TODO: mark handled?
			return;
		}

		// already has avatar, delete old (TODO: or check hash)
		if (contact.all_of<Contact::Components::AvatarObj>()) {
			clearAvatarFromContact(contact);
		}

		// crude
		// TODO: queue/async instead
		// check file id for existing hash
		if (std::filesystem::is_regular_file(file_path)) {
			//const auto& supposed_file_hash = h.get<Message::Components::Transfer::FileID>().id;
			// load file
			// hash file
			//_t.toxHash();

			std::filesystem::remove(file_path); // hack, hard replace existing file
		}

		if (!_sb_tcs.attach(o)) {
			std::cerr << "TAM error: failed to attach backend??\n";
			return;
		}

		o.emplace_or_replace<ObjComp::F::SingleInfoLocal>(file_path);

		// ... do we do anything here?
		// like set "accepted" tag comp or something

		std::cout << "TAM: accepted avatar ft\n";
	}
}

bool ToxAvatarManager::onEvent(const ObjectStore::Events::ObjectConstruct& e) {
	checkObj(e.e);
	return false;
}

bool ToxAvatarManager::onEvent(const ObjectStore::Events::ObjectUpdate& e) {
	checkObj(e.e);
	return false;
}

bool ToxAvatarManager::onEvent(const ObjectStore::Events::ObjectDestory& e) {
	// TODO: avatar contact comp instead?
	// TODO: generic contact comp? (hard, very usecase dep)
#if 0
	if (!e.e.all_of<ObjComp::Ephemeral::File::Sender>()) {
		// cant be reasonable be attributed to a contact
		return false;
	}

	// TODO: remove obj from contact
#endif
	if (!e.e.all_of<ObjComp::Ephemeral::ToxContact>()) {
		// cant be reasonable be attributed to a contact
		return false;
	}

	auto c = e.e.get<ObjComp::Ephemeral::ToxContact>().c;
	if (!static_cast<bool>(c)) {
		// meh
		return false;
	}

	if (!c.all_of<Contact::Components::AvatarObj>()) {
		// funny
		return false;
	}

	if (c.get<Contact::Components::AvatarObj>().obj != e.e) {
		// maybe got replace?
		// TODO: make error and do proper cleanup
		return false;
	}

	c.remove<Contact::Components::AvatarObj>();
	c.emplace_or_replace<Contact::Components::TagAvatarInvalidate>();
	// TODO: throw contact update!!!

	return false;
}
