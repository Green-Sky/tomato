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

StartScreen::StartScreen(const std::vector<std::string_view>& args, SDL_Renderer* renderer, Theme& theme) : _renderer(renderer), _theme(theme) {
	for (size_t ai = 1; ai < args.size(); ai++) {
		if (args.at(ai) == "--config" || args.at(ai) == "-c") {
			if (args.size() == ai+1) {
				std::cerr << "TOMATO error: argument '" << args.at(ai) << "' missing parameter!\n";
				break;
			}
			ai++;

			const auto& config_path = args.at(ai);
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

	{ // seed tox save path
		if (_conf.has_string("tox", "save_file_path")) {
			_tox_profile_path = _conf.get_string("tox", "save_file_path").value();
		}
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
}

Screen* StartScreen::render(float, bool&) {
	const float TEXT_LOAD_WIDTH = ImGui::CalcTextSize("load").x;
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();


	// TODO: imgui tox profile selector?

	// +----------------------------
	// | |*tox profile*| plugins |
	// | +------+ +--------
	// | | ICON | | fileselector/dropdown?
	// | |      | | password input
	// | +------+ +--------
	// +----------------------------

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
		ImGui::EndTabBar();
	}

	ImGui::Separator();

	if (!_new_save && !std::filesystem::is_regular_file(_tox_profile_path)) {
		// load but file missing

		ImGui::BeginDisabled();
		ImGui::Button("load", {TEXT_LOAD_WIDTH*1.5f, TEXT_BASE_HEIGHT*1.5f});
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("file does not exist");
		}
	} else if (_new_save && std::filesystem::exists(_tox_profile_path)) {
		// new but file exists

		ImGui::BeginDisabled();
		ImGui::Button("load", {TEXT_LOAD_WIDTH*1.5f, TEXT_BASE_HEIGHT*1.5f});
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("file already exists");
		}
	} else {
		if (ImGui::Button("load", {TEXT_LOAD_WIDTH*1.5f, TEXT_BASE_HEIGHT*1.5f})) {

			auto new_screen = std::make_unique<MainScreen>(std::move(_conf), _renderer, _theme, _tox_profile_path, _password, _user_name, queued_plugin_paths);
			return new_screen.release();
		}
	}

	_fss.render();

	return nullptr;
}

Screen* StartScreen::tick(float, bool&) {
	return nullptr;
}

