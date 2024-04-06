#include "./object_store.hpp"

#include "./meta_components.hpp"

#include <nlohmann/json.hpp> // this sucks

#include <iostream>

static bool serl_json_data_enc_type(const ObjectHandle oh, nlohmann::json& out) {
	out = static_cast<std::underlying_type_t<Encryption>>(
		oh.get<FragComp::DataEncryptionType>().enc
	);
	return true;
}

static bool deserl_json_data_enc_type(ObjectHandle oh, const nlohmann::json& in) {
	oh.emplace_or_replace<FragComp::DataEncryptionType>(
		static_cast<Encryption>(
			static_cast<std::underlying_type_t<Encryption>>(in)
		)
	);
	return true;
}

static bool serl_json_data_comp_type(const ObjectHandle oh, nlohmann::json& out) {
	out = static_cast<std::underlying_type_t<Compression>>(
		oh.get<FragComp::DataCompressionType>().comp
	);
	return true;
}

static bool deserl_json_data_comp_type(ObjectHandle oh, const nlohmann::json& in) {
	oh.emplace_or_replace<FragComp::DataCompressionType>(
		static_cast<Compression>(
			static_cast<std::underlying_type_t<Compression>>(in)
		)
	);
	return true;
}

ObjectStore2::ObjectStore2(void) {
	_sc.registerSerializerJson<FragComp::DataEncryptionType>(serl_json_data_enc_type);
	_sc.registerDeSerializerJson<FragComp::DataEncryptionType>(deserl_json_data_enc_type);
	_sc.registerSerializerJson<FragComp::DataCompressionType>(serl_json_data_comp_type);
	_sc.registerDeSerializerJson<FragComp::DataCompressionType>(deserl_json_data_comp_type);
}

ObjectStore2::~ObjectStore2(void) {
}

ObjectRegistry& ObjectStore2::registry(void) {
	return _reg;
}

ObjectHandle ObjectStore2::objectHandle(const Object o) {
	return {_reg, o};
}

void ObjectStore2::throwEventConstruct(const Object o) {
	std::cout << "OS debug: event construct " << entt::to_integral(o) << "\n";
	dispatch(
		ObjectStore_Event::object_construct,
		ObjectStore::Events::ObjectConstruct{
			ObjectHandle{_reg, o}
		}
	);
}

void ObjectStore2::throwEventUpdate(const Object o) {
	std::cout << "OS debug: event update " << entt::to_integral(o) << "\n";
	dispatch(
		ObjectStore_Event::object_update,
		ObjectStore::Events::ObjectUpdate{
			ObjectHandle{_reg, o}
		}
	);
}

void ObjectStore2::throwEventDestroy(const Object o) {
	std::cout << "OS debug: event destroy " << entt::to_integral(o) << "\n";
	dispatch(
		ObjectStore_Event::object_destroy,
		ObjectStore::Events::ObjectUpdate{
			ObjectHandle{_reg, o}
		}
	);
}

