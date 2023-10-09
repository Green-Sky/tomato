#include "./image_loader.hpp"

#include <cassert>

ImageLoaderI::ImageResult ImageLoaderI::ImageResult::crop(int32_t c_x, int32_t c_y, int32_t c_w, int32_t c_h) const {
	// TODO: proper error handling
	assert(c_x+c_w <= width);
	assert(c_y+c_h <= height);

	ImageLoaderI::ImageResult new_image;
	new_image.width = c_w;
	new_image.height = c_h;
	new_image.file_ext = file_ext;

	for (const auto& input_frame : frames) {
		auto& new_frame = new_image.frames.emplace_back();
		new_frame.ms = input_frame.ms;

		// TODO: improve this, this is super inefficent
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

