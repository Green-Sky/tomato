#pragma once

#include <solanaceae/util/event_provider.hpp>
#include <solanaceae/util/span.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <cstdint>

// internal id
enum class Object : uint32_t {};
using ObjectRegistry = entt::basic_registry<Object>;
using ObjectHandle = entt::basic_handle<ObjectRegistry>;

// fwd
struct ObjectStore2;

struct StorageBackendI {
	// OR or OS ?
	ObjectStore2& _os;

	StorageBackendI(ObjectStore2& os);

	// default impl fails, acting like a read only store
	virtual ObjectHandle newObject(ByteSpan id);

	// ========== write object to storage ==========
	using write_to_storage_fetch_data_cb = uint64_t(uint8_t* request_buffer, uint64_t buffer_size);
	// calls data_cb with a buffer to be filled in, cb returns actual count of data. if returned < max, its the last buffer.
	virtual bool write(Object o, std::function<write_to_storage_fetch_data_cb>& data_cb) = 0;
	bool write(Object o, const ByteSpan data);

	// ========== read object from storage ==========
	using read_from_storage_put_data_cb = void(const ByteSpan buffer);
	virtual bool read(Object o, std::function<read_from_storage_put_data_cb>& data_cb) = 0;

};

namespace ObjectStore::Events {

	struct ObjectConstruct {
		const ObjectHandle e;
	};
	struct ObjectUpdate {
		const ObjectHandle e;
	};
	struct ObjectDestory {
		const ObjectHandle e;
	};

} // ObjectStore::Events

enum class ObjectStore_Event : uint16_t {
	object_construct,
	object_update,
	object_destroy,

	MAX
};

struct ObjectStoreEventI {
	using enumType = ObjectStore_Event;

	virtual ~ObjectStoreEventI(void) {}

	virtual bool onEvent(const ObjectStore::Events::ObjectConstruct&) { return false; }
	virtual bool onEvent(const ObjectStore::Events::ObjectUpdate&) { return false; }
	virtual bool onEvent(const ObjectStore::Events::ObjectDestory&) { return false; }
};
using ObjectStoreEventProviderI = EventProviderI<ObjectStoreEventI>;

struct ObjectStore2 : public ObjectStoreEventProviderI {
	static constexpr const char* version {"2"};

	ObjectRegistry _reg;

	// TODO: default backend?

	ObjectStore2(void);
	virtual ~ObjectStore2(void);

	ObjectRegistry& registry(void);
	ObjectHandle objectHandle(const Object o);

	// TODO: properly think about multiple objects witht he same id / different backends
	ObjectHandle getOneObjectByID(const ByteSpan id);

	// sync?

	void throwEventConstruct(const Object o);
	void throwEventUpdate(const Object o);
	void throwEventDestroy(const Object o);
};

