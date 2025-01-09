#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include <SDL3/SDL.h>

// service that sets window and tray icon depending on program state

class StatusIndicator {
	RegistryMessageModelI& _rmm;
	Contact3Registry& _cr;

	SDL_Window* _main_window;
	// systray ptr here

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
			Contact3Registry& cr,
			SDL_Window* main_window
		);

		// does not actually render, just on the render thread
		void render(float delta);
};

