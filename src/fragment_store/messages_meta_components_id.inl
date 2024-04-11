#pragma once

#include "./messages_meta_components.hpp"

#include <entt/core/type_info.hpp>

// TODO: move more central
#define DEFINE_COMP_ID(x) \
template<> \
constexpr entt::id_type entt::type_hash<x>::value() noexcept { \
	using namespace entt::literals; \
	return #x##_hs; \
} \
template<> \
constexpr std::string_view entt::type_name<x>::value() noexcept { \
	return #x; \
}

// cross compiler stable ids

DEFINE_COMP_ID(ObjComp::MessagesVersion)
DEFINE_COMP_ID(ObjComp::MessagesTSRange)
DEFINE_COMP_ID(ObjComp::MessagesContact)

// old stuff
//DEFINE_COMP_ID(FragComp::MessagesTSRange)
//DEFINE_COMP_ID(FragComp::MessagesContact)

#undef DEFINE_COMP_ID


