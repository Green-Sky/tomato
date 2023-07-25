#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>

#include "./theme.hpp"

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

	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer {
		SDL_CreateRenderer(window.get(), nullptr, 0),
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

	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
				break;
			}
			ImGui_ImplSDL3_ProcessEvent(&event);
		}
		if (quit) {
			break;
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData());

		SDL_RenderPresent(renderer.get());
		// clearing after present is (should) more performant, but first frame is a mess
		SDL_SetRenderDrawColor(renderer.get(), 0x10, 0x10, 0x10, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer.get());
	}

	return 0;
}

