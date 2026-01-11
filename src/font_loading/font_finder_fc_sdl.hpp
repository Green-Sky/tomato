#pragma once

#include "./font_finder_i.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <vector>
#include <string_view>


// provides a fontconfig (typically linux/unix) implementation
// using commandline tools using sdl process management
struct FontFinder_FontConfigSDL : public FontFinderInterface {
	struct SystemFont {
		std::string family;
		std::string processed_family;

		// lang
		// for emojies: und-zsye
		// langs are concatinated with |
		std::string lang;

		bool color {false};
	};

	std::vector<SystemFont> _cache;

	bool fillCache(void);

	static std::string getFontFile(const std::string& font_name);

	FontFinder_FontConfigSDL(void);

	void reset(void);

	const char* name(void) const override;

	std::string findBest(std::string_view family, std::string_view lang = "", bool color = false) const override;
};
