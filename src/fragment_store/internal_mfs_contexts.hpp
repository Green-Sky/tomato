#pragma once

#include <solanaceae/object_store/object_store.hpp>

#include <entt/container/dense_set.hpp>
#include <entt/container/dense_map.hpp>

// everything assumes a single object registry (and unique objects)

namespace Message::Contexts {

	// ctx
	struct OpenFragments {
		// only contains fragments with <1024 messages and <2h tsrage (or whatever)
		entt::dense_set<Object> open_frags;
	};

	// all message fragments of this contact
	struct ContactFragments final {
		// kept up-to-date by events
		struct InternalEntry {
			// indecies into the sorted arrays
			size_t i_b;
			size_t i_e;
		};
		entt::dense_map<Object, InternalEntry> frags;

		// add 2 sorted contact lists for both range begin and end
		// TODO: adding and removing becomes expensive with enough frags, consider splitting or heap
		std::vector<Object> sorted_begin;
		std::vector<Object> sorted_end;

		// api
		// return true if it was actually inserted
		bool insert(ObjectHandle frag);
		bool erase(Object frag);
		// update? (just erase() + insert())

		// uses range begin to go back in time
		Object prev(Object frag) const;
		// uses range end to go forward in time
		Object next(Object frag) const;
	};

	// all LOADED message fragments
	// TODO: merge into ContactFragments (and pull in openfrags)
	struct LoadedContactFragments final {
		// kept up-to-date by events
		entt::dense_set<Object> loaded_frags;
	};

} // Message::Contexts

