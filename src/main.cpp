#include "SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>
#include <future>
#include <iostream>

int main(int argc, char** argv) {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::cerr << "SDL_Init failed (" << SDL_GetError() << ")\n";
		return 1;
	}
	// me just messing with RAII cleanup
	auto sdl_scope = std::async(std::launch::deferred, &SDL_Quit);


	// more RAII
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window {
		SDL_CreateWindow("tomato", 640, 480, SDL_WINDOW_RESIZABLE),
		&SDL_DestroyWindow
	};

	if (!window) {
		std::cerr << "SDL_CreateWindow failed (" << SDL_GetError() << ")\n";
		return 1;
	}

	// more RAII
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer {
		SDL_CreateRenderer(window.get(), nullptr, 0),
		&SDL_DestroyRenderer
	};

	if (!window) {
		std::cerr << "SDL_CreateWindow failed (" << SDL_GetError() << ")\n";
		return 1;
	}

	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
				break;
			}
		}
		if (quit) {
			break;
		}

		SDL_SetRenderDrawColor(renderer.get(), 0x10, 0x10, 0x10, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer.get());

		SDL_FRect rect{100, 100, 100, 100};
		SDL_SetRenderDrawColor(renderer.get(), 0xff, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderRect(renderer.get(), &rect);

		SDL_RenderPresent(renderer.get());
	}

	return 0;
}

