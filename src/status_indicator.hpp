#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./sys_tray.hpp"

#include <SDL3/SDL.h>

// service that sets window and tray icon depending on program state

class StatusIndicator {
	RegistryMessageModelI& _rmm;
	ContactStore4I& _cs;

	SDL_Window* _main_window;
	SystemTray* _tray;

	float _cooldown {1.f};

	enum class State {
		base,
		unread,
		none, // only used for initialization
	} _state = State::none;

	void updateState(State state);

	public:
		StatusIndicator(
			RegistryMessageModelI& rmm,
			ContactStore4I& cs,
			SDL_Window* main_window,
			SystemTray* tray = nullptr
		);

		// does not actually render, just on the render thread
		void render(float delta);
};

