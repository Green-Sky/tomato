#include "./message_fragment_store.hpp"

#include <solanaceae/message3/components.hpp>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cassert>

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

void MessageFragmentStore::handleMessage(const Message3Handle& m) {
	if (!static_cast<bool>(m)) {
		return; // huh?
	}

	if (!m.all_of<Message::Components::Timestamp>()) {
		return; // we only handle msg with ts
	}

	const auto msg_ts = m.get<Message::Components::Timestamp>().ts;

	if (!m.all_of<Message::Components::FUID>()) {
		// missing fuid
		// find closesed non-sealed off fragment

		std::vector<uint8_t> fragment_uid;

		// first search for fragment where the ts falls into the range
		for (const auto& [tsrage, fid] : _fuid_open) {
			if (tsrage.ts_begin <= msg_ts && tsrage.ts_end >= msg_ts) {
				fragment_uid = fid;
				// TODO: check conditions for open here
				// TODO: mark msg (and frag?) dirty
			}
		}

		// if it did not fit into an existing fragment, we next look for fragments that could be extended
		if (fragment_uid.empty()) {
			for (const auto& [tsrage, fid] : _fuid_open) {
				const int64_t frag_range = int64_t(tsrage.ts_end) - int64_t(tsrage.ts_begin);
				//constexpr static int64_t max_frag_ts_extent {1000*60*60};
				constexpr static int64_t max_frag_ts_extent {1000*60*3}; // 3min for testing
				const int64_t possible_extention = max_frag_ts_extent - frag_range;

				// which direction
				if ((tsrage.ts_begin - possible_extention) <= msg_ts) {
					fragment_uid = fid;
					auto fh = _fs.fragmentHandle(_fs.getFragmentByID(fid));
					assert(static_cast<bool>(fh));

					// assuming ts range exists
					auto& fts_comp = fh.get<FragComp::MessagesTSRange>();
					fts_comp.begin = msg_ts; // extend into the past

					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				} else if ((tsrage.ts_end + possible_extention) >= msg_ts) {
					fragment_uid = fid;
					auto fh = _fs.fragmentHandle(_fs.getFragmentByID(fid));
					assert(static_cast<bool>(fh));

					// assuming ts range exists
					auto& fts_comp = fh.get<FragComp::MessagesTSRange>();
					fts_comp.end = msg_ts; // extend into the future

					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				}
			}
		}

		// if its still not found, we need a new fragment
		if (fragment_uid.empty()) {
		}

		// if this is still empty, something is very wrong and we exit here
		if (fragment_uid.empty()) {
			return;
		}

		m.emplace<Message::Components::FUID>(fragment_uid);
	}

	// TODO: do we use fid?

	// on new message: assign fuid
	// on new and update: mark as fragment dirty
}

MessageFragmentStore::MessageFragmentStore(
	RegistryMessageModel& rmm,
	FragmentStore& fs
) : _rmm(rmm), _fs(fs) {
	_rmm.subscribe(this, RegistryMessageModel_Event::message_construct);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_updated);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_destroy);

	_fs._sc.registerSerializerJson<FragComp::MessagesTSRange>(serl_json_msg_ts_range);
}

MessageFragmentStore::~MessageFragmentStore(void) {
}

float MessageFragmentStore::tick(float time_delta) {
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

