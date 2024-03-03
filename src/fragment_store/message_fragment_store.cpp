#include "./message_fragment_store.hpp"

#include "../json/message_components.hpp"

#include <solanaceae/util/utils.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/message3/contact_components.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>
#include <cstdint>
#include <cassert>
#include <iostream>

// https://youtu.be/CU2exyhYPfA

// everything assumes a single fragment registry

namespace Message::Components {

	// ctx
	struct OpenFragments {
		//struct OpenFrag final {
			////std::vector<uint8_t> uid;
			//FragmentID id;
		//};
		// only contains fragments with <1024 messages and <28h tsrage (or whatever)
		entt::dense_set<FragmentID> fid_open;
	};

	// all message fragments of this contact
	struct ContactFragments final {
		// kept up-to-date by events
		struct InternalEntry {
			// indecies into the sorted arrays
			size_t i_b;
			size_t i_e;
		};
		entt::dense_map<FragmentID, InternalEntry> frags;

		// add 2 sorted contact lists for both range begin and end
		// TODO: adding and removing becomes expensive with enough frags, consider splitting or heap
		std::vector<FragmentID> sorted_begin;
		std::vector<FragmentID> sorted_end;

		// api
		// return true if it was actually inserted
		bool insert(FragmentHandle frag);
		bool erase(FragmentID frag);
		// update? (just erase() + insert())

		// uses range begin to go back in time
		FragmentID prev(FragmentID frag) const;
		// uses range end to go forward in time
		FragmentID next(FragmentID frag) const;
	};

	// all LOADED message fragments
	// TODO: merge into ContactFragments (and pull in openfrags)
	struct LoadedContactFragments final {
		// kept up-to-date by events
		entt::dense_set<FragmentID> frags;
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

	_potentially_dirty_contacts.emplace(m.registry()->ctx().get<Contact3>()); // always mark dirty here
	if (m.any_of<Message::Components::ViewCurserBegin, Message::Components::ViewCurserEnd>()) {
		// not an actual message, but we probalby need to check and see if we need to load fragments
		//std::cout << "MFS: new or updated curser\n";
		return;
	}

