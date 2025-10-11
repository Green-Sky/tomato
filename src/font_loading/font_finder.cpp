#include "./font_finder.hpp"

std::vector<std::string_view> getPlatformDefaultUIFamilies(void) {
#if defined(_WIN32) || defined(WIN32)
	return {
		"Segoe UI",
		"sans-serif",
		"Noto Sans",
		"Helvetica",
		"serif",
	};
#elif __ANDROID__
	return {
		"Noto Sans",
		"sans-serif",
		"Droid Sans",
		"Roboto",
		"serif",
	};
#elif __APPLE__
	return {
		"Helvetica",
		"sans-serif",
		"Noto Sans",
		"serif",
	};
#else // assume linux, prs welcome
	return {
		"sans-serif",
		"Noto Sans",
		"Ubuntu",
		"serif",
	};
#endif
}

std::vector<std::string_view> getPlatformDefaultColorEmojiFamilies(void) {
#if defined(_WIN32) || defined(WIN32)
	return {
		"Seguiemj", // Segoe UI Emoji
		"Color Emoji",
		"Emoji",
	};
#elif __ANDROID__
	return {
		"Noto Color Emoji",
		"Color Emoji",
		"Emoji",
	};
#elif __APPLE__
	return {
		"Apple Color Emoji",
		"Color Emoji",
		"Emoji",
	};
#else // assume linux, other platform prs welcome
	return {
		"Color Emoji",
		"Emoji",
	};
#endif
}
