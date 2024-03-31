#pragma once

#include <vector>
#include <array>
#include <random>
#include <cstdint>

struct UUIDGeneratorI {
	virtual std::vector<uint8_t> operator()(void) = 0;
};

// TODO: templates?
struct UUIDGenerator_128_128 final : public UUIDGeneratorI {
	private:
		std::array<uint8_t, 16> _uuid_namespace;
		std::minstd_rand _rng{std::random_device{}()};

	public:
		UUIDGenerator_128_128(void); // default randomly initializes namespace
		UUIDGenerator_128_128(const std::array<uint8_t, 16>& uuid_namespace);

		std::vector<uint8_t> operator()(void) override;
};

