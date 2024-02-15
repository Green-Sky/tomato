#pragma once

#include <entt/entity/registry.hpp>

#include <cstdint>

// internal id
enum class FragmentID : uint32_t {};
using FragmentRegistry = entt::basic_registry<FragmentID>;
using FragmentHandle = entt::basic_handle<FragmentRegistry>;

struct FragmentStoreI {
	virtual ~FragmentStoreI(void) {}
};

