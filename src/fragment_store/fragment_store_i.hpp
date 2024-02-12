#pragma once

#include <cstdint>

// internal id
enum class FragmentID : uint32_t {};

struct FragmentStoreI {
	virtual ~FragmentStoreI(void) {}
};

