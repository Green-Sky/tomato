#pragma once

#include "./frame_stream2.hpp"

#include <solanaceae/util/span.hpp>

#include <cstdint>
#include <variant>
#include <vector>

// raw audio
// channels make samples interleaved,
// planar channels are not supported
struct AudioFrame {
	// sequence number, to detect gaps
	uint32_t seq {0};
	// TODO: maybe use ts instead to discard old?
	// since buffer size is variable, some timestamp would be needed to estimate the lost time

	// samples per second
	uint32_t sample_rate {48'000};

	size_t channels {0};
	std::variant<
		std::vector<int16_t>, // S16, platform endianess
		Span<int16_t>, // non owning variant, for direct consumption

		std::vector<float>, // f32
		Span<float> // non owning variant, for direct consumption
	> buffer;

	// helpers

	bool isS16(void) const {
		return
			std::holds_alternative<std::vector<int16_t>>(buffer) ||
			std::holds_alternative<Span<int16_t>>(buffer)
		;
	}
	bool isF32(void) const {
		return
			std::holds_alternative<std::vector<float>>(buffer) ||
			std::holds_alternative<Span<float>>(buffer)
		;
	}
	template<typename T>
	Span<T> getSpan(void) const {
		static_assert(std::is_same_v<int16_t, T> || std::is_same_v<float, T>);
		if constexpr (std::is_same_v<int16_t, T>) {
			assert(isS16());
			if (std::holds_alternative<std::vector<int16_t>>(buffer)) {
				return Span<int16_t>{std::get<std::vector<int16_t>>(buffer)};
			} else {
				return std::get<Span<int16_t>>(buffer);
			}
		} else if constexpr (std::is_same_v<float, T>) {
			assert(isF32());
			if (std::holds_alternative<std::vector<float>>(buffer)) {
				return Span<float>{std::get<std::vector<float>>(buffer)};
			} else {
				return std::get<Span<float>>(buffer);
			}
		}
		return {};
	}
};

using AudioFrameStream2I = FrameStream2I<AudioFrame>;

