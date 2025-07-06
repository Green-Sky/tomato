#include "./main_screen.hpp"

#include <solanaceae/message3/nj/message_components_serializer.hpp>
#include <solanaceae/tox_messages/nj/tox_message_components_serializer.hpp>

#include <solanaceae/contact/components.hpp>

#include "./frame_streams/sdl/sdl_audio2_frame_stream2.hpp"
#include "./frame_streams/sdl/sdl_video_frame_stream2.hpp"

#include <imgui.h>
#include <implot.h>

#include <SDL3/SDL.h>

#include <memory>
#include <cmath>
#include <string_view>

static std::unique_ptr<SystemTray> constructSystemTray(SimpleConfigModel& conf, SDL_Window* window) {
	bool conf_system_tray {false};
	if (auto value_stt = conf.get_bool("tomato", "start_to_tray"); value_stt.has_value) {
		conf_system_tray = value_stt.value();
		// TODO: warn or error if "system_try" is false
	} else if (auto value_st = conf.get_bool("tomato", "system_tray"); value_st.has_value) {
		conf_system_tray = value_st.value();
	}

	if (conf_system_tray) {
		return std::make_unique<SystemTray>(window);
	} else {
		return nullptr;
	}
}

MainScreen::MainScreen(const SimpleConfigModel& conf_, SDL_Renderer* renderer_, Theme& theme_, std::string save_path, std::string save_password, std::string new_username, std::vector<std::string> plugins) :
	renderer(renderer_),
	conf(conf_),
	rmm(cs),
	msnj{cs, os, {}, {}},
	mts(rmm),
	sdlvis(os),
	sm(os),
	tc(conf, save_path, save_password),
	tel(tc, std::cout),
	tpi(tc.getTox()),
	ad(tc),
	tcm(cs, tc, tc),
	tmm(rmm, cs, tcm, tc, tc),
	ttm(rmm, cs, tcm, tc, tc, os),
	tffom(cs, rmm, tcm, tc, tc),
#if TOMATO_TOX_AV
	tav(tc.getTox()),
	tavvoip(os, tav, cs, tcm),
