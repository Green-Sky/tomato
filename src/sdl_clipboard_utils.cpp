#include "./sdl_clipboard_utils.hpp"

#include <SDL3/SDL.h>

#include <string_view>
#include <vector>
#include <algorithm>

static const char* clipboardHas(const std::vector<std::string_view>& filter_mime_types) {
	for (const auto& mime_type : filter_mime_types) {
		// ASSERTS that stringview is null terminated
		if (SDL_HasClipboardData(mime_type.data())) {
			return mime_type.data();
		}
	}

	return nullptr;
}

const static std::vector<std::string_view> image_mime_types {
	"image/svg+xml",
	"image/apng",
	"image/webp",
	"image/png",
	"image/avif",
	"image/jxl",
	"image/gif",
	"image/jpeg",
	"image/qoi",
	"image/bmp",
	// tiff?
};

const static std::vector<std::string_view> file_list_mime_types {
	"text/uri-list",
	"text/x-moz-url",
};

const char* clipboardHasImage(void) {
	return clipboardHas(image_mime_types);
}

const char* clipboardHasFileList(void) {
	return clipboardHas(file_list_mime_types);
}

bool mimeIsImage(const char* mime_type) {
	if (mime_type == nullptr) {
		return false;
	}

	std::string_view mt_sv {mime_type};
	auto it = std::find(image_mime_types.cbegin(), image_mime_types.cend(), mime_type);

	return it != image_mime_types.cend();
}

bool mimeIsFileList(const char* mime_type) {
	if (mime_type == nullptr) {
		return false;
	}

	std::string_view mt_sv {mime_type};
	auto it = std::find(file_list_mime_types.cbegin(), file_list_mime_types.cend(), mime_type);

	return it != image_mime_types.cend();
}

