#include "./image_loader_stb.hpp"

#include <stb/stb_image.h>

ImageLoaderSTB::ImageInfo ImageLoaderSTB::loadInfoFromMemory(const uint8_t* data, uint64_t data_size) {
	ImageInfo res;

	int x,y;
	if (stbi_info_from_memory(data, data_size, &x, &y, nullptr) == 0) {
		return res; // failed to load
	}

	res.width = x;
	res.height = y;

	return res;
}

ImageLoaderSTB::ImageResult ImageLoaderSTB::loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) {
	ImageResult res;

	int x = 0;
	int y = 0;
	{ // try gif before to support animations
		int* delays = nullptr;
		int z = 0; // number of frames
		uint8_t* img_data = stbi_load_gif_from_memory(data, data_size, &delays, &x, &y, &z, nullptr, 4);

		if (img_data) {
			res.width = x;
			res.height = y;
			res.file_ext = "gif";

			const size_t stride = x * y * 4;

			// TODO: test this with asan
			for (int i = 0; i < z; i++) {
				auto& new_frame = res.frames.emplace_back();
				new_frame.ms = delays[i];
				new_frame.data.insert(new_frame.data.cbegin(), img_data + (i*stride), img_data + ((i+1)*stride));
			}

			stbi_image_free(delays); // hope this is right
			stbi_image_free(img_data);
			return res;
		}
	}

	uint8_t* img_data = stbi_load_from_memory(data, data_size, &x, &y, nullptr, 4);

	if (img_data == nullptr) {
		// not readable
		return res;
	}

	res.width = x;
	res.height = y;

	auto& new_frame = res.frames.emplace_back();
	new_frame.ms = 0;
	new_frame.data.insert(new_frame.data.cbegin(), img_data, img_data+(x*y*4));

	stbi_image_free(img_data);
	return res;
}

