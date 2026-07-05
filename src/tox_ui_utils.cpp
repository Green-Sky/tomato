#include "./tox_ui_utils.hpp"

#include "./tox_client.hpp"

#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/toxcore/tox_private_interface.hpp>
#include <solanaceae/tox_contacts/tox_contact_model2.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/tox_contacts/components.hpp>

#include <tox/tox.h>

#include <solanaceae/util/utils.hpp>

#include <solanaceae/util/config_model.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <cstring>
#include <stdexcept>

static void renderContextGroup(ContactHandle4 c) {
	if (!c.all_of<Contact::Components::ToxGroupEphemeral, Contact::Components::Parent>()) {
		return;
	}

	if (!ImGui::BeginMenu("Tox")) {
		return;
	}

	const auto parent = ContactHandle4{*c.registry(), c.get<Contact::Components::Parent>().parent};
	if (!static_cast<bool>(parent) || !parent.all_of<Contact::Components::TagRoot, ToxI*>()) {
		ImGui::EndMenu();
		return;
	}

	ToxI* t = parent.get<ToxI*>();

	if (t == nullptr) {
		ImGui::EndMenu();
		return;
	}

	//ImGui::SeparatorText("Group");

	if (ImGui::MenuItem("Reconnect")) {
		t->toxGroupReconnect(c.get<Contact::Components::ToxGroupEphemeral>().group_number);
	}

	{
		bool connected = false;
		if (const auto cs_comp_ptr = c.try_get<Contact::Components::ConnectionState>(); cs_comp_ptr != nullptr) {
			connected = cs_comp_ptr->state != Contact::Components::ConnectionState::disconnected;
		}
		if (ImGui::MenuItem("Disconnect", nullptr, false, connected)) {
			t->toxGroupDisconnect(c.get<Contact::Components::ToxGroupEphemeral>().group_number);
		}
		ImGui::SetItemTooltip("Disconnect from group, without wiping your keys.\nUse reconnect to rejoin later.");
	}

	const auto group_number = c.get<Contact::Components::ToxGroupEphemeral>().group_number;
	bool is_founder = false;
	bool is_mod = false;
	bool is_user = false;
	if (auto self_id_opt = t->toxGroupSelfGetPeerId(group_number); self_id_opt) {
		auto [role_opt, _] = t->toxGroupPeerGetRole(group_number, self_id_opt.value());
		if (role_opt) {
			switch (role_opt.value()) {
				case TOX_GROUP_ROLE_FOUNDER:
					is_founder = true;
					ImGui::TextDisabled("Self Role: Founder");
					break;
				case TOX_GROUP_ROLE_MODERATOR:
					is_mod = true;
					ImGui::TextDisabled("Self Role: Moderator");
					break;
				case TOX_GROUP_ROLE_USER:
					is_user = true;
					ImGui::TextDisabled("Self Role: User");
					break;
				case TOX_GROUP_ROLE_OBSERVER:
					ImGui::TextDisabled("Self Role: Observer");
					break;
			}
		}
	}

	if (is_founder) {
		// TODO: show pw set

		// password
		if (ImGui::MenuItem("Change password")) {
			if (ToxUIUtils** tui = parent.try_get<ToxUIUtils*>(); tui != nullptr && *tui != nullptr) {
				(*tui)->openGroupPassword(c);
			}
		}
	}

	{ // peer limit
		const uint16_t peer_limit = t->toxGroupGetPeerLimit(group_number).value_or(0);
		ImGui::TextDisabled("PeerLimit: %d", peer_limit);
		if (is_founder && ImGui::MenuItem("Change PeerLimit")) {
			if (ToxUIUtils** tui = parent.try_get<ToxUIUtils*>(); tui != nullptr && *tui != nullptr) {
				(*tui)->openGroupPeerLimit(c);
			}
		}
	}

	{ // topic
		const bool topic_locked = t->toxGroupGetTopicLock(group_number).value_or(true);
		std::string topic_m_str = std::string("Topic ") + (topic_locked ? ">locked<" : "<unlocked>") + "##topic lock";
		if (ImGui::BeginMenu(topic_m_str.c_str(), is_founder)) {
			if (ImGui::MenuItem("lock", nullptr, false, !topic_locked)) {
				t->toxGroupSetTopicLock(group_number, true);
			}

			if (ImGui::MenuItem("unlock", nullptr, false, topic_locked)) {
				t->toxGroupSetTopicLock(group_number, false);
			}

			ImGui::EndMenu();
		}

		const bool can_edit_topic = is_mod || is_founder || (!topic_locked && is_user);
		if (can_edit_topic) {
			if (ImGui::MenuItem("Change Topic")) {
				if (ToxUIUtils** tui = parent.try_get<ToxUIUtils*>(); tui != nullptr && *tui != nullptr) {
					(*tui)->openGroupTopic(c);
				}
			}
		}
	}

	{ // privacy state
		auto state_opt = t->toxGroupGetPrivacyState(group_number);
		if (state_opt) {
			switch (state_opt.value()) {
				case TOX_GROUP_PRIVACY_STATE_PUBLIC:
					ImGui::TextDisabled("PrivacyState: Public");
					break;
				case TOX_GROUP_PRIVACY_STATE_PRIVATE:
					ImGui::TextDisabled("PrivacyState: Private");
					break;
			}
			ImGui::SetItemTooltip("Public means the peers announce the group + themselfs to the DHT.");
			if (is_founder) {
				if (ImGui::BeginMenu("Set PrivacyState")) {
					if (ImGui::MenuItem("Public", nullptr, false, state_opt.value() != Tox_Group_Privacy_State::TOX_GROUP_PRIVACY_STATE_PUBLIC)) {
						t->toxGroupSetPrivacyState(group_number, Tox_Group_Privacy_State::TOX_GROUP_PRIVACY_STATE_PUBLIC);
					}
					if (ImGui::MenuItem("Private", nullptr, false, state_opt.value() != Tox_Group_Privacy_State::TOX_GROUP_PRIVACY_STATE_PRIVATE)) {
						t->toxGroupSetPrivacyState(group_number, Tox_Group_Privacy_State::TOX_GROUP_PRIVACY_STATE_PRIVATE);
					}
					ImGui::EndMenu();
				}
			}
		}
	}

	{ // voice state
		auto state_opt = t->toxGroupGetVoiceState(group_number);
		if (state_opt) {
			switch (state_opt.value()) {
				case TOX_GROUP_VOICE_STATE_ALL:
					ImGui::TextDisabled("VoiceState: All");
					break;
				case TOX_GROUP_VOICE_STATE_MODERATOR:
					ImGui::TextDisabled("VoiceState: Moderators");
					break;
				case TOX_GROUP_VOICE_STATE_FOUNDER:
					ImGui::TextDisabled("VoiceState: Founder");
					break;
			}
			if (is_founder) {
				if (ImGui::BeginMenu("Set VoiceState")) {
					if (ImGui::MenuItem("All", nullptr, false, state_opt.value() != Tox_Group_Voice_State::TOX_GROUP_VOICE_STATE_ALL)) {
						t->toxGroupSetVoiceState(group_number, Tox_Group_Voice_State::TOX_GROUP_VOICE_STATE_ALL);
					}
					if (ImGui::MenuItem("Moderators", nullptr, false, state_opt.value() != Tox_Group_Voice_State::TOX_GROUP_VOICE_STATE_MODERATOR)) {
						t->toxGroupSetVoiceState(group_number, Tox_Group_Voice_State::TOX_GROUP_VOICE_STATE_MODERATOR);
					}
					if (ImGui::MenuItem("Founder", nullptr, false, state_opt.value() != Tox_Group_Voice_State::TOX_GROUP_VOICE_STATE_FOUNDER)) {
						t->toxGroupSetVoiceState(group_number, Tox_Group_Voice_State::TOX_GROUP_VOICE_STATE_FOUNDER);
					}
					ImGui::EndMenu();
				}
			}
		}
	}

	ImGui::EndMenu();
}

