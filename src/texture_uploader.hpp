#pragma once

#include <cstdint>

struct TextureUploaderI {
	static constexpr const char* version {"1"};

	enum Filter {
		NEAREST,
		LINEAR,
	};

	virtual ~TextureUploaderI(void) {}

	virtual uint64_t uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter = LINEAR) = 0;

	virtual void destroy(uint64_t tex_id) = 0;
};

