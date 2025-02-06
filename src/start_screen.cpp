#include "./start_screen.hpp"

#include "./main_screen.hpp"

#include "./json_to_config.hpp"

#include <nlohmann/json.hpp>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <cctype>
#include <memory>
#include <filesystem>
#include <fstream>
#include <exception>
#include <stdexcept>

StartScreen::StartScreen(const std::vector<std::string_view>& args, SDL_Renderer* renderer, Theme& theme) : _renderer(renderer), _theme(theme) {
	bool config_loaded {false};
	std::string config_path;
	for (size_t ai = 1; ai < args.size(); ai++) {
		if (args.at(ai) == "--config" || args.at(ai) == "-c") {
			if (config_loaded) {
				std::cerr << "TOMATO error: config specified more than once!\n";
				break;
			}
			if (args.size() == ai+1) {
				std::cerr << "TOMATO error: argument '" << args.at(ai) << "' missing parameter!\n";
				break;
			}
			ai++;

			config_path = args.at(ai);
			auto config_file = std::ifstream(static_cast<std::string>(config_path));
			if (!config_file.is_open()) {
				std::cerr << "TOMATO error: failed to open config file '" << config_path << "'\n";
				break;
			}

			auto config_json = nlohmann::ordered_json::parse(config_file);
			if (!load_json_into_config(config_json, _conf)) {
				std::cerr << "TOMATO error in config json, exiting...\n";
				break;
			}
			config_loaded = true;
		} else if (args.at(ai) == "--plugin" || args.at(ai) == "-p") {
			if (args.size() == ai+1) {
				std::cerr << "TOMATO error: argument '" << args.at(ai) << "' missing parameter!\n";
				break;
			}
			ai++;

			const auto& plugin_path = args.at(ai);
			// TODO: check for dups
			queued_plugin_paths.push_back(static_cast<std::string>(plugin_path));
		} else {
			std::cerr << "TOMATO error: unknown cli arg: '" << args.at(ai) << "'\n";
		}
	}

	// seed tox save path
	if (_conf.has_string("tox", "save_file_path")) {
		const auto config_path_base = std::filesystem::path{config_path}.parent_path();
		std::filesystem::path real_tox_profile_path = static_cast<std::string>(_conf.get_string("tox", "save_file_path").value());
		if (real_tox_profile_path.is_relative()) {
			real_tox_profile_path = config_path_base / real_tox_profile_path;
		}
		_tox_profile_path = real_tox_profile_path.u8string();
	}

	float display_scale = SDL_GetWindowDisplayScale(SDL_GetRenderWindow(renderer));
	if (display_scale < 0.001f) {
		// error?
		display_scale = 1.f;
	}

	{
		auto* font_atlas = ImGui::GetIO().Fonts;
		font_atlas->ClearFonts();
		// for now we also always merge
		//bool has_font {false};

		ImFontGlyphRangesBuilder glyphbld;
		ImVector<ImWchar> glyph_ranges;
		{ // build ranges
			glyphbld.AddRanges(font_atlas->GetGlyphRangesDefault());
			glyphbld.AddRanges(font_atlas->GetGlyphRangesGreek());
			glyphbld.AddRanges(font_atlas->GetGlyphRangesCyrillic());
			glyphbld.AddRanges(font_atlas->GetGlyphRangesChineseSimplifiedCommon()); // contains CJK
			glyphbld.AddText("™"); // somehow missing
			// popular emojies
			glyphbld.AddText(u8"😂❤️🤣👍😭🙏😘🥰😍😊🎉😁💕🥺😅🔥☺️🤦♥️🤷🙄😆🤗😉🎂🤔👏🙂😳🥳😎👌💜😔💪✨💖👀😋😏😢👉💗😩💯🌹💞🎈💙😃😡💐😜🙈🤞😄🤤🙌🤪❣️😀💋💀👇💔😌💓🤩🙃😬😱😴🤭😐🌞😒😇🌸😈🎶✌️🎊🥵😞💚☀️🖤💰😚👑🎁💥🙋☹️😑🥴👈💩✅👋🤮😤🤢🌟❗😥🌈💛😝😫😲🖕‼️🔴🌻🤯💃👊🤬🏃😕👁️⚡☕🍀💦⭐🦋🤨🌺😹🤘🌷💝💤🤝🐰😓💘🍻😟😣🧐😠🤠😻🌙😛🤙🙊");

			if (const auto sv_opt = _conf.get_string("ImGuiFonts", "atlas_extra_text"); sv_opt.has_value) {
				glyphbld.AddText(sv_opt.s.start, sv_opt.s.start+sv_opt.s.extend);
			}
			glyphbld.BuildRanges(&glyph_ranges);
		}

		ImFontConfig fontcfg;
		//fontcfg.SizePixels = 16.f*display_scale;
		fontcfg.SizePixels = _conf.get_int("ImGuiFonts", "size").value_or(13) * display_scale;
		fontcfg.RasterizerDensity = 1.f;
		fontcfg.OversampleH = 2;
		fontcfg.OversampleV = 1;
		fontcfg.MergeMode = false;

		for (const auto [font_path, should_load] : _conf.entries_bool("ImGuiFonts", "fonts")) {
			if (!should_load) {
				continue;
			}

			std::cout << "Font: loading '" << font_path << "'\n";
			const auto* resulting_font = font_atlas->AddFontFromFileTTF(
				font_path.c_str(),
				_conf.get_int("ImGuiFonts", "size", font_path).value_or(0) * display_scale,
				&fontcfg,
				&(glyph_ranges[0])
			);

			if (resulting_font != nullptr) {
				//has_font = true;
				fontcfg.MergeMode = true;
			} else {
				std::cerr << "Font: failed to load '" << "path" << "' !\n";
			}
		}

		// always append the default as a fallback (merge in)
		{
#if 0
			ImFontConfig fontcfg;

			// upsampling to int looks almost ok
			//const float font_size_scale = 1.3f * display_scale;
			const float font_size_scale = 1.0f * display_scale;
			const float font_oversample = 4.f;

			// default font is pixel perfect at 13
			fontcfg.SizePixels = 13.f * font_size_scale;
			fontcfg.RasterizerDensity = font_oversample/font_size_scale;
			// normally density would be set to dpi scale of the display

			fontcfg.MergeMode = has_font;
#endif

			font_atlas->AddFontDefault(&fontcfg);
		}

		font_atlas->Build();
	}

	if (config_loaded) { // pull plugins from config
		const auto config_path_base = std::filesystem::path{config_path}.parent_path();

		for (const auto& [plugin_path, autoload] : _conf.entries_bool("PluginManager", "autoload")) {
			if (!autoload) {
				continue;
			}

			std::filesystem::path real_plugin_path = plugin_path;
			if (real_plugin_path.is_relative()) {
				real_plugin_path = config_path_base / real_plugin_path;
			}

			queued_plugin_paths.push_back(real_plugin_path.u8string());
		}
	}
}

