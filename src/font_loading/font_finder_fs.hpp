#pragma once

#include "./font_finder_i.hpp"

#include <string>
#include <string_view>
#include <vector>

// using raw file and folder paths provided and tries to give the best fitting font
// eg on android (/system/fonts/)
struct FontFinder_FileSystem : public FontFinderInterface {
	struct SystemFont {
		std::string family;
		std::string style;
		std::string file;

		std::string processed_family;
	};

	std::vector<SystemFont> _cache;

	FontFinder_FileSystem(void);
	FontFinder_FileSystem(std::string_view folder_path);

	// more optional params?
	void addFontDir(std::string_view path);
	void addFontFile(std::string_view path);

	const char* name(void) const override;

	std::string findBest(std::string_view family, std::string_view lang = "", bool color = false) const override;
};
