#include "./message_fragment_store.hpp"

#include <solanaceae/util/utils.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cassert>
#include <iostream>

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

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Timestamp, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TimestampProcessed, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TimestampWritten, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ContactFrom, c)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ContactTo, c)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Read, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessageText, text)

	namespace Transfer {
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileInfo::FileDirEntry, file_name, file_size)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileInfo, file_list, total_size)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileInfoLocal, file_list)
	} // Transfer

} // Message::Components

namespace Fragment::Components {
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessagesTSRange, begin, end)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessagesContact, id)
} // Fragment::Components

template<typename T>
static bool serl_json_default(void* comp, nlohmann::json& out) {
	if constexpr (!std::is_empty_v<T>) {
		out = *reinterpret_cast<T*>(comp);
	} // do nothing if empty type
	return true;
}

static bool serl_json_msg_ts_range(void* comp, nlohmann::json& out) {
	if (comp == nullptr) {
		return false;
	}

	out = nlohmann::json::object();

	auto& r_comp = *reinterpret_cast<FragComp::MessagesTSRange*>(comp);

	out["begin"] = r_comp.begin;
	out["end"] = r_comp.end;

	return true;
}

static bool serl_json_msg_c_id(void* comp, nlohmann::json& out) {
	if (comp == nullptr) {
		return false;
	}

	out = nlohmann::json::object();

	auto& r_comp = *reinterpret_cast<FragComp::MessagesContact*>(comp);

	out["id"] = r_comp.id;

	return true;
}

void MessageFragmentStore::handleMessage(const Message3Handle& m) {
	if (!static_cast<bool>(m)) {
		return; // huh?
	}

	if (!m.all_of<Message::Components::Timestamp>()) {
		return; // we only handle msg with ts
	}

	if (!m.registry()->ctx().contains<Message::Components::OpenFragments>()) {
		// first message in this reg
		m.registry()->ctx().emplace<Message::Components::OpenFragments>();
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

MessageFragmentStore::MessageFragmentStore(
	Contact3Registry& cr,
	RegistryMessageModel& rmm,
	FragmentStore& fs
) : _cr(cr), _rmm(rmm), _fs(fs) {
	_rmm.subscribe(this, RegistryMessageModel_Event::message_construct);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_updated);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_destroy);

	_fs._sc.registerSerializerJson<FragComp::MessagesTSRange>(serl_json_msg_ts_range);
	_fs._sc.registerDeSerializerJson<FragComp::MessagesTSRange>();
	_fs._sc.registerSerializerJson<FragComp::MessagesContact>(serl_json_msg_c_id);
	_fs._sc.registerDeSerializerJson<FragComp::MessagesContact>();

	_sc.registerSerializerJson<Message::Components::Timestamp>(serl_json_default<Message::Components::Timestamp>);
	_sc.registerDeSerializerJson<Message::Components::Timestamp>();
	_sc.registerSerializerJson<Message::Components::TimestampProcessed>(serl_json_default<Message::Components::TimestampProcessed>);
	_sc.registerDeSerializerJson<Message::Components::TimestampProcessed>();
	_sc.registerSerializerJson<Message::Components::TimestampWritten>(serl_json_default<Message::Components::TimestampWritten>);
	_sc.registerDeSerializerJson<Message::Components::TimestampWritten>();
	_sc.registerSerializerJson<Message::Components::ContactFrom>(serl_json_default<Message::Components::ContactFrom>);
	_sc.registerDeSerializerJson<Message::Components::ContactFrom>();
	_sc.registerSerializerJson<Message::Components::ContactTo>(serl_json_default<Message::Components::ContactTo>);
	_sc.registerDeSerializerJson<Message::Components::ContactTo>();
	_sc.registerSerializerJson<Message::Components::TagUnread>(serl_json_default<Message::Components::TagUnread>);
	_sc.registerDeSerializerJson<Message::Components::TagUnread>();
	_sc.registerSerializerJson<Message::Components::Read>(serl_json_default<Message::Components::Read>);
	_sc.registerDeSerializerJson<Message::Components::Read>();
	_sc.registerSerializerJson<Message::Components::MessageText>(serl_json_default<Message::Components::MessageText>);
	_sc.registerDeSerializerJson<Message::Components::MessageText>();
	_sc.registerSerializerJson<Message::Components::TagMessageIsAction>(serl_json_default<Message::Components::TagMessageIsAction>);
	_sc.registerDeSerializerJson<Message::Components::TagMessageIsAction>();

	// files
	//_sc.registerSerializerJson<Message::Components::Transfer::FileID>()
	_sc.registerSerializerJson<Message::Components::Transfer::FileInfo>(serl_json_default<Message::Components::Transfer::FileInfo>);
	_sc.registerDeSerializerJson<Message::Components::Transfer::FileInfo>();
	_sc.registerSerializerJson<Message::Components::Transfer::FileInfoLocal>(serl_json_default<Message::Components::Transfer::FileInfoLocal>);
	_sc.registerDeSerializerJson<Message::Components::Transfer::FileInfoLocal>();
	_sc.registerSerializerJson<Message::Components::Transfer::TagHaveAll>(serl_json_default<Message::Components::Transfer::TagHaveAll>);
	_sc.registerDeSerializerJson<Message::Components::Transfer::TagHaveAll>();

	_fs.scanStoragePath("test_message_store/");
}

MessageFragmentStore::~MessageFragmentStore(void) {
	// TODO: sync all dirty fragments
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
		for (const Message3 m : msg_view) {
			if (!reg->all_of<Message::Components::FUID, Message::Components::ContactFrom, Message::Components::ContactTo>(m)) {
				continue;
			}

			if (!reg->any_of<Message::Components::MessageText, Message::Components::Transfer::FileInfo>(m)) {
				continue;
			}

			auto& j_entry = j.emplace_back(nlohmann::json::object());

			for (const auto& [type_id, storage] : reg->storage()) {
				if (!storage.contains(m)) {
					continue;
				}

				std::cout << "storage type: type_id:" << type_id << " name:" << storage.type().name() << "\n";

				// use type_id to find serializer
				auto s_cb_it = _sc._serl_json.find(type_id);
				if (s_cb_it == _sc._serl_json.end()) {
					// could not find serializer, not saving
					std::cout << "missing " << storage.type().name() << "\n";
					continue;
				}

				s_cb_it->second(storage.value(m), j_entry[storage.type().name()]);
			}
		}

		// if save as binary
		//nlohmann::json::to_msgpack(j);
		auto j_dump = j.dump(2, ' ', true);
		if (_fs.syncToStorage(fid, reinterpret_cast<const uint8_t*>(j_dump.data()), j_dump.size())) {
			std::cout << "MFS: dumped " << j_dump << "\n";
			// succ
			_fuid_save_queue.pop();
		}
	}

	return 1000.f*60.f*60.f;
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

