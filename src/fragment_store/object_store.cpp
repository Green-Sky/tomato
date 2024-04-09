#include "./object_store.hpp"

#include "./meta_components.hpp"

#include <nlohmann/json.hpp> // this sucks

#include <iostream>

StorageBackendI::StorageBackendI(ObjectStore2& os) : _os(os) {
}

bool StorageBackendI::write(Object o, const ByteSpan data) {
	std::function<write_to_storage_fetch_data_cb> fn_cb = [read = 0ull, data](uint8_t* request_buffer, uint64_t buffer_size) mutable -> uint64_t {
		uint64_t i = 0;
		for (; i+read < data.size && i < buffer_size; i++) {
			request_buffer[i] = data[i+read];
		}
		read += i;

		return i;
	};
	return write(o, fn_cb);
}

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

