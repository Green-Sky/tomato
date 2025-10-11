#pragma once

#include <string_view>
#include <vector>

// returns a list of "family" names, that when put into the backends
// on current platform, will likely yield a match.
std::vector<std::string_view> getPlatformDefaultUIFamilies(void);
std::vector<std::string_view> getPlatformDefaultColorEmojiFamilies(void);
