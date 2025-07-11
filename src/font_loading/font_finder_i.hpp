#pragma once

#include <string>

struct FontFinderInterface {
	virtual ~FontFinderInterface(void) {}

	//struct FontIteratorI {
	//};
	//virtual FontIteratorI begin(void) const {}
	//virtual FontIteratorI end(void) const {}

	// (family is the name of the font)
	// "und-zsye" is the preffered emoji font lang
	// not every impl supports every field
	// returns a font file path, might be empty
	virtual std::string findBest(std::string_view family, std::string_view lang = "", bool color = false) const = 0;
};
