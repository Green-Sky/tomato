#pragma once

#include <cstdint>
#include <cstddef>

struct TextureUploaderI {
	static constexpr const char* version {"3"};

	enum Filter {
		NEAREST,
		LINEAR,
	};

	enum Access {
		STATIC,
		STREAMING,
		// target?
	};

	enum Format {
		RGBA,
		//RGB,

		IYUV,
		YV12,

		NV12,
		NV21,

		MAX
	};

	virtual ~TextureUploaderI(void) {}

	[[deprecated]] virtual uint64_t uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter = LINEAR, Access access = STATIC) = 0;

	// keeps width height filter
	// TODO: wh instead of size?
	[[deprecated]] virtual bool updateRGBA(uint64_t tex_id, const uint8_t* data, size_t size) = 0;

	// use upload to create a texture, and update to update existing
	virtual uint64_t upload(const uint8_t* data, uint32_t width, uint32_t height, Format format = RGBA, Filter filter = LINEAR, Access access = STATIC) = 0;
	virtual bool update(uint64_t tex_id, const uint8_t* data, size_t size) = 0;

	virtual void destroy(uint64_t tex_id) = 0;
};

