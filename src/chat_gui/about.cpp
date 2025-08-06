#include "./about.hpp"

#include "./version.hpp"

#include <imgui.h>

#include <string>

void ImGuiTomatoAbout(void) {
	if (ImGui::BeginTabBar("about")) {
		if (ImGui::BeginTabItem("tomato")) {
			std::string tomato_version_string {"tomato " TOMATO_VERSION_STR};
			if (TOMATO_GIT_DEPTH != 0) {
				tomato_version_string += "-" + std::to_string(TOMATO_GIT_DEPTH);
			}
			if (std::string_view{TOMATO_GIT_COMMIT} != "UNK") {
				tomato_version_string += "+git.";
				tomato_version_string += TOMATO_GIT_COMMIT;
			}

			ImGui::TextUnformatted(tomato_version_string.c_str());

			ImGui::SameLine();
			if (ImGui::SmallButton("click to copy")) {
				ImGui::SetClipboardText(tomato_version_string.c_str());
			}

			ImGui::Separator();

			ImGui::TextUnformatted("TODO: tomato license");

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("imgui")) {
			ImGui::ShowAboutWindow();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("others")) {
			ImGui::TextUnformatted("TODO: list all the other libs and their licenses");
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("funding")) {
			ImGui::TextWrapped("Your Help is needed, to keep the project alive, expand it's features and to inspire new features!");
			ImGui::TextLinkOpenURL("https://github.com/sponsors/Green-Sky");
			ImGui::TextUnformatted("Contact Me for more ways to help the project. :)");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}
