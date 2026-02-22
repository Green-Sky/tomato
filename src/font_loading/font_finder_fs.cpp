#include "./font_finder_fs.hpp"

#include <filesystem>
#include <stdexcept>
#include <algorithm>

#include <iostream>
#include <string>

FontFinder_FileSystem::FontFinder_FileSystem(void) {
}

FontFinder_FileSystem::FontFinder_FileSystem(std::string_view folder_path) {
	std::filesystem::path dir{folder_path};
	if (!std::filesystem::is_directory(dir)) {
		throw std::runtime_error("path not a directory");
	}

	addFontDir(dir.generic_u8string());
}

// more optional params?
void FontFinder_FileSystem::addFontDir(std::string_view path) {
	for (const auto& dir_entry : std::filesystem::directory_iterator(path)) {
		if (dir_entry.is_directory()) {
			addFontDir(dir_entry.path().generic_u8string());
		} else if (dir_entry.is_regular_file()) {
			// TODO: test symlinks?
			addFontFile(dir_entry.path().generic_u8string());
		}
	}
}

// more optional params?
void FontFinder_FileSystem::addFontFile(std::string_view file_path) {
	std::filesystem::path path{file_path};
	std::string filename = path.filename().generic_u8string();
	std::string extension = path.extension().generic_u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

	// TODO: more? better ttc?
	if (extension != ".ttf" && extension != ".otf" && extension != ".ttc") {
		std::cout << "unsupported file extension '" << extension << "'\n";
		return;
	}

	if (filename.size() <= extension.size()) {
		return;
	}

	// remove extension from filename
	filename = filename.substr(0, filename.size()-extension.size());
	// filename should now contain UpperCamelCase and an optional -Style

	std::string processed_filename = filename;
	processed_filename.erase(std::remove(processed_filename.begin(), processed_filename.end(), ' '), processed_filename.end());
	std::transform(processed_filename.begin(), processed_filename.end(), processed_filename.begin(), [](unsigned char c) { return std::tolower(c); });

	_cache.push_back({
		filename,
		"", // TODO: style
		path.generic_u8string(),
		processed_filename
	});
}

const char* FontFinder_FileSystem::name(void) const {
	return "FileSystem";
}

std::string FontFinder_FileSystem::findBest(std::string_view family, std::string_view lang, bool color) const {
	const SystemFont* best_ptr = nullptr;
	int best_score = 0;

	if (_cache.empty()) {
		std::cerr << "Empty system cache\n";
	}

	for (const auto& it : _cache) {
		int score = 0;
		if (it.family == family) {
			score += 10;
		} else {
			std::string processed_family = std::string{family};
			processed_family.erase(std::remove(processed_family.begin(), processed_family.end(), ' '), processed_family.end());
			std::transform(processed_family.begin(), processed_family.end(), processed_family.begin(), [](unsigned char c) { return std::tolower(c); });

			if (it.processed_family == processed_family) {
				score += 9;
			} else {
				// search for family as substring in it.family (processed)
				if (it.processed_family.find(processed_family) != std::string::npos) {
					score += 2;
				}
			}
		}
		// TODO: fuzzy compare/edit distance

		// TODO: lang!

		//if (it.color != color) {
		//    score -= 1;
		//}

		if (best_score < score) {
			best_score = score;
			best_ptr = &it;
		}
		//std::cout << it.family << " had score " << score << "\n";
	}

	if (best_ptr == nullptr) {
		return "";
	} else {
		return best_ptr->file;
	}
}

