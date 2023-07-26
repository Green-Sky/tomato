#pragma once

#include "./screen.hpp"

#include <string>

struct MainScreen final : public Screen {
	MainScreen(std::string save_path);
	~MainScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