#endif
	theme(theme_),
	mmil(rmm),
	tam(os, cs, conf, tc),
	tas(os, cs, rmm),
	sdlrtu(renderer_),
	tal(cs, os),
	contact_tc(tal, sdlrtu),
	mil(),
	msg_tc(mil, sdlrtu),
	st(constructSystemTray(conf, SDL_GetRenderWindow(renderer_))),
	si(rmm, cs, SDL_GetRenderWindow(renderer_), st.get()),
	cg(conf, os, rmm, cs, sdlrtu, contact_tc, msg_tc, theme),
	sw(conf),
	osui(os),
	tuiu(tc, conf, &tpi),
	tdch(tpi),
	tnui(tpi),
	smui(os, sm),
	dvt(os, sm, sdlrtu)
{
	tel.subscribeAll();

	registerMessageComponents(msnj);
	registerToxMessageComponents(msnj);

	conf.set("tox", "save_file_path", save_path);

	{ // name stuff
		// a new profile will not have this set
		auto name = tc.toxSelfGetName();
		if (name.empty()) {
			name = new_username;
		}
		conf.set("tox", "name", name);
		tc.setSelfName(name); // TODO: this is ugly
	}

	// TODO: remove
	std::cout << "own address: " << tc.toxSelfGetAddressStr() << "\n";

	{ // setup plugin instances
		g_provideInstance<ObjectStore2>("ObjectStore2", "host", &os);

		g_provideInstance<ConfigModelI>("ConfigModelI", "host", &conf);
		g_provideInstance<ContactStore4I>("ContactStore4I", "host", &cs);
		g_provideInstance<RegistryMessageModelI>("RegistryMessageModelI", "host", &rmm);
		g_provideInstance<MessageSerializerNJ>("MessageSerializerNJ", "host", &msnj);

		g_provideInstance<ToxI>("ToxI", "host", &tc);
		g_provideInstance<ToxPrivateI>("ToxPrivateI", "host", &tpi);
		g_provideInstance<ToxEventProviderI>("ToxEventProviderI", "host", &tc);
#if TOMATO_TOX_AV
		g_provideInstance<ToxAVI>("ToxAVI", "host", &tav);
#endif
		g_provideInstance<ToxContactModel2>("ToxContactModel2", "host", &tcm);

		// TODO: pm?

		// graphics
		g_provideInstance<ImGuiContext>("ImGuiContext", ImGui::GetVersion(), "host", ImGui::GetCurrentContext());
		{
			ImGuiMemAllocFunc alloc_func = nullptr;
			ImGuiMemFreeFunc free_func = nullptr;
			void* user_data = nullptr;
			ImGui::GetAllocatorFunctions(&alloc_func, &free_func, &user_data);

			// function pointers are funky
			g_provideInstance("ImGuiMemAllocFunc", ImGui::GetVersion(), "host", reinterpret_cast<void*>(alloc_func));
			g_provideInstance("ImGuiMemFreeFunc", ImGui::GetVersion(), "host", reinterpret_cast<void*>(free_func));
			if (user_data != nullptr) { // dont register nullptrs (can be valid, we rely on the other 2 pointers to indicate)
				g_provideInstance("ImGuiMemUserData", ImGui::GetVersion(), "host", user_data);
			}
		}
		g_provideInstance<ImPlotContext>("ImPlotContext", IMPLOT_VERSION, "host", ImPlot::GetCurrentContext());

		g_provideInstance<TextureUploaderI>("TextureUploaderI", "host", &sdlrtu);
	}

	for (const auto& ppath : plugins) {
		if (!pm.add(ppath)) {
			std::cerr << "MS error: loading plugin '" << ppath << "' failed!\n";

			// thow?
			//assert(false && "failed to load plugin");
			SDL_MessageBoxData mb;
			mb.flags = SDL_MESSAGEBOX_ERROR;
			mb.window = nullptr;
			mb.title = "Tomato failed to load a plugin!";
			std::string message {"Failed to load plugin '"};
			message += ppath;
			message += "' !";
			mb.message = message.c_str();

			mb.numbuttons = 2;
			SDL_MessageBoxButtonData mb_buttons[2] {};
			mb_buttons[0].buttonID = 1;
			mb_buttons[0].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
			mb_buttons[0].text = "Abort";
			mb_buttons[1].buttonID = 2;
			mb_buttons[1].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
			mb_buttons[1].text = "Continue Anyway";
			mb.buttons = mb_buttons;

			mb.colorScheme = nullptr;

			int button_id = 0;
			SDL_ShowMessageBox(&mb, &button_id);
			if (button_id == 1) {
				exit(1);
			}
		}
	}

	conf.dump();

	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		// add system audio devices
		{ // audio in
			ObjectHandle asrc {os.registry(), os.registry().create()};
			try {
				asrc.emplace<Components::FrameStream2Source<AudioFrame2>>(
					std::make_unique<SDLAudio2InputDevice>()
				);

				asrc.emplace<Components::StreamSource>(Components::StreamSource::create<AudioFrame2>("SDL Audio Default Recording Device"));
				asrc.emplace<Components::TagDefaultTarget>();

				os.throwEventConstruct(asrc);
			} catch (...) {
				os.registry().destroy(asrc);
			}
		}
		{ // audio out
			ObjectHandle asink {os.registry(), os.registry().create()};
			try {
				asink.emplace<Components::FrameStream2Sink<AudioFrame2>>(
					std::make_unique<SDLAudio2OutputDeviceDefaultSink>()
				);

				asink.emplace<Components::StreamSink>(Components::StreamSink::create<AudioFrame2>("SDL Audio Default Playback Device"));
				asink.emplace<Components::TagDefaultTarget>();

				os.throwEventConstruct(asink);
			} catch (...) {
				os.registry().destroy(asink);
			}
		}
	} else {
		std::cerr << "MS warning: no sdl audio: " << SDL_GetError() << "\n";
	}
}

MainScreen::~MainScreen(void) {
	pm.stopAll();
	// TODO: quit sdl audio
}

