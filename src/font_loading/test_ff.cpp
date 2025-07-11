#include "./font_finder_i.hpp"
#include "./font_finder_fs.hpp"
#include "./font_finder_fc_sdl.hpp"

#include <iostream>

int main(void) {
	std::cout << "Font Finder Tests\n";

	auto fun_tests = [](FontFinderInterface& ff) {
		std::cout << "findBest('Emoji', '', true): '" << ff.findBest("Emoji", "", true) << "'\n";

		std::cout << "findBest('NotoColorEmoji'): '" << ff.findBest("NotoColorEmoji") << "'\n";
		std::cout << "findBest('Noto Color Emoji'): '" << ff.findBest("Noto Color Emoji") << "'\n";
		std::cout << "findBest('NotoSans'): '" << ff.findBest("NotoSans") << "'\n";
		std::cout << "findBest('Noto Sans'): '" << ff.findBest("Noto Sans") << "'\n";

		// windows file name
		std::cout << "findBest('Seguiemj'): '" << ff.findBest("Seguiemj") << "'\n";
		std::cout << "findBest('Segoe UI Emoji'): '" << ff.findBest("Segoe UI Emoji") << "'\n";
		std::cout << "findBest('Segoe UI'): '" << ff.findBest("Segoe UI") << "'\n";
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
//#elif mac
///System Folder/Fonts/
#else
		FontFinder_FileSystem ff{"/usr/share/fonts"};
#endif
		fun_tests(ff);
	} catch (...) {
		std::cerr << "caught exception\n";
	}

	return 0;
}

