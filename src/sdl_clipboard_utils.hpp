#pragma once

#include <entt/container/dense_map.hpp>

#include <filesystem>
#include <vector>
#include <mutex>

// returns the mimetype (c-string) of the image in the clipboard
// or nullptr, if there is none
const char* clipboardHasImage(void);
const char* clipboardHasFileList(void);

bool mimeIsImage(const char* mime_type);
bool mimeIsFileList(const char* mime_type);

// TODO: add is file

// TODO: split out
std::string file_path_url_escape(const std::string&& value);
std::string file_path_to_file_url(const std::filesystem::path& path);

struct Clipboard final {
	~Clipboard(void);

	void setClipboardData(std::vector<std::string> mime_types, std::shared_ptr<std::vector<uint8_t>>&& data);

	private:
		// mimetype -> data
		entt::dense_map<std::string, std::shared_ptr<std::vector<uint8_t>>> _set_clipboard_data;
		std::mutex _set_clipboard_data_mutex; // might be called out of order

		static const void* clipboard_callback(void* userdata, const char* mime_type, size_t* size);
};
