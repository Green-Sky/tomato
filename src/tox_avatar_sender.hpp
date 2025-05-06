#pragma once

#include "solanaceae/contact/fwd.hpp"
#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/message3/registry_message_model.hpp>

#include <solanaceae/contact/components.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <string>
#include <vector>


class ToxAvatarSender : public ContactStore4EventI, public RegistryMessageModelEventI {
	ObjectStore2& _os;
	ContactStore4I& _cs;
	ContactStore4I::SubscriptionReference _cs_sr;
	RegistryMessageModelI& _rmm;
	RegistryMessageModelI::SubscriptionReference _rmm_sr;

	struct Entry {
		ContactHandle4 c;
		Contact::Components::ConnectionState::State ls;
		float timer {62.122f};
	};
	std::vector<Entry> _queue;

	bool checkContact(ContactHandle4 c);
	bool inQueue(ContactHandle4 c) const;
	void addToQueue(ContactHandle4 c);

	void sendAvatar(ContactHandle4 c);

	public:
		ToxAvatarSender(
			ObjectStore2& os,
			ContactStore4I& cs,
			RegistryMessageModelI& rmm
		);

		void iterate(float delta);

	protected:
		bool onEvent(const ContactStore::Events::Contact4Construct&) override;
		bool onEvent(const ContactStore::Events::Contact4Update&) override;
		bool onEvent(const ContactStore::Events::Contact4Destory&) override;
};

