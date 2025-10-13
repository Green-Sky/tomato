#include "./font_finder.hpp"

#include "./font_finder_fc_sdl.hpp"
#include "./font_finder_fs.hpp"

#include <stdexcept>

#include <iostream>

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

std::vector<std::unique_ptr<FontFinderInterface>> constructPlatformDefaultFinderBackends(void) {
	std::vector<std::unique_ptr<FontFinderInterface>> list;

	try {
		// TODO: should we skip this on windows?
		std::unique_ptr<FontFinderInterface> ff = std::make_unique<FontFinder_FontConfigSDL>();
		list.push_back(std::move(ff));
	} catch (const std::runtime_error& e) {
		std::cerr << "caught runtime_error exception '" << e.what() << "'\n";
	} catch (...) {
		std::cerr << "caught exception\n";
	}

	try {

#if defined(_WIN32) || defined(WIN32)
		std::unique_ptr<FontFinderInterface> ff = std::make_unique<FontFinder_FileSystem>("C:\\Windows\\Fonts");
#elif __ANDROID__
		std::unique_ptr<FontFinderInterface> ff = std::make_unique<FontFinder_FileSystem>("/system/fonts");
#elif __APPLE__
		// TODO: macos only
		std::unique_ptr<FontFinderInterface> ff = std::make_unique<FontFinder_FileSystem>("/System/Library/Fonts");
#else
		std::unique_ptr<FontFinderInterface> ff = std::make_unique<FontFinder_FileSystem>("/usr/share/fonts");
#endif

		list.push_back(std::move(ff));
	} catch (const std::runtime_error& e) {
		std::cerr << "caught runtime_error exception '" << e.what() << "'\n";
	} catch (...) {
		std::cerr << "caught exception\n";
	}

	return list;
}

std::string getBestMatch(
	const std::vector<std::unique_ptr<FontFinderInterface>>& backends,
	std::string_view family,
	std::string_view lang,
	bool color
) {
	for (const auto& backend : backends) {
		try {
			auto res = backend->findBest(family, lang, color);
			if (!res.empty()) {
				return res;
			}
		} catch (const std::runtime_error& e) {
			std::cerr << "caught runtime_error exception '" << e.what() << "'\n";
		} catch (...) {
			std::cerr << "caught exception\n";
		}
	}

	return ""; // none found
}

