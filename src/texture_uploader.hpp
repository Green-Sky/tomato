#pragma once

#include <cstdint>
#include <cstddef>

struct TextureUploaderI {
	static constexpr const char* version {"2"};

	enum Filter {
		NEAREST,
		LINEAR,
	};

	enum Access {
		STATIC,
		STREAMING,
		// target?
	};

	virtual ~TextureUploaderI(void) {}

	virtual uint64_t uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter = LINEAR, Access access = STATIC) = 0;

	// keeps width height filter
	// TODO: wh instead of size?
	virtual bool updateRGBA(uint64_t tex_id, const uint8_t* data, size_t size) = 0;

	virtual void destroy(uint64_t tex_id) = 0;
};

