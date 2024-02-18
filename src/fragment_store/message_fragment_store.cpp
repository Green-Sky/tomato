#include "./message_fragment_store.hpp"

#include "../json/message_components.hpp"

#include <solanaceae/util/utils.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/message3/contact_components.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <cstdint>
#include <cassert>
#include <iostream>

// https://youtu.be/CU2exyhYPfA

namespace Message::Components {

	// ctx
	struct OpenFragments {
		struct OpenFrag final {
			uint64_t ts_begin {0};
			uint64_t ts_end {0};
			std::vector<uint8_t> uid;
		};
		// only contains fragments with <1024 messages and <28h tsrage
		// TODO: this needs to move into the message reg
		std::vector<OpenFrag> fuid_open;
	};

} // Message::Components

namespace Fragment::Components {
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessagesTSRange, begin, end)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessagesContact, id)
} // Fragment::Components

void MessageFragmentStore::handleMessage(const Message3Handle& m) {
	if (_fs_ignore_event) {
		// message event because of us loading a fragment, ignore
		// TODO: this barely makes a difference
		return;
	}

	if (!static_cast<bool>(m)) {
		return; // huh?
	}

	if (!m.all_of<Message::Components::Timestamp>()) {
		return; // we only handle msg with ts
	}

	if (!m.registry()->ctx().contains<Message::Components::OpenFragments>()) {
		// first message in this reg
		m.registry()->ctx().emplace<Message::Components::OpenFragments>();

		// TODO: move this to async
		// new reg -> load all fragments for this contact (for now, ranges later)
		for (const auto& [fid, tsrange, fmc] : _fs._reg.view<FragComp::MessagesTSRange, FragComp::MessagesContact>().each()) {
			Contact3 frag_contact = entt::null;
			// TODO: id lookup table, this is very inefficent
			for (const auto& [c_it, id_it] : _cr.view<Contact::Components::ID>().each()) {
				if (fmc.id == id_it.data) {
					//h.emplace_or_replace<Message::Components::ContactTo>(c_it);
					//return true;
					frag_contact = c_it;
					break;
				}
			}
			if (!_cr.valid(frag_contact)) {
				// unkown contact
				continue;
			}

			// registry is the same as the one the message event is for
			if (static_cast<const RegistryMessageModel&>(_rmm).get(frag_contact) == m.registry()) {
				loadFragment(*m.registry(), FragmentHandle{_fs._reg, fid});
			}
		}
	}

	auto& fuid_open = m.registry()->ctx().get<Message::Components::OpenFragments>().fuid_open;

	const auto msg_ts = m.get<Message::Components::Timestamp>().ts;

	if (!m.all_of<Message::Components::FUID>()) {
		// missing fuid
		// find closesed non-sealed off fragment

		std::vector<uint8_t> fragment_uid;

		// first search for fragment where the ts falls into the range
		for (const auto& [ts_begin, ts_end, fid] : fuid_open) {
			if (ts_begin <= msg_ts && ts_end >= msg_ts) {
				fragment_uid = fid;
				// TODO: check conditions for open here
				// TODO: mark msg (and frag?) dirty
			}
		}

		// if it did not fit into an existing fragment, we next look for fragments that could be extended
		if (fragment_uid.empty()) {
			for (auto& [ts_begin, ts_end, fid] : fuid_open) {
				const int64_t frag_range = int64_t(ts_end) - int64_t(ts_begin);
				//constexpr static int64_t max_frag_ts_extent {1000*60*60};
				constexpr static int64_t max_frag_ts_extent {1000*60*3}; // 3min for testing
				const int64_t possible_extention = max_frag_ts_extent - frag_range;

				// which direction
				if ((ts_begin - possible_extention) <= msg_ts && ts_begin > msg_ts) {
					fragment_uid = fid;
					auto fh = _fs.fragmentHandle(_fs.getFragmentByID(fid));
					assert(static_cast<bool>(fh));

					std::cout << "MFS: extended begin from " << ts_begin << " to " << msg_ts << "\n";

					// assuming ts range exists
					auto& fts_comp = fh.get<FragComp::MessagesTSRange>();
					fts_comp.begin = msg_ts; // extend into the past
					ts_begin = msg_ts;


					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				} else if ((ts_end + possible_extention) >= msg_ts && ts_end < msg_ts) {
					fragment_uid = fid;
					auto fh = _fs.fragmentHandle(_fs.getFragmentByID(fid));
					assert(static_cast<bool>(fh));

					std::cout << "MFS: extended end from " << ts_end << " to " << msg_ts << "\n";

					// assuming ts range exists
					auto& fts_comp = fh.get<FragComp::MessagesTSRange>();
					fts_comp.end = msg_ts; // extend into the future
					ts_end = msg_ts;

					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				}
			}
		}

		// if its still not found, we need a new fragment
		if (fragment_uid.empty()) {
			const auto new_fid = _fs.newFragmentFile("test_message_store/", MetaFileType::TEXT_JSON);
			auto fh = _fs.fragmentHandle(new_fid);
			if (!static_cast<bool>(fh)) {
				std::cout << "MFS error: failed to create new fragment for message\n";
				return;
			}

			fragment_uid = fh.get<FragComp::ID>().v;

			fh.emplace_or_replace<FragComp::DataCompressionType>().comp = Compression::ZSTD;

			auto& new_ts_range = fh.emplace<FragComp::MessagesTSRange>();
			new_ts_range.begin = msg_ts;
			new_ts_range.end = msg_ts;

			{
				const auto msg_reg_contact = m.registry()->ctx().get<Contact3>();
				if (_cr.all_of<Contact::Components::ID>(msg_reg_contact)) {
					fh.emplace<FragComp::MessagesContact>(_cr.get<Contact::Components::ID>(msg_reg_contact).data);
				} else {
					// ? rage quit?
				}
			}

			fuid_open.emplace_back(Message::Components::OpenFragments::OpenFrag{msg_ts, msg_ts, fragment_uid});

			std::cout << "MFS: created new fragment " << bin2hex(fragment_uid) << "\n";

			_fs_ignore_event = true;
			_fs.throwEventConstruct(fh);
			_fs_ignore_event = false;
		}

		// if this is still empty, something is very wrong and we exit here
		if (fragment_uid.empty()) {
			std::cout << "MFS error: failed to find/create fragment for message\n";
			return;
		}

		m.emplace<Message::Components::FUID>(fragment_uid);

		// in this case we know the fragment needs an update
		_fuid_save_queue.push({Message::getTimeMS(), fragment_uid, m.registry()});
	}

	// TODO: do we use fid?

	// on new message: assign fuid
	// on new and update: mark as fragment dirty
}

