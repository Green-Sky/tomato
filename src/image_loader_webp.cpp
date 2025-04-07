#include "./image_loader_webp.hpp"

#include <memory>
#include <webp/demux.h>
#include <webp/mux.h>
#include <webp/encode.h>

#include <cassert>

#include <iostream>

ImageLoaderWebP::ImageInfo ImageLoaderWebP::loadInfoFromMemory(const uint8_t* data, uint64_t data_size) {
	ImageInfo res;

	WebPData webp_data;
	WebPDataInit(&webp_data);
	webp_data.bytes = data;
	webp_data.size = data_size;

	WebPAnimDecoderOptions dec_options;
	WebPAnimDecoderOptionsInit(&dec_options);
	// Tune 'dec_options' as needed.
	dec_options.color_mode = MODE_RGBA;

	std::unique_ptr<WebPAnimDecoder, decltype(&WebPAnimDecoderDelete)> dec{
		WebPAnimDecoderNew(&webp_data, &dec_options),
		&WebPAnimDecoderDelete
	};
	if (!static_cast<bool>(dec)) {
		return res;
	}

	WebPAnimInfo anim_info;
	WebPAnimDecoderGetInfo(dec.get(), &anim_info);
	res.width = anim_info.canvas_width;
	res.height = anim_info.canvas_height;
	res.file_ext = "webp";

	return res;
}

ImageLoaderWebP::ImageResult ImageLoaderWebP::loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) {
	ImageResult res;

	WebPData webp_data;
	WebPDataInit(&webp_data);
	webp_data.bytes = data;
	webp_data.size = data_size;

	WebPAnimDecoderOptions dec_options;
	WebPAnimDecoderOptionsInit(&dec_options);
	// Tune 'dec_options' as needed.
	dec_options.color_mode = MODE_RGBA;

	std::unique_ptr<WebPAnimDecoder, decltype(&WebPAnimDecoderDelete)> dec{
		WebPAnimDecoderNew(&webp_data, &dec_options),
		&WebPAnimDecoderDelete
	};
	if (!static_cast<bool>(dec)) {
		return res;
	}

	WebPAnimInfo anim_info;
	WebPAnimDecoderGetInfo(dec.get(), &anim_info);
	res.width = anim_info.canvas_width;
	res.height = anim_info.canvas_height;
	res.file_ext = "webp";

	int prev_timestamp = 0;
	while (WebPAnimDecoderHasMoreFrames(dec.get())) {
		uint8_t* buf;
		int timestamp;
		WebPAnimDecoderGetNext(dec.get(), &buf, &timestamp);
		// ... (Render 'buf' based on 'timestamp').
		// ... (Do NOT free 'buf', as it is owned by 'dec').

		// just be dumb and append
		auto& new_frame = res.frames.emplace_back();
		new_frame.ms = timestamp-prev_timestamp;
		prev_timestamp = timestamp;
		new_frame.data = {buf, buf+(res.width*res.height*4)};
	}

	assert(anim_info.frame_count == res.frames.size());

	return res;
}

std::vector<uint8_t> ImageEncoderWebP::encodeToMemoryRGBA(const ImageResult& input_image, const std::map<std::string, float>& extra_options) {
	// setup options
	float quality = 80.f;
	if (extra_options.count("quality")) {
		quality = extra_options.at("quality");
	}
	bool lossless = false;
	int compression_level = 6;
	if (extra_options.count("compression_level")) {
		// if compression_level is set, we assume lossless
		lossless = true;
		compression_level = extra_options.at("compression_level");
	}

	// start encoding

	WebPAnimEncoderOptions enc_options;
	if (!WebPAnimEncoderOptionsInit(&enc_options)) {
		std::cerr << "IEWebP error: WebPAnimEncoderOptionsInit()\n";
		return {};
	}

	// Tune 'enc_options' as needed.
	enc_options.minimize_size = 1; // might be slow? optimize for size, no key-frame insertion

	std::unique_ptr<WebPAnimEncoder, decltype(&WebPAnimEncoderDelete)> enc {
		WebPAnimEncoderNew(input_image.width, input_image.height, &enc_options),
		&WebPAnimEncoderDelete
	};
	if (!enc) {
		std::cerr << "IEWebP error: WebPAnimEncoderNew()\n";
		return {};
	}

	WebPConfig config;
	if (!WebPConfigPreset(&config, WebPPreset::WEBP_PRESET_DEFAULT, quality)) {
		std::cerr << "IEWebP error: WebPConfigPreset()\n";
		return {};
	}
	if (lossless) {
		if (!WebPConfigLosslessPreset(&config, compression_level)) {
			std::cerr << "IEWebP error: WebPConfigLosslessPreset()\n";
			return {};
		}
	}

	int prev_timestamp = 0;
	for (const auto& frame : input_image.frames) {
		WebPPicture frame_webp;
		if (!WebPPictureInit(&frame_webp)) {
			std::cerr << "IEWebP error: WebPPictureInit()\n";
			return {};
		}
		frame_webp.width = input_image.width;
		frame_webp.height = input_image.height;
		if (!WebPPictureImportRGBA(&frame_webp, frame.data.data(), 4*input_image.width)) {
			std::cerr << "IEWebP error: WebPPictureImportRGBA()\n";
			return {};
		}

		if (!WebPAnimEncoderAdd(enc.get(), &frame_webp, prev_timestamp, &config)) {
			std::cerr << "IEWebP error: WebPAnimEncoderAdd(): " << frame_webp.error_code << "\n";
			WebPPictureFree(&frame_webp);
			return {};
		}
		prev_timestamp += frame.ms;
		WebPPictureFree(&frame_webp);
	}
	if (!WebPAnimEncoderAdd(enc.get(), NULL, prev_timestamp, NULL)) { // tell anim encoder its the end
		std::cerr << "IEWebP error: WebPAnimEncoderAdd(NULL)\n";
		return {};
	}

	WebPData webp_data;
	WebPDataInit(&webp_data);
	if (!WebPAnimEncoderAssemble(enc.get(), &webp_data)) {
		std::cerr << "IEWebP error: WebPAnimEncoderAdd(NULL)\n";
		return {};
	}

	// Write the 'webp_data' to a file, or re-mux it further.
	// TODO: make it not a copy
	std::vector<uint8_t> new_data{webp_data.bytes, webp_data.bytes+webp_data.size};

	WebPDataClear(&webp_data);

	return new_data;
}

