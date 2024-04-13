#include "./internal_mfs_contexts.hpp"

#include "./message_fragment_store.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/message3/contact_components.hpp>

#include <iostream>

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

bool Message::Contexts::ContactFragments::insert(ObjectHandle frag) {
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
			[frag](const Object a) -> bool {
				const auto begin_a = frag.registry()->get<ObjComp::MessagesTSRange>(a).begin;
				const auto begin_frag = frag.get<ObjComp::MessagesTSRange>().begin;
				if (begin_a > begin_frag) {
					return true;
				} else if (begin_a < begin_frag) {
					return false;
				} else {
					// equal ts, we need to fall back to id (id can not be equal)
					return isLess(frag.get<ObjComp::ID>().v, frag.registry()->get<ObjComp::ID>(a).v);
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
			[frag](const Object a) -> bool {
				const auto end_a = frag.registry()->get<ObjComp::MessagesTSRange>(a).end;
				const auto end_frag = frag.get<ObjComp::MessagesTSRange>().end;
				if (end_a > end_frag) {
					return true;
				} else if (end_a < end_frag) {
					return false;
				} else {
					// equal ts, we need to fall back to id (id can not be equal)
					return isLess(frag.get<ObjComp::ID>().v, frag.registry()->get<ObjComp::ID>(a).v);
				}
			}
		);

		end_index = std::distance(sorted_end.cbegin(), pos);

		// we need to insert before pos (end is valid here)
		sorted_end.insert(pos, frag);
	}

	frags.emplace(frag, InternalEntry{begin_index, end_index});

	// now adjust all indicies of fragments coming after the insert position
	for (size_t i = begin_index + 1; i < sorted_begin.size(); i++) {
		frags.at(sorted_begin[i]).i_b = i;
	}
	for (size_t i = end_index + 1; i < sorted_end.size(); i++) {
		frags.at(sorted_end[i]).i_e = i;
	}

	return true;
}

bool Message::Contexts::ContactFragments::erase(Object frag) {
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

Object Message::Contexts::ContactFragments::prev(Object frag) const {
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

Object Message::Contexts::ContactFragments::next(Object frag) const {
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

