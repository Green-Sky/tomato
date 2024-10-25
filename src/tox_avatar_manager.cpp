#include "./tox_avatar_manager.hpp"

// TODO: this whole thing needs a rewrite and is currently disabled

#include <solanaceae/util/config_model.hpp>

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
	//RegistryMessageModel& rmm,
	ObjectStore2& os,
	Contact3Registry& cr,
	ConfigModelI& conf
) : /*_rmm(rmm)*/ _os(os), _os_sr(_os.newSubRef(this)), _cr(cr), _conf(conf) {
	_os_sr
		.subscribe(ObjectStore_Event::object_construct)
		.subscribe(ObjectStore_Event::object_update)
	;

	if (!_conf.has_string("ToxAvatarManager", "save_path")) {
		// or on linux: $HOME/.config/tox/avatars/
		_conf.set("ToxAvatarManager", "save_path", std::string_view{"tmp_avatar_dir"});
	}

	//_conf.set("TransferAutoAccept", "autoaccept_limit", int64_t(50l*1024l*1024l)); // sane default

	const std::string_view avatar_save_path {_conf.get_string("ToxAvatarManager", "save_path").value()};
	// make sure it exists
	std::filesystem::create_directories(avatar_save_path);

	{ // scan tox contacts for cached avatars
		// old sts says pubkey.png

		_cr.view<Contact::Components::ToxFriendPersistent>().each([this](auto c, const Contact::Components::ToxFriendPersistent& tox_pers) {
			addAvatarFileToContact(c, tox_pers.key);
		});

		_cr.view<Contact::Components::ToxGroupPersistent>().each([this](auto c, const Contact::Components::ToxGroupPersistent& tox_pers) {
			addAvatarFileToContact(c, tox_pers.chat_id);
		});

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
	const auto pub_key_string = bin2hex({key.data.cbegin(), key.data.cend()});
	const auto file_path = std::filesystem::path(avatar_save_path) / (pub_key_string + ".png");
	return file_path.generic_u8string();
}

void ToxAvatarManager::addAvatarFileToContact(const Contact3 c, const ToxKey& key) {
	const auto file_path = getAvatarPath(key);
	if (std::filesystem::is_regular_file(file_path)) {
		// avatar file png file exists
		_cr.emplace_or_replace<Contact::Components::AvatarFile>(c, file_path);
		_cr.emplace_or_replace<Contact::Components::TagAvatarInvalidate>(c);
	}
}

void ToxAvatarManager::clearAvatarFromContact(const Contact3 c) {
	if (_cr.all_of<Contact::Components::AvatarFile>(c)) {
		std::filesystem::remove(_cr.get<Contact::Components::AvatarFile>(c).file_path);
		_cr.remove<Contact::Components::AvatarFile>(c);
		_cr.emplace_or_replace<Contact::Components::TagAvatarInvalidate>(c);
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

	if (!o.all_of<
		ObjComp::Tox::TagIncomming,
		//ObjComp::Ephemeral::Backend,
		ObjComp::F::SingleInfo,
		ObjComp::Tox::FileKind
		// TODO: mesage? how do we know where a file is from??
		//Message::Components::ContactFrom // should always be there, just making sure
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

#if 0 // TODO: make avatars work again !!!!!

	// TCS-2.2.10
	if (file_info.file_name.empty() || file_info.file_size == 0) {
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

	std::string file_path;
	if (_cr.all_of<Contact::Components::ToxFriendPersistent>(contact)) {
		file_path = getAvatarPath(_cr.get<Contact::Components::ToxFriendPersistent>(contact).key);
	} else if (_cr.all_of<Contact::Components::ToxGroupPersistent>(contact)) {
		file_path = getAvatarPath(_cr.get<Contact::Components::ToxGroupPersistent>(contact).chat_id);
	} else {
		std::cerr << "TAM error: cant get toxkey for contact\n";
		// TODO: mark handled?
		return;
	}

	if (o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		std::cout << "TAM: full avatar received\n";

		if (_cr.all_of<Contact::Components::ToxFriendPersistent>(contact)) {
			addAvatarFileToContact(contact, _cr.get<Contact::Components::ToxFriendPersistent>(contact).key);
		} else if (_cr.all_of<Contact::Components::ToxGroupPersistent>(contact)) {
			addAvatarFileToContact(contact, _cr.get<Contact::Components::ToxGroupPersistent>(contact).chat_id);
		} else {
			std::cerr << "TAM error: cant get toxkey for contact\n";
		}

		o.emplace_or_replace<Components::TagAvatarImageHandled>();
	} else {
		// check file id for existing hash
		if (std::filesystem::is_regular_file(file_path)) {
			//const auto& supposed_file_hash = h.get<Message::Components::Transfer::FileID>().id;
			// load file
			// hash file
			//_t.toxHash();

			std::filesystem::remove(file_path); // hack, hard replace existing file
		}

		std::cout << "TAM: accepted avatar ft\n";

		// if not already on disk
		_accept_queue.push_back(AcceptEntry{o, file_path});
	}
#endif
}

bool ToxAvatarManager::onEvent(const ObjectStore::Events::ObjectConstruct& e) {
	checkObj(e.e);
	return false;
}

bool ToxAvatarManager::onEvent(const ObjectStore::Events::ObjectUpdate& e) {
	checkObj(e.e);
	return false;
}

