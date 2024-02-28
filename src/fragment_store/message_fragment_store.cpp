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
		struct OpenFrag final {
			std::vector<uint8_t> uid;
		};
		// only contains fragments with <1024 messages and <28h tsrage
		// TODO: this needs to move into the message reg
		std::vector<OpenFrag> fuid_open;
	};

	// all message fragments of this contact
	struct ContactFragments final {
		// kept up-to-date by events
		//entt::dense_set<FragmentID> frags;
		struct InternalEntry {
			// indecies into the sorted arrays
			size_t i_b;
			size_t i_e;
		};
		entt::dense_map<FragmentID, InternalEntry> frags;

		// add 2 sorted contact lists for both range begin and end
		std::vector<FragmentID> sorted_begin;
		std::vector<FragmentID> sorted_end;

		// api
		// return true if it was actually inserted
		bool insert(FragmentHandle frag);
		bool erase(FragmentID frag);
		// update? (just erase() + insert())
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
		std::cout << "MFS: new or updated curser\n";
		return;
	}

	if (!m.all_of<Message::Components::FUID>()) {
		std::cout << "MFS: new msg missing FUID\n";
		if (!m.registry()->ctx().contains<Message::Components::OpenFragments>()) {
			m.registry()->ctx().emplace<Message::Components::OpenFragments>();

#if 0
			// TODO: move this to async
			// TODO: move this to tick and just respect the dirty
			FragmentHandle most_recent_fag;
			uint64_t most_recent_ts{0};
			if (m.registry()->ctx().contains<Message::Components::ContactFragments>()) {
				for (const auto& [fid, si] : m.registry()->ctx().get<Message::Components::ContactFragments>().frags) {
					auto fh = _fs.fragmentHandle(fid);
					if (!static_cast<bool>(fh) || !fh.all_of<FragComp::MessagesTSRange, FragComp::MessagesContact>()) {
						// TODO: remove at this point?
						continue;
					}

					const uint64_t f_ts = fh.get<FragComp::MessagesTSRange>().begin;
					if (f_ts > most_recent_ts) {
						// this makes no sense, we retry to load the first fragment on every new message and bail here, bc it was already
						if (m.registry()->ctx().contains<Message::Components::LoadedContactFragments>()) {
							if (m.registry()->ctx().get<Message::Components::LoadedContactFragments>().frags.contains(fh)) {
								continue; // already loaded
							}
						}
						most_recent_ts = f_ts;
						most_recent_fag = {_fs._reg, fid};
					}
				}
			}

			if (static_cast<bool>(most_recent_fag)) {
				loadFragment(*m.registry(), most_recent_fag);
			}
#endif
		}

		auto& fuid_open = m.registry()->ctx().get<Message::Components::OpenFragments>().fuid_open;

		const auto msg_ts = m.get<Message::Components::Timestamp>().ts;
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

	// load needed fragments here

	// last check event frags
	// only checks if it collides with ranges, not adjacent
	// bc ~range~ msgreg will be marked dirty and checked next tick
	if (!_event_check_queue.empty()) {
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
		}

		std::cout << "MFS: event check none\n";
		return 0.05f; // only one but soon again
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

			// TODO: this is all extreamly unperformant code !!!!!!
			// rewrite using some bounding range tree to perform collision checks !!!

			// for each view
			auto c_b_view = msg_reg->view<Message::Components::Timestamp, Message::Components::ViewCurserBegin>();
			c_b_view.use<Message::Components::ViewCurserBegin>();
			for (const auto& [m, ts_begin_comp, vcb] : c_b_view.each()) {
				// track down both in the same run
				FragmentID frag_newest {entt::null};
				uint64_t frag_newest_ts {};
				FragmentID frag_oldest {entt::null};
				uint64_t frag_oldest_ts {};

				// find newest frag in range
				for (const auto& [fid, si] : msg_reg->ctx().get<Message::Components::ContactFragments>().frags) {
					// we want to find the last and first fragment of the range (if not all hits are loaded, we did something wrong)
					if (!loaded_frags.contains(fid)) {
						continue;
					}

					// not checking frags for validity here, we checked above
					auto fh = _fs.fragmentHandle(fid);

					const auto [range_begin, range_end] = fh.get<FragComp::MessagesTSRange>();

					// perf, only check begin curser fist
					if (ts_begin_comp.ts < range_end) {
					//if (ts_begin_comp.ts < range_end || ts_end > range_begin) {
						continue;
					}

					if (ts_begin_comp.ts > range_begin) {
						// begin curser does not hit the frag, but end might still hit/contain it
						// if has curser end, check that
						if (!msg_reg->valid(vcb.curser_end) || !msg_reg->all_of<Message::Components::ViewCurserEnd, Message::Components::Timestamp>(vcb.curser_end)) {
							// no end, save no hit
							continue;
						}
						const auto ts_end = msg_reg->get<Message::Components::Timestamp>(vcb.curser_end).ts;

						if (ts_end > range_begin) {
							continue;
						}
					}
					//std::cout << "------------ hit\n";

					// save hit
					if (!_fs._reg.valid(frag_newest)) {
						frag_newest = fid;
						frag_newest_ts = range_begin; // new only compare against begin
					} else {
						// now we check if >= prev
						if (range_begin < frag_newest_ts) {
							continue;
						}

						if (range_begin == frag_newest_ts) {
							// equal ts -> fallback to id
							if (isLess(fh.get<FragComp::ID>().v, _fs._reg.get<FragComp::ID>(frag_newest).v)) {
								// we dont "care" about equality here, since that *should* never happen
								continue;
							}
						}

						frag_newest = fid;
						frag_newest_ts = range_begin; // new only compare against begin
					}

					if (!_fs._reg.valid(frag_oldest)) {
						frag_oldest = fid;
						frag_oldest_ts = range_end; // old only compare against end
					}

					if (fid != frag_oldest && fid != frag_newest) {
						// now check against old
						if (range_end > frag_oldest_ts) {
							continue;
						}

						if (range_end == frag_oldest_ts) {
							// equal ts -> fallback to id
							if (!isLess(fh.get<FragComp::ID>().v, _fs._reg.get<FragComp::ID>(frag_oldest).v)) {
								// we dont "care" about equality here, since that *should* never happen
								continue;
							}
						}

						frag_oldest = fid;
						frag_oldest_ts = range_end; // old only compare against end
					}
				}

				auto frag_after = _fs.fragmentHandle(fragmentAfter(frag_newest));
				if (static_cast<bool>(frag_after) && !loaded_frags.contains(frag_after)) {
					std::cout << "MFS: loading frag after newest\n";
					loadFragment(*msg_reg, frag_after);
					return 0.05f;
				}

				auto frag_before = _fs.fragmentHandle(fragmentBefore(frag_oldest));
				if (static_cast<bool>(frag_before) && !loaded_frags.contains(frag_before)) {
					std::cout << "MFS: loading frag before oldest\n";
					loadFragment(*msg_reg, frag_before);
					return 0.05f;
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

FragmentID MessageFragmentStore::fragmentBefore(FragmentID fid) {
	auto fh = _fs.fragmentHandle(fid);
	if (!fh.all_of<FragComp::MessagesTSRange>()) {
		return entt::null;
	}

	const auto& mtsrange = fh.get<FragComp::MessagesTSRange>();
	const auto& m_c_id = fh.get<FragComp::MessagesContact>().id;
	const auto& fuid = fh.get<FragComp::ID>();

	FragmentHandle current;

	auto mts_view = fh.registry()->view<FragComp::MessagesTSRange, FragComp::MessagesContact, FragComp::ID>();
	for (const auto& [it_f, it_mtsrange, it_m_c_id, it_fuid] : mts_view.each()) {
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

		// now we check contact (might be less cheap than range check)
		if (it_m_c_id.id != m_c_id) {
			continue;
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
	const auto& m_c_id = fh.get<FragComp::MessagesContact>().id;
	const auto& fuid = fh.get<FragComp::ID>();

	FragmentHandle current;

	auto mts_view = fh.registry()->view<FragComp::MessagesTSRange, FragComp::MessagesContact, FragComp::ID>();
	for (const auto& [it_f, it_mtsrange, it_m_c_id, it_fuid] : mts_view.each()) {
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

		// now we check contact (might be less cheap than range check)
		if (it_m_c_id.id != m_c_id) {
			continue;
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

