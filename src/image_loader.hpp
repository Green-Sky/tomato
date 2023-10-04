#pragma once

#include <cstdint>
#include <vector>

struct ImageLoaderI {
	virtual ~ImageLoaderI(void) {}

	struct ImageInfo {
		uint32_t width {0};
		uint32_t height {0};
		const char* file_ext {nullptr};
	};
	virtual ImageInfo loadInfoFromMemory(const uint8_t* data, uint64_t data_size) = 0;

	struct ImageResult {
		uint32_t width {0};
		uint32_t height {0};
		struct Frame {
			int32_t ms {0};
			std::vector<uint8_t> data;
		};
		std::vector<Frame> frames;
		const char* file_ext {nullptr};
	};
	virtual ImageResult loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) = 0;
};

