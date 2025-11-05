#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>

struct ImageLoaderI {
	virtual ~ImageLoaderI(void) {}

	struct ImageInfo final {
		uint32_t width {0};
		uint32_t height {0};
		const char* file_ext {nullptr};
	};
	virtual ImageInfo loadInfoFromMemory(const uint8_t* data, uint64_t data_size) = 0;

	struct ImageResult final {
		uint32_t width {0};
		uint32_t height {0};
		struct Frame {
			int32_t ms {0};
			std::vector<uint8_t> data;
		};
		std::vector<Frame> frames;
		const char* file_ext {nullptr};

		// only positive values are valid
		ImageResult crop(int32_t c_x, int32_t c_y, int32_t c_w, int32_t c_h) const;

		// only values > 0 are valid
		ImageResult scale(int32_t w, int32_t h) const;
	};
	virtual ImageResult loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) = 0;
};

struct ImageEncoderI {
	virtual ~ImageEncoderI(void) {}

	using ImageResult = ImageLoaderI::ImageResult;

	virtual std::vector<uint8_t> encodeToMemoryRGBA(const ImageResult& input_image, const std::map<std::string, float>& extra_options = {}) = 0;
};

