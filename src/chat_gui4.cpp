#include "./chat_gui4.hpp"

#include "./file_selector.hpp"

#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/components.hpp>
#include <solanaceae/contact/components.hpp>

//#include "./media_meta_info_loader.hpp"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <SDL3/SDL.h>

#include "./sdl_clipboard_utils.hpp"

#include <string>
#include <variant>
#include <vector>
#include <chrono>
#include <filesystem>
#include <ctime>

ChatGui4::ChatGui4(
	ConfigModelI& conf,
	RegistryMessageModel& rmm,
	Contact3Registry& cr,
	TextureUploaderI& tu
) : _conf(conf), _rmm(rmm), _cr(cr), _tal(_cr), _contact_tc(_tal, tu) {
}

void ChatGui4::render(void) {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
	TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	constexpr auto bg_window_flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	if (ImGui::Begin("tomato", nullptr, bg_window_flags)) {
		if (ImGui::BeginMenuBar()) {
			//ImGui::Separator();
			ImGui::Text("%.1fFPS", ImGui::GetIO().Framerate);
			ImGui::EndMenuBar();
		}

		renderContactList();
		ImGui::SameLine();

		if (_selected_contact) {
			const std::string chat_label = "chat " + std::to_string(entt::to_integral(*_selected_contact));
			if (ImGui::BeginChild(chat_label.c_str(), {0, 0}, true)) {
				static std::string text_buffer;

				if (_cr.all_of<Contact::Components::ParentOf>(*_selected_contact)) {
					const auto sub_contacts = _cr.get<Contact::Components::ParentOf>(*_selected_contact).subs;
					if (!sub_contacts.empty()) {
						if (ImGui::BeginChild("subcontacts", {150, -100}, true)) {
							ImGui::Text("subs: %zu", sub_contacts.size());
							ImGui::Separator();
							for (const auto& c : sub_contacts) {
								if (renderSubContactListContact(c)) {
									text_buffer.insert(0, (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>") + ": ");
								}
							}
						}
						ImGui::EndChild();
						ImGui::SameLine();
					}
				}

				const bool request_incoming = _cr.all_of<Contact::Components::RequestIncoming>(*_selected_contact);
				const bool request_outgoing = _cr.all_of<Contact::Components::TagRequestOutgoing>(*_selected_contact);
				if (request_incoming || request_outgoing) {
					// TODO: theming
					ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.90f, 0.70f, 0.00f, 0.32f});
					if (ImGui::BeginChild("request", {0, TEXT_BASE_HEIGHT*6.1f}, true, ImGuiWindowFlags_NoScrollbar)) {
						if (request_incoming) {
							const auto& ri = _cr.get<Contact::Components::RequestIncoming>(*_selected_contact);
							ImGui::TextUnformatted("You got a request to add this contact.");

							static std::string self_name = _conf.get_string("tox", "name").value_or("default_tomato");
							if (ri.name) {
								ImGui::InputText("name to join with", &self_name);
							} else {
								//ImGui::TextUnformatted("");
								ImGui::Dummy({0, TEXT_BASE_HEIGHT});
							}

							static std::string password;
							if (ri.password) {
								ImGui::InputText("password to join with", &password);
							} else {
								////ImGui::TextUnformatted("");
								ImGui::Dummy({0, TEXT_BASE_HEIGHT});
							}

							if (ImGui::Button("Accept")) {
								_cr.get<Contact::Components::ContactModel>(*_selected_contact)->acceptRequest(*_selected_contact, self_name, password);
								password.clear();
							}
							ImGui::SameLine();
							if (ImGui::Button("Decline")) {
							}
						} else {
							ImGui::TextUnformatted("You sent a reqeust to add this contact.");
						}
					}
					ImGui::PopStyleColor();
					ImGui::EndChild();
				}

				if (ImGui::BeginChild("message_log", {0, -100}, false, ImGuiWindowFlags_MenuBar)) {
					if (ImGui::BeginMenuBar()) {
						ImGui::Checkbox("show extra info", &_show_chat_extra_info);
						ImGui::EndMenuBar();
					}

					auto* msg_reg_ptr = _rmm.get(*_selected_contact);

					constexpr ImGuiTableFlags table_flags =
						ImGuiTableFlags_BordersInnerV |
						ImGuiTableFlags_RowBg |
						ImGuiTableFlags_SizingFixedFit
					;
					if (msg_reg_ptr != nullptr && ImGui::BeginTable("chat_table", 4, table_flags)) {
						ImGui::TableSetupColumn("name", 0, TEXT_BASE_WIDTH * 15.f);
						ImGui::TableSetupColumn("message", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("timestamp");
						ImGui::TableSetupColumn("extra_info", _show_chat_extra_info ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_Disabled);

						// very hacky, and we have variable hight entries
						//ImGuiListClipper clipper;

						// fake empty placeholders
						// TODO: save/calc height for each row
						//  - use number of lines for text
						//  - save img dims (capped)
						//  - other static sizes
						//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);
						//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

						Message3Registry& msg_reg = *msg_reg_ptr;
						msg_reg.view<Message::Components::ContactFrom, Message::Components::ContactTo, Message::Components::Timestamp>()
							.use<Message::Components::Timestamp>()
							.each([&](const Message3 e, Message::Components::ContactFrom& c_from, Message::Components::ContactTo& c_to, Message::Components::Timestamp ts
						) {
							// TODO: why?
							ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

							ImGui::PushID(entt::to_integral(e));

							// name
							if (ImGui::TableNextColumn()) {
								if (_cr.all_of<Contact::Components::Name>(c_from.c)) {
									ImGui::TextUnformatted(_cr.get<Contact::Components::Name>(c_from.c).name.c_str());
								} else {
									ImGui::TextUnformatted("<unk>");
								}

								// highlight self
								if (_cr.any_of<Contact::Components::TagSelfWeak, Contact::Components::TagSelfStrong>(c_from.c)) {
									ImU32 cell_bg_color = ImGui::GetColorU32(ImVec4(0.3f, 0.7f, 0.3f, 0.20f));
									ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
								} else {
									//based on power level?
									//ImU32 cell_bg_color = ImGui::GetColorU32(ImVec4(0.3f, 0.7f, 0.3f, 0.65f));
									//ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
								}

								// private group message
								if (_cr.any_of<Contact::Components::TagSelfWeak, Contact::Components::TagSelfStrong>(c_to.c)) {
									ImU32 row_bg_color = ImGui::GetColorU32(ImVec4(0.5f, 0.2f, 0.5f, 0.35f));
									ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);
								}
							}

							// content (msgtext/file)
							ImGui::TableNextColumn();
							if (msg_reg.all_of<Message::Components::MessageText>(e)) {
								renderMessageBodyText(msg_reg, e);
							} else if (msg_reg.any_of<Message::Components::Transfer::FileInfo>(e)) { // add more comps?
								renderMessageBodyFile(msg_reg, e);
							} else {
								ImGui::TextUnformatted("---");
							}

							// ts
							if (ImGui::TableNextColumn()) {
								auto time = std::chrono::system_clock::to_time_t(
									std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>{std::chrono::milliseconds{ts.ts}}
								);
								auto localtime = std::localtime(&time);

								ImGui::Text("%.2d:%.2d", localtime->tm_hour, localtime->tm_min);
							}

							// extra
							if (ImGui::TableNextColumn()) {
								renderMessageExtra(msg_reg, e);
							}

							ImGui::PopID(); // ent
						});

						// fake empty placeholders
						//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);
						//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

						ImGui::EndTable();
					}

					if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
						ImGui::SetScrollHereY(1.f);
					}
				}
				ImGui::EndChild();

				if (ImGui::BeginChild("text_input", {-150, 0})) {
					static bool evil_enter_looses_focus_hack = false;
					if (evil_enter_looses_focus_hack) {
						ImGui::SetKeyboardFocusHere();
						evil_enter_looses_focus_hack = false;
					}

					constexpr ImGuiInputTextFlags input_flags =
						ImGuiInputTextFlags_EnterReturnsTrue |
						//ImGuiInputTextFlags_AllowTabInput |
						ImGuiInputTextFlags_NoHorizontalScroll |
						ImGuiInputTextFlags_CtrlEnterForNewLine;

					if (ImGui::InputTextMultiline("##text_input", &text_buffer, {-0.001f, -0.001f}, input_flags)) {
						//_mm.sendMessage(*_selected_contact, MessageType::TEXT, text_buffer);
						_rmm.sendText(*_selected_contact, text_buffer);
						text_buffer.clear();
						evil_enter_looses_focus_hack = true;
					}
				}
				ImGui::EndChild();
				ImGui::SameLine();

				if (ImGui::BeginChild("buttons")) {
					if (ImGui::Button("send\nfile", {-FLT_MIN, 0})) {
						_fss.requestFile(
							[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
							[this](const auto& path){
								_rmm.sendFilePath(*_selected_contact, path.filename().u8string(), path.u8string());
							},
							[](){}
						);
					}
				}
				ImGui::EndChild();

				if (ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::IsKeyPressed(ImGuiMod_Shortcut)) {
					if (const auto* mime_type = clipboardHasImage(); mime_type != nullptr) {
						size_t data_size = 0;
						const auto* data = SDL_GetClipboardData(mime_type, &data_size);
						// open file send preview.rawpixels
					}
				}
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();

	_fss.render();

	_contact_tc.update();
	_contact_tc.workLoadQueue();
}

// has MessageText
void ChatGui4::renderMessageBodyText(Message3Registry& reg, const Message3 e) {
	const auto& msgtext = reg.get<Message::Components::MessageText>(e).text;

	// TODO: set word wrap
	ImVec2 text_size = ImGui::CalcTextSize(msgtext.c_str(), msgtext.c_str()+msgtext.size());
	text_size.x = -FLT_MIN; // fill width (suppresses label)
	text_size.y += ImGui::GetStyle().FramePadding.y; // single pad

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0}); // make align with text height
	ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.f, 0.f, 0.f, 0.f}); // remove text input box

	ImGui::InputTextMultiline(
		"",
		const_cast<char*>(msgtext.c_str()), // ugly const cast
		msgtext.size() + 1, // needs to include '\0'
		text_size,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll
	);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void ChatGui4::renderMessageBodyFile(Message3Registry& reg, const Message3 e) {
#if 0
	if (msg_reg.all_of<Components::TransferState>(e)) {
		switch (msg_reg.get<Components::TransferState>(e).state) {
			case Components::TransferState::State::running: ImGui::TextUnformatted("running"); break;
			case Components::TransferState::State::paused: ImGui::TextUnformatted("paused"); break;
			case Components::TransferState::State::failed: ImGui::TextUnformatted("failed"); break;
			case Components::TransferState::State::finished: ImGui::TextUnformatted("finished"); break;
		}
	} else {
		assert(false);
	}
#endif
	// TODO: better way to display state
	if (reg.all_of<Message::Components::Transfer::TagPaused>(e)) {
		ImGui::TextUnformatted("paused");
	} else {
		// TODO: missing other staes
		ImGui::TextUnformatted("running");
	}

	// if in offered state
	// paused, never started
	if (
		reg.all_of<Message::Components::Transfer::TagReceiving>(e) &&
		reg.all_of<Message::Components::Transfer::TagPaused>(e) &&
		// TODO: how does restarting a broken/incomplete transfer look like?
		!reg.all_of<Message::Components::Transfer::FileInfoLocal>(e) &&
		!reg.all_of<Message::Components::Transfer::ActionAccept>(e)
	) {
		if (ImGui::Button("save to")) {
			_fss.requestFile(
				[](const auto& path) -> bool { return std::filesystem::is_directory(path); },
				[this, &reg, e](const auto& path) {
					if (reg.valid(e)) { // still valid
						reg.emplace<Message::Components::Transfer::ActionAccept>(e, path.string());
						_rmm.throwEventUpdate(reg, e);
					}
				},
				[](){}
			);
		}
	}

	// down progress
	if (reg.all_of<Message::Components::Transfer::TagReceiving>(e)) {
		ImGui::TextUnformatted("down");
		if (reg.all_of<Message::Components::Transfer::BytesReceived>(e)) {
			ImGui::SameLine();
			ImGui::ProgressBar(
				float(reg.get<Message::Components::Transfer::BytesReceived>(e).total) / reg.get<Message::Components::Transfer::FileInfo>(e).total_size,
				{-FLT_MIN, TEXT_BASE_HEIGHT}
			);
			// TODO: numbers
		}
	}

	// (can be both)
	// up progess
	if (reg.all_of<Message::Components::Transfer::TagSending>(e)) {
		ImGui::TextUnformatted("  up");
		if (reg.all_of<Message::Components::Transfer::BytesSent>(e)) {
			ImGui::SameLine();
			ImGui::ProgressBar(
				float(reg.get<Message::Components::Transfer::BytesSent>(e).total) / reg.get<Message::Components::Transfer::FileInfo>(e).total_size,
				{-FLT_MIN, TEXT_BASE_HEIGHT}
			);
			// TODO: numbers
		}
	}

	const auto file_list = reg.get<Message::Components::Transfer::FileInfo>(e).file_list;

	// if has local, display save base path?

	for (size_t i = 0; i < file_list.size(); i++) {
		// TODO: selectable text widget
		ImGui::Bullet(); ImGui::Text("%s (%lu)", file_list[i].file_name.c_str(), file_list[i].file_size);
	}

#if 0

	// TODO: use frame dims
	if (file_list.size() == 1 && reg.all_of<Message::Components::Transfer::FileInfoLocal, Message::Components::FrameDims>(e)) {
		auto [id, width, height] = _tc.get(reg.get<Message::Components::Transfer::FileInfoLocal>(e).file_list.front());

		// if cache gives 0s, fall back to frame dims (eg if pic not loaded yet)
		if (width > 0 || height > 0) {
			const auto& frame_dims = reg.get<Message::Components::FrameDims>(e);
			width = frame_dims.width;
			height = frame_dims.height;
		}

		// TODO: config
		const auto max_inline_height = 10*TEXT_BASE_HEIGHT;
		if (height > max_inline_height) {
			const float scale = max_inline_height / height;
			height = max_inline_height;
			width *= scale;
		}
		ImGui::Image(id, ImVec2{static_cast<float>(width), static_cast<float>(height)});
	}
#endif
}

