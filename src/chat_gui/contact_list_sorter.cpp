#include "./contact_list_sorter.hpp"

#include <solanaceae/contact/components.hpp>

ContactListSorter::comperator_fn ContactListSorter::getSortGroupsOverPrivates(void) {
	return [](const ContactRegistry4& cr, const Contact4 lhs, const Contact4 rhs) -> std::optional<bool> {

		// - groups (> privates)
		if (cr.all_of<Contact::Components::TagGroup>(lhs) && !cr.all_of<Contact::Components::TagGroup>(rhs)) {
			return true;
		} else if (!cr.all_of<Contact::Components::TagGroup>(lhs) && cr.all_of<Contact::Components::TagGroup>(rhs)) {
			return false;
		}

		return std::nullopt;
	};
}

ContactListSorter::comperator_fn ContactListSorter::getSortAcitivty(void) {
	return [](const ContactRegistry4& cr, const Contact4 lhs, const Contact4 rhs) -> std::optional<bool> {
		// - activity (exists)
		if (cr.all_of<Contact::Components::LastActivity>(lhs) && !cr.all_of<Contact::Components::LastActivity>(rhs)) {
			return true;
		} else if (!cr.all_of<Contact::Components::LastActivity>(lhs) && cr.all_of<Contact::Components::LastActivity>(rhs)) {
			return false;
		}
		// else - we can assume both have or dont have LastActivity

		// - activity new > old
		if (cr.all_of<Contact::Components::LastActivity>(lhs)) {
			const auto l = cr.get<Contact::Components::LastActivity>(lhs).ts;
			const auto r = cr.get<Contact::Components::LastActivity>(rhs).ts;
			if (l > r) {
				return true;
			} else if (l < r) {
				return false;
			}
		}

		return std::nullopt;
	};
}

ContactListSorter::comperator_fn ContactListSorter::getSortFirstSeen(void) {
	return [](const ContactRegistry4& cr, const Contact4 lhs, const Contact4 rhs) -> std::optional<bool> {
		// - first seen (exists)
		if (cr.all_of<Contact::Components::FirstSeen>(lhs) && !cr.all_of<Contact::Components::FirstSeen>(rhs)) {
			return true;
		} else if (!cr.all_of<Contact::Components::FirstSeen>(lhs) && cr.all_of<Contact::Components::FirstSeen>(rhs)) {
			return false;
		}
		// else - we can assume both have or dont have FirstSeen

		// - first seen new > old
		if (cr.all_of<Contact::Components::FirstSeen>(lhs)) {
			const auto l = cr.get<Contact::Components::FirstSeen>(lhs).ts;
			const auto r = cr.get<Contact::Components::FirstSeen>(rhs).ts;
			if (l > r) {
				return true;
			} else if (l < r) {
				return false;
			}
		}

		return std::nullopt;
	};
}

bool ContactListSorter::resolveStack(const std::vector<comperator_fn>& stack, const ContactRegistry4& cr, const Contact4 lhs, const Contact4 rhs) {
	for (const auto& fn : stack) {
		auto res = fn(cr, lhs, rhs);
		if (res.has_value()) {
			return res.value();
		}
	}

	// fallback to numberical ordering, making sure the ordering can be strong
	return entt::to_integral(lhs) > entt::to_integral(rhs);
}

ContactListSorter::ContactListSorter(ContactStore4I& cs) :
	_cs(cs), _cs_sr(_cs.newSubRef(this))
{
	_sort_stack.reserve(3);
	_sort_stack.emplace_back(getSortGroupsOverPrivates());
	_sort_stack.emplace_back(getSortAcitivty());
	_sort_stack.emplace_back(getSortFirstSeen());

	_cs_sr
		.subscribe(ContactStore4_Event::contact_construct)
		.subscribe(ContactStore4_Event::contact_update)
		.subscribe(ContactStore4_Event::contact_destroy)
	;
}

ContactListSorter::~ContactListSorter(void) {
}

void ContactListSorter::sort(void) {
	// TODO: timer
	if (!_dirty) {
		return;
	}

	auto& cr = _cs.registry();

	// first: make sure every cantact we want to have in the list has the tag
	// do we pass exclusion to the list widget, or update sort comp? - the later
	cr.clear<Contact::Components::ContactSortTag>();
	for (const auto cv : cr.view<Contact::Components::TagBig>()) {
		(void)cr.get_or_emplace<Contact::Components::ContactSortTag>(cv);
	}

	// second: sort
	cr.sort<Contact::Components::ContactSortTag>(
		[this, &cr](const Contact4 lhs, const Contact4 rhs) -> bool {
			return resolveStack(_sort_stack, cr, lhs, rhs);
		},
		entt::insertion_sort{} // o(n) in >90% of cases
	);

	_dirty = false;
}

bool ContactListSorter::onEvent(const ContactStore::Events::Contact4Construct&) {
	_dirty = true;
	return false;
}

bool ContactListSorter::onEvent(const ContactStore::Events::Contact4Update&) {
	_dirty = true;
	return false;
}

bool ContactListSorter::onEvent(const ContactStore::Events::Contact4Destory&) {
	_dirty = true;
	return false;
}
