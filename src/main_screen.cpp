#include "./main_screen.hpp"

#include <solanaceae/contact/components.hpp>

#include <imgui/imgui.h>

#include <SDL3/SDL.h>

#include <memory>
#include <cmath>

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
	tffom(cr, rmm, tcm, tc, tc),
	mmil(rmm),
	tam(rmm, cr, conf),
	sdlrtu(renderer_),
	tal(cr),
	contact_tc(tal, sdlrtu),
	mil(),
	msg_tc(mil, sdlrtu),
	cg(conf, rmm, cr, sdlrtu, contact_tc, msg_tc),
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
		g_provideInstance<Contact3Registry>("Contact3Registry", "1", "host", &cr);
		g_provideInstance<RegistryMessageModel>("RegistryMessageModel", "host", &rmm);

		g_provideInstance<ToxI>("ToxI", "host", &tc);
		g_provideInstance<ToxPrivateI>("ToxPrivateI", "host", &tpi);
		g_provideInstance<ToxEventProviderI>("ToxEventProviderI", "host", &tc);
		g_provideInstance<ToxContactModel2>("ToxContactModel2", "host", &tcm);

		// TODO: pm?

		// graphics
		g_provideInstance("ImGuiContext", ImGui::GetVersion(), "host", ImGui::GetCurrentContext());
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
		std::cout << "DROP FILE: " << e.drop.data << "\n";
		cg.sendFilePath(e.drop.data);
		_render_interval = 1.f/60.f; // TODO: magic
		_time_since_event = 0.f;
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

				if (_window_hidden) {
					// if window was previously hidden, we shorten the wait for the next frame
					_render_interval = 1.f/60.f;
				}

				_window_hidden = false;
				//std::cout << "TOMAT: window shown " << e.window.timestamp << "\n";
			}
		}
		_render_interval = 1.f/60.f; // TODO: magic
		_time_since_event = 0.f;
		return true; // forward?
	}

	if (
		_fps_perf_mode <= 1 && (
			// those are all the events imgui polls
			e.type == SDL_EVENT_MOUSE_MOTION ||
			e.type == SDL_EVENT_MOUSE_WHEEL ||
			e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
			e.type == SDL_EVENT_MOUSE_BUTTON_UP ||
			e.type == SDL_EVENT_TEXT_INPUT ||
			e.type == SDL_EVENT_KEY_DOWN ||
			e.type == SDL_EVENT_KEY_UP ||
			e.type == SDL_EVENT_WINDOW_MOUSE_ENTER ||
			e.type == SDL_EVENT_WINDOW_MOUSE_LEAVE ||
			e.type == SDL_EVENT_WINDOW_FOCUS_GAINED ||
			e.type == SDL_EVENT_WINDOW_FOCUS_LOST
		)
	) {
		_render_interval = 1.f/60.f; // TODO: magic
		_time_since_event = 0.f;
	}

	return false;
}