static void renderContextGroupPeer(ContactHandle4 c) {
	if (!c.all_of<Contact::Components::ToxGroupPeerEphemeral, Contact::Components::Parent>()) {
		return;
	}

	if (!ImGui::BeginMenu("Tox")) {
		return;
	}

	// need tox from parent's parent
	const auto parent = ContactHandle4{*c.registry(), c.get<Contact::Components::Parent>().parent};
	if (!static_cast<bool>(parent) || !parent.all_of<Contact::Components::ToxGroupEphemeral, Contact::Components::Parent>()) {
		ImGui::TextDisabled("parent missing parent or tge");
		ImGui::EndMenu();
		return;
	}

	const auto parent_parent = ContactHandle4{*c.registry(), parent.get<Contact::Components::Parent>().parent};
	if (!static_cast<bool>(parent_parent) || !parent_parent.all_of<Contact::Components::TagRoot, ToxI*>()) {
		ImGui::TextDisabled("parent's parent missing root or toxi");
		ImGui::EndMenu();
		return;
	}

	ToxI* t = parent_parent.get<ToxI*>();

	if (t == nullptr) {
		ImGui::EndMenu();
		return;
	}

	const auto [group_number, peer_number] = c.get<Contact::Components::ToxGroupPeerEphemeral>();

	{ // status
		// TODO: remove and make proper generic api/comp
		auto [status_opt, _] = t->toxGroupPeerGetStatus(group_number, peer_number);
		if (status_opt) {
			switch (status_opt.value()) {
				case TOX_USER_STATUS_NONE:
					ImGui::TextDisabled("Status: None");
					break;
				case TOX_USER_STATUS_AWAY:
					ImGui::TextDisabled("Status: Away");
					break;
				case TOX_USER_STATUS_BUSY:
					ImGui::TextDisabled("Status: Busy");
					break;
			}

			if (c.all_of<Contact::Components::TagSelfStrong>()) {
				if (ImGui::BeginMenu("Set self status")) {
					if (ImGui::MenuItem("None", nullptr, false, status_opt.value() != Tox_User_Status::TOX_USER_STATUS_NONE)) {
						t->toxGroupSelfSetStatus(group_number, Tox_User_Status::TOX_USER_STATUS_NONE);
					}
					if (ImGui::MenuItem("Away", nullptr, false, status_opt.value() != Tox_User_Status::TOX_USER_STATUS_AWAY)) {
						t->toxGroupSelfSetStatus(group_number, Tox_User_Status::TOX_USER_STATUS_AWAY);
					}
					if (ImGui::MenuItem("Busy", nullptr, false, status_opt.value() != Tox_User_Status::TOX_USER_STATUS_BUSY)) {
						t->toxGroupSelfSetStatus(group_number, Tox_User_Status::TOX_USER_STATUS_BUSY);
					}
					ImGui::EndMenu();
				}
			}
		}
	}

	{ // role
		auto [role_opt, _] = t->toxGroupPeerGetRole(group_number, peer_number);
		if (role_opt.has_value()) {
			switch (role_opt.value()) {
				case TOX_GROUP_ROLE_FOUNDER:
					ImGui::TextDisabled("Role: Founder");
					break;
				case TOX_GROUP_ROLE_MODERATOR:
					ImGui::TextDisabled("Role: Moderator");
					break;
				case TOX_GROUP_ROLE_USER:
					ImGui::TextDisabled("Role: User");
					break;
				case TOX_GROUP_ROLE_OBSERVER:
					ImGui::TextDisabled("Role: Observer");
					break;
			}

			if (!c.all_of<Contact::Components::TagSelfStrong>()) {
				const auto self_role = t->toxGroupSelfGetRole(group_number).value_or(TOX_GROUP_ROLE_OBSERVER);
				if (self_role == TOX_GROUP_ROLE_MODERATOR || self_role == TOX_GROUP_ROLE_FOUNDER) {
					if (ImGui::BeginMenu("Set role")) {
						if (self_role == TOX_GROUP_ROLE_FOUNDER && ImGui::MenuItem("Moderator", nullptr, false, role_opt.value() != TOX_GROUP_ROLE_MODERATOR)) {
							if (t->toxGroupSetRole(group_number, peer_number, TOX_GROUP_ROLE_MODERATOR) == TOX_ERR_GROUP_SET_ROLE_OK) {
								c.emplace_or_replace<Contact::Components::Roles>().rs.emplace_back(Tox_Group_Role::TOX_GROUP_ROLE_MODERATOR);
							}
						}
						if (ImGui::MenuItem("User", nullptr, false, role_opt.value() != TOX_GROUP_ROLE_USER)) {
							if (t->toxGroupSetRole(group_number, peer_number, TOX_GROUP_ROLE_USER) == TOX_ERR_GROUP_SET_ROLE_OK) {
								c.emplace_or_replace<Contact::Components::Roles>().rs.emplace_back(Tox_Group_Role::TOX_GROUP_ROLE_USER);
							}
						}
						if (ImGui::MenuItem("Observer", nullptr, false, role_opt.value() != TOX_GROUP_ROLE_OBSERVER)) {
							if (t->toxGroupSetRole(group_number, peer_number, TOX_GROUP_ROLE_OBSERVER) == TOX_ERR_GROUP_SET_ROLE_OK) {
								c.emplace_or_replace<Contact::Components::Roles>().rs.emplace_back(Tox_Group_Role::TOX_GROUP_ROLE_OBSERVER);
							}
						}
						ImGui::EndMenu();
					}
				}

				// if self >= mod && self > peer
				if (
					self_role == TOX_GROUP_ROLE_FOUNDER ||
					(self_role == TOX_GROUP_ROLE_MODERATOR && (role_opt.value() == TOX_GROUP_ROLE_USER || role_opt.value() == TOX_GROUP_ROLE_OBSERVER))
				) {
					if (ImGui::MenuItem("Kick Peer")) {
						t->toxGroupKickPeer(group_number, peer_number);

						// HACK: no event n stuff
						c.remove<Contact::Components::ToxGroupPeerEphemeral>();
						c.emplace_or_replace<Contact::Components::ConnectionState>(Contact::Components::ConnectionState::State::disconnected);
					}
					ImGui::SetItemTooltip("Tell the peer to disconnect and broadcast a notice to all other connected peers to do the same.");
				}
			}
		}
	}

	{ // ignore
		if (!c.all_of<Contact::Components::TagSelfStrong>()) {
			// no way to get current ignore state
			// TODO: make toxcore provide better api and adjust the menu
			// will need that to support custom protocol ignores, rn only observer supported
			// TODO: add generic ignore comp to contacts
			if (ImGui::BeginMenu("Set Ignore")) {
				if (ImGui::MenuItem("True")) {
					t->toxGroupSetIgnore(group_number, peer_number, true);
				}
				if (ImGui::MenuItem("False")) {
					t->toxGroupSetIgnore(group_number, peer_number, false);
				}
				ImGui::EndMenu();
			}
		}
	}

	// self name
	if (c.all_of<Contact::Components::TagSelfStrong>()) {
		if (ImGui::MenuItem("Change your Name")) {
			if (ToxUIUtils** tui = parent_parent.try_get<ToxUIUtils*>(); tui != nullptr && *tui != nullptr) {
				(*tui)->openGroupSelfName(parent, c);
			}
		}
	}

	ImGui::EndMenu();
}