void ChatGui4::renderMessageExtra(Message3Registry& reg, const Message3 e) {
	if (reg.all_of<Message::Components::Transfer::FileKind>(e)) {
		ImGui::TextDisabled("fk:%lu", reg.get<Message::Components::Transfer::FileKind>(e).kind);
	}
	if (reg.all_of<Message::Components::Transfer::ToxTransferFriend>(e)) {
		ImGui::TextDisabled("ttf:%u", reg.get<Message::Components::Transfer::ToxTransferFriend>(e).transfer_number);
	}

	if (reg.all_of<Message::Components::ToxGroupMessageID>(e)) {
		ImGui::TextDisabled("msgid:%u", reg.get<Message::Components::ToxGroupMessageID>(e).id);
	}

	if (reg.all_of<Message::Components::SyncedBy>(e)) {
		std::string synced_by_text {"syncedBy:"};
		const auto& synced_by = reg.get<Message::Components::SyncedBy>(e).list;
		for (const auto& c : synced_by) {
			if (_cr.all_of<Contact::Components::TagSelfStrong>(c)) {
				synced_by_text += "\n sself";
			} else if (_cr.all_of<Contact::Components::TagSelfWeak>(c)) {
				synced_by_text += "\n wself";
			} else {
				synced_by_text += "\n >" + (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>");
			}
		}
		ImGui::TextDisabled("%s", synced_by_text.c_str());
	}
}

void ChatGui4::renderContactList(void) {
	if (ImGui::BeginChild("contacts", {TEXT_BASE_WIDTH*35, 0})) {
		//for (const auto& c : _cm.getBigContacts()) {
		for (const auto& c : _cr.view<Contact::Components::TagBig>()) {
			if (renderContactListContactBig(c)) {
				_selected_contact = c;
			}
		}
	}
	ImGui::EndChild();
}

bool ChatGui4::renderContactListContactBig(const Contact3 c) {
	// TODO:
	// - unread message
	// - avatar img
	// - connection status
	// - user status
	// - status message
	// - context menu n shit?

	// +------+
	// |	  | *Name (Alias?)
	// |Avatar| Satus Message
	// |	  | user status (online/away/busy)-direct/relayed / offline
	// +------+

	auto label = "###" + std::to_string(entt::to_integral(c));

	const bool request_incoming = _cr.all_of<Contact::Components::RequestIncoming>(c);
	const bool request_outgoing = _cr.all_of<Contact::Components::TagRequestOutgoing>(c);

	ImVec2 orig_curser_pos = ImGui::GetCursorPos();
	// HACK: fake selected to make it draw a box for us
	const bool show_selected = request_incoming || request_outgoing || (_selected_contact.has_value() && *_selected_contact == c);
	if (request_incoming) {
		// TODO: theming
		ImGui::PushStyleColor(ImGuiCol_Header, {0.98f, 0.41f, 0.26f, 0.52f});
	} else if (request_outgoing) {
		// TODO: theming
		ImGui::PushStyleColor(ImGuiCol_Header, {0.98f, 0.26f, 0.41f, 0.52f});
	}

	const bool selected = ImGui::Selectable(label.c_str(), show_selected, 0, {0,3*TEXT_BASE_HEIGHT});

	if (request_incoming || request_outgoing) {
		ImGui::PopStyleColor();
	}
	ImVec2 post_curser_pos = ImGui::GetCursorPos();

	ImVec2 img_curser {
		orig_curser_pos.x + ImGui::GetStyle().FramePadding.x,
		orig_curser_pos.y + ImGui::GetStyle().FramePadding.y
	};

	float img_y {
		//(post_curser_pos.y - orig_curser_pos.y) - ImGui::GetStyle().FramePadding.y*2
		TEXT_BASE_HEIGHT*3 - ImGui::GetStyle().FramePadding.y*2
	};

	ImGui::SetCursorPos(img_curser);
	const ImVec4 color_online_direct{0.3, 1, 0, 1};
	const ImVec4 color_online_cloud{0, 1, 0.8, 1};
	const ImVec4 color_offline{0.4, 0.4, 0.4, 1};

	ImVec4 color_current = color_offline;
	if (_cr.all_of<Contact::Components::ConnectionState>(c)) {
		const auto c_state = _cr.get<Contact::Components::ConnectionState>(c).state;
		if (c_state == Contact::Components::ConnectionState::State::direct) {
			color_current = color_online_direct;
		} else if (c_state == Contact::Components::ConnectionState::State::cloud) {
			color_current = color_online_cloud;
		}
	}

	// avatar
#if 1
	const auto [id, width, height] = _contact_tc.get(c);
	ImGui::Image(
		id,
		ImVec2{img_y, img_y},
		{0, 0},
		{1, 1},
		{1, 1, 1, 1},
		color_current
	);
#else
	ImGui::Dummy({img_y, img_y});
#endif

	ImGui::SameLine();
	ImGui::BeginGroup();
	{
		ImGui::Text("%s", (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name.c_str() : "<unk>"));
		if (request_incoming) {
			ImGui::TextUnformatted("Incoming request/invite");
		} else if (request_outgoing) {
			ImGui::TextUnformatted("Outgoing request/invite");
		}
		//ImGui::Text("status message...");
		//ImGui::TextDisabled("hi");
		//ImGui::RenderTextEllipsis
	}
	ImGui::EndGroup();

	ImGui::SetCursorPos(post_curser_pos);
	return selected;
}

bool ChatGui4::renderContactListContactSmall(const Contact3 c) {
	std::string label;

	label += (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name.c_str() : "<unk>");
	label += "###";
	label += std::to_string(entt::to_integral(c));

	return ImGui::Selectable(label.c_str(), _selected_contact.has_value() && *_selected_contact == c);
}

bool ChatGui4::renderSubContactListContact(const Contact3 c) {
	std::string label;

	if (_cr.all_of<Contact::Components::ConnectionState>(c)) {
		const auto c_state = _cr.get<Contact::Components::ConnectionState>(c).state;
		if (c_state == Contact::Components::ConnectionState::State::direct) {
			label += "[D] ";
		} else if (c_state == Contact::Components::ConnectionState::State::cloud) {
			label += "[C] ";
		} else {
			label += "[-] ";
		}
	} else {
		label += "[?] ";
	}

	label += (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name.c_str() : "<unk>");
	label += "###";
	label += std::to_string(entt::to_integral(c));

	return ImGui::Selectable(label.c_str(), _selected_contact.has_value() && *_selected_contact == c);
}

