#include "./sdl_clipboard_utils.hpp"

#include <SDL3/SDL.h>

#include <string_view>
#include <sstream>
#include <algorithm>
#include <iostream>

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

std::string file_path_url_escape(const std::string&& value) {
	std::ostringstream escaped;

	escaped << std::hex;
	escaped.fill('0');

	for (const char c : value) {
		if (
			c == '-' || c == '_' || c == '.' || c == '~' || // normal allowed url chars
			std::isalnum(static_cast<unsigned char>(c)) || // more normal
			c == '/' // special bc its a file://
		) {
			escaped << c;
		} else {
			escaped
				<< std::uppercase
				<< '%' <<
				std::setw(2) << static_cast<int>((static_cast<unsigned char>(c)))
				<< std::nouppercase
			;
		}
	}

	return escaped.str();
}

std::string file_path_to_file_url(const std::filesystem::path& path) {
	const auto can_path = std::filesystem::canonical(path);
	std::string url {"file://"};
	// special windows detection <.<
	// we detect a drive letter here
	if (can_path.has_root_name() && can_path.root_name().generic_u8string().back() == ':') {
		const std::string root_name = can_path.root_name().generic_u8string();
		// drive letters have a colon, which needs skipping the url escaping
		url += "/";
		url += root_name;

		//url += "/";
		// bugged, does not work (but it should, open msvc stl issue?)
		//url += file_path_url_escape(can_path.lexically_proximate(can_path.root_name()).generic_u8string());

		// remove drive letter
		url += file_path_url_escape(can_path.generic_u8string().substr(root_name.size()));
	} else {
		url += file_path_url_escape(can_path.generic_u8string());
	}

	return url;
}

Clipboard::~Clipboard(void) {
	// TODO: this is probably not safe
	SDL_ClearClipboardData();

	// this might be better, need to see if this works (docs needs improving)
	//for (const auto& [k, _] : _set_clipboard_data) {
		//const auto* tmp_mime_type = k.c_str();
		//SDL_SetClipboardData(nullptr, nullptr, nullptr, &tmp_mime_type, 1);
	//}
}

void Clipboard::setClipboardData(std::vector<std::string> mime_types, std::shared_ptr<std::vector<uint8_t>>&& data) {
	if (!static_cast<bool>(data)) {
		std::cerr << "CG error: tried to set clipboard with empty shp\n";
		return;
	}

	if (data->empty()) {
		std::cerr << "CG error: tried to set clipboard with empty data\n";
		return;
	}

	std::vector<const char*> tmp_mimetype_list;

	{
		std::lock_guard lg{_set_clipboard_data_mutex};
		for (const auto& mime_type : mime_types) {
			tmp_mimetype_list.push_back(mime_type.data());
			_set_clipboard_data[mime_type] = data;
		}

		// release lock, since on some platforms the callback is called immediatly
	}

	SDL_SetClipboardData(clipboard_callback, nullptr, this, tmp_mimetype_list.data(), tmp_mimetype_list.size());
}

const void* Clipboard::clipboard_callback(void* userdata, const char* mime_type, size_t* size) {
	if (mime_type == nullptr) {
		// cleared or new data is set
		return nullptr;
	}

	if (userdata == nullptr) {
		// error
		return nullptr;
	}

	auto* cg = static_cast<Clipboard*>(userdata);
	std::lock_guard lg{cg->_set_clipboard_data_mutex};
	if (!cg->_set_clipboard_data.count(mime_type)) {
		return nullptr;
	}

	const auto& sh_vec = cg->_set_clipboard_data.at(mime_type);
	if (!static_cast<bool>(sh_vec)) {
		// error, empty shared pointer
		return nullptr;
	}

	*size = sh_vec->size();

	return sh_vec->data();
}

