#pragma once

#include <solanaceae/util/event_provider.hpp>

#include "./serializer.hpp" // TODO: get rid of the tight nljson integration

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <cstdint>

// internal id
enum class Object : uint32_t {};
using ObjectRegistry = entt::basic_registry<Object>;
using ObjectHandle = entt::basic_handle<ObjectRegistry>;

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

	SerializerCallbacks<Object> _sc;

	ObjectStore2(void);
	virtual ~ObjectStore2(void);

	ObjectRegistry& registry(void);
	ObjectHandle objectHandle(const Object o);

	void throwEventConstruct(const Object o);
	void throwEventUpdate(const Object o);
	void throwEventDestroy(const Object o);
};

