#pragma once

#include <string>

struct FontInfo {
	std::string path;
	bool force_bitmap; // if info available
};

struct FontFinderInterface {
	virtual ~FontFinderInterface(void) {}

	virtual const char* name(void) const = 0;

	//struct FontIteratorI {
	//};
	//virtual FontIteratorI begin(void) const {}
	//virtual FontIteratorI end(void) const {}

	// (family is the name of the font)
	// "und-zsye" is the preffered emoji font lang
	// not every impl supports every field
	// returns
	// 	- a font file path, might be empty
	// 	- a flag when there is no vector info
	virtual FontInfo findBest(std::string_view family, std::string_view lang = "", bool color = false) const = 0;
};