// assumes new frag
// need update from frag
void MessageFragmentStore::loadFragment(Message3Registry& reg, FragmentHandle fh) {
	std::cout << "MFS: loadFragment\n";
	const auto j = _fs.loadFromStorageNJ(fh);

	if (!j.is_array()) {
		// wrong data
		return;
	}

	for (const auto& j_entry : j) {
		auto new_real_msg = Message3Handle{reg, reg.create()};
		// load into staging reg
		for (const auto& [k, v] : j_entry.items()) {
			//std::cout << "K:" << k << " V:" << v.dump() << "\n";
			const auto type_id = entt::hashed_string(k.data(), k.size());
			const auto deserl_fn_it = _sc._deserl_json.find(type_id);
			if (deserl_fn_it != _sc._deserl_json.cend()) {
				try {
					if (!deserl_fn_it->second(_sc, new_real_msg, v)) {
						std::cerr << "MFS error: failed deserializing '" << k << "'\n";
					}
				} catch(...) {
					std::cerr << "MFS error: failed deserializing (threw) '" << k << "'\n";
				}
			} else {
				std::cerr << "MFS warning: missing deserializer for meta key '" << k << "'\n";
			}
		}

		new_real_msg.emplace_or_replace<Message::Components::FUID>(fh.get<FragComp::ID>());

		// dup check (hacky, specific to protocols)
		Message3 dup_msg {entt::null};
		{
			// get comparator from contact
			if (reg.ctx().contains<Contact3>()) {
				const auto c = reg.ctx().get<Contact3>();
				if (_cr.all_of<Contact::Components::MessageIsSame>(c)) {
					auto& comp = _cr.get<Contact::Components::MessageIsSame>(c).comp;
					// walking EVERY existing message OOF
					// this needs optimizing
					for (const Message3 other_msg : reg.view<Message::Components::Timestamp, Message::Components::ContactFrom, Message::Components::ContactTo>()) {
						if (other_msg == new_real_msg) {
							continue; // skip self
						}

						if (comp({reg, other_msg}, new_real_msg)) {
							// dup
							dup_msg = other_msg;
							break;
						}
					}
				}
			}
		}

		if (reg.valid(dup_msg)) {
			//  -> merge with preexisting
			//  -> throw update
			reg.destroy(new_real_msg);
			//_rmm.throwEventUpdate(reg, new_real_msg);
		} else {
			if (!new_real_msg.all_of<Message::Components::Timestamp, Message::Components::ContactFrom, Message::Components::ContactTo>()) {
				// does not have needed components to be stand alone
				reg.destroy(new_real_msg);
				std::cerr << "MFS warning: message with missing basic compoments\n";
				continue;
			}

			//  -> throw create
			_rmm.throwEventConstruct(reg, new_real_msg);
		}
	}
}