Screen* StartScreen::render(float, bool&) {
	const float TEXT_PROCEED_WIDTH = ImGui::CalcTextSize("proceed").x;
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	ImGui::SetNextWindowSize({656,334}, ImGuiCond_FirstUseEver);
	ImGui::Begin("start screen");

	// TODO: imgui tox profile selector?

	// +----------------------------
	// | |*tox profile*| plugins | toxcore config |
	// | +------+ +--------
	// | | ICON | | fileselector/dropdown?
	// | |      | | password input
	// | +------+ +--------
	// | [proceed]
	// +----------------------------

	if (ImGui::BeginChild("conf", {0, ImGui::GetContentRegionAvail().y - TEXT_BASE_HEIGHT*2.f})) {
		if (ImGui::BeginTabBar("view")) {
			if (ImGui::BeginTabItem("load profile")) {
				_new_save = false;

				ImGui::TextUnformatted("profile :");
				ImGui::SameLine();
				if (ImGui::Button("select")) {
					_fss.requestFile(
						[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
						[this](const auto& path) {
							_tox_profile_path = path.string();
						},
						[](){}
					);
				}
				ImGui::SameLine();
				ImGui::TextUnformatted(_tox_profile_path.c_str());

				ImGui::TextUnformatted("password:");
				ImGui::SameLine();
				if (_show_password) {
					ImGui::InputText("##password", &_password);
				} else {
					ImGui::InputText("##password", &_password, ImGuiInputTextFlags_Password);
				}
				ImGui::SameLine();
				ImGui::Checkbox("show password", &_show_password);

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("create profile")) {
				_new_save = true;

				ImGui::TextUnformatted("username:");
				ImGui::SameLine();
				if (ImGui::InputText("##user_name", &_user_name)) {
					std::string tmp_copy = _user_name;
					for (auto& c : tmp_copy) {
						if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '.') {
							c = '_';
						}
					}

					if (tmp_copy.empty()) {
						tmp_copy = "unnamed-tomato";
					}

					_tox_profile_path = tmp_copy + ".tox";
				}

				ImGui::TextUnformatted("password:");
				ImGui::SameLine();
				if (_show_password) {
					ImGui::InputText("##password", &_password);
				} else {
					ImGui::InputText("##password", &_password, ImGuiInputTextFlags_Password);
				}
				ImGui::SameLine();
				ImGui::Checkbox("show password", &_show_password);

				ImGui::TextUnformatted("TODO: profile path (current path for now)");

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("plugins")) {
				// list of selected plugins (in order)
				for (auto it = queued_plugin_paths.begin(); it != queued_plugin_paths.end();) {
					ImGui::PushID(it->c_str());
					if (ImGui::SmallButton("-")) {
						it = queued_plugin_paths.erase(it);
						ImGui::PopID();
						continue;
					}
					ImGui::SameLine();
					ImGui::TextUnformatted(it->c_str());

					ImGui::PopID();
					it++;
				}

				if (ImGui::Button("+")) {
					_fss.requestFile(
						[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
						[this](const auto& path) {
							queued_plugin_paths.push_back(path.string());
						},
						[](){}
					);
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("toxcore")) {
				ImGui::TextDisabled("Be advised that no settings are written to disk.\nUse a config file if you don't want to set these values every start.");

				ImGui::SeparatorText("DNS");

				{
					static bool value {true};
					if (ImGui::Checkbox("DNS lookups", &value)) {
						_conf.set("tox", "dns", value);
					}
					ImGui::SetItemTooltip("Allow toxcore to use your systems name resolver.");
				}

				ImGui::SeparatorText("Proxy");

				static int proxy_type {0};
				if (ImGui::Combo("proxy type", &proxy_type, "NONE\0HTTP\0SOCKS5\0")) {
					if (proxy_type == 0) {
						_conf.set("tox", "proxy_type", std::string_view{"NONE"});
					} else if (proxy_type == 1) {
						_conf.set("tox", "proxy_type", std::string_view{"HTTP"});
					} else if (proxy_type == 2) {
						_conf.set("tox", "proxy_type", std::string_view{"SOCKS5"});
					}
				}

				ImGui::BeginDisabled(proxy_type == 0);
				{
					{
						static std::string value;
						if (ImGui::InputText("host", &value)) {
							_conf.set("tox", "proxy_host", value);
						}
						ImGui::SetItemTooltip("toxcore does not currently support authentication.");
					}

					{
						static uint16_t value {0};
						if (ImGui::InputScalar("port", ImGuiDataType_U16, &value)) {
							_conf.set("tox", "proxy_port", int64_t(value));
						}
					}
				}
				ImGui::EndDisabled();

				ImGui::SeparatorText("IP connectivity");

				{
					static bool value {true};
					if (ImGui::Checkbox("ipv6", &value)) {
						_conf.set("tox", "ipv6_enabled", value);
					}
				}

				{
					static bool value {true};
					if (ImGui::Checkbox("udp", &value)) {
						_conf.set("tox", "udp_enabled", value);
					}
				}

				{
					static bool value {true};
					if (ImGui::Checkbox("hole punching", &value)) {
						_conf.set("tox", "hole_punching_enabled", value);
					}
					ImGui::SetItemTooltip("Perform NAT hole punching.\nOnly meaningful if udp is enabled.");
				}

				{
					static uint16_t value {0};
					if (ImGui::InputScalar("start port", ImGuiDataType_U16, &value)) {
						_conf.set("tox", "start_port", int64_t(value));
					}
					ImGui::SetItemTooltip("The range in which toxcore finds a free port.\nOnly meaningful if udp is enabled.");
				}
				{
					static uint16_t value {0};
					if (ImGui::InputScalar("end port", ImGuiDataType_U16, &value)) {
						_conf.set("tox", "end_port", int64_t(value));
					}
					ImGui::SetItemTooltip("The range in which toxcore finds a free port.\nOnly meaningful if udp is enabled.");
				}

				ImGui::SeparatorText("local discovery");
				{
					static bool value {true};
					if (ImGui::Checkbox("local discovery", &value)) {
						_conf.set("tox", "local_discovery_enabled", value);
					}
					ImGui::SetItemTooltip("Perform broadcasts in your local networks to find other peers.\nOnly meaningful if udp is enabled.");
				}

				ImGui::SeparatorText("tcp relay server");
				{
					static uint16_t value {0};
					if (ImGui::InputScalar("server port", ImGuiDataType_U16, &value)) {
						_conf.set("tox", "tcp_port", int64_t(value));
					}
					ImGui::SetItemTooltip("Run a tcp relay server in your client, aiding the network with another relay node.\n!! Check local juristiction and law to not get in trouble.\n0 is disabled");
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::EndChild();

	ImGui::Separator();

	if (!_new_save && !std::filesystem::is_regular_file(_tox_profile_path)) {
		// load but file missing

		ImGui::BeginDisabled();
		ImGui::Button("proceed", {TEXT_PROCEED_WIDTH*1.5f, TEXT_BASE_HEIGHT*1.5f});
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("file does not exist");
		}
	} else if (_new_save && std::filesystem::exists(_tox_profile_path)) {
		// new but file exists

		ImGui::BeginDisabled();
		ImGui::Button("proceed", {TEXT_PROCEED_WIDTH*1.5f, TEXT_BASE_HEIGHT*1.5f});
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("file already exists");
		}
	} else {
		if (ImGui::Button("proceed", {TEXT_PROCEED_WIDTH*1.5f, TEXT_BASE_HEIGHT*1.5f})) {
			_error_string.clear();

			try {
				auto new_screen = std::make_unique<MainScreen>(_conf, _renderer, _theme, _tox_profile_path, _password, _user_name, queued_plugin_paths);
				if (!new_screen) {
					throw std::runtime_error("failed to init main screen.");
				}
				ImGui::End(); // start screen
				return new_screen.release();
			} catch (const std::exception& e) {
				_error_string = std::string{"ToxCore/MainScreen creation failed with: "} + e.what();
			} catch (...) {
				_error_string = "ToxCore/MainScreen creation failed with unknown error";
			}
		}

		if (!_error_string.empty()) {
			ImGui::SameLine();
			ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, "%s", _error_string.c_str());
		}
	}

	ImGui::End(); // start screen

	_fss.render();

	// TODO: dont check every frame
	// do in tick instead?
	const bool start_to_tray = _conf.get_bool("tomato", "start_to_tray").value_or(false);
	const bool skip_setup = _conf.get_bool("tomato", "skip_setup_and_load").value_or(false);
	if (start_to_tray || skip_setup) {
		if (start_to_tray) {
			// TODO: the window should really be created hidden in the first place
			SDL_HideWindow(SDL_GetRenderWindow(_renderer));
		}
		auto new_screen = std::make_unique<MainScreen>(std::move(_conf), _renderer, _theme, _tox_profile_path, _password, _user_name, queued_plugin_paths);
		return new_screen.release();
	} else {
		return nullptr;
	}
}

Screen* StartScreen::tick(float, bool&) {
	return nullptr;
}

