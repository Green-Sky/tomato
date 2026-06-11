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
		FontInfo match;
		for (const auto family : getPlatformDefaultUIFamilies()) {
			match = getBestMatch(backends, family);
			if (!match.path.empty()) {
				break;
			}
		}
		if (!match.path.empty()) {
			std::cout << "found getPlatformDefaultUIFamily: '" << match.path << "' fbitmap:" << match.force_bitmap << "\n";
		} else {
			std::cout << "did not find getPlatformDefaultUIFamily\n";
		}
	}
	{
		FontInfo match;
		for (const auto family : getPlatformDefaultColorEmojiFamilies()) {
			match = getBestMatch(backends, family, "", true);
			if (!match.path.empty()) {
				break;
			}
		}
		if (!match.path.empty()) {
			std::cout << "found getPlatformDefaultColorEmojiFamily: '" << match.path << "' fbitmap:" << match.force_bitmap << "\n";
		} else {
			std::cout << "did not find getPlatformDefaultColorEmojiFamily\n";
		}
	}

	std::cout << "=== Extra tests ===\n";

	for (const auto& backend : backends) {
		const auto& ff = *backend;
		std::cout << "--- " << backend->name() << " ---" << "\n";
		std::cout << "findBest('ColorEmoji', '', true): '" << ff.findBest("ColorEmoji", "", true).path << "'\n";
		std::cout << "findBest('Emoji', '', true): '" << ff.findBest("Emoji", "", true).path << "'\n";
		std::cout << "findBest('Emoji', '', false): '" << ff.findBest("Emoji", "", false).path << "'\n";

		std::cout << "findBest('NotoColorEmoji'): '" << ff.findBest("NotoColorEmoji").path << "'\n";
		std::cout << "findBest('Noto Color Emoji'): '" << ff.findBest("Noto Color Emoji").path << "'\n";
		std::cout << "findBest('NotoSans'): '" << ff.findBest("NotoSans").path << "'\n";
		std::cout << "findBest('Noto Sans'): '" << ff.findBest("Noto Sans").path << "'\n";
		std::cout << "findBest('Ubuntu'): '" << ff.findBest("Ubuntu").path << "'\n";

		// windows file name
		std::cout << "findBest('Seguiemj'): '" << ff.findBest("Seguiemj").path << "'\n";
		std::cout << "findBest('Segoe UI Emoji'): '" << ff.findBest("Segoe UI Emoji").path << "'\n";
		std::cout << "findBest('Segoe UI'): '" << ff.findBest("Segoe UI").path << "'\n";

		// macos
		std::cout << "findBest('Apple Color Emoji'): '" << ff.findBest("Apple Color Emoji").path << "'\n";
		std::cout << "findBest('Helvetica'): '" << ff.findBest("Helvetica").path << "'\n";
		std::cout << "findBest('San Francisco'): '" << ff.findBest("San Francisco").path << "'\n";

		// system default aliases (typical on linux with fc)
		std::cout << "findBest('serif'): '" << ff.findBest("serif").path << "'\n";
		std::cout << "findBest('serif', 'ar'): '" << ff.findBest("serif", "ar").path << "'\n";
		std::cout << "findBest('serif', 'ja'): '" << ff.findBest("serif", "ja").path << "'\n";
		std::cout << "findBest('sans-serif'): '" << ff.findBest("sans-serif").path << "'\n";
		std::cout << "findBest('sans-serif', 'ar'): '" << ff.findBest("sans-serif", "ar").path << "'\n";
		std::cout << "findBest('sans-serif', 'ja'): '" << ff.findBest("sans-serif", "ja").path << "'\n";
		std::cout << "findBest('monospace'): '" << ff.findBest("monospace").path << "'\n";

		{
			FontInfo match;
			for (const auto family : getPlatformDefaultUIFamilies()) {
				match = ff.findBest(family);
				if (!match.path.empty()) {
					break;
				}
			}
			if (!match.path.empty()) {
				std::cout << "found getPlatformDefaultUIFamily: '" << match.path << "' fbitmap:" << match.force_bitmap << "\n";
			} else {
				std::cout << "did not find getPlatformDefaultUIFamily\n";
			}
		}

		{
			FontInfo match;
			for (const auto family : getPlatformDefaultColorEmojiFamilies()) {
				match = ff.findBest(family, "", true);
				if (!match.path.empty()) {
					break;
				}
			}
			if (!match.path.empty()) {
				std::cout << "found getPlatformDefaultColorEmojiFamily: '" << match.path << "' fbitmap:" << match.force_bitmap << "\n";
			} else {
				std::cout << "did not find getPlatformDefaultColorEmojiFamily\n";
			}
		}
	}

	return 0;
}