	// TODO: use fid, seving full fuid for every message consumes alot of memory (and heap frag)
	if (!m.all_of<Message::Components::FID>()) {
		std::cout << "MFS: new msg missing FID\n";
		if (!m.registry()->ctx().contains<Message::Components::OpenFragments>()) {
			m.registry()->ctx().emplace<Message::Components::OpenFragments>();
		}

		auto& fid_open = m.registry()->ctx().get<Message::Components::OpenFragments>().fid_open;

		const auto msg_ts = m.get<Message::Components::Timestamp>().ts;
		// missing fuid
		// find closesed non-sealed off fragment

		FragmentID fragment_id{entt::null};

		// first search for fragment where the ts falls into the range
		for (const auto& fid : fid_open) {
			auto fh = _fs.fragmentHandle(fid);
			assert(static_cast<bool>(fh));

			// assuming ts range exists
			auto& fts_comp = fh.get<FragComp::MessagesTSRange>();

			if (fts_comp.begin <= msg_ts && fts_comp.end >= msg_ts) {
				fragment_id = fid;
				// TODO: check conditions for open here
				// TODO: mark msg (and frag?) dirty
			}
		}

		// if it did not fit into an existing fragment, we next look for fragments that could be extended
		if (!_fs._reg.valid(fragment_id)) {
			for (const auto& fid : fid_open) {
				auto fh = _fs.fragmentHandle(fid);
				assert(static_cast<bool>(fh));

				// assuming ts range exists
				auto& fts_comp = fh.get<FragComp::MessagesTSRange>();

				const int64_t frag_range = int64_t(fts_comp.end) - int64_t(fts_comp.begin);
				constexpr static int64_t max_frag_ts_extent {1000*60*60};
				//constexpr static int64_t max_frag_ts_extent {1000*60*3}; // 3min for testing
				const int64_t possible_extention = max_frag_ts_extent - frag_range;

				// which direction
				if ((fts_comp.begin - possible_extention) <= msg_ts && fts_comp.begin > msg_ts) {
					fragment_id = fid;

					std::cout << "MFS: extended begin from " << fts_comp.begin << " to " << msg_ts << "\n";

					// assuming ts range exists
					fts_comp.begin = msg_ts; // extend into the past

					if (m.registry()->ctx().contains<Message::Components::ContactFragments>()) {
						// should be the case
						m.registry()->ctx().get<Message::Components::ContactFragments>().erase(fh);
						m.registry()->ctx().get<Message::Components::ContactFragments>().insert(fh);
					}


					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				} else if ((fts_comp.end + possible_extention) >= msg_ts && fts_comp.end < msg_ts) {
					fragment_id = fid;

					std::cout << "MFS: extended end from " << fts_comp.end << " to " << msg_ts << "\n";

					// assuming ts range exists
					fts_comp.end = msg_ts; // extend into the future

					if (m.registry()->ctx().contains<Message::Components::ContactFragments>()) {
						// should be the case
						m.registry()->ctx().get<Message::Components::ContactFragments>().erase(fh);
						m.registry()->ctx().get<Message::Components::ContactFragments>().insert(fh);
					}

					// TODO: check conditions for open here
					// TODO: mark msg (and frag?) dirty
				}
			}
		}

		// if its still not found, we need a new fragment
		if (!_fs._reg.valid(fragment_id)) {
			const auto new_fid = _fs.newFragmentFile("test_message_store/", MetaFileType::BINARY_MSGPACK);
			auto fh = _fs.fragmentHandle(new_fid);
			if (!static_cast<bool>(fh)) {
				std::cout << "MFS error: failed to create new fragment for message\n";
				return;
			}

			fragment_id = fh;

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

			// contact frag
			if (!m.registry()->ctx().contains<Message::Components::ContactFragments>()) {
				m.registry()->ctx().emplace<Message::Components::ContactFragments>();
			}
			m.registry()->ctx().get<Message::Components::ContactFragments>().insert(fh);

			// loaded contact frag
			if (!m.registry()->ctx().contains<Message::Components::LoadedContactFragments>()) {
				m.registry()->ctx().emplace<Message::Components::LoadedContactFragments>();
			}
			m.registry()->ctx().get<Message::Components::LoadedContactFragments>().frags.emplace(fh);

			fid_open.emplace(fragment_id);

			std::cout << "MFS: created new fragment " << bin2hex(fh.get<FragComp::ID>().v) << "\n";

			_fs_ignore_event = true;
			_fs.throwEventConstruct(fh);
			_fs_ignore_event = false;
		}

		// if this is still empty, something is very wrong and we exit here
		if (!_fs._reg.valid(fragment_id)) {
			std::cout << "MFS error: failed to find/create fragment for message\n";
			return;
		}

		m.emplace_or_replace<Message::Components::FID>(fragment_id);

		// in this case we know the fragment needs an update
		_fuid_save_queue.push({Message::getTimeMS(), fragment_id, m.registry()});
		return; // done
	}

	const auto msg_fh = _fs.fragmentHandle(m.get<Message::Components::FID>().fid);
	if (!static_cast<bool>(msg_fh)) {
		std::cerr << "MFS error: fid in message is invalid\n";
		return; // TODO: properly handle this case
	}

	if (!m.registry()->ctx().contains<Message::Components::OpenFragments>()) {
		m.registry()->ctx().emplace<Message::Components::OpenFragments>();
	}

	auto& fid_open = m.registry()->ctx().get<Message::Components::OpenFragments>().fid_open;

	if (fid_open.contains(msg_fh)) {
		// TODO: dedup events
		// TODO: cooldown per fragsave
		_fuid_save_queue.push({Message::getTimeMS(), msg_fh, m.registry()});
		return;
	}

	// TODO: save updates to old fragments, but writing them to a new fragment that would overwrite on merge
	// new fragment?, since we dont write to others fragments?


