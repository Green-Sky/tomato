#include "./tox_ui_utils.hpp"

#include "./tox_client.hpp"

#include <solanaceae/toxcore/tox_private_interface.hpp>
#include <solanaceae/tox_contacts/tox_contact_model2.hpp>

#include <tox/tox.h>

#include <solanaceae/util/utils.hpp>

#include <solanaceae/util/config_model.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <cstring>

ToxUIUtils::ToxUIUtils(
	ToxClient& tc,
	ToxContactModel2& tcm,
	ConfigModelI& conf,
	ToxPrivateI* tp
) : _tc(tc), _tcm(tcm), _conf(conf), _tp(tp) {
}

void ToxUIUtils::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("Tox")) {
					ImGui::SeparatorText("Friends");

					if (ImGui::MenuItem("add Friend by ID", nullptr, _show_add_friend_window)) {
						_show_add_friend_window = !_show_add_friend_window;
					}
					if (ImGui::MenuItem("copy own ToxID")) {
						ImGui::SetClipboardText(_tc.toxSelfGetAddressStr().c_str());
					}

					ImGui::SeparatorText("Groups");
					if (ImGui::MenuItem("join Group by ID (ngc)", nullptr, _show_add_group_window)) {
						_show_add_group_window = !_show_add_group_window;
					}

					if (ImGui::BeginMenu("common public groups")) {
						struct {
							std::string name;
							std::string_view id;
						} groups[] {
							{"TokTok-dev (toxcore dev)", "360497da684bce2a500c1af9b3a5ce949bbb9f6fb1f91589806fb04ca039e313"},
							{"Lobby (general offtopic)", "d325f0095cb4d10f5ed668b854e2e10c131f7256949625e5e2dddadd8143dffa"},
							// TODO: offical solanaceae/tomato group
						};

						for (const auto& [name, id] : groups) {
							if (ImGui::MenuItem(name.c_str(), nullptr, false, true)) {
								std::memcpy(_chat_id, id.data(), id.size());
								_show_add_group_window = true;
								ImGui::SetWindowFocus("Tox join Group");
							}
						}

						ImGui::EndMenu();
					}

					if (ImGui::MenuItem("new Group (ngc)", nullptr, _show_new_group_window)) {
						_show_new_group_window = !_show_new_group_window;
					}

					ImGui::SeparatorText("DHT");

					if (ImGui::MenuItem("rerun bootstrap")) {
						_tc.runBootstrap();
					}

					if (ImGui::MenuItem("connect node", nullptr, _show_dht_connect_node)) {
						_show_dht_connect_node = !_show_dht_connect_node;
					}

					{ // self node info
						bool either = false;
						{
							auto [port_opt, port_err] = _tc.toxSelfGetUDPPort();
							if (port_opt.has_value()) {
								either = true;
								ImGui::TextDisabled("udp online; port: %d", port_opt.value());
							} else {
								ImGui::TextDisabled("udp disabled");
							}
						}
						{
							auto [port_opt, port_err] = _tc.toxSelfGetTCPPort();
							if (port_opt.has_value()) {
								either = true;
								ImGui::TextDisabled("tcp relay server online; port: %d", port_opt.value());
							} else {
								ImGui::TextDisabled("tcp relay server disabled");
							}
						}

						if (either && ImGui::MenuItem("copy own DHT id/pubkey")) {
							ImGui::SetClipboardText(bin2hex(_tc.toxSelfGetDHTID()).c_str());
						}
					}

					ImGui::EndMenu();
				}

				switch (_tc.toxSelfGetConnectionStatus()) {
					case TOX_CONNECTION_NONE:
						ImGui::TextColored({1.0,0.5,0.5,0.8}, "Disconnected");
						break;
					case TOX_CONNECTION_TCP:
						ImGui::TextColored({0.0,1.0,0.8,0.8}, "Connected-TCP");
						break;
					case TOX_CONNECTION_UDP:
						ImGui::TextColored({0.3,1.0,0.0,0.8}, "Connected-UDP");
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
				auto [_, err_r] = _tcm.createContactFriend(
					std::string_view{tox_id, std::size(tox_id)-1},
					message
				);
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

			ImGui::InputText("chat ID", _chat_id, TOX_GROUP_CHAT_ID_SIZE*2+1);

			static std::string self_name = _conf.get_string("tox", "name").value_or("default_tomato");
			ImGui::InputText("name to join with", &self_name);

			static std::string password;
			ImGui::InputText("password to join with", &password);

			static Tox_Err_Group_Join err = Tox_Err_Group_Join::TOX_ERR_GROUP_JOIN_OK;
			if (ImGui::Button("join/reconnect")) {
				auto [_, err_r] = _tcm.createContactGroupJoin(
					std::string_view{_chat_id, std::size(_chat_id)-1},
					self_name,
					password
				);
				err = err_r;

				{ // reset everything
					std::memset(_chat_id, '\0', std::size(_chat_id));

					self_name = _conf.get_string("tox", "name").value_or("default_tomato");

					for (size_t i = 0; i < password.size(); i++) {
						password.at(i) = '\0';
					}
					password.clear();
				}
			}
			if (err != Tox_Err_Group_Join::TOX_ERR_GROUP_JOIN_OK) {
				ImGui::SameLine();
				ImGui::Text("error joining group '%s' (%d)", tox_err_group_join_to_string(err), err);
			}
		}
		ImGui::End();
	}

	if (_show_new_group_window) {
		if (ImGui::Begin("Tox new Group", &_show_new_group_window)) {
			ImGui::TextDisabled("NGC refers to the New DHT enabled Group Chats.");
			ImGui::TextDisabled("Connecting via ID might take a very long time.");

			static int privacy_state{Tox_Group_Privacy_State::TOX_GROUP_PRIVACY_STATE_PUBLIC};
			ImGui::Combo("privacy state", &privacy_state, "public\0private\0");
			ImGui::SetItemTooltip("Public groups get announced to the DHT.\nAnyone can scrape the DHT and find your group.\n\nPrivate groups can not be joined via ID (DHT lookup).");

			static std::string name;
			ImGui::InputText("name of the group", &name);

			static std::string self_name = _conf.get_string("tox", "name").value_or("default_tomato");
			ImGui::InputText("name of yourself", &self_name);

			static std::string password;
			ImGui::InputText("password for group", &password);

			static Tox_Err_Group_New err = Tox_Err_Group_New::TOX_ERR_GROUP_NEW_OK;
			static Tox_Err_Group_Set_Password err_pw = Tox_Err_Group_Set_Password::TOX_ERR_GROUP_SET_PASSWORD_OK;
			if (ImGui::Button("create")) {
				auto [new_c, err_r, err_pw_r] = _tcm.createContactGroupNew(
					static_cast<Tox_Group_Privacy_State>(privacy_state),
					name,
					self_name,
					password
				);
				err = err_r;
				err_pw = err_pw_r;

				{ // reset everything
					for (size_t i = 0; i < name.size(); i++) {
						name.at(i) = '\0';
					}
					name.clear();

					self_name = _conf.get_string("tox", "name").value_or("default_tomato");

					for (size_t i = 0; i < password.size(); i++) {
						password.at(i) = '\0';
					}
					password.clear();
				}
			}
			if (err != Tox_Err_Group_New::TOX_ERR_GROUP_NEW_OK || err_pw != Tox_Err_Group_Set_Password::TOX_ERR_GROUP_SET_PASSWORD_OK) {
				ImGui::SameLine();
				ImGui::Text("error creating group!");
				ImGui::Text("group: '%s' (%d)", tox_err_group_new_to_string(err), err);
				ImGui::Text("pw:    '%s' (%d)", tox_err_group_set_password_to_string(err_pw), err_pw);
			}
		}
		ImGui::End();
	}

	if (_show_dht_connect_node) {
		if (ImGui::Begin("Tox connect DHT node", &_show_dht_connect_node)) {
			ImGui::BeginDisabled();
			ImGui::TextWrapped(
				"Here you can manually connect to a DHT node (or/and tcp-relay) by address and id/pubkey.\n"
				"This is equivalent to what 'DHT Bootstrapping' does, but not with hardcoded nodes.\n"
				"Keep in mind that your own DHT id/pubkey changes everytime you start the program, unlike dedicated bootstrap nodes.\n"
				"If DNS querries where not disabled at launch, domain names can be used too."
				"A public list can be found at: nodes.tox.chat\n"
			);
			ImGui::EndDisabled();

			static std::string addr;
			static uint16_t port {33445};
			static char pubkey[TOX_PUBLIC_KEY_SIZE*2 + 1]; // 1 for null terminator

			if (ImGui::BeginTable("node", 2, ImGuiTableFlags_SizingFixedFit)) {
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextColumn();
				ImGui::TextUnformatted("address");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##address", &addr);

				ImGui::TableNextColumn();
				ImGui::TextUnformatted("port");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				ImGui::InputScalar("##port", ImGuiDataType_U16, &port);

				ImGui::TableNextColumn();
				ImGui::TextUnformatted("pubkey");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##pubkey", pubkey, TOX_PUBLIC_KEY_SIZE*2+1);

				// add as
				// - udp dht node (default)
				// - udp dht node + tcp relay
				// - tcp relay

				ImGui::EndTable();
			}

			static std::string last_error;

			bool valid_input = !addr.empty() && port != 0;
			if (!valid_input) ImGui::BeginDisabled();
			if (ImGui::Button("connect")) {
				std::vector<uint8_t> bin_pubkey = hex2bin(std::string_view{pubkey, TOX_PUBLIC_KEY_SIZE*2});

				last_error.clear();

				{
					Tox_Err_Bootstrap err = _tc.toxBootstrap(addr, port, bin_pubkey);
					if (err != Tox_Err_Bootstrap::TOX_ERR_BOOTSTRAP_OK) {
						last_error += "add udp node failed with " + std::to_string(err) + "\n";
					}
				}
				{
					Tox_Err_Bootstrap err = _tc.toxAddTcpRelay(addr, port, bin_pubkey);
					if (err != Tox_Err_Bootstrap::TOX_ERR_BOOTSTRAP_OK) {
						last_error += "add tcp relay failed with " + std::to_string(err) + "\n";
					}
				}
			}
			if (!valid_input) ImGui::EndDisabled();
			if (!last_error.empty()) {
				ImGui::TextUnformatted(last_error.c_str());
			}
		}
		ImGui::End();
	}
}

