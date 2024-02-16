#include "./message_serializer.hpp"

#include <solanaceae/message3/components.hpp>
#include <solanaceae/contact/components.hpp>

#include <nlohmann/json.hpp>

#include <iostream>

template<>
bool MessageSerializerCallbacks::component_get_json<Message::Components::ContactFrom>(MessageSerializerCallbacks& msc, const Handle h, nlohmann::json& j) {
	const Contact3 c = h.get<Message::Components::ContactFrom>().c;
	if (!msc.cr.valid(c)) {
		// while this is invalid registry state, it is valid serialization
		j = nullptr;
		std::cerr << "MSC warning: encountered invalid contact\n";
		return true;
	}

	if (!msc.cr.all_of<Contact::Components::ID>(c)) {
		// unlucky, this contact is purely ephemeral
		j = nullptr;
		std::cerr << "MSC warning: encountered contact without ID\n";
		return true;
	}

	j = nlohmann::json::binary(msc.cr.get<Contact::Components::ID>(c).data);

	return true;
}
template<>
bool MessageSerializerCallbacks::component_emplace_or_replace_json<Message::Components::ContactFrom>(MessageSerializerCallbacks& msc, Handle h, const nlohmann::json& j) {
	if (j.is_null()) {
		std::cerr << "MSC warning: encountered null contact\n";
		h.emplace_or_replace<Message::Components::ContactFrom>();
		return true;
	}

	const auto id = static_cast<std::vector<uint8_t>>(j);

	// TODO: id lookup table, this is very inefficent
	for (const auto& [c_it, id_it] : msc.cr.view<Contact::Components::ID>().each()) {
		if (id == id_it.data) {
			h.emplace_or_replace<Message::Components::ContactFrom>(c_it);
			return true;
		}
	}

	// TODO: should we really return false if the contact is unknown??
	return false;
}

template<>
bool MessageSerializerCallbacks::component_get_json<Message::Components::ContactTo>(MessageSerializerCallbacks& msc, const Handle h, nlohmann::json& j) {
	const Contact3 c = h.get<Message::Components::ContactTo>().c;
	if (!msc.cr.valid(c)) {
		// while this is invalid registry state, it is valid serialization
		j = nullptr;
		std::cerr << "MSC warning: encountered invalid contact\n";
		return true;
	}

	if (!msc.cr.all_of<Contact::Components::ID>(c)) {
		// unlucky, this contact is purely ephemeral
		j = nullptr;
		std::cerr << "MSC warning: encountered contact without ID\n";
		return true;
	}

	j = nlohmann::json::binary(msc.cr.get<Contact::Components::ID>(c).data);

	return true;
}
template<>
bool MessageSerializerCallbacks::component_emplace_or_replace_json<Message::Components::ContactTo>(MessageSerializerCallbacks& msc, Handle h, const nlohmann::json& j) {
	if (j.is_null()) {
		std::cerr << "MSC warning: encountered null contact\n";
		h.emplace_or_replace<Message::Components::ContactTo>();
		return true;
	}

	const auto id = static_cast<std::vector<uint8_t>>(j);

	// TODO: id lookup table, this is very inefficent
	for (const auto& [c_it, id_it] : msc.cr.view<Contact::Components::ID>().each()) {
		if (id == id_it.data) {
			h.emplace_or_replace<Message::Components::ContactTo>(c_it);
			return true;
		}
	}

	// TODO: should we really return false if the contact is unknown??
	return false;
}
