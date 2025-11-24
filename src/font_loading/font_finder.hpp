#pragma once

#include "./font_finder_i.hpp"

#include <string_view>
#include <vector>
#include <memory>

// for typical usage, you want to:
// 1. construct finder backends once
// 2. have a desired font family or get the family list eg. getPlatformDefaultUIFamilies()
// 3. pass it to the getBestMatch() function


// returns a list of "family" names, that when put into the backends
// on current platform, will likely yield a match.
std::vector<std::string_view> getPlatformDefaultUIFamilies(void);
std::vector<std::string_view> getPlatformDefaultColorEmojiFamilies(void);

std::vector<std::unique_ptr<FontFinderInterface>> constructPlatformDefaultFinderBackends(void);

std::string getBestMatch(
	const std::vector<std::unique_ptr<FontFinderInterface>>& backends,
	std::string_view family,
	std::string_view lang = "",
	bool color = false
);

