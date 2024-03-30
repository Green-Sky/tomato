#pragma once

#include <solanaceae/util/event_provider.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <cstdint>

// internal id
enum class FragmentID : uint32_t {};
using FragmentRegistry = entt::basic_registry<FragmentID>;
using FragmentHandle = entt::basic_handle<FragmentRegistry>;

namespace Fragment::Events {

	struct FragmentConstruct {
		const FragmentHandle e;
	};
	struct FragmentUpdated {
		const FragmentHandle e;
	};
	//struct MessageDestory {
		//const Message3Handle e;
	//};

} // Fragment::Events

enum class FragmentStore_Event : uint32_t {
	fragment_construct,
	fragment_updated,
	//message_destroy,

	MAX
};

struct FragmentStoreEventI {
	using enumType = FragmentStore_Event;

	virtual ~FragmentStoreEventI(void) {}

	virtual bool onEvent(const Fragment::Events::FragmentConstruct&) { return false; }
	virtual bool onEvent(const Fragment::Events::FragmentUpdated&) { return false; }
	//virtual bool onEvent(const Fragment::Events::MessageDestory&) { return false; }
};
using FragmentStoreEventProviderI = EventProviderI<FragmentStoreEventI>;

struct FragmentStoreI  : public FragmentStoreEventProviderI {
	static constexpr const char* version {"1"};

	FragmentRegistry _reg;

	virtual ~FragmentStoreI(void) {}

	FragmentRegistry& registry(void);
	FragmentHandle fragmentHandle(const FragmentID fid);

	void throwEventConstruct(const FragmentID fid);
	void throwEventUpdate(const FragmentID fid);
	// TODO
	//void throwEventDestroy();
};

