#include "./start_screen.hpp"

#include "./main_screen.hpp"

#include <memory>

StartScreen::StartScreen(SDL_Renderer* renderer) : _renderer(renderer) {
}

Screen* StartScreen::poll(bool&) {

	// TODO: imgui tox profile selector?
	// +----------------------------
	// | |*tox profile*| plugins |
	// | +------+ +--------
	// | | ICON | | fileselector/dropdown?
	// | |      | | password input
	// | +------+ +--------
	// +----------------------------

	auto new_screen = std::make_unique<MainScreen>(_renderer, "tomato.tox");
	return new_screen.release();
}

