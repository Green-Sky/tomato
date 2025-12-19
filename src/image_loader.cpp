#include "./image_loader.hpp"

#include <cassert>
#include <cstdint>

#include "./image_scaler.hpp"

ImageLoaderI::ImageResult ImageLoaderI::ImageResult::crop(int32_t c_x, int32_t c_y, int32_t c_w, int32_t c_h) const {
	if (
		c_x < 0 || c_y < 0 || c_w < 1 || c_h < 1 ||
		int64_t(c_x) + int64_t(c_w) > int64_t(width) || int64_t(c_y) + int64_t(c_h) > int64_t(height)
	) {
		// unreachable
		assert(false && "invalid image crop");
		return *this;
	}

	ImageLoaderI::ImageResult new_image;
	new_image.width = c_w;
	new_image.height = c_h;
	new_image.file_ext = file_ext;

	for (const auto& input_frame : frames) {
		auto& new_frame = new_image.frames.emplace_back();
		new_frame.ms = input_frame.ms;

		// TODO: improve this, this is inefficent
		for (int64_t y = c_y; y < c_y + c_h; y++) {
			for (int64_t x = c_x; x < c_x + c_w; x++) {
				new_frame.data.push_back(input_frame.data.at(y*width*4+x*4+0));
				new_frame.data.push_back(input_frame.data.at(y*width*4+x*4+1));
				new_frame.data.push_back(input_frame.data.at(y*width*4+x*4+2));
				new_frame.data.push_back(input_frame.data.at(y*width*4+x*4+3));
			}
		}
	}

	return new_image;
}

ImageLoaderI::ImageResult ImageLoaderI::ImageResult::scale(int32_t w, int32_t h) const {
	assert(w > 0);
	assert(h > 0);

	ImageLoaderI::ImageResult new_image;
	new_image.width = w;
	new_image.height = h;
	new_image.file_ext = file_ext;

	for (const auto& input_frame : frames) {
		auto& new_frame = new_image.frames.emplace_back();
		new_frame.ms = input_frame.ms;

		new_frame.data.resize(w*h*4);

		image_scale(new_frame.data.data(), w, h, const_cast<uint8_t*>(input_frame.data.data()), width, height);
	}

	return new_image;
}
