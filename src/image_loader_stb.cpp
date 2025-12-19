#include "./image_loader_stb.hpp"

#include <cstdint>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>

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
				new_frame.data = {img_data + (i*stride), img_data + ((i+1)*stride)};
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
	new_frame.data = {img_data, img_data+(x*y*4)};

	stbi_image_free(img_data);
	return res;
}

std::vector<uint8_t> ImageEncoderSTBPNG::encodeToMemoryRGBA(const ImageResult& input_image, const std::map<std::string, float>& extra_options) {
	if (input_image.frames.empty()) {
		std::cerr << "IESTBPNG error: empty image\n";
		return {};
	}

	if (input_image.frames.size() > 1) {
		std::cerr << "IESTBPNG warning: image with animation, only first frame will be encoded!\n";
		return {};
	}

	int png_compression_level = 8;
	if (extra_options.count("png_compression_level")) {
		png_compression_level = extra_options.at("png_compression_level");
	}
	// TODO:
	//int stbi_write_force_png_filter;         // defaults to -1; set to 0..5 to force a filter mode

	struct Context {
		std::vector<uint8_t> new_data;
	} context;
	auto write_f = +[](void* context_, void* data, int size) -> void {
		Context* ctx = reinterpret_cast<Context*>(context_);
		uint8_t* d = reinterpret_cast<uint8_t*>(data);
		ctx->new_data.insert(ctx->new_data.cend(), d, d + size);
	};

	stbi_write_png_compression_level = png_compression_level;

	if (!stbi_write_png_to_func(write_f, &context, input_image.width, input_image.height, 4, input_image.frames.front().data.data(), 4*input_image.width)) {
		std::cerr << "IESTBPNG error: stbi_write_png failed!\n";
		return {};
	}

	return context.new_data;
}

std::vector<uint8_t> ImageEncoderSTBJpeg::encodeToMemoryRGBA(const ImageResult& input_image, const std::map<std::string, float>& extra_options) {
	if (input_image.frames.empty()) {
		std::cerr << "IESTBJpeg error: empty image\n";
		return {};
	}

	if (input_image.frames.size() > 1) {
		std::cerr << "IESTBJpeg warning: image with animation, only first frame will be encoded!\n";
		return {};
	}

	// setup options
	float quality = 80.f;
	if (extra_options.count("quality")) {
		quality = extra_options.at("quality");
	}


	struct Context {
		std::vector<uint8_t> new_data;
	} context;
	auto write_f = +[](void* context_, void* data, int size) -> void {
		Context* ctx = reinterpret_cast<Context*>(context_);
		uint8_t* d = reinterpret_cast<uint8_t*>(data);
		ctx->new_data.insert(ctx->new_data.cend(), d, d + size);
	};

	if (!stbi_write_jpg_to_func(write_f, &context, input_image.width, input_image.height, 4, input_image.frames.front().data.data(), quality)) {
		std::cerr << "IESTBJpeg error: stbi_write_jpg failed!\n";
		return {};
	}

	return context.new_data;
}

