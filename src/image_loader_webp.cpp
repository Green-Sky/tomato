#include "./image_loader_webp.hpp"

#include <webp/demux.h>
#include <webp/mux.h>

#include <cassert>

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

	WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options);
	if (dec == nullptr) {
		return res;
	}

	WebPAnimInfo anim_info;
	WebPAnimDecoderGetInfo(dec, &anim_info);
	res.width = anim_info.canvas_width;
	res.height = anim_info.canvas_height;

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

	WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options);
	if (dec == nullptr) {
		return res;
	}

	WebPAnimInfo anim_info;
	WebPAnimDecoderGetInfo(dec, &anim_info);
	res.width = anim_info.canvas_width;
	res.height = anim_info.canvas_height;

	int prev_timestamp = 0;
	while (WebPAnimDecoderHasMoreFrames(dec)) {
		uint8_t* buf;
		int timestamp;
		WebPAnimDecoderGetNext(dec, &buf, &timestamp);
		// ... (Render 'buf' based on 'timestamp').
		// ... (Do NOT free 'buf', as it is owned by 'dec').

		// just be dumb and append
		auto& new_frame = res.frames.emplace_back();
		new_frame.ms = timestamp-prev_timestamp;
		prev_timestamp = timestamp;
		new_frame.data.insert(new_frame.data.end(), buf, buf+(res.width*res.height*4));
	}

	WebPAnimDecoderDelete(dec);

	assert(anim_info.frame_count == res.frames.size());

	return res;
}

