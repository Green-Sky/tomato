#pragma once

#include <SDL3/SDL.h>

class SystemTray {
	SDL_Tray* _tray {nullptr};

	public:
		SystemTray(void);
		~SystemTray(void);
};

