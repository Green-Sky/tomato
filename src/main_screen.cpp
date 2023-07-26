#include "./main_screen.hpp"

#include <imgui/imgui.h>

#include <memory>

MainScreen::MainScreen(std::string save_path) {
}

Screen* MainScreen::poll(bool& quit) {
	bool open = !quit;
	ImGui::ShowDemoWindow(&open);
	quit = !open;

	return nullptr;
}

