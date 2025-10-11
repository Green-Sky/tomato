#include "./font_finder_i.hpp"
#include "./font_finder_fs.hpp"
#include "./font_finder_fc_sdl.hpp"

#include <iostream>
#include <stdexcept>

int main(void) {
	std::cout << "Font Finder Tests\n";

	auto fun_tests = [](FontFinderInterface& ff) {
		std::cout << "findBest('ColorEmoji', '', true): '" << ff.findBest("ColorEmoji", "", true) << "'\n";
		std::cout << "findBest('Emoji', '', true): '" << ff.findBest("Emoji", "", true) << "'\n";
		std::cout << "findBest('Emoji', '', false): '" << ff.findBest("Emoji", "", false) << "'\n";

		std::cout << "findBest('NotoColorEmoji'): '" << ff.findBest("NotoColorEmoji") << "'\n";
		std::cout << "findBest('Noto Color Emoji'): '" << ff.findBest("Noto Color Emoji") << "'\n";
		std::cout << "findBest('NotoSans'): '" << ff.findBest("NotoSans") << "'\n";
		std::cout << "findBest('Noto Sans'): '" << ff.findBest("Noto Sans") << "'\n";

		// windows file name
		std::cout << "findBest('Seguiemj'): '" << ff.findBest("Seguiemj") << "'\n";
		std::cout << "findBest('Segoe UI Emoji'): '" << ff.findBest("Segoe UI Emoji") << "'\n";
		std::cout << "findBest('Segoe UI'): '" << ff.findBest("Segoe UI") << "'\n";

		// macos
		std::cout << "findBest('Apple Color Emoji'): '" << ff.findBest("Apple Color Emoji") << "'\n";
		std::cout << "findBest('Helvetica'): '" << ff.findBest("Helvetica") << "'\n";
		std::cout << "findBest('San Francisco'): '" << ff.findBest("San Francisco") << "'\n";
	};


	std::cout << std::string(20, '=') << " FCSDL " << std::string(20, '=') << "\n";
	try {
		FontFinder_FontConfigSDL ff;
		fun_tests(ff);
	} catch (...) {
		std::cerr << "caught exception\n";
	}

	std::cout << std::string(20, '=') << " FS " << std::string(20, '=') << "\n";
	try {
#if defined(_WIN32) || defined(WIN32)
		FontFinder_FileSystem ff{"C:\\Windows\\Fonts"};
#elif __ANDROID__
		FontFinder_FileSystem ff{"/system/fonts"};
#elif __APPLE__
		// TODO: macos only
		FontFinder_FileSystem ff{"/System/Library/Fonts"};
#else
		FontFinder_FileSystem ff{"/usr/share/fonts"};
#endif
		fun_tests(ff);
	} catch (const std::runtime_error& e) {
		std::cerr << "caught runtime_error exception '" << e.what() << "'\n";
	} catch (...) {
		std::cerr << "caught exception\n";
	}

	return 0;
}

