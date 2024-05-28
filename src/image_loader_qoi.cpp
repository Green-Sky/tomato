#include "./image_loader_qoi.hpp"

#include <cstdint>
#include <qoi/qoi.h>

#include <iostream>

ImageLoaderQOI::ImageInfo ImageLoaderQOI::loadInfoFromMemory(const uint8_t* data, uint64_t data_size) {
	ImageInfo res;

	qoi_desc desc;
	// TODO: only read the header
	auto* ret = qoi_decode(data, data_size, &desc, 4);
	if (ret == nullptr) {
		return res;
	}
	free(ret);

	res.width = desc.width;
	res.height = desc.height;
	//desc.colorspace;

	return res;
}

ImageLoaderQOI::ImageResult ImageLoaderQOI::loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) {
	ImageResult res;

	qoi_desc desc;

	uint8_t* img_data = static_cast<uint8_t*>(
		qoi_decode(data, data_size, &desc, 4)
	);
	if (img_data == nullptr) {
		// not readable
		return res;
	}

	res.width = desc.width;
	res.height = desc.height;

	auto& new_frame = res.frames.emplace_back();
	new_frame.ms = 0;
	new_frame.data = {img_data, img_data+(desc.width*desc.height*4)};

	free(img_data);
	return res;
}

std::vector<uint8_t> ImageEncoderQOI::encodeToMemoryRGBA(const ImageResult& input_image, const std::map<std::string, float>&) {
	if (input_image.frames.empty()) {
		std::cerr << "IEQOI error: empty image\n";
		return {};
	}

	if (input_image.frames.size() > 1) {
		std::cerr << "IEQOI warning: image with animation, only first frame will be encoded!\n";
		return {};
	}

	// TODO: look into RDO (eg https://github.com/richgel999/rdopng)
	//int png_compression_level = 8;
	//if (extra_options.count("png_compression_level")) {
		//png_compression_level = extra_options.at("png_compression_level");
	//}

	qoi_desc desc;
	desc.width = input_image.width;
	desc.height = input_image.height;
	desc.channels = 4;
	desc.colorspace = QOI_SRGB; // TODO: decide

	int out_len {0};
	uint8_t* enc_data = static_cast<uint8_t*>(qoi_encode(
		input_image.frames.front().data.data(),
		&desc,
		&out_len
	));

	if (enc_data == nullptr) {
		std::cerr << "IEQOI error: qoi_encode failed!\n";
		return {};
	}

	std::vector<uint8_t> new_data(enc_data, enc_data+out_len);

	free(enc_data); // TODO: a streaming encoder would be better

	return new_data;
}

