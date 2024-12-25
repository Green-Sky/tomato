#include "./sys_tray.hpp"

#include "./icon_generator.hpp"

#include <memory>
#include <iostream>
#include <stdexcept>

SystemTray::SystemTray(SDL_Window* main_window) : _main_window(main_window) {
	std::cout << "ST: adding system tray\n";

	_tray = SDL_CreateTray(nullptr, "tomato");
	if (_tray == nullptr) {
		//std::cerr << "ST: failed to create SystemTray: " << SDL_GetError() << "\n";
		throw std::runtime_error(std::string{"ST: failed to create SystemTray: "} + SDL_GetError());
		return;
	}

	auto* tray_menu = SDL_CreateTrayMenu(_tray);
	{
		auto* entry_quit = SDL_InsertTrayEntryAt(tray_menu, 0, "Quit Tomato", SDL_TRAYENTRY_BUTTON);
		SDL_SetTrayEntryCallback(entry_quit,
			+[](void*, SDL_TrayEntry*){
				// this is thread safe and triggers the shutdown in the main thread
				SDL_Event quit_event;
				quit_event.quit = {
					SDL_EVENT_QUIT,
					0,
					SDL_GetTicksNS(),
				};
				SDL_PushEvent(&quit_event);
			}
		, nullptr);
	}
	{
		_entry_showhide = SDL_InsertTrayEntryAt(tray_menu, 0, "Hide Tomato", SDL_TRAYENTRY_BUTTON);
		SDL_SetTrayEntryCallback(_entry_showhide,
			+[](void* userdata, SDL_TrayEntry*){
				SDL_HideWindow(static_cast<SystemTray*>(userdata)->_main_window);
			}
		, this);
	}
}

SystemTray::~SystemTray(void) {
	if (_tray != nullptr) {
		SDL_DestroyTray(_tray);
		_tray = nullptr;
	}
}

void SystemTray::setIcon(SDL_Surface* surf) {
	if (_tray == nullptr) {
		return;
	}

	SDL_SetTrayIcon(_tray, surf);
}

void SystemTray::setStatusText(const std::string& status) {
	if (_tray == nullptr) {
		return;
	}

	if (_entry_status == nullptr) {
		_entry_status = SDL_InsertTrayEntryAt(SDL_GetTrayMenu(_tray), 0, status.c_str(), SDL_TRAYENTRY_DISABLED);
		return;
	}

	SDL_SetTrayEntryLabel(_entry_status, status.c_str());
}

void SystemTray::update(void) {
	if (_tray == nullptr) {
		return;
	}

	if (_entry_showhide != nullptr) {
		// check if window is open and adjust text and callback
		const bool hidden {(SDL_GetWindowFlags(_main_window) & SDL_WINDOW_HIDDEN) > 0};
		// TODO: cache state

		if (hidden) {
			SDL_SetTrayEntryLabel(_entry_showhide, "Show Tomato");
			SDL_SetTrayEntryCallback(_entry_showhide,
				+[](void* userdata, SDL_TrayEntry*){
					SDL_ShowWindow(static_cast<SystemTray*>(userdata)->_main_window);
				}
			, this);
		} else {
			SDL_SetTrayEntryLabel(_entry_showhide, "Hide Tomato");
			SDL_SetTrayEntryCallback(_entry_showhide,
				+[](void* userdata, SDL_TrayEntry*){
					SDL_HideWindow(static_cast<SystemTray*>(userdata)->_main_window);
				}
			, this);
		}
	}
}

