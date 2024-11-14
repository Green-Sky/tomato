#include "./tox_ui_utils.hpp"

#include "./tox_client.hpp"

#include <solanaceae/toxcore/tox_private_interface.hpp>

#include <tox/tox.h>

#include <solanaceae/util/utils.hpp>

#include <solanaceae/util/config_model.hpp>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

ToxUIUtils::ToxUIUtils(
	ToxClient& tc,
	ConfigModelI& conf,
	ToxPrivateI* tp
) : _tc(tc), _conf(conf), _tp(tp) {
}

void ToxUIUtils::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("Tox")) {
					ImGui::SeparatorText("Friends/Groups");

					if (ImGui::MenuItem("add Friend by ID", nullptr, _show_add_friend_window)) {
						_show_add_friend_window = !_show_add_friend_window;
					}
					if (ImGui::MenuItem("copy own ToxID")) {
						ImGui::SetClipboardText(_tc.toxSelfGetAddressStr().c_str());
					}
					if (ImGui::MenuItem("join Group by ID (ngc)", nullptr, _show_add_group_window)) {
						_show_add_group_window = !_show_add_group_window;
					}

					ImGui::SeparatorText("DHT");

					if (ImGui::MenuItem("rerun bootstrap")) {
						_tc.runBootstrap();
					}

					ImGui::EndMenu();
				}

				switch (_tc.toxSelfGetConnectionStatus()) {
					case TOX_CONNECTION_NONE:
						ImGui::TextColored({1.0,0.5,0.5,0.8}, "Offline");
						break;
					case TOX_CONNECTION_TCP:
						ImGui::TextColored({0.0,1.0,0.8,0.8}, "Online-TCP");
						break;
					case TOX_CONNECTION_UDP:
						ImGui::TextColored({0.3,1.0,0.0,0.8}, "Online-UDP");
						break;
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Tox Onion connection status");
				}

				if (_tp != nullptr) {
					ImGui::Text("(%d)", _tp->toxDHTGetNumCloselist());
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Number of connected DHT nodes");
					}
				}

				ImGui::EndMenuBar();
			}

		}
		ImGui::End();
	}

	if (_show_add_friend_window) {
		if (ImGui::Begin("Tox add Friend", &_show_add_friend_window)) {
			static char tox_id[TOX_ADDRESS_SIZE*2+1] = {};
			ImGui::InputText("Tox ID", tox_id, TOX_ADDRESS_SIZE*2+1);

			static std::string message = "Add me, I'm tomat";
			ImGui::InputText("message", &message);

			static Tox_Err_Friend_Add err = Tox_Err_Friend_Add::TOX_ERR_FRIEND_ADD_OK;
			if (ImGui::Button("add")) {
				// TODO: add string_view variant to utils
				auto [_, err_r] = _tc.toxFriendAdd(hex2bin(std::string{tox_id}), message);
				err = err_r;

				{ // reset everything
					for (size_t i = 0; i < sizeof(tox_id); i++) {
						tox_id[i] = '\0';
					}
				}
			}
			if (err != Tox_Err_Friend_Add::TOX_ERR_FRIEND_ADD_OK) {
				ImGui::SameLine();
				ImGui::Text("error adding friend (code: %d)", err);
			}
		}
		ImGui::End();
	}

	if (_show_add_group_window) {
		if (ImGui::Begin("Tox join Group", &_show_add_group_window)) {
			ImGui::TextDisabled("NGC refers to the New DHT enabled Group Chats.");
			ImGui::TextDisabled("Connecting via ID might take a very long time.");

			static char chat_id[TOX_GROUP_CHAT_ID_SIZE*2+1] = {};
			ImGui::InputText("chat ID", chat_id, TOX_GROUP_CHAT_ID_SIZE*2+1);

			static std::string self_name = _conf.get_string("tox", "name").value_or("default_tomato");
			ImGui::InputText("name to join with", &self_name);

			static std::string password;
			ImGui::InputText("password to join with", &password);

			static Tox_Err_Group_Join err = Tox_Err_Group_Join::TOX_ERR_GROUP_JOIN_OK;
			if (ImGui::Button("join")) {
				auto [_, err_r] = _tc.toxGroupJoin(
					hex2bin(std::string{chat_id}), // TODO: add string_view variant to utils
					self_name,
					password
				);
				err = err_r;

				{ // reset everything
					for (size_t i = 0; i < sizeof(chat_id); i++) {
						chat_id[i] = '\0';
					}

					self_name = _conf.get_string("tox", "name").value_or("default_tomato");

					for (size_t i = 0; i < password.size(); i++) {
						password.at(i) = '\0';
					}
					password.clear();
				}
			}
			if (err != Tox_Err_Group_Join::TOX_ERR_GROUP_JOIN_OK) {
				ImGui::SameLine();
				ImGui::Text("error joining group (code: %d)", err);
			}
		}
		ImGui::End();
	}
}

