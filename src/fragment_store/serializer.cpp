#include "./serializer.hpp"

void SerializerCallbacks::registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info) {
	_serl_json[type_info.hash()] = fn;
}

void SerializerCallbacks::registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info) {
	_deserl_json[type_info.hash()] = fn;
}

