#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>

#include "./theme.hpp"

#include "./start_screen.hpp"

#include <memory>
#include <future>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
	// setup hints
	if (SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1") != SDL_TRUE) {
		std::cerr << "Failed to set '" << SDL_HINT_VIDEO_ALLOW_SCREENSAVER << "' to 1\n";
	}

	auto last_time = std::chrono::steady_clock::now();

	// actual setup
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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

	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer {
		SDL_CreateRenderer(window.get(), nullptr, SDL_RENDERER_PRESENTVSYNC),
		&SDL_DestroyRenderer
	};
	if (!renderer) {
		std::cerr << "SDL_CreateRenderer failed (" << SDL_GetError() << ")\n";
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	//ImGui::StyleColorsDark();
	setThemeGreen();

	ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get());
	auto imgui_sdl_scope = std::async(std::launch::deferred, &ImGui_ImplSDL3_Shutdown);
	ImGui_ImplSDLRenderer3_Init(renderer.get());
	auto imgui_sdlrenderer_scope = std::async(std::launch::deferred, &ImGui_ImplSDLRenderer3_Shutdown);

	std::unique_ptr<Screen> screen = std::make_unique<StartScreen>(renderer.get());


	bool quit = false;
	while (!quit) {
		auto new_time = std::chrono::steady_clock::now();
		//const float time_delta {std::chrono::duration<float, std::chrono::seconds::period>(new_time - last_time).count()};
		last_time = new_time;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
				break;
			}

			if (screen->handleEvent(event)) {
				continue;
			}

			ImGui_ImplSDL3_ProcessEvent(&event);
		}
		if (quit) {
			break;
		}

		//float fps_target = 60.f;
		//if (SDL_GetWindowFlags(window.get()) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) {
			//fps_target = 30.f;
		//}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		//ImGui::ShowDemoWindow();

		Screen* ret_screen = screen->poll(quit);
		if (ret_screen != nullptr) {
			screen.reset(ret_screen);
		}

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData());

		SDL_RenderPresent(renderer.get());
		// clearing after present is (should) more performant, but first frame is a mess
		SDL_SetRenderDrawColor(renderer.get(), 0x10, 0x10, 0x10, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer.get());

		std::this_thread::sleep_for( // time left to get to 60fps
			std::chrono::duration<float, std::chrono::seconds::period>(0.0166f) // 60fps frame duration
			- std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::steady_clock::now() - new_time) // time used for rendering
		);
	}

	return 0;
}

