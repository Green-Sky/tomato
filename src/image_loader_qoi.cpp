#include "./image_loader_qoi.hpp"

#include <qoi/qoi.h>
#include <qoirdo.hpp>

#include <cstdint>
#include <mutex>
#include <cassert>

#include <iostream>

#define QOI_HEADER_SIZE 14

// true if data looks like qoi
static bool probe_qoi(const uint8_t* data, int64_t data_size) {
	// smallest possible qoi
	// magic
	// dims (w h)
	// chan
	// col
	if (data_size < QOI_HEADER_SIZE+8) {
		return false;
	}

	if (
		data[0] != 'q' ||
		data[1] != 'o' ||
		data[2] != 'i' ||
		data[3] != 'f'
	) {
		return false;
	}

	return true;
}

// every qoi ends with 0,0,0,0,0,0,0,1
// returns the index for the first byte after padding (aka the size of the qoi)
static int64_t probe_qoi_padding(const uint8_t* data, int64_t data_size) {
	assert(data);
	assert(data_size >= 8);

	// loop checks if 0x00 or 0x01, otherwise skips 2
	// we start at the last possible first position
	for (int64_t pos = 7; pos < data_size;) {
		if (data[pos] == 0x00) {
			pos += 1; // not last char in padding, but we might be in the padding
		} else if (data[pos] == 0x01) {
			// last char from padding
			if (
				data[pos-7] == 0x00 &&
				data[pos-6] == 0x00 &&
				data[pos-5] == 0x00 &&
				data[pos-4] == 0x00 &&
				data[pos-3] == 0x00 &&
				data[pos-2] == 0x00 &&
				data[pos-1] == 0x00
			) {
				//return pos - 7; // first byte of padding
				return pos+1;
			} else {
				pos += 8; // cant be last byte for another 7
			}
		} else {
			pos += 2;
		}
	}

	return -1; // not found
}

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

	if (!probe_qoi(data, data_size)) {
		return res;
	}

	for (size_t data_pos = 0; data_pos < data_size;) {
		const auto qoi_size = probe_qoi_padding(data+data_pos, data_size-data_pos);
		if (qoi_size < QOI_HEADER_SIZE+8) {
			// too small
			break;
		}

		qoi_desc desc;

		uint8_t* img_data = static_cast<uint8_t*>(
			qoi_decode(data + data_pos, qoi_size, &desc, 4)
		);
		if (img_data == nullptr) {
			// not readable
			return res;
		}

		if (res.width == 0 || res.height == 0) {
			res.width = desc.width;
			res.height = desc.height;
		}

		if (res.width == desc.width && res.height == desc.height) {
			auto& new_frame = res.frames.emplace_back();
			new_frame.ms = 25; // ffmpeg default
			new_frame.data = {img_data, img_data+(desc.width*desc.height*4)};
		} else {
			// dim mismatch, what do we do? abort, continue?
			data_pos = data_size; // force exit loop
		}

		free(img_data);

		data_pos += qoi_size;
	}

	return res;
}

std::vector<uint8_t> ImageEncoderQOI::encodeToMemoryRGBA(const ImageResult& input_image, const std::map<std::string, float>& extra_options) {
	if (input_image.frames.empty()) {
		std::cerr << "IEQOI error: empty image\n";
		return {};
	}

	// TODO: look into RDO (eg https://github.com/richgel999/rdopng)
	//int png_compression_level = 8;
	//if (extra_options.count("png_compression_level")) {
		//png_compression_level = extra_options.at("png_compression_level");
	//}

	bool lossless = true;
	int quality = 90;
	if (extra_options.count("quality")) {
		lossless = false;
		quality = extra_options.at("quality");
	}

	std::vector<uint8_t> new_data;
	if (lossless) {
		qoi_desc desc;
		desc.width = input_image.width;
		desc.height = input_image.height;
		desc.channels = 4;
		desc.colorspace = QOI_SRGB; // TODO: decide

		for (const auto& frame : input_image.frames) {
			int out_len {0};
			uint8_t* enc_data = static_cast<uint8_t*>(qoi_encode(
				frame.data.data(),
				&desc,
				&out_len
			));

			if (enc_data == nullptr) {
				std::cerr << "IEQOI error: qoi_encode failed!\n";
				break;
			}

			// qoi_pipe like animation support. simple concatination of images
			if (new_data.empty()) {
				new_data = std::vector<uint8_t>(enc_data, enc_data+out_len);
			} else {
				new_data.insert(new_data.cend(), enc_data, enc_data+out_len);
			}

			free(enc_data); // TODO: a streaming encoder would be better
		}
	} else { // rdo
		// current rdo is not thread safe
		static std::mutex rdo_mutex{};
		std::lock_guard lg{rdo_mutex};
		init_qoi_rdo();

		// TODO: merge with qoi?
		qoi_rdo_desc desc;
		desc.width = input_image.width;
		desc.height = input_image.height;
		desc.channels = 4;
		desc.colorspace = QOI_SRGB; // TODO: decide

		for (const auto& frame : input_image.frames) {
			auto enc_data = encode_qoi_rdo_simple(
				frame.data.data(),
				desc,
				quality
			);

			if (enc_data.empty()) {
				std::cerr << "IEQOI error: encode_qoi_rdo_simple failed!\n";
				break;
			}

			// qoi_pipe like animation support. simple concatination of images
			if (new_data.empty()) {
				new_data = enc_data;
			} else {
				new_data.insert(new_data.cend(), enc_data.cbegin(), enc_data.cend());
			}
		}

		quit_qoi_rdo();
	}

	return new_data;
}

