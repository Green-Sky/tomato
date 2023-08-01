#pragma once

#include "./image_loader.hpp"

struct ImageLoaderSTB : public ImageLoaderI {
	ImageInfo loadInfoFromMemory(const uint8_t* data, uint64_t data_size) override;
	ImageResult loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) override;
};

