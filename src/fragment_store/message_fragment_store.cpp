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
		for (const auto& fuid : fuid_open) {
			auto fh = _fs.fragmentHandle(_fs.getFragmentByID(fuid.uid));
			assert(static_cast<bool>(fh));

			// assuming ts range exists
			auto& fts_comp = fh.get<FragComp::MessagesTSRange>();

			if (fts_comp.begin <= msg_ts && fts_comp.end >= msg_ts) {
				fragment_uid = fuid.uid;
				// TODO: check conditions for open here
				// TODO: mark msg (and frag?) dirty
			}
		}

		// if it did not fit into an existing fragment, we next look for fragments that could be extended
		if (fragment_uid.empty()) {
			for (const auto& fuid : fuid_open) {
				auto fh = _fs.fragmentHandle(_fs.getFragmentByID(fuid.uid));
				assert(static_cast<bool>(fh));

				// assuming ts range exists
				auto& fts_comp = fh.get<FragComp::MessagesTSRange>();

				const int64_t frag_range = int64_t(fts_comp.end) - int64_t(fts_comp.begin);
				constexpr static int64_t max_frag_ts_extent {1000*60*60};
				//constexpr static int64_t max_frag_ts_extent {1000*60*3}; // 3min for testing
				const int64_t possible_extention = max_frag_ts_extent - frag_range;

				// which direction
				if ((fts_comp.begin - possible_extention) <= msg_ts && fts_comp.begin > msg_ts) {
					fragment_uid = fuid.uid;

					std::cout << "MFS: extended begin from " << fts_comp.begin << " to " << msg_ts << "\n";

					// assuming ts range exists
					fts_comp.begin = msg_ts; // extend into the past


					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				} else if ((fts_comp.end + possible_extention) >= msg_ts && fts_comp.end < msg_ts) {
					fragment_uid = fuid.uid;

					std::cout << "MFS: extended end from " << fts_comp.end << " to " << msg_ts << "\n";

					// assuming ts range exists
					fts_comp.end = msg_ts; // extend into the future

					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				}
			}
		}

		// if its still not found, we need a new fragment
		if (fragment_uid.empty()) {
			const auto new_fid = _fs.newFragmentFile("test_message_store/", MetaFileType::BINARY_MSGPACK);
			auto fh = _fs.fragmentHandle(new_fid);
			if (!static_cast<bool>(fh)) {
				std::cout << "MFS error: failed to create new fragment for message\n";
				return;
			}

			fragment_uid = fh.get<FragComp::ID>().v;

			fh.emplace_or_replace<FragComp::Ephemeral::MetaCompressionType>().comp = Compression::ZSTD;
			fh.emplace_or_replace<FragComp::DataCompressionType>().comp = Compression::ZSTD;

			auto& new_ts_range = fh.emplace_or_replace<FragComp::MessagesTSRange>();
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

			fuid_open.emplace_back(Message::Components::OpenFragments::OpenFrag{fragment_uid});

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

		auto fh = _fs.fragmentHandle(fid);
		auto& ftsrange = fh.get_or_emplace<FragComp::MessagesTSRange>(Message::getTimeMS(), Message::getTimeMS());

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

			{ // potentially adjust tsrange (some external processes can change timestamps)
				const auto msg_ts = msg_view.get<Message::Components::Timestamp>(m).ts;
				if (ftsrange.begin > msg_ts) {
					ftsrange.begin = msg_ts;
				} else if (ftsrange.end < msg_ts) {
					ftsrange.end = msg_ts;
				}
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
					//std::cout << "missing " << storage.type().name() << "(" << type_id << ")\n";
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

static bool isLess(const std::vector<uint8_t>& lhs, const std::vector<uint8_t>& rhs) {
	size_t i = 0;
	for (; i < lhs.size() && i < rhs.size(); i++) {
		if (lhs[i] < rhs[i]) {
			return true;
		} else if (lhs[i] > rhs[i]) {
			return false;
		}
		// else continue
	}

	// here we have equality of common lenths

	// we define smaller arrays to be less
	return lhs.size() < rhs.size();
}

FragmentID MessageFragmentStore::fragmentBefore(FragmentID fid) {
	auto fh = _fs.fragmentHandle(fid);
	if (!fh.all_of<FragComp::MessagesTSRange>()) {
		return entt::null;
	}

	const auto& mtsrange = fh.get<FragComp::MessagesTSRange>();
	const auto& fuid = fh.get<FragComp::ID>();

	FragmentHandle current;

	auto mts_view = fh.registry()->view<FragComp::MessagesTSRange, FragComp::ID>();
	for (const auto& [it_f, it_mtsrange, it_fuid] : mts_view.each()) {
		// before means we compare end, so we dont jump over any

		if (it_mtsrange.end > mtsrange.end) {
			continue;
		}

		if (it_mtsrange.end == mtsrange.end) {
			// equal ts -> fallback to id
			if (!isLess(it_fuid.v, fuid.v)) {
				// we dont "care" about equality here, since that *should* never happen
				continue;
			}
		}

		// here we know that "it" is before
		// now we check for the largest, so the closest

		if (!static_cast<bool>(current)) {
			current = {*fh.registry(), it_f};
			continue;
		}

		const auto& curr_mtsrange = fh.get<FragComp::MessagesTSRange>();
		if (it_mtsrange.end < curr_mtsrange.end) {
			continue; // it is older than current selection
		}

		if (it_mtsrange.end == curr_mtsrange.end) {
			const auto& curr_fuid = fh.get<FragComp::ID>();
			// it needs to be larger to be considered (ignoring equality again)
			if (isLess(it_fuid.v, curr_fuid.v)) {
				continue;
			}
		}

		// new best fit
		current = {*fh.registry(), it_f};
	}

	return current;
}

FragmentID MessageFragmentStore::fragmentAfter(FragmentID fid) {
	auto fh = _fs.fragmentHandle(fid);
	if (!fh.all_of<FragComp::MessagesTSRange>()) {
		return entt::null;
	}

	const auto& mtsrange = fh.get<FragComp::MessagesTSRange>();
	const auto& fuid = fh.get<FragComp::ID>();

	FragmentHandle current;

	auto mts_view = fh.registry()->view<FragComp::MessagesTSRange, FragComp::ID>();
	for (const auto& [it_f, it_mtsrange, it_fuid] : mts_view.each()) {
		// after means we compare begin

		if (it_mtsrange.begin < mtsrange.begin) {
			continue;
		}

		if (it_mtsrange.begin == mtsrange.begin) {
			// equal ts -> fallback to id
			// id needs to be larger
			if (isLess(it_fuid.v, fuid.v)) {
				// we dont "care" about equality here, since that *should* never happen
				continue;
			}
		}

		// here we know that "it" is after
		// now we check for the smallest, so the closest

		if (!static_cast<bool>(current)) {
			current = {*fh.registry(), it_f};
			continue;
		}

		const auto& curr_mtsrange = fh.get<FragComp::MessagesTSRange>();
		if (it_mtsrange.begin > curr_mtsrange.begin) {
			continue; // it is newer than current selection
		}

		if (it_mtsrange.begin == curr_mtsrange.begin) {
			const auto& curr_fuid = fh.get<FragComp::ID>();
			// it needs to be smaller to be considered (ignoring equality again)
			if (!isLess(it_fuid.v, curr_fuid.v)) {
				continue;
			}
		}

		// new best fit
		current = {*fh.registry(), it_f};
	}

	return current;
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

