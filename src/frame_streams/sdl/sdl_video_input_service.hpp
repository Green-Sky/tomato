#pragma once

#include <solanaceae/object_store/fwd.hpp>

#include <SDL3/SDL.h>

// manages sdl camera sources
class SDLVideoInputService {
	ObjectStore2& _os;

	bool _subsystem_init{false};

	bool addDevice(SDL_CameraID device);
	bool removeDevice(SDL_CameraID device);

	public:
		SDLVideoInputService(ObjectStore2& os);
		~SDLVideoInputService(void);

		bool handleEvent(SDL_Event& e);
};

