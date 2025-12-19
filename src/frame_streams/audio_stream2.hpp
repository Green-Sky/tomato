#pragma once

#include "./frame_stream2.hpp"

#include <solanaceae/util/span.hpp>

#include <cstdint>
#include <variant>
#include <vector>

// raw audio
// channels make samples interleaved,
// planar channels are not supported
// s16 only stopgap audio frame (simplified)
struct AudioFrame2 {
	// samples per second
	uint32_t sample_rate {48'000};

	// only >0 is valid
	size_t channels {0};

	std::variant<
		std::vector<int16_t>, // S16, platform endianess
		Span<int16_t> // non owning variant, for direct consumption
	> buffer;

	AudioFrame2(void) = default;
	AudioFrame2(const AudioFrame2&) = default;
	AudioFrame2(AudioFrame2&& other) :
		sample_rate(other.sample_rate),
		channels(other.channels),
		buffer(std::move(other.buffer))
	{}
	AudioFrame2(uint32_t sample_rate_, size_t channels_, const std::variant<std::vector<int16_t>, Span<int16_t>>& buffer_) :
		sample_rate(sample_rate_),
		channels(channels_),
		buffer(buffer_)
	{}
	AudioFrame2(uint32_t sample_rate_, size_t channels_, std::variant<std::vector<int16_t>, Span<int16_t>>&& buffer_) :
		sample_rate(sample_rate_),
		channels(channels_),
		buffer(std::move(buffer_))
	{}

	// helpers
	Span<int16_t> getSpan(void) const {
		if (std::holds_alternative<std::vector<int16_t>>(buffer)) {
			return Span<int16_t>{std::get<std::vector<int16_t>>(buffer)};
		} else {
			return std::get<Span<int16_t>>(buffer);
		}
		return {};
	}
};

template<>
constexpr bool frameHasBytes<AudioFrame2>(void) {
	return true;
}

template<>
inline uint64_t frameGetBytes(const AudioFrame2& frame) {
	return frame.getSpan().size * sizeof(int16_t);
}

using AudioFrame2Stream2I = FrameStream2I<AudioFrame2>;

