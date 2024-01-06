#include "./main_screen.hpp"

#include <imgui/imgui.h>

#include <SDL3/SDL.h>

#include <memory>

MainScreen::MainScreen(SDL_Renderer* renderer_, std::string save_path, std::string save_password, std::vector<std::string> plugins) :
	renderer(renderer_),
	rmm(cr),
	mts(rmm),
	tc(save_path, save_password),
	tpi(tc.getTox()),
	ad(tc),
	tcm(cr, tc, tc),
	tmm(rmm, cr, tcm, tc, tc),
	ttm(rmm, cr, tcm, tc, tc),
	mmil(rmm),
	tam(rmm, cr, conf),
	sdlrtu(renderer_),
	cg(conf, rmm, cr, sdlrtu),
	sw(conf),
	tuiu(tc, conf),
	tdch(tpi)
{
	tel.subscribeAll(tc);

	conf.set("tox", "save_file_path", save_path);

	{ // name stuff
		auto name = tc.toxSelfGetName();
		if (name.empty()) {
			name = "tomato";
		}
		conf.set("tox", "name", name);
		tc.setSelfName(name); // TODO: this is ugly
	}

	// TODO: remove
	std::cout << "own address: " << tc.toxSelfGetAddressStr() << "\n";

	{ // setup plugin instances
		g_provideInstance<ConfigModelI>("ConfigModelI", "host", &conf);
		g_provideInstance<Contact3Registry>("Contact3Registry", "host", &cr);
		g_provideInstance<RegistryMessageModel>("RegistryMessageModel", "host", &rmm);

		g_provideInstance<ToxI>("ToxI", "host", &tc);
		g_provideInstance<ToxPrivateI>("ToxPrivateI", "host", &tpi);
		g_provideInstance<ToxEventProviderI>("ToxEventProviderI", "host", &tc);
		g_provideInstance<ToxContactModel2>("ToxContactModel2", "host", &tcm);

		// TODO: pm?

		// graphics
		g_provideInstance("ImGuiContext", "host", ImGui::GetCurrentContext());
		g_provideInstance<TextureUploaderI>("TextureUploaderI", "host", &sdlrtu);
	}

	for (const auto& ppath : plugins) {
		if (!pm.add(ppath)) {
			std::cerr << "MS error: loading plugin '" << ppath << "' failed!\n";
			// thow?
			assert(false && "failed to load plugin");
		}
	}

	conf.dump();
}

MainScreen::~MainScreen(void) {
}

bool MainScreen::handleEvent(SDL_Event& e) {
	if (e.type == SDL_EVENT_DROP_FILE) {
		std::cout << "DROP FILE: " << e.drop.file << "\n";
		cg.sendFilePath(e.drop.file);
		return true; // TODO: forward return succ from sendFilePath()
	}

	if (
		e.type == SDL_EVENT_WINDOW_MINIMIZED ||
		e.type == SDL_EVENT_WINDOW_HIDDEN ||
		e.type == SDL_EVENT_WINDOW_OCCLUDED // does this trigger on partial occlusion?
	) {
		auto* window = SDL_GetWindowFromID(e.window.windowID);
		auto* event_renderer = SDL_GetRenderer(window);
		if (event_renderer != nullptr && event_renderer == renderer) {
			// our window is now obstructed
			if (_window_hidden_ts < e.window.timestamp) {
				_window_hidden_ts = e.window.timestamp;
				_window_hidden = true;
				//std::cout << "TOMAT: window hidden " << e.window.timestamp << "\n";
			}
		}
		return true; // forward?
	}

	if (
		e.type == SDL_EVENT_WINDOW_SHOWN ||
		e.type == SDL_EVENT_WINDOW_RESTORED ||
		e.type == SDL_EVENT_WINDOW_EXPOSED
	) {
		auto* window = SDL_GetWindowFromID(e.window.windowID);
		auto* event_renderer = SDL_GetRenderer(window);
		if (event_renderer != nullptr && event_renderer == renderer) {
			if (_window_hidden_ts <= e.window.timestamp) {
				_window_hidden_ts = e.window.timestamp;
				_window_hidden = false;
				//std::cout << "TOMAT: window shown " << e.window.timestamp << "\n";
			}
		}
		return true; // forward?
	}

	return false;
}

Screen* MainScreen::render(float time_delta, bool& quit) {
	quit = !tc.iterate();

	tcm.iterate(time_delta);

	tam.iterate();

	// HACK: render the tomato main window first, with proper flags set.
	// flags need to be set the first time begin() is called.
	// and plugins are run before the main cg is run.
	{
		// TODO: maybe render cg earlier? or move the main window out of cg?
		constexpr auto bg_window_flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::Begin("tomato", nullptr, bg_window_flags);
		ImGui::End();
	}

	pm.tick(time_delta);
	tdch.tick(time_delta);

	mts.iterate();

	cg.render();
	sw.render();
	tuiu.render();
	tdch.render();

	{ // main window menubar injection
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				// ImGui::Separator(); // why do we not need this????
				if (ImGui::BeginMenu("Performance")) {
					{ // fps
						const auto targets = "normal\0power save\0";
						ImGui::SetNextItemWidth(ImGui::GetFontSize()*10);
						ImGui::Combo("fps mode", &_fps_perf_mode, targets, 4);
					}

					{ // compute
						const auto targets = "normal\0power save\0";
						ImGui::SetNextItemWidth(ImGui::GetFontSize()*10);
						ImGui::Combo("compute mode", &_compute_perf_mode, targets, 4);
						ImGui::SetItemTooltip("Limiting compute can slow down things filetransfers.");
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

		}
		ImGui::End();
	}


	if constexpr (false) {
		ImGui::ShowDemoWindow();
	}

	if (
		_fps_perf_mode == 1 || // TODO: magic
		_window_hidden
	) {
		_render_interval = 1.f/4.f;
	} else {
		_render_interval = 1.f/60.f;
	}

	return nullptr;
}

Screen* MainScreen::tick(float time_delta, bool& quit) {
	_min_tick_interval = std::max<float>(
		std::min<float>(
			tc.toxIterationInterval()/1000.f,
			0.03f // HACK: 30ms upper bound, should be the same as tox but will change
		),
		(_compute_perf_mode == 0 ? 0.001f : 0.1f) // in powersave fix the lowerbound to 100ms
	);

	return nullptr;
}

