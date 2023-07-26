#include "./start_screen.hpp"

#include "./main_screen.hpp"

#include <memory>

StartScreen::StartScreen(void) {
}

Screen* StartScreen::poll(bool&) {

	// TODO: imgui tox profile selector?
	// +------------------------
	// | +------+ +--------
	// | | ICON | | fileselector/dropdown?
	// | |      | | password input
	// | +------+ +--------
	// +------------------------

	auto new_screen = std::make_unique<MainScreen>("tomato.tox");
	return new_screen.release();
}