MessageFragmentStore::MessageFragmentStore(
	Contact3Registry& cr,
	RegistryMessageModel& rmm,
	FragmentStore& fs
) : _cr(cr), _rmm(rmm), _fs(fs), _sc{_cr, {}, {}} {
	_rmm.subscribe(this, RegistryMessageModel_Event::message_construct);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_updated);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_destroy);

	_fs._sc.registerSerializerJson<FragComp::MessagesTSRange>();
	_fs._sc.registerDeSerializerJson<FragComp::MessagesTSRange>();
	_fs._sc.registerSerializerJson<FragComp::MessagesContact>();
	_fs._sc.registerDeSerializerJson<FragComp::MessagesContact>();

	_fs.subscribe(this, FragmentStore_Event::fragment_construct);
}

MessageFragmentStore::~MessageFragmentStore(void) {
	// TODO: sync all dirty fragments
}

MessageSerializerCallbacks& MessageFragmentStore::getMSC(void) {
	return _sc;
}

float MessageFragmentStore::tick(float time_delta) {
	// sync dirty fragments here

	if (!_fuid_save_queue.empty()) {
		const auto fid = _fs.getFragmentByID(_fuid_save_queue.front().id);
		auto* reg = _fuid_save_queue.front().reg;
		assert(reg != nullptr);
		auto j = nlohmann::json::array();

		// TODO: does every message have ts?
		auto msg_view = reg->view<Message::Components::Timestamp>();
		// we also assume all messages have fuid (hack: call handle when not?)
		for (auto it = msg_view.rbegin(), it_end = msg_view.rend(); it != it_end; it++) {
			const Message3 m = *it;

			if (!reg->all_of<Message::Components::FUID, Message::Components::ContactFrom, Message::Components::ContactTo>(m)) {
				continue;
			}

			// require msg for now
			if (!reg->any_of<Message::Components::MessageText/*, Message::Components::Transfer::FileInfo*/>(m)) {
				continue;
			}

			if (_fuid_save_queue.front().id != reg->get<Message::Components::FUID>(m).v) {
				continue; // not ours
			}

			auto& j_entry = j.emplace_back(nlohmann::json::object());

			for (const auto& [type_id, storage] : reg->storage()) {
				if (!storage.contains(m)) {
					continue;
				}

				//std::cout << "storage type: type_id:" << type_id << " name:" << storage.type().name() << "\n";

				// use type_id to find serializer
				auto s_cb_it = _sc._serl_json.find(type_id);
				if (s_cb_it == _sc._serl_json.end()) {
					// could not find serializer, not saving
					std::cout << "missing " << storage.type().name() << "(" << type_id << ")\n";
					continue;
				}

				s_cb_it->second(_sc, {*reg, m}, j_entry[storage.type().name()]);
			}
		}

		// if save as binary
		//nlohmann::json::to_msgpack(j);
		auto j_dump = j.dump(2, ' ', true);
		if (_fs.syncToStorage(fid, reinterpret_cast<const uint8_t*>(j_dump.data()), j_dump.size())) {
			//std::cout << "MFS: dumped " << j_dump << "\n";
			// succ
			_fuid_save_queue.pop();
		}
	}

	return 1000.f*60.f*60.f;
}

void MessageFragmentStore::triggerScan(void) {
	_fs.scanStoragePath("test_message_store/");
}

bool MessageFragmentStore::onEvent(const Message::Events::MessageConstruct& e) {
	handleMessage(e.e);
	return false;
}

bool MessageFragmentStore::onEvent(const Message::Events::MessageUpdated& e) {
	handleMessage(e.e);
	return false;
}

// TODO: handle deletes? diff between unload?

bool MessageFragmentStore::onEvent(const Fragment::Events::FragmentConstruct& e) {
	if (_fs_ignore_event) {
		return false; // skip self
	}

	if (!e.e.all_of<FragComp::MessagesTSRange, FragComp::MessagesContact>()) {
		return false; // not for us
	}

	// TODO: are we sure it is a *new* fragment?

	//std::cout << "MFS: got frag for us!\n";

	Contact3 frag_contact = entt::null;
	{ // get contact
		const auto& frag_contact_id = e.e.get<FragComp::MessagesContact>().id;
		// TODO: id lookup table, this is very inefficent
		for (const auto& [c_it, id_it] : _cr.view<Contact::Components::ID>().each()) {
			if (frag_contact_id == id_it.data) {
				//h.emplace_or_replace<Message::Components::ContactTo>(c_it);
				//return true;
				frag_contact = c_it;
				break;
			}
		}
		if (!_cr.valid(frag_contact)) {
			// unkown contact
			return false;
		}
	}

	// only load if msg reg open
	auto* msg_reg = static_cast<const RegistryMessageModel&>(_rmm).get(frag_contact);
	if (msg_reg == nullptr) {
		// msg reg not created yet
		return false;
	}

	// TODO: should this be done async / on tick() instead of on event?
	loadFragment(*msg_reg, e.e);

	return false;
}

