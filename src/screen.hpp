#pragma once

struct Screen {
	virtual ~Screen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	virtual Screen* poll(bool& quit) = 0;
};

