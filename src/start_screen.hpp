#pragma once

#include "./screen.hpp"

struct StartScreen final : public Screen {
	StartScreen(void);
	~StartScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

