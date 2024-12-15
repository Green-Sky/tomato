#include "./texture_cache.hpp"

#include <chrono>
#include <array>
#include <limits>

int64_t TextureEntry::doAnimation(const int64_t ts_now) {
	if (frame_duration.size() > 1) { // is animation
		do { // why is this loop so ugly
			const int64_t duration = getDuration();
			if (ts_now - timestamp_last_rendered >= duration) {
				timestamp_last_rendered += duration;
				next();
			} else {
				// return ts for next frame
				return timestamp_last_rendered + duration;
			}
		} while (true);
	} else {
		timestamp_last_rendered = ts_now;
		return std::numeric_limits<int64_t>::max(); // static image
	}
}

TextureEntry generateTestAnim(TextureUploaderI& tu) {
	TextureEntry new_entry;
	new_entry.timestamp_last_rendered = Message::getTimeMS();
	new_entry.current_texture = 0;
	for (size_t i = 0; i < 4; i++) {
		// hack
		// generate frame
		const size_t width {2};
		const size_t height {2};
		std::array<uint8_t, width*height*4> pixels;
		for (size_t pen = 0; pen < width*height; pen++) {
			if (pen == i) {
				pixels[pen*4+0] = 0xff;
				pixels[pen*4+1] = 0xff;
				pixels[pen*4+2] = 0xff;
				pixels[pen*4+3] = 0xff;
			} else {
				pixels[pen*4+0] = 0x00;
				pixels[pen*4+1] = 0x00;
				pixels[pen*4+2] = 0x00;
				pixels[pen*4+3] = 0xff;
			}
		}

		const auto n_t = tu.upload(pixels.data(), width, height);

		// this is so ugly
		new_entry.textures.emplace_back(n_t);
		new_entry.frame_duration.emplace_back(250);
	}
	new_entry.width = 0;
	new_entry.height = 0;
	return new_entry;
}

