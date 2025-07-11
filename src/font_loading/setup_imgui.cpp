#include "./setup_imgui.hpp"

#include "./font_finder_i.hpp"
#include "./font_finder_fs.hpp"
#include "./font_finder_fc_sdl.hpp"

#include <imgui.h>

#include <iostream>

void setup_imgui_fonts(void) {
	FontFinder_FontConfigSDL ff_fc{};

	std::cout << "ff_fc:findBest('NotoColorEmoji'): '" << ff_fc.findBest("NotoColorEmoji") << "'\n";
	std::cout << "ff_fc:findBest('Noto Color Emoji'): '" << ff_fc.findBest("Noto Color Emoji") << "'\n";
	std::cout << "ff_fc:findBest('NotoSans'): '" << ff_fc.findBest("NotoSans") << "'\n";
	std::cout << "ff_fc:findBest('Noto Sans'): '" << ff_fc.findBest("Noto Sans") << "'\n";
}

