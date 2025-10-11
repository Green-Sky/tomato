#pragma once

#include "./font_finder_i.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <vector>
#include <string_view>
#include <algorithm>

#include <iostream>

// provides a fontconfig (typically linux/unix) implementation
// using commandline tools using sdl process management
struct FontFinder_FontConfigSDL : public FontFinderInterface {
	struct SystemFont {
		std::string family;
		std::string processed_family;
		bool color {false};

		// lang
		// for emojies:
		// lang: und-zsye
	};

	std::vector<SystemFont> _cache;

	bool fillCache(void);

	static std::string getFontFile(const std::string& font_name);

	FontFinder_FontConfigSDL(void);

	void reset(void);

	std::string findBest(std::string_view family, std::string_view lang = "", bool color = false) const override;
};
