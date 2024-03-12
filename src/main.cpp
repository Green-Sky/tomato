#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>

#include "./theme.hpp"

#include "./start_screen.hpp"

#include <memory>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
	// setup hints
	if (SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1") != SDL_TRUE) {
		std::cerr << "Failed to set '" << SDL_HINT_VIDEO_ALLOW_SCREENSAVER << "' to 1\n";
	}

	auto last_time_render = std::chrono::steady_clock::now();
	auto last_time_tick = std::chrono::steady_clock::now();

	// actual setup
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDL_Init failed (" << SDL_GetError() << ")\n";
		return 1;
	}

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
	{
		SDL_RendererInfo ri;
		if (SDL_GetRendererInfo(renderer.get(), &ri) == 0) {
			std::cout << "SDL Renderer: " << ri.name << "(f:" << ri.flags << ")\n";
		}
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_LIGHT) {
		ImGui::StyleColorsLight();
	} else {
		//ImGui::StyleColorsDark();
		setThemeGreen();
	}

	{
		ImGui::GetIO().Fonts->ClearFonts();
		ImFontConfig fontcfg;

		// upsampling to int looks almost ok
		const float font_size_scale = 1.3f;
		const float font_oversample = 4.f;

		// default font is pixel perfect at 13
		fontcfg.SizePixels = 13.f * font_size_scale;
		fontcfg.RasterizerDensity = font_oversample/font_size_scale;
		// normally density would be set to dpi scale of the display

		ImGui::GetIO().Fonts->AddFontDefault(&fontcfg);
		ImGui::GetIO().Fonts->Build();
	}

	ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get());
	ImGui_ImplSDLRenderer3_Init(renderer.get());

	std::unique_ptr<Screen> screen = std::make_unique<StartScreen>(renderer.get());


	bool quit = false;
	while (!quit) {
		auto new_time = std::chrono::steady_clock::now();

		const float time_delta_tick = std::chrono::duration<float, std::chrono::seconds::period>(new_time - last_time_tick).count();
		const float time_delta_render = std::chrono::duration<float, std::chrono::seconds::period>(new_time - last_time_render).count();

		bool tick = time_delta_tick >= screen->nextTick();
		bool render = time_delta_render >= screen->nextRender();

		if (tick) {
			Screen* ret_screen = screen->tick(time_delta_tick, quit);
			if (ret_screen != nullptr) {
				screen.reset(ret_screen);
			}
			last_time_tick = new_time;
		}

		// do events outside of tick/render, so they can influence reported intervals
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

		// can do both in the same loop
		if (render) {
			ImGui_ImplSDLRenderer3_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();

			{ // render
				Screen* ret_screen = screen->render(time_delta_render, quit);
				if (ret_screen != nullptr) {
					screen.reset(ret_screen);
				}
			}

			ImGui::Render();
			ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData());

			SDL_RenderPresent(renderer.get());
			// clearing after present is (should) more performant, but first frame is a mess
			SDL_SetRenderDrawColor(renderer.get(), 0x10, 0x10, 0x10, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer.get());

			last_time_render = new_time;
		}

		//// TODO: seperate out render and tick
		//const float time_to_next_loop = std::min<float>(screen->nextRender(), screen->nextTick());

		//std::this_thread::sleep_for( // time left to get to 60fps
			//std::chrono::duration<float, std::chrono::seconds::period>(time_to_next_loop)
			//- std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::steady_clock::now() - new_time) // time used for rendering
		//);


#if 1
		if (render || tick) {
			// why is windows like this
			//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield for 1ms
			SDL_Delay(1); // yield for 1ms
		} else {

	#if 0
			// pretty hacky and spins if close to next update
			// if next loop >= 1ms away, wait 1ms
			if (time_delta_tick+0.001f < screen->nextTick() && time_delta_render+0.001f < screen->nextRender()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield for 1ms
			}
	#else
			// dynamic sleep, sleeps the reminder till next update
			//std::this_thread::sleep_for(std::chrono::duration<float, std::chrono::seconds::period>(
				//std::min<float>(
					//screen->nextTick() - time_delta_tick,
					//screen->nextRender() - time_delta_render
				//)
			//));

			const float min_delay =
				std::min<float>(
					std::min<float>(
						screen->nextTick() - time_delta_tick,
						screen->nextRender() - time_delta_render
					),
					0.25f // dont sleep too long
				) * 1000.f;

			if (min_delay > 0.f) {
				SDL_Delay(uint32_t(min_delay));
			}

			// better in theory, but consumes more cpu on linux for some reason
			//SDL_WaitEventTimeout(nullptr, int32_t(
				//std::min<float>(
					//screen->nextTick() - time_delta_tick,
					//screen->nextRender() - time_delta_render
				//) * 1000.f
			//));
	#endif
		}
#else
		// why is windows like this
		//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield for 1ms
		SDL_Delay(1); // yield for 1ms
#endif
	}

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_Quit();

	return 0;
}

