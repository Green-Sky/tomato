#pragma once

#include <SDL3/SDL.h>

#include <string>

class SystemTray {
	SDL_Window* _main_window {nullptr};
	SDL_Tray* _tray {nullptr};

	SDL_TrayEntry* _entry_showhide {nullptr};
	SDL_TrayEntry* _entry_status {nullptr};

	public:
		SystemTray(SDL_Window* main_window);
		~SystemTray(void);

		void setIcon(SDL_Surface* surf);
		void setStatusText(const std::string& status);

		// check if window is visible and adjust text
		void update(void);
};

