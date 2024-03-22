#pragma once

#include <entt/core/type_info.hpp>
#include <entt/container/dense_map.hpp>

#include <solanaceae/message3/registry_message_model.hpp>

#include <nlohmann/json_fwd.hpp>

struct MessageSerializerCallbacks {
	using Registry = Message3Registry;
	using Handle = Message3Handle;

	Contact3Registry& cr;

	// nlohmann
	// json/msgpack
	using serialize_json_fn = bool(*)(MessageSerializerCallbacks& msc, const Handle h, nlohmann::json& out);
	entt::dense_map<entt::id_type, serialize_json_fn> _serl_json;

	using deserialize_json_fn = bool(*)(MessageSerializerCallbacks& msc, Handle h, const nlohmann::json& in);
	entt::dense_map<entt::id_type, deserialize_json_fn> _deserl_json;

	template<typename T>
	static bool component_get_json(MessageSerializerCallbacks&, const Handle h, nlohmann::json& j) {
		if (h.template all_of<T>()) {
			if constexpr (!std::is_empty_v<T>) {
				j = h.template get<T>();
			}
			return true;
		}

		return false;
	}

	template<typename T>
	static bool component_emplace_or_replace_json(MessageSerializerCallbacks&, Handle h, const nlohmann::json& j) {
		if constexpr (std::is_empty_v<T>) {
			h.template emplace_or_replace<T>(); // assert empty json?
		} else {
			h.template emplace_or_replace<T>(static_cast<T>(j));
		}
		return true;
	}

	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info) {
		_serl_json[type_info.hash()] = fn;
	}

	template<typename CompType>
	void registerSerializerJson(
		serialize_json_fn fn = component_get_json<CompType>,
		const entt::type_info& type_info = entt::type_id<CompType>()
	) {
		registerSerializerJson(fn, type_info);
	}

	void registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info) {
		_deserl_json[type_info.hash()] = fn;
	}

	template<typename CompType>
	void registerDeSerializerJson(
		deserialize_json_fn fn = component_emplace_or_replace_json<CompType>,
		const entt::type_info& type_info = entt::type_id<CompType>()
	) {
		registerDeSerializerJson(fn, type_info);
	}
};

// fwd
namespace Message::Components {
struct ContactFrom;
struct ContactTo;
}

// make specializations known
template<>
bool MessageSerializerCallbacks::component_get_json<Message::Components::ContactFrom>(MessageSerializerCallbacks& msc, const Handle h, nlohmann::json& j);
template<>
bool MessageSerializerCallbacks::component_emplace_or_replace_json<Message::Components::ContactFrom>(MessageSerializerCallbacks& msc, Handle h, const nlohmann::json& j);
template<>
bool MessageSerializerCallbacks::component_get_json<Message::Components::ContactTo>(MessageSerializerCallbacks& msc, const Handle h, nlohmann::json& j);
template<>
bool MessageSerializerCallbacks::component_emplace_or_replace_json<Message::Components::ContactTo>(MessageSerializerCallbacks& msc, Handle h, const nlohmann::json& j);
