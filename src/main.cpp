#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <implot.h>

#include "version.hpp"

#include "./theme.hpp"
#include "./chat_gui/theme.hpp"

#include "./sys_check.hpp"

#include "./start_screen.hpp"

#include <filesystem>
#include <memory>
#include <iostream>
#include <string_view>
#include <thread>
#include <chrono>

#ifdef TOMATO_BREAKPAD
#	include "./breakpad_client.hpp"
#endif

int main(int argc, char** argv) {
	runSysCheck();

	std::cout << "tomato " TOMATO_VERSION_STR;
	if (TOMATO_GIT_DEPTH != 0) {
		std::cout << "-" << TOMATO_GIT_DEPTH;
	}
	if (std::string_view{TOMATO_GIT_COMMIT} != "UNK") {
		std::cout << "+git." << TOMATO_GIT_COMMIT;
	}
	std::cout << "\n";

#ifdef TOMATO_BREAKPAD
	// TODO: maybe run before sys check?
	BREAKPAD_MAIN_INIT;
	std::cout << "Breakpad handler installed.\n";
#endif

	// better args
	std::vector<std::string_view> args;
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}


	SDL_SetAppMetadata("tomato", TOMATO_VERSION_STR, nullptr);

#ifdef __ANDROID__
	// change current working dir to internal storage
	std::filesystem::current_path(SDL_GetAndroidInternalStoragePath());
#endif

	// setup hints
#ifndef __ANDROID__
	if (!SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1")) {
		std::cerr << "Failed to set '" << SDL_HINT_VIDEO_ALLOW_SCREENSAVER << "' to 1\n";
	}
#endif

	auto last_time_render = std::chrono::steady_clock::now();
	auto last_time_tick = std::chrono::steady_clock::now();

	// actual setup
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << "SDL_Init failed (" << SDL_GetError() << ")\n";
		return 1;
	}

	// more RAII
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window {
		SDL_CreateWindow("tomato", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY),
		&SDL_DestroyWindow
	};

	if (!window) {
		std::cerr << "SDL_CreateWindow failed (" << SDL_GetError() << ")\n";
		return 1;
	}

	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer {
		SDL_CreateRenderer(window.get(), nullptr),
		&SDL_DestroyRenderer
	};
	if (!renderer) {
		std::cerr << "SDL_CreateRenderer failed (" << SDL_GetError() << ")\n";
		return 1;
	}

	//SDL_SetRenderVSync(renderer.get(), SDL_RENDERER_VSYNC_ADAPTIVE);
	SDL_SetRenderVSync(renderer.get(), SDL_RENDERER_VSYNC_DISABLED);

	{
		std::cout << "SDL Renderer: '" << SDL_GetRendererName(renderer.get());

		int vsync {SDL_RENDERER_VSYNC_DISABLED};
		if (SDL_GetRenderVSync(renderer.get(), &vsync)) {
			std::cout << "' vsync: " << vsync << "\n";
		} else {
			std::cout << "'\n";
		}
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	float display_scale = SDL_GetWindowDisplayScale(window.get());
	if (display_scale < 0.001f) {
		// error?
		display_scale = 1.f;
	}
	ImGui::GetStyle().ScaleAllSizes(display_scale);

	Theme theme;
	if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_LIGHT) {
		ImGui::StyleColorsLight();
		theme = getDefaultThemeLight();
	} else {
		//ImGui::StyleColorsDark();
		setThemeGreen();
		theme = getDefaultThemeDark();
	}


	ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get());
	ImGui_ImplSDLRenderer3_Init(renderer.get());

	std::unique_ptr<Screen> screen = std::make_unique<StartScreen>(args, renderer.get(), theme);

	bool is_background = false;
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
			} else if (event.type == SDL_EVENT_WILL_ENTER_BACKGROUND) {
				is_background = true;
			} else if (event.type == SDL_EVENT_DID_ENTER_FOREGROUND) {
				is_background = false;
			}

			if (screen->handleEvent(event)) {
				continue;
			}

			ImGui_ImplSDL3_ProcessEvent(&event);
		}
		if (quit) {
			break;
		}

		if (is_background) {
			render = false;
		}

		// can do both in the same loop
		if (render) {
			const auto render_start_time = std::chrono::steady_clock::now();

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
			ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());

			// TODO: time mesurements and dynamically reduce fps if present takes long
			SDL_RenderPresent(renderer.get());
			// clearing after present is (should) more performant, but first frame is a mess
			SDL_SetRenderDrawColor(renderer.get(), 0x10, 0x10, 0x10, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer.get());

			const auto render_end_time = std::chrono::steady_clock::now();

			float render_duration = std::chrono::duration<float, std::chrono::seconds::period>(render_end_time - render_start_time).count();
			if (render_duration > 0.1666f) { // ~60fps
				std::cerr << "MAIN warning: render took very long: " << render_duration << "s\n";
			}

			last_time_render = new_time;
		}

		if (render || tick) {
			// why is windows like this
			//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield for 1ms
			SDL_Delay(1); // yield for 1ms
		} else {
			const float min_delay =
				std::min<float>(
					std::min<float>(
						screen->nextTick() - time_delta_tick,
						screen->nextRender() - time_delta_render
					),
					0.5f // dont sleep too long
				) * 1000.f;

			// mix both worlds to try to reasonable improve responsivenes
			if (min_delay > 200.f) {
				SDL_Delay(uint32_t(min_delay - 200.f));
			} else if (min_delay > 0.f) {
				// you are annoyingly in efficent
				SDL_WaitEventTimeout(nullptr, int32_t(min_delay));
			}

			// better in theory, but consumes more cpu on linux for some reason
			//SDL_WaitEventTimeout(nullptr, int32_t(min_delay));
		}
	}

	// TODO: use scope for the unique ptrs

	screen.reset();

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	renderer.reset();
	window.reset();

	SDL_Quit();

	return 0;
}

