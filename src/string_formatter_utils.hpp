#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

// returns divider and places static suffix string into suffix_out
static inline int64_t sizeToHumanReadable(int64_t file_size, const char*& suffix_out) {
	static const char* suffix_arr[] {
		"Bytes",
		"KiB",
		"MiB",
		"GiB",
		"TiB",
		"PiB",
		// TODO: fix upper bound behaviour
	};
	int64_t divider = 1024;
	for (size_t ij = 0; ij < std::size(suffix_arr); ij++, divider *= 1024) {
		if (file_size < divider) {
			suffix_out = suffix_arr[ij];
			break;
		}
	}

	return (divider > 1024) ? (divider / 1024) : 1;
}

// returns divider and places static suffix string into suffix_out
static inline int64_t durationToHumanReadable(int64_t t, const char*& suffix_out) {
	static const char* suffix_arr[] {
		"ms",
		"s",
		"min",
		"h",
		"d",
		"a",
	};
	static const int64_t divider_arr[] {
		1000, // ms -> s
		60, // s -> min
		60, // min -> h
		24, // h -> d
		256, // d -> a // aprox
	};

	if (t <= 0) {
		suffix_out = suffix_arr[0];
		return 1;
	}

	int64_t divider {1};
	for (size_t i = 0; i < std::size(divider_arr); i++) {
		if (t < divider * divider_arr[i]) {
			suffix_out = suffix_arr[i];
			return divider;
		}

		divider *= divider_arr[i];
	}

	// if we are here, we are in the last element
	// 5 and 4
	suffix_out = suffix_arr[5];
	return divider;
}