ToxUIUtils::ToxUIUtils(
	ToxClient& tc,
	ContactStore4I& cs,
	ToxContactModel2& tcm,
	ConfigModelI& conf,
	ToxPrivateI* tp
) : _tc(tc), _cs(cs), _tcm(tcm), _conf(conf), _tp(tp) {
	// TODO: should live in the contact instead
	// emplace self onto root
	// find root with same tox ptr
	bool emplaced = false;
	for (const auto [c, toxi] : cs.registry().view<Contact::Components::TagRoot, ToxI*>().each()) {
		if (toxi == &tc) {
			cs.registry().emplace_or_replace<ToxUIUtils*>(c, this);
			emplaced = true;
			break;
		}
	}
	if (!emplaced) {
		throw std::runtime_error("could not find root contact with same tox interface");
	}

	if (!_cs.registerImGuiContext(
		entt::type_id<Contact::Components::ToxGroupEphemeral>().hash(),
		renderContextGroup
	)) {
		throw std::runtime_error("failed to register tox group imgui context menu");
	}

	if (!_cs.registerImGuiContext(
		entt::type_id<Contact::Components::ToxGroupPeerEphemeral>().hash(),
		renderContextGroupPeer
	)) {
		throw std::runtime_error("failed to register tox group peer imgui context menu");
	}
}

