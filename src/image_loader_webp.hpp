#pragma once

#include "./image_loader.hpp"

struct ImageLoaderWebP : public ImageLoaderI {
	ImageInfo loadInfoFromMemory(const uint8_t* data, uint64_t data_size) override;
	ImageResult loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) override;
};

struct ImageEncoderWebP : public ImageEncoderI {
	std::vector<uint8_t> encodeToMemoryRGBA(const ImageResult& input_image) override;
	std::vector<uint8_t> encodeToMemoryRGBAExt(const ImageResult& input_image, const std::map<std::string, float>& extra_options) override;
};