Screen* MainScreen::render(float time_delta, bool&) {
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

	const float pm_interval = pm.render(time_delta); // render

	// TODO: move this somewhere else!!!
	// needs both tal and tc <.<
	if (!cr.storage<Contact::Components::TagAvatarInvalidate>().empty()) { // handle force-reloads for avatars
		std::vector<Contact3> to_purge;
		cr.view<Contact::Components::TagAvatarInvalidate>().each([&to_purge](const Contact3 c) {
			to_purge.push_back(c);
		});
		cr.remove<Contact::Components::TagAvatarInvalidate>(to_purge.cbegin(), to_purge.cend());
		contact_tc.invalidate(to_purge);
	}
	// ACTUALLY NOT IF RENDERED, MOVED LOGIC TO ABOVE
	// it might unload textures, so it needs to be done before rendering
	const float ctc_interval = contact_tc.update();
	const float msgtc_interval = msg_tc.update();

	const float cg_interval = cg.render(time_delta); // render
	sw.render(); // render
	tuiu.render(); // render
	tdch.render(); // render

	{ // main window menubar injection
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				// ImGui::Separator(); // why do we not need this????
				if (ImGui::BeginMenu("Performance")) {
					{ // fps
						const auto targets = "normal\0reduced\0powersave\0";
						ImGui::SetNextItemWidth(ImGui::GetFontSize()*10);
						ImGui::Combo("fps mode", &_fps_perf_mode, targets, 4);
					}

					{ // compute
						const auto targets = "normal\0powersave\0extreme powersave\0";
						ImGui::SetNextItemWidth(ImGui::GetFontSize()*10);
						ImGui::Combo("compute mode", &_compute_perf_mode, targets, 4);
						ImGui::SetItemTooltip("Limiting compute can slow down things like filetransfers!");
					}

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Settings")) {
					ImGui::SeparatorText("ImGui");

					if (ImGui::MenuItem("Style Editor")) {
						_show_tool_style_editor = true;
					}

					if (ImGui::MenuItem("Metrics")) {
						_show_tool_metrics = true;
					}

					if (ImGui::MenuItem("Debug Log")) {
						_show_tool_debug_log = true;
					}

					if (ImGui::MenuItem("ID Stack Tool")) {
						_show_tool_id_stack = true;
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

		}
		ImGui::End();
	}

	if (_show_tool_style_editor) {
		if (ImGui::Begin("Dear ImGui Style Editor", &_show_tool_style_editor)) {
			ImGui::ShowStyleEditor();
		}
		ImGui::End();
	}

	if (_show_tool_metrics) {
		ImGui::ShowMetricsWindow(&_show_tool_metrics);
	}

	if (_show_tool_debug_log) {
		ImGui::ShowDebugLogWindow(&_show_tool_debug_log);
	}

	if (_show_tool_id_stack) {
		ImGui::ShowIDStackToolWindow(&_show_tool_id_stack);
	}

	if constexpr (false) {
		ImGui::ShowDemoWindow();
	}

	float tc_unfinished_queue_interval;
	{ // load rendered but not loaded textures
		bool unfinished_work_queue = contact_tc.workLoadQueue();
		unfinished_work_queue = unfinished_work_queue || msg_tc.workLoadQueue();

		if (unfinished_work_queue) {
			tc_unfinished_queue_interval = 0.1f; // so we can get images loaded faster
		} else {
			tc_unfinished_queue_interval = 1.f; // TODO: higher min fps?
		}
	}

	// calculate interval for next frame
	// normal:
	//  - if < 1.5sec since last event
	//    - min all and clamp(1/60, 1/1)
	//  - if < 30sec since last event
	//    - min all (anim + everything else) clamp(1/60, 1/1) (maybe less?)
	//  - else
	//    - min without anim and clamp(1/60, 1/1) (maybe more?)
	// reduced:
	//  - if < 1sec since last event
	//    - min all and clamp(1/60, 1/1)
	//  - if < 10sec since last event
	//    - min all (anim + everything else) clamp(1/10, 1/1)
	//  - else
	//    - min without anim and max clamp(1/10, 1/1)
	// powersave:
	//  - if < 0sec since last event
	//    - (ignored)
	//  - if < 1sec since last event
	//    - min all (anim + everything else) clamp(1/8, 1/1)
	//  - else
	//    - min without anim and clamp(1/1, 1/1)
	struct PerfProfileRender {
		float low_delay_window {1.5f};
		float low_delay_min {1.f/60.f};
		float low_delay_max {1.f/60.f};

		float mid_delay_window {30.f};
		float mid_delay_min {1.f/60.f};
		float mid_delay_max {1.f/2.f};

		// also when main window hidden
		float else_delay_min {1.f/60.f};
		float else_delay_max {1.f/2.f};
	};

	const static PerfProfileRender normalPerfProfile{
		//1.5f,		// low_delay_window
		//1.f/60.f,	// low_delay_min
		//1.f/60.f,	// low_delay_max

		//30.f,		// mid_delay_window
		//1.f/60.f,	// mid_delay_min
		//1.f/2.f,	// mid_delay_max

		//1.f/60.f,	// else_delay_min
		//1.f/2.f,	// else_delay_max
	};
	const static PerfProfileRender reducedPerfProfile{
		1.f,		// low_delay_window
		1.f/60.f,	// low_delay_min
		1.f/30.f,	// low_delay_max

		10.f,		// mid_delay_window
		1.f/10.f,	// mid_delay_min
		1.f/4.f,	// mid_delay_max

		1.f/10.f,	// else_delay_min
		1.f,		// else_delay_max
	};
	// TODO: fix powersave by adjusting it in the events handler (make ppr member)
	const static PerfProfileRender powersavePerfProfile{
		// no window -> ignore first case
		0.f,		// low_delay_window
		1.f,		// low_delay_min
		1.f,		// low_delay_max

		1.f,		// mid_delay_window
		1.f/8.f,	// mid_delay_min
		1.f/4.f,	// mid_delay_max

		1.f,		// else_delay_min
		1.f,		// else_delay_max
	};

	const PerfProfileRender& curr_profile =
		// TODO: magic
		_fps_perf_mode > 1
		? powersavePerfProfile
		: (
			_fps_perf_mode == 1
			? reducedPerfProfile
			: normalPerfProfile
		)
	;

	// min over non animations in all cases
	_render_interval = std::min<float>(pm_interval, cg_interval);
	_render_interval = std::min<float>(_render_interval, tc_unfinished_queue_interval);

	// low delay time window
	if (!_window_hidden && _time_since_event < curr_profile.low_delay_window) {
		_render_interval = std::min<float>(_render_interval, ctc_interval);
		_render_interval = std::min<float>(_render_interval, msgtc_interval);

		_render_interval = std::clamp(
			_render_interval,
			curr_profile.low_delay_min,
			curr_profile.low_delay_max
		);
	// mid delay time window
	} else if (!_window_hidden && _time_since_event < curr_profile.mid_delay_window) {
		_render_interval = std::min<float>(_render_interval, ctc_interval);
		_render_interval = std::min<float>(_render_interval, msgtc_interval);

		_render_interval = std::clamp(
			_render_interval,
			curr_profile.mid_delay_min,
			curr_profile.mid_delay_max
		);
	// timed out or window hidden
	} else {
		// no animation timing here
		_render_interval = std::clamp(
			_render_interval,
			curr_profile.else_delay_min,
			curr_profile.else_delay_max
		);
	}

	_time_since_event += time_delta;

	return nullptr;
}

Screen* MainScreen::tick(float time_delta, bool& quit) {
	quit = !tc.iterate(time_delta); // compute

	tcm.iterate(time_delta); // compute

	const float fo_interval = tffom.tick(time_delta);

	tam.iterate(); // compute

	const float pm_interval = pm.tick(time_delta); // compute

	tdch.tick(time_delta); // compute

	mts.iterate(); // compute

	_min_tick_interval = std::min<float>(
		// HACK: pow by 1.6 to increase 50 -> ~500 (~522)
		// and it does not change 1
		std::pow(tc.toxIterationInterval(), 1.6f)/1000.f,
		pm_interval
	);
	_min_tick_interval = std::min<float>(
		_min_tick_interval,
		fo_interval
	);

	//std::cout << "MS: min tick interval: " << _min_tick_interval << "\n";

	switch (_compute_perf_mode) {
		// normal 1ms lower bound
		case 0: _min_tick_interval = std::max<float>(_min_tick_interval, 0.001f); break;
		// in powersave fix the lowerbound to 100ms
		case 1: _min_tick_interval = std::max<float>(_min_tick_interval, 0.1f); break;
		// extreme 2s
		case 2: _min_tick_interval = std::max<float>(_min_tick_interval, 2.f); break;
		default: std::cerr << "unknown compute perf mode\n"; std::exit(-1);
	}

	return nullptr;
}