ToxUIUtils::~ToxUIUtils(void) {
	_cs.unregisterImGuiContext(
		entt::type_id<Contact::Components::ToxGroupPeerEphemeral>().hash()
	);

	_cs.unregisterImGuiContext(
		entt::type_id<Contact::Components::ToxGroupEphemeral>().hash()
	);

	for (const auto [c, toxui] : _cs.registry().view<Contact::Components::TagRoot, ToxUIUtils*>().each()) {
		if (toxui == this) {
			_cs.registry().remove<ToxUIUtils*>(c);
			break;
		}
	}
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
							{"solanaceae/tomato (tomato offical)", "f7a8a98986707c27b72148dd1a3c11d23fc8896670b100d27f8a9d3e098c4bda"},
							{"TokTok-dev (toxcore dev)", "360497da684bce2a500c1af9b3a5ce949bbb9f6fb1f91589806fb04ca039e313"},
							{"Lobby (general offtopic)", "d325f0095cb4d10f5ed668b854e2e10c131f7256949625e5e2dddadd8143dffa"},
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
			ImGui::TextDisabled("Connecting via ID requires the group to be public and might take a very long time.");

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
				"If DNS querries where not disabled at launch, domain names can be used too.\n"
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

	if (_open_group_self_name) {
		ImGui::OpenPopup("Group self name");
		_open_group_self_name = false;
	}

	if (ImGui::BeginPopup("Group self name")) {
		ImGui::Text("Change your name in this group and this group only.");
		ImGui::InputText("##new name", &_gsn_name);

		if (ImGui::Button("Cancel", {ImGui::GetContentRegionAvail().x/2.f, 0})) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Ok", {-FLT_MIN, 0})) {
			if (const auto* tge_ptr = _cs.registry().try_get<Contact::Components::ToxGroupEphemeral>(_gsn_group); tge_ptr != nullptr) {
				_tc.toxGroupSelfSetName(tge_ptr->group_number, _gsn_name);
				if (auto* name_ptr = _cs.registry().try_get<Contact::Components::Name>(_gsn_self); name_ptr != nullptr) {
					name_ptr->name = _gsn_name;
					_cs.throwEventUpdate(_gsn_self);
				}
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (_open_group_password) {
		ImGui::OpenPopup("Group password");
		_open_group_password = false;
	}

	if (ImGui::BeginPopup("Group password")) {
		ImGui::Text("Set a new password for this group.");
		ImGui::InputText(
			"##password", &_gp_password,
			ImGuiInputTextFlags_EnterReturnsTrue | (!_gp_show_password ? ImGuiInputTextFlags_Password : ImGuiInputTextFlags_None)
		);
		ImGui::SameLine();
		ImGui::Checkbox("show password", &_gp_show_password);

		if (ImGui::Button("Cancel", {ImGui::GetContentRegionAvail().x/2.f, 0})) {
			_gp_password.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Ok", {-FLT_MIN, 0})) {
			if (const auto* tge_ptr = _cs.registry().try_get<Contact::Components::ToxGroupEphemeral>(_gp_group); tge_ptr != nullptr) {
				_tc.toxGroupSetPassword(tge_ptr->group_number, _gp_password);
				_gp_password.clear();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}

	if (_open_group_peer_limit) {
		ImGui::OpenPopup("Group peer limit");
		_open_group_peer_limit = false;
	}

	if (ImGui::BeginPopup("Group peer limit")) {
		ImGui::Text("Change the peer limit for this group.");
		ImGui::InputScalar("##limit", ImGuiDataType_U16, &_gpl_peer_limit);

		if (ImGui::Button("Cancel", {ImGui::GetContentRegionAvail().x/2.f, 0})) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Ok", {-FLT_MIN, 0})) {
			if (const auto* tge_ptr = _cs.registry().try_get<Contact::Components::ToxGroupEphemeral>(_gpl_group); tge_ptr != nullptr) {
				_tc.toxGroupSetPeerLimit(tge_ptr->group_number, _gpl_peer_limit);
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (_open_group_topic) {
		ImGui::OpenPopup("Group topic");
		_open_group_topic = false;
	}

	if (ImGui::BeginPopup("Group topic")) {
		ImGui::Text("Change the topic for this group.");
		ImGui::InputTextMultiline("##topic", &_gt_topic);

		if (ImGui::Button("Cancel", {ImGui::GetContentRegionAvail().x/2.f, 0})) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Ok", {-FLT_MIN, 0})) {
			if (const auto* tge_ptr = _cs.registry().try_get<Contact::Components::ToxGroupEphemeral>(_gt_group); tge_ptr != nullptr) {
				if (_tc.toxGroupSetTopic(tge_ptr->group_number, _gt_topic) == TOX_ERR_GROUP_TOPIC_SET_OK) {
					if (auto* status_ptr = _cs.registry().try_get<Contact::Components::StatusText>(_gt_group); status_ptr != nullptr) {
						status_ptr->text = _gt_topic;
						status_ptr->fillFirstLineLength();
						_cs.throwEventUpdate(_gt_group);
					}
				}
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void ToxUIUtils::openGroupSelfName(Contact4 group_v, Contact4 self_v) {
	auto cg = _cs.contactHandle(group_v);
	if (!static_cast<bool>(cg)) {
		return;
	}

	auto c = _cs.contactHandle(self_v);
	if (!static_cast<bool>(c)) {
		return;
	}

	if (auto* name_ptr = c.try_get<Contact::Components::Name>(); name_ptr != nullptr) {
		_gsn_name = name_ptr->name;
	}

	_gsn_group = group_v;
	_gsn_self = self_v;

	_open_group_self_name = true;
}

void ToxUIUtils::openGroupPassword(Contact4 group_v) {
	auto cg = _cs.contactHandle(group_v);
	if (!static_cast<bool>(cg)) {
		return;
	}

	_gp_password.clear();

	if (auto* tge_ptr = cg.try_get<Contact::Components::ToxGroupEphemeral>(); tge_ptr != nullptr) {
		_gp_password = _tc.toxGroupGetPassword(tge_ptr->group_number).value_or("");
	}

	_gp_show_password = false;
	_gp_group = group_v;

	_open_group_password = true;
}

void ToxUIUtils::openGroupPeerLimit(Contact4 group_v) {
	auto cg = _cs.contactHandle(group_v);
	if (!static_cast<bool>(cg)) {
		return;
	}

	if (auto* tge_ptr = cg.try_get<Contact::Components::ToxGroupEphemeral>(); tge_ptr != nullptr) {
		_gpl_peer_limit = _tc.toxGroupGetPeerLimit(tge_ptr->group_number).value_or(0);
	}

	_gpl_group = group_v;

	_open_group_peer_limit = true;
}

void ToxUIUtils::openGroupTopic(Contact4 group_v) {
	auto cg = _cs.contactHandle(group_v);
	if (!static_cast<bool>(cg)) {
		return;
	}

	if (auto* status_ptr = cg.try_get<Contact::Components::StatusText>(); status_ptr != nullptr) {
		_gt_topic = status_ptr->text;
	}

	_gt_group = group_v;

	_open_group_topic = true;
}
