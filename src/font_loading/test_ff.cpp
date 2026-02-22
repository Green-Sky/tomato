#include "./font_finder_i.hpp"

#include "./font_finder.hpp"

#include <iostream>

// https://youtu.be/zOVuhrg-4ns

int main(void) {
	std::cout << "### Font Finder Tests ###\n";

	std::cout << "=== Construction of Backends ===\n";

	std::vector<std::unique_ptr<FontFinderInterface>> backends;
	backends = constructPlatformDefaultFinderBackends();

	std::cout << "=== getBestMatch ===\n";

	{
		std::string path;
		for (const auto family : getPlatformDefaultUIFamilies()) {
			path = getBestMatch(backends, family);
			if (!path.empty()) {
				break;
			}
		}
		if (!path.empty()) {
			std::cout << "found getPlatformDefaultUIFamily: '" << path << "'\n";
		} else {
			std::cout << "did not find getPlatformDefaultUIFamily\n";
		}
	}
	{
		std::string path;
		for (const auto family : getPlatformDefaultColorEmojiFamilies()) {
			path = getBestMatch(backends, family, "", true);
			if (!path.empty()) {
				break;
			}
		}
		if (!path.empty()) {
			std::cout << "found getPlatformDefaultColorEmojiFamily: '" << path << "'\n";
		} else {
			std::cout << "did not find getPlatformDefaultColorEmojiFamily\n";
		}
	}

	std::cout << "=== Extra tests ===\n";

	for (const auto& backend : backends) {
		const auto& ff = *backend;
		std::cout << "--- " << backend->name() << " ---" << "\n";
		std::cout << "findBest('ColorEmoji', '', true): '" << ff.findBest("ColorEmoji", "", true) << "'\n";
		std::cout << "findBest('Emoji', '', true): '" << ff.findBest("Emoji", "", true) << "'\n";
		std::cout << "findBest('Emoji', '', false): '" << ff.findBest("Emoji", "", false) << "'\n";

		std::cout << "findBest('NotoColorEmoji'): '" << ff.findBest("NotoColorEmoji") << "'\n";
		std::cout << "findBest('Noto Color Emoji'): '" << ff.findBest("Noto Color Emoji") << "'\n";
		std::cout << "findBest('NotoSans'): '" << ff.findBest("NotoSans") << "'\n";
		std::cout << "findBest('Noto Sans'): '" << ff.findBest("Noto Sans") << "'\n";
		std::cout << "findBest('Ubuntu'): '" << ff.findBest("Ubuntu") << "'\n";

		// windows file name
		std::cout << "findBest('Seguiemj'): '" << ff.findBest("Seguiemj") << "'\n";
		std::cout << "findBest('Segoe UI Emoji'): '" << ff.findBest("Segoe UI Emoji") << "'\n";
		std::cout << "findBest('Segoe UI'): '" << ff.findBest("Segoe UI") << "'\n";

		// macos
		std::cout << "findBest('Apple Color Emoji'): '" << ff.findBest("Apple Color Emoji") << "'\n";
		std::cout << "findBest('Helvetica'): '" << ff.findBest("Helvetica") << "'\n";
		std::cout << "findBest('San Francisco'): '" << ff.findBest("San Francisco") << "'\n";

		// system default aliases (typical on linux with fc)
		std::cout << "findBest('serif'): '" << ff.findBest("serif") << "'\n";
		std::cout << "findBest('serif', 'ar'): '" << ff.findBest("serif", "ar") << "'\n";
		std::cout << "findBest('serif', 'ja'): '" << ff.findBest("serif", "ja") << "'\n";
		std::cout << "findBest('sans-serif'): '" << ff.findBest("sans-serif") << "'\n";
		std::cout << "findBest('sans-serif', 'ar'): '" << ff.findBest("sans-serif", "ar") << "'\n";
		std::cout << "findBest('sans-serif', 'ja'): '" << ff.findBest("sans-serif", "ja") << "'\n";
		std::cout << "findBest('monospace'): '" << ff.findBest("monospace") << "'\n";

		{
			std::string path;
			for (const auto family : getPlatformDefaultUIFamilies()) {
				path = ff.findBest(family);
				if (!path.empty()) {
					break;
				}
			}
			if (!path.empty()) {
				std::cout << "found getPlatformDefaultUIFamily: '" << path << "'\n";
			} else {
				std::cout << "did not find getPlatformDefaultUIFamily\n";
			}
		}

		{
			std::string path;
			for (const auto family : getPlatformDefaultColorEmojiFamilies()) {
				path = ff.findBest(family, "", true);
				if (!path.empty()) {
					break;
				}
			}
			if (!path.empty()) {
				std::cout << "found getPlatformDefaultColorEmojiFamily: '" << path << "'\n";
			} else {
				std::cout << "did not find getPlatformDefaultColorEmojiFamily\n";
			}
		}
	}

	return 0;
}

