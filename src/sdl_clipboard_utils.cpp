#include "./sdl_clipboard_utils.hpp"

#include <SDL3/SDL.h>

#include <vector>

const char* clipboardHasImage(void) {
	const static std::vector<const char*> image_mime_types {
		"image/png",
		"image/webp",
		"image/gif",
		"image/jpeg",
		"image/bmp",
	};

	for (const char* mime_type : image_mime_types) {
		if (SDL_HasClipboardData(mime_type) == SDL_TRUE) {
			return mime_type;
		}
	}

	return nullptr;
}