	// on new message: assign fuid
	// on new and update: mark as fragment dirty
}

// assumes not loaded frag
// need update from frag
void MessageFragmentStore::loadFragment(Message3Registry& reg, FragmentHandle fh) {
	std::cout << "MFS: loadFragment\n";
	const auto j = _fs.loadFromStorageNJ(fh);

	if (!j.is_array()) {
		// wrong data
		return;
	}

	// TODO: this should probably never be the case, since we already know here that it is a msg frag
	if (!reg.ctx().contains<Message::Components::ContactFragments>()) {
		reg.ctx().emplace<Message::Components::ContactFragments>();
	}
	reg.ctx().get<Message::Components::ContactFragments>().insert(fh);

	// mark loaded
	if (!reg.ctx().contains<Message::Components::LoadedContactFragments>()) {
		reg.ctx().emplace<Message::Components::LoadedContactFragments>();
	}
	reg.ctx().get<Message::Components::LoadedContactFragments>().frags.emplace(fh);

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

		new_real_msg.emplace_or_replace<Message::Components::FID>(fh);

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
			//  -> merge with preexisting (needs to be order independent)
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

bool MessageFragmentStore::syncFragToStorage(FragmentHandle fh, Message3Registry& reg) {
	auto& ftsrange = fh.get_or_emplace<FragComp::MessagesTSRange>(Message::getTimeMS(), Message::getTimeMS());

	auto j = nlohmann::json::array();

	// TODO: does every message have ts?
	auto msg_view = reg.view<Message::Components::Timestamp>();
	// we also assume all messages have fid
	for (auto it = msg_view.rbegin(), it_end = msg_view.rend(); it != it_end; it++) {
		const Message3 m = *it;

		if (!reg.all_of<Message::Components::FID, Message::Components::ContactFrom, Message::Components::ContactTo>(m)) {
			continue;
		}

		// require msg for now
		if (!reg.any_of<Message::Components::MessageText/*, Message::Components::Transfer::FileInfo*/>(m)) {
			continue;
		}

		if (_fuid_save_queue.front().id != reg.get<Message::Components::FID>(m).fid) {
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

		for (const auto& [type_id, storage] : reg.storage()) {
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

			s_cb_it->second(_sc, {reg, m}, j_entry[storage.type().name()]);
		}
	}

	// we cant skip if array is empty (in theory it will not be empty later on)

	// if save as binary
	//nlohmann::json::to_msgpack(j);
	auto j_dump = j.dump(2, ' ', true);
	if (_fs.syncToStorage(fh, reinterpret_cast<const uint8_t*>(j_dump.data()), j_dump.size())) {
		//std::cout << "MFS: dumped " << j_dump << "\n";
		// succ
		return true;
	}

	// TODO: error
	return false;
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
	while (!_fuid_save_queue.empty()) {
		auto fh = _fs.fragmentHandle(_fuid_save_queue.front().id);
		auto* reg = _fuid_save_queue.front().reg;
		assert(reg != nullptr);
		syncFragToStorage(fh, *reg);
		_fuid_save_queue.pop(); // pop unconditionally
	}
}

MessageSerializerCallbacks& MessageFragmentStore::getMSC(void) {
	return _sc;
}

// checks range against all cursers in msgreg
static bool rangeVisible(uint64_t range_begin, uint64_t range_end, const Message3Registry& msg_reg) {
	// 1D collision checks:
	//  - for range vs range:
	//    r1 rhs >= r0 lhs AND r1 lhs <= r0 rhs
	//  - for range vs point:
	//    p >= r0 lhs AND p <= r0 rhs
	// NOTE: directions for us are reversed (begin has larger values as end)

	auto c_b_view = msg_reg.view<Message::Components::Timestamp, Message::Components::ViewCurserBegin>();
	c_b_view.use<Message::Components::ViewCurserBegin>();
	for (const auto& [m, ts_begin_comp, vcb] : c_b_view.each()) {
		// p and r1 rhs can be seen as the same
		// but first we need to know if a curser begin is a point or a range

		// TODO: margin?
		auto ts_begin = ts_begin_comp.ts;
		auto ts_end = ts_begin_comp.ts; // simplyfy code by making a single begin curser act as an infinitly small range
		if (msg_reg.valid(vcb.curser_end) && msg_reg.all_of<Message::Components::ViewCurserEnd>(vcb.curser_end)) {
			// TODO: respect curser end's begin?
			// TODO: remember which ends we checked and check remaining
			ts_end = msg_reg.get<Message::Components::Timestamp>(vcb.curser_end).ts;

			// sanity check curser order
			if (ts_end > ts_begin) {
				std::cerr << "MFS warning: begin curser and end curser of view swapped!!\n";
				std::swap(ts_begin, ts_end);
			}
		}

		// perform both checks here
		if (ts_begin < range_end || ts_end > range_begin) {
			continue;
		}

		// range hits a view
		return true;
	}

	return false;
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

float MessageFragmentStore::tick(float time_delta) {
	// sync dirty fragments here
	if (!_fuid_save_queue.empty()) {
		auto fh = _fs.fragmentHandle(_fuid_save_queue.front().id);
		auto* reg = _fuid_save_queue.front().reg;
		assert(reg != nullptr);
		if (syncFragToStorage(fh, *reg)) {
			_fuid_save_queue.pop();
		}
	}

	// load needed fragments here

	// last check event frags
	// only checks if it collides with ranges, not adjacent
	// bc ~range~ msgreg will be marked dirty and checked next tick
	const bool had_events = !_event_check_queue.empty();
	for (size_t i = 0; i < 10 && !_event_check_queue.empty(); i++) {
		std::cout << "MFS: event check\n";
		auto fh = _fs.fragmentHandle(_event_check_queue.front().fid);
		auto c = _event_check_queue.front().c;
		_event_check_queue.pop();

		if (!static_cast<bool>(fh)) {
			return 0.05f;
		}

		if (!fh.all_of<FragComp::MessagesTSRange>()) {
			return 0.05f;
		}

		// get ts range of frag and collide with all curser(s/ranges)
		const auto& frag_range = fh.get<FragComp::MessagesTSRange>();

		auto* msg_reg = _rmm.get(c);
		if (msg_reg == nullptr) {
			return 0.05f;
		}

		if (rangeVisible(frag_range.begin, frag_range.end, !msg_reg)) {
			loadFragment(*msg_reg, fh);
			_potentially_dirty_contacts.emplace(c);
			return 0.05f; // only one but soon again
		}
	}
	if (had_events) {
		std::cout << "MFS: event check none\n";
		return 0.05f; // only check events, even if non where hit
	}

	if (!_potentially_dirty_contacts.empty()) {
		std::cout << "MFS: pdc\n";
		// here we check if any view of said contact needs frag loading
		// only once per tick tho

		// TODO: this makes order depend on internal order and is not fair
		auto it = _potentially_dirty_contacts.cbegin();

		auto* msg_reg = _rmm.get(*it);

		// first do collision check agains every contact associated fragment
		// that is not already loaded !!
		if (msg_reg->ctx().contains<Message::Components::ContactFragments>()) {
			const auto& cf = msg_reg->ctx().get<Message::Components::ContactFragments>();
			if (!cf.frags.empty()) {
				if (!msg_reg->ctx().contains<Message::Components::LoadedContactFragments>()) {
					msg_reg->ctx().emplace<Message::Components::LoadedContactFragments>();
				}
				const auto& loaded_frags = msg_reg->ctx().get<Message::Components::LoadedContactFragments>().frags;

				for (const auto& [fid, si] : msg_reg->ctx().get<Message::Components::ContactFragments>().frags) {
					if (loaded_frags.contains(fid)) {
						continue;
					}

					auto fh = _fs.fragmentHandle(fid);

					if (!static_cast<bool>(fh)) {
						std::cerr << "MFS error: frag is invalid\n";
						// WHAT
						msg_reg->ctx().get<Message::Components::ContactFragments>().erase(fid);
						return 0.05f;
					}

					if (!fh.all_of<FragComp::MessagesTSRange>()) {
						std::cerr << "MFS error: frag has no range\n";
						// ????
						msg_reg->ctx().get<Message::Components::ContactFragments>().erase(fid);
						return 0.05f;
					}

					// get ts range of frag and collide with all curser(s/ranges)
					const auto& [range_begin, range_end] = fh.get<FragComp::MessagesTSRange>();

					if (rangeVisible(range_begin, range_end, *msg_reg)) {
						std::cout << "MFS: frag hit by vis range\n";
						loadFragment(*msg_reg, fh);
						return 0.05f;
					}
				}
				// no new visible fragment
				std::cout << "MFS: no new frag directly visible\n";

				// now, finally, check for adjecent fragments that need to be loaded
				// we do this by finding the outermost fragment in a rage, and extend it by one

				// TODO: rewrite using some bounding range tree to perform collision checks !!!
				// (this is now performing better, but still)


				// for each view
				auto c_b_view = msg_reg->view<Message::Components::Timestamp, Message::Components::ViewCurserBegin>();
				c_b_view.use<Message::Components::ViewCurserBegin>();
				for (const auto& [_, ts_begin_comp, vcb] : c_b_view.each()) {
					// aka "scroll down"
					{ // find newest(-ish) frag in range
						// or in reverse frag end <= range begin


						// lower bound of frag end and range begin
						const auto right = std::lower_bound(
							cf.sorted_end.crbegin(),
							cf.sorted_end.crend(),
							ts_begin_comp.ts,
							[&](const FragmentID element, const auto& value) -> bool {
								return _fs._reg.get<FragComp::MessagesTSRange>(element).end >= value;
							}
						);

						FragmentID next_frag{entt::null};
						if (right != cf.sorted_end.crend()) {
							next_frag = cf.next(*right);
						}
						// we checked earlier that cf is not empty
						if (!_fs._reg.valid(next_frag)) {
							// fall back to closest, cf is not empty
							next_frag = cf.sorted_end.front();
						}

						// a single adjacent frag is often not enough
						// only ok bc next is cheap
						for (size_t i = 0; i < 5 && _fs._reg.valid(next_frag); i++) {
							if (!loaded_frags.contains(next_frag)) {
								std::cout << "MFS: next frag of range\n";
								loadFragment(*msg_reg, {_fs._reg, next_frag});
								return 0.05f;
							}

							next_frag = cf.next(next_frag);
						}
					}

					// curser end
					if (!msg_reg->valid(vcb.curser_end) || !msg_reg->all_of<Message::Components::Timestamp>(vcb.curser_end)) {
						continue;
					}
					const auto ts_end = msg_reg->get<Message::Components::Timestamp>(vcb.curser_end).ts;

					// aka "scroll up"
					{ // find oldest(-ish) frag in range
						// frag begin >= range end

						// lower bound of frag begin and range end
						const auto left = std::lower_bound(
							cf.sorted_begin.cbegin(),
							cf.sorted_begin.cend(),
							ts_end,
							[&](const FragmentID element, const auto& value) -> bool {
								return _fs._reg.get<FragComp::MessagesTSRange>(element).begin < value;
							}
						);

						FragmentID prev_frag{entt::null};
						if (left != cf.sorted_begin.cend()) {
							prev_frag = cf.prev(*left);
						}
						// we checked earlier that cf is not empty
						if (!_fs._reg.valid(prev_frag)) {
							// fall back to closest, cf is not empty
							prev_frag = cf.sorted_begin.back();
						}

						// a single adjacent frag is often not enough
						// only ok bc next is cheap
						for (size_t i = 0; i < 5 && _fs._reg.valid(prev_frag); i++) {
							if (!loaded_frags.contains(prev_frag)) {
								std::cout << "MFS: prev frag of range\n";
								loadFragment(*msg_reg, {_fs._reg, prev_frag});
								return 0.05f;
							}

							prev_frag = cf.prev(prev_frag);
						}
					}
				}
			}
		} else {
			// contact has no fragments, skip
		}

		_potentially_dirty_contacts.erase(it);

		return 0.05f;
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

	Contact3 frag_contact = entt::null;
	{ // get contact
		const auto& frag_contact_id = e.e.get<FragComp::MessagesContact>().id;
		// TODO: id lookup table, this is very inefficent
		for (const auto& [c_it, id_it] : _cr.view<Contact::Components::ID>().each()) {
			if (frag_contact_id == id_it.data) {
				frag_contact = c_it;
				break;
			}
		}
		if (!_cr.valid(frag_contact)) {
			// unkown contact
			return false;
		}
	}

	// create if not exist
	auto* msg_reg = _rmm.get(frag_contact);
	if (msg_reg == nullptr) {
		// msg reg not created yet
		// TODO: this is an erroious path
		return false;
	}

	if (!msg_reg->ctx().contains<Message::Components::ContactFragments>()) {
		msg_reg->ctx().emplace<Message::Components::ContactFragments>();
	}
	msg_reg->ctx().get<Message::Components::ContactFragments>().erase(e.e); // TODO: check/update/fragment update
	msg_reg->ctx().get<Message::Components::ContactFragments>().insert(e.e);

	_event_check_queue.push(ECQueueEntry{e.e, frag_contact});

	return false;
}

bool Message::Components::ContactFragments::insert(FragmentHandle frag) {
	if (frags.contains(frag)) {
		return false;
	}

	// both sorted arrays are sorted ascending
	// so for insertion we search for the last index that is <= and insert after it
	// or we search for the first > (or end) and insert before it    <---
	// since equal fragments are UB, we can assume they are only > or <

	size_t begin_index {0};
	{ // begin
		const auto pos = std::find_if(
			sorted_begin.cbegin(),
			sorted_begin.cend(),
			[frag](const FragmentID a) -> bool {
				const auto begin_a = frag.registry()->get<FragComp::MessagesTSRange>(a).begin;
				const auto begin_frag = frag.get<FragComp::MessagesTSRange>().begin;
				if (begin_a > begin_frag) {
					return true;
				} else if (begin_a < begin_frag) {
					return false;
				} else {
					// equal ts, we need to fall back to id (id can not be equal)
					return isLess(frag.get<FragComp::ID>().v, frag.registry()->get<FragComp::ID>(a).v);
				}
			}
		);

		begin_index = std::distance(sorted_begin.cbegin(), pos);

		// we need to insert before pos (end is valid here)
		sorted_begin.insert(pos, frag);
	}

	size_t end_index {0};
	{ // end
		const auto pos = std::find_if_not(
			sorted_end.cbegin(),
			sorted_end.cend(),
			[frag](const FragmentID a) -> bool {
				const auto end_a = frag.registry()->get<FragComp::MessagesTSRange>(a).end;
				const auto end_frag = frag.get<FragComp::MessagesTSRange>().end;
				if (end_a > end_frag) {
					return true;
				} else if (end_a < end_frag) {
					return false;
				} else {
					// equal ts, we need to fall back to id (id can not be equal)
					return isLess(frag.get<FragComp::ID>().v, frag.registry()->get<FragComp::ID>(a).v);
				}
			}
		);

		end_index = std::distance(sorted_end.cbegin(), pos);

		// we need to insert before pos (end is valid here)
		sorted_end.insert(pos, frag);
	}

	frags.emplace(frag, InternalEntry{begin_index, end_index});

	return true;
}

bool Message::Components::ContactFragments::erase(FragmentID frag) {
	auto frags_it = frags.find(frag);
	if (frags_it == frags.end()) {
		return false;
	}

	assert(sorted_begin.size() == sorted_end.size());
	assert(sorted_begin.size() > frags_it->second.i_b);

	sorted_begin.erase(sorted_begin.begin() + frags_it->second.i_b);
	sorted_end.erase(sorted_end.begin() + frags_it->second.i_e);

	frags.erase(frags_it);

	return true;
}

FragmentID Message::Components::ContactFragments::prev(FragmentID frag) const {
	// uses range begin to go back in time

	auto it = frags.find(frag);
	if (it == frags.end()) {
		return entt::null;
	}

	const auto src_i = it->second.i_b;
	if (src_i > 0) {
		return sorted_begin[src_i-1];
	}

	return entt::null;
}

FragmentID Message::Components::ContactFragments::next(FragmentID frag) const {
	// uses range end to go forward in time

	auto it = frags.find(frag);
	if (it == frags.end()) {
		return entt::null;
	}

	const auto src_i = it->second.i_e;
	if (src_i+1 < sorted_end.size()) {
		return sorted_end[src_i+1];
	}

	return entt::null;
}