bool MainScreen::handleEvent(SDL_Event& e) {
	if (e.type == SDL_EVENT_DROP_FILE) {
		std::cout << "DROP FILE: " << e.drop.data << "\n";
		_dopped_files.emplace_back(e.drop.data);
		_render_interval = 1.f/60.f; // TODO: magic
		_time_since_event = 0.f;
		return true;
	}

	if (
		e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
		e.type == SDL_EVENT_WINDOW_MINIMIZED ||
		e.type == SDL_EVENT_WINDOW_HIDDEN ||
		e.type == SDL_EVENT_WINDOW_OCCLUDED // does this trigger on partial occlusion?
	) {
		auto* window = SDL_GetWindowFromID(e.window.windowID);
		auto* event_renderer = SDL_GetRenderer(window);
		if (event_renderer != nullptr && event_renderer == renderer) {
			if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
				// this event only fires if not last window (systrays count as windows)
				SDL_HideWindow(window);
			}

			// our window is now obstructed
			if (_window_hidden_ts < e.window.timestamp) {
				_window_hidden_ts = e.window.timestamp;
				_window_hidden = true;
				//std::cout << "TOMAT: window hidden " << e.type << " " << e.window.timestamp << "\n";
			}
		}
		if (st) {
			st->update();
		}
		return true; // forward?
	}

	if (
		e.type == SDL_EVENT_WINDOW_SHOWN ||
		e.type == SDL_EVENT_WINDOW_RESTORED || // required for android
		e.type == SDL_EVENT_WINDOW_EXPOSED
	) {
		auto* window = SDL_GetWindowFromID(e.window.windowID);
		auto* event_renderer = SDL_GetRenderer(window);
		if (event_renderer != nullptr && event_renderer == renderer) {
			if (_window_hidden_ts <= e.window.timestamp) {
				_window_hidden_ts = e.window.timestamp;

#if 0
				if (_window_hidden) {
					// if window was previously hidden, we shorten the wait for the next frame
					_render_interval = 1.f/60.f;
				}
#endif

				_window_hidden = false;
				//std::cout << "TOMAT: window shown " << e.type << " " << e.window.timestamp << "\n";
			}
		}
		_render_interval = 1.f/60.f; // TODO: magic
		_time_since_event = 0.f;
		if (st) {
			st->update(); // TODO: limit this
		}
		return true; // forward?
	}

	if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
		_window_focused = true;
	}

	if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
		_window_focused = false;
	}

	if (
		_fps_perf_mode <= 1 && (
			// those are all the events imgui polls
			e.type == SDL_EVENT_MOUSE_MOTION ||
			e.type == SDL_EVENT_MOUSE_WHEEL ||
			e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
			e.type == SDL_EVENT_MOUSE_BUTTON_UP ||
			e.type == SDL_EVENT_FINGER_DOWN ||
			e.type == SDL_EVENT_FINGER_UP ||
			e.type == SDL_EVENT_TEXT_INPUT ||
			e.type == SDL_EVENT_TEXT_EDITING ||
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

	if (sdlvis.handleEvent(e)) {
		return true;
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
	// TODO: move this code to events
	if (!cs.registry().storage<Contact::Components::TagAvatarInvalidate>().empty()) { // handle force-reloads for avatars
		std::vector<Contact4> to_purge;
		cs.registry().view<Contact::Components::TagAvatarInvalidate>().each([&to_purge](const Contact4 c) {
			to_purge.push_back(c);
		});
		cs.registry().remove<Contact::Components::TagAvatarInvalidate>(to_purge.cbegin(), to_purge.cend());
		contact_tc.invalidate(to_purge);
	}
	// ACTUALLY NOT IF RENDERED, MOVED LOGIC TO ABOVE
	// it might unload textures, so it needs to be done before rendering
	const float ctc_interval = contact_tc.update();
	const float msgtc_interval = msg_tc.update();

	si.render(time_delta);

	if (_window_hidden && _window_focused) {
		_window_focused = false;
	}

	const float cg_interval = cg.render(time_delta, _window_hidden, _window_focused); // render
	sw.render(); // render
	osui.render();
	tuiu.render(); // render
	tdch.render(); // render
	const float tnui_interval = tnui.render(time_delta);
	smui.render();
	const float dvt_interval = dvt.render();

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

					ImGui::Separator();

					ImGui::Text("render interval: %.0fms (%.2ffps)", _render_interval*1000.f, 1.f/_render_interval);
					ImGui::Text("tick   interval: %.0fms (%.2ftps)", _min_tick_interval*1000.f, 1.f/_min_tick_interval);

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Settings")) {
					ImGui::SeparatorText("ImGui");

					if (ImGui::MenuItem("Style Editor", nullptr, _show_tool_style_editor)) {
						_show_tool_style_editor = !_show_tool_style_editor;
					}

					if (ImGui::MenuItem("Metrics", nullptr, _show_tool_metrics)) {
						_show_tool_metrics = !_show_tool_metrics;
					}

					if (ImGui::MenuItem("Debug Log", nullptr, _show_tool_debug_log)) {
						_show_tool_debug_log = !_show_tool_debug_log;
					}

					if (ImGui::MenuItem("ID Stack Tool", nullptr, _show_tool_id_stack)) {
						_show_tool_id_stack = !_show_tool_id_stack;
					}

					if (ImGui::MenuItem("Demo", nullptr, _show_tool_demo)) {
						_show_tool_demo = !_show_tool_demo;
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

	if (_show_tool_demo) {
		ImGui::ShowDemoWindow(&_show_tool_demo);
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
	_render_interval = std::min<float>(_render_interval, dvt_interval);

	// low delay time window
	if (!_window_hidden && _time_since_event < curr_profile.low_delay_window) {
		_render_interval = std::min<float>(_render_interval, ctc_interval);
		_render_interval = std::min<float>(_render_interval, msgtc_interval);
		_render_interval = std::min<float>(_render_interval, tnui_interval);

		_render_interval = std::clamp(
			_render_interval,
			curr_profile.low_delay_min,
			curr_profile.low_delay_max
		);
	// mid delay time window
	} else if (!_window_hidden && _time_since_event < curr_profile.mid_delay_window) {
		_render_interval = std::min<float>(_render_interval, ctc_interval);
		_render_interval = std::min<float>(_render_interval, msgtc_interval);
		_render_interval = std::min<float>(_render_interval, tnui_interval);

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
	const float sm_interval = sm.tick(time_delta);

	quit = !tc.iterate(time_delta); // compute

#if TOMATO_TOX_AV
	tav.toxavIterate();
	// breaks it
	// HACK: pow by 1.18 to increase 200 -> ~500
	//const float av_interval = std::pow(tav.toxavIterationInterval(), 1.18)/1000.f;
	const float av_interval = tav.toxavIterationInterval()/1000.f;

	const float av_voip_interval = tavvoip.tick();
#endif

	tcm.iterate(time_delta); // compute

	const float fo_interval = tffom.tick(time_delta);

	tam.iterate(); // compute
	tas.iterate(time_delta);

	const float pm_interval = pm.tick(time_delta); // compute

	tdch.tick(time_delta); // compute
	tnui.tick(time_delta); // compute

	mts.iterate(); // compute (after mfs)

	if (!_dopped_files.empty()) {
		std::vector<std::string_view> tmp_view;
		for (const std::string& it : _dopped_files) {
			tmp_view.push_back(std::string_view{it});
		}

		cg.sendFileList(tmp_view);

		_dopped_files.clear();
	}

	_min_tick_interval = std::min<float>(
		// HACK: pow by 1.6 to increase 50 -> ~500 (~523)
		// and it keeps 1
		std::pow(tc.toxIterationInterval(), 1.6f)/1000.f,
		pm_interval
	);
	_min_tick_interval = std::min<float>(
		_min_tick_interval,
		sm_interval
	);
	_min_tick_interval = std::min<float>(
		_min_tick_interval,
		fo_interval
	);

#if TOMATO_TOX_AV
	_min_tick_interval = std::min<float>(
		_min_tick_interval,
		av_interval
	);
	_min_tick_interval = std::min<float>(
		_min_tick_interval,
		av_voip_interval
	);
#endif

	//std::cout << "MS: min tick interval: " << _min_tick_interval << "\n";

	switch (_compute_perf_mode) {
		// normal 1ms lower bound
		case 0: _min_tick_interval = std::max<float>(_min_tick_interval, 0.001f); break;
		// in powersave fix the lowerbound to 100ms
		case 1: _min_tick_interval = std::max<float>(_min_tick_interval, 0.1f); break;
		// extreme 2s
		case 2: _min_tick_interval = std::max<float>(_min_tick_interval, 1.5f); break;
		default: std::cerr << "unknown compute perf mode\n"; std::exit(-1);
	}

	return nullptr;
}

