#pragma once

#include <solanaceae/contact/contact_store_events.hpp>
#include <solanaceae/contact/contact_store_i.hpp>

#include "./contact_list.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace Contact::Components {

	// empty contact comp that is sorted in the set for displaying
	struct ContactSortTag {};

} // Contact::Components


class ContactListSorter : public ContactStore4EventI {
	public:
		using comperator_fn = std::function<std::optional<bool>(const ContactRegistry4& cr, const Contact4 lhs, const Contact4 rhs)>;

		static comperator_fn getSortGroupsOverPrivates(void);
		static comperator_fn getSortAcitivty(void);
		static comperator_fn getSortFirstSeen(void);

	private:
		ContactStore4I& _cs;
		ContactStore4I::SubscriptionReference _cs_sr;
		std::vector<comperator_fn> _sort_stack;

		uint64_t _last_sort {0};
		bool _dirty {true};
		// TODO: timer, to guarantie a sort ever X seconds?
		// (turns out we dont throw on new messages <.<)

	private:
		static bool resolveStack(const std::vector<comperator_fn>& stack, const ContactRegistry4& cr, const Contact4 lhs, const Contact4 rhs);

	public:
		// TODO: expose sort stack
		ContactListSorter(ContactStore4I& cs);
		~ContactListSorter(void);

		// optionally perform the sort
		void sort(void);

	protected:
		bool onEvent(const ContactStore::Events::Contact4Construct&) override;
		bool onEvent(const ContactStore::Events::Contact4Update&) override;
		bool onEvent(const ContactStore::Events::Contact4Destory&) override;
};
