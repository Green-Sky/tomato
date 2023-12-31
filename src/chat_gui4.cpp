#include "./chat_gui4.hpp"

#include "./file_selector.hpp"

#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/components.hpp>
#include <solanaceae/contact/components.hpp>

// HACK: remove them
#include <solanaceae/tox_contacts/components.hpp>
#include <solanaceae/toxcore/utils.hpp>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <SDL3/SDL.h>

#include "./media_meta_info_loader.hpp"
#include "./sdl_clipboard_utils.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Components {

	struct UnreadFade {
		// fades form 1 to 0
		float fade {1.f};
	};

} // Components

static float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

ChatGui4::ChatGui4(
	ConfigModelI& conf,
	RegistryMessageModel& rmm,
	Contact3Registry& cr,
	TextureUploaderI& tu
) : _conf(conf), _rmm(rmm), _cr(cr), _tal(_cr), _contact_tc(_tal, tu), _msg_tc(_mil, tu), _sip(tu) {
}

void ChatGui4::render(float time_delta) {
	if (!_cr.storage<Contact::Components::TagAvatarInvalidate>().empty()) { // handle force-reloads for avatars
		std::vector<Contact3> to_purge;
		_cr.view<Contact::Components::TagAvatarInvalidate>().each([&to_purge](const Contact3 c) {
			to_purge.push_back(c);
		});
		_cr.remove<Contact::Components::TagAvatarInvalidate>(to_purge.cbegin(), to_purge.cend());
		_contact_tc.invalidate(to_purge);
	}
	// ACTUALLY NOT IF RENDERED, MOVED LOGIC TO ABOVE
	// it might unload textures, so it needs to be done before rendering
	_contact_tc.update();
	_msg_tc.update();

	_fss.render();
	_sip.render(time_delta);

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
				if (_cr.all_of<Contact::Components::ParentOf>(*_selected_contact)) {
					const auto sub_contacts = _cr.get<Contact::Components::ParentOf>(*_selected_contact).subs;
					if (!sub_contacts.empty()) {
						if (ImGui::BeginChild("subcontacts", {150, -100}, true)) {
							ImGui::Text("subs: %zu", sub_contacts.size());
							ImGui::Separator();
							for (const auto& c : sub_contacts) {
								// TODO: can a sub be selected? no
								if (renderSubContactListContact(c, _selected_contact.has_value() && *_selected_contact == c)) {
									_text_input_buffer.insert(0, (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>") + ": ");
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
						if (ImGui::BeginMenu("debug")) {
							ImGui::Checkbox("show extra info", &_show_chat_extra_info);
							ImGui::Checkbox("show avatar transfers", &_show_chat_avatar_tf);

							ImGui::SeparatorText("tox");

							if (_cr.all_of<Contact::Components::ToxGroupPersistent>(*_selected_contact)) {
								if (ImGui::MenuItem("copy ngc chatid")) {
									const auto& chat_id = _cr.get<Contact::Components::ToxGroupPersistent>(*_selected_contact).chat_id.data;
									const auto chat_id_str = bin2hex(std::vector<uint8_t>{chat_id.begin(), chat_id.end()});
									ImGui::SetClipboardText(chat_id_str.c_str());
								}
							}

							ImGui::EndMenu();
						}
						ImGui::EndMenuBar();
					}

					auto* msg_reg_ptr = _rmm.get(*_selected_contact);

					if (msg_reg_ptr != nullptr) {
						const auto& mm = *msg_reg_ptr;
						//const auto& unread_storage = mm.storage<Message::Components::TagUnread>();
						if (const auto* unread_storage = mm.storage<Message::Components::TagUnread>(); unread_storage != nullptr && !unread_storage->empty()) {
							//assert(unread_storage->size() == 0);
							//assert(unread_storage.cbegin() == unread_storage.cend());

#if 0
							std::cout << "UNREAD ";
							Message3 prev_ent = entt::null;
							for (const Message3 e : mm.view<Message::Components::TagUnread>()) {
								std::cout << entt::to_integral(e) << " ";
								if (prev_ent == e) {
									assert(false && "dup");
								}
								prev_ent = e;
							}
							std::cout << "\n";
#endif
						}
					}

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

						// do systems TODO: extract
						{ // fade system
							std::vector<Message3> to_remove;
							msg_reg.view<Components::UnreadFade>().each([&to_remove](const Message3 e, Components::UnreadFade& fade) {
								// TODO: configurable
								const float fade_duration = 7.5f;
								// TODO: dynamic fps
								fade.fade -= 1.f/fade_duration * (1.f/60.f);
								if (fade.fade <= 0.f) {
									to_remove.push_back(e);
								}
							});
							msg_reg.remove<Components::UnreadFade>(to_remove.cbegin(), to_remove.cend());
						}

						auto tmp_view = msg_reg.view<Message::Components::ContactFrom, Message::Components::ContactTo, Message::Components::Timestamp>();
						tmp_view.use<Message::Components::Timestamp>();
						tmp_view.each([&](const Message3 e, Message::Components::ContactFrom& c_from, Message::Components::ContactTo& c_to, Message::Components::Timestamp ts
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

								// use username as visibility test
								if (ImGui::IsItemVisible() && msg_reg.all_of<Message::Components::TagUnread>(e)) {
									// get time now
									const uint64_t ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
									msg_reg.emplace_or_replace<Message::Components::Read>(e, ts_now);
									msg_reg.remove<Message::Components::TagUnread>(e);
									msg_reg.emplace_or_replace<Components::UnreadFade>(e, 1.f);
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

								std::optional<ImVec4> row_bg;

								// private group message
								if (_cr.any_of<Contact::Components::TagSelfWeak, Contact::Components::TagSelfStrong>(c_to.c)) {
									const ImVec4 priv_msg_hi_col = ImVec4(0.5f, 0.2f, 0.5f, 0.35f);
									ImU32 row_bg_color = ImGui::GetColorU32(priv_msg_hi_col);
									ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, row_bg_color);
									row_bg = priv_msg_hi_col;
								}

								// fade
								if (msg_reg.all_of<Components::UnreadFade>(e)) {
									ImVec4 hi_color = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogramHovered);
									hi_color.w = 0.8f;
									const ImVec4 orig_color = row_bg.value_or(ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg)); // imgui defaults to 0,0,0,0
									const float fade_frac = msg_reg.get<Components::UnreadFade>(e).fade;

									ImVec4 res_color{
										lerp(orig_color.x, hi_color.x, fade_frac),
										lerp(orig_color.y, hi_color.y, fade_frac),
										lerp(orig_color.z, hi_color.z, fade_frac),
										lerp(orig_color.w, hi_color.w, fade_frac),
									};

									ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(res_color));
								}
							}

							// content (msgtext/file)
							ImGui::TableNextColumn();
							if (msg_reg.all_of<Message::Components::MessageText>(e)) {
								renderMessageBodyText(msg_reg, e);
							} else if (msg_reg.any_of<Message::Components::Transfer::FileInfo>(e)) { // add more comps?
								renderMessageBodyFile(msg_reg, e);
							} else {
								ImGui::TextDisabled("---");
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

					if (ImGui::InputTextMultiline("##text_input", &_text_input_buffer, {-0.001f, -0.001f}, input_flags)) {
						//_mm.sendMessage(*_selected_contact, MessageType::TEXT, text_buffer);
						_rmm.sendText(*_selected_contact, _text_input_buffer);
						_text_input_buffer.clear();
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

					{
						// TODO: add support for more than images
						// !!! polling each frame can be VERY expensive !!!
						//const auto* mime_type = clipboardHasImage();
						//ImGui::BeginDisabled(mime_type == nullptr);
						if (ImGui::Button("paste\nfile", {-FLT_MIN, 0})) {
							const auto* mime_type = clipboardHasImage();
							if (mime_type != nullptr) { // making sure
								pasteFile(mime_type);
							}
						//} else if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						} else if (ImGui::BeginPopupContextItem(nullptr, ImGuiMouseButton_Right)) {
							const static std::vector<const char*> image_mime_types {
								"image/png",
								"image/webp",
								"image/gif",
								"image/jpeg",
								"image/bmp",
							};

							for (const char* mime_type : image_mime_types) {
								if (ImGui::MenuItem(mime_type)) {
									pasteFile(mime_type);
								}
							}
							ImGui::EndPopup();
						}
						//ImGui::EndDisabled();
					}
				}
				ImGui::EndChild();

#if 0
				// if preview window not open?
				if (ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::IsKeyPressed(ImGuiMod_Shortcut, false)) {
					std::cout << "CG: paste?\n";
					if (const auto* mime_type = clipboardHasImage(); mime_type != nullptr) {
						size_t data_size = 0;
						const auto* data = SDL_GetClipboardData(mime_type, &data_size);
						// open file send preview.rawpixels
						std::cout << "CG: pasted image of size " << data_size << " mime " << mime_type << "\n";
					}
				}
#endif
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();

	_contact_tc.workLoadQueue();
	_msg_tc.workLoadQueue();
}

void ChatGui4::sendFilePath(const char* file_path) {
	if (_selected_contact && std::filesystem::is_regular_file(file_path)) {
		_rmm.sendFilePath(*_selected_contact, std::filesystem::path(file_path).filename().u8string(), file_path);
	}
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
		"##text",
		const_cast<char*>(msgtext.c_str()), // ugly const cast
		msgtext.size() + 1, // needs to include '\0'
		text_size,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll
	);
	if (ImGui::BeginPopupContextItem("##text")) {
		if (ImGui::MenuItem("quote")) {
			//text_buffer.insert(0, (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>") + ": ");
			if (!_text_input_buffer.empty()) {
				_text_input_buffer += "\n";
			}

			_text_input_buffer += "> ";

			for (const char c : msgtext) {
				_text_input_buffer += c;

				if (c == '\n') {
					_text_input_buffer += "> ";
				}
			}
		}
		ImGui::EndPopup();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void ChatGui4::renderMessageBodyFile(Message3Registry& reg, const Message3 e) {
	if (
		!_show_chat_avatar_tf
		&& (
			reg.all_of<Message::Components::Transfer::FileKind>(e)
			&& reg.get<Message::Components::Transfer::FileKind>(e).kind == 1
		)
	) {
		// TODO: this looks ugly
		ImGui::TextDisabled("set avatar");
		return;
	}

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
	} else if (reg.all_of<Message::Components::Transfer::TagReceiving, Message::Components::Transfer::TagHaveAll>(e)) {
		ImGui::TextUnformatted("done");
	} else {
		// TODO: missing other states
		ImGui::TextUnformatted("running");
	}

	if (reg.all_of<Message::Components::Transfer::TagHaveAll, Message::Components::Transfer::FileInfoLocal>(e)) {
		// hack lul
		ImGui::SameLine();
		if (ImGui::SmallButton("forward")) {
			ImGui::OpenPopup("forward to contact");
		}

		if (ImGui::BeginPopup("forward to contact")) {
			// TODO: make exclusion work
			//for (const auto& c : _cr.view<entt::get_t<Contact::Components::TagBig>, entt::exclude_t<Contact::Components::RequestIncoming, Contact::Components::TagRequestOutgoing>>()) {
			for (const auto& c : _cr.view<Contact::Components::TagBig>()) {
				if (renderContactListContactSmall(c, false)) {
					//_rmm.sendFilePath(*_selected_contact, path.filename().u8string(), path.u8string());
					const auto& fil = reg.get<Message::Components::Transfer::FileInfoLocal>(e);
					for (const auto& path : fil.file_list) {
						_rmm.sendFilePath(c, std::filesystem::path{path}.filename().u8string(), path);
					}
				}
			}
			ImGui::EndPopup();
		}
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
						// TODO: trim file?
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

			float fraction = float(reg.get<Message::Components::Transfer::BytesReceived>(e).total) / reg.get<Message::Components::Transfer::FileInfo>(e).total_size;

			char overlay_buf[32];
			std::snprintf(overlay_buf, sizeof(overlay_buf), "%.1f%%", fraction * 100 + 0.01f);

			ImGui::ProgressBar(
				fraction,
				{-FLT_MIN, TEXT_BASE_HEIGHT},
				overlay_buf
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

			float fraction = float(reg.get<Message::Components::Transfer::BytesSent>(e).total) / reg.get<Message::Components::Transfer::FileInfo>(e).total_size;

			char overlay_buf[32];
			std::snprintf(overlay_buf, sizeof(overlay_buf), "%.1f%%", fraction * 100 + 0.01f);

			ImGui::ProgressBar(
				fraction,
				{-FLT_MIN, TEXT_BASE_HEIGHT},
				overlay_buf
			);
			// TODO: numbers
		}
	}

	const auto file_list = reg.get<Message::Components::Transfer::FileInfo>(e).file_list;

	// if has local, display save base path?

	for (size_t i = 0; i < file_list.size(); i++) {
		ImGui::PushID(i);

		// TODO: selectable text widget ?
		ImGui::Bullet(); ImGui::Text("%s (%lu)", file_list[i].file_name.c_str(), file_list[i].file_size);

		if (reg.all_of<Message::Components::Transfer::FileInfoLocal>(e)) {
			const auto& local_info = reg.get<Message::Components::Transfer::FileInfoLocal>(e);
			if (local_info.file_list.size() > i && ImGui::BeginPopupContextItem("##file_c")) {
				if (ImGui::MenuItem("open")) {
					std::string url{"file://" + std::filesystem::canonical(local_info.file_list.at(i)).u8string()};
					std::cout << "opening file '" << url << "'\n";

					SDL_OpenURL(url.c_str());
				}
				ImGui::EndPopup();
			}
		}

		ImGui::PopID();
	}

	if (file_list.size() == 1 && reg.all_of<Message::Components::Transfer::FileInfoLocal, Message::Components::FrameDims>(e)) {
		auto [id, width, height] = _msg_tc.get(Message3Handle{reg, e});

		// if cache gives 0s, fall back to frame dims (eg if pic not loaded yet)
		if (width == 0 || height == 0) {
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
		// TODO: clickable to open in internal image viewer
	}
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
			if (renderContactListContactBig(c, _selected_contact.has_value() && *_selected_contact == c)) {
				_selected_contact = c;
			}
		}
	}
	ImGui::EndChild();
}

bool ChatGui4::renderContactListContactBig(const Contact3 c, const bool selected) {
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
	const bool show_selected = request_incoming || request_outgoing || selected;
	if (request_incoming) {
		// TODO: theming
		ImGui::PushStyleColor(ImGuiCol_Header, {0.98f, 0.41f, 0.26f, 0.52f});
	} else if (request_outgoing) {
		// TODO: theming
		ImGui::PushStyleColor(ImGuiCol_Header, {0.98f, 0.26f, 0.41f, 0.52f});
	}

	const bool got_selected = ImGui::Selectable(label.c_str(), show_selected, 0, {0,3*TEXT_BASE_HEIGHT});

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
	const auto [id, width, height] = _contact_tc.get(c);
	ImGui::Image(
		id,
		ImVec2{img_y, img_y},
		{0, 0},
		{1, 1},
		{1, 1, 1, 1},
		color_current
	);

	// TODO: move this out of chat gui
	any_unread = false;

	ImGui::SameLine();
	ImGui::BeginGroup();
	{
		// TODO: is there a better way?
		// maybe cache mm?
		bool has_unread = false;
		if (const auto* mm = _rmm.get(c); mm != nullptr) {
			if (const auto* unread_storage = mm->storage<Message::Components::TagUnread>(); unread_storage != nullptr && !unread_storage->empty()) {
#if 0
				assert(unread_storage.size() == 0);
				assert(unread_storage.cbegin() == unread_storage.cend());
				std::cout << "UNREAD ";
				for (const auto e : mm->view<Message::Components::TagUnread>()) {
					std::cout << entt::to_integral(e) << " ";
				}
				std::cout << "\n";
#endif
				has_unread = true;
				any_unread = true;
			}
		}

		ImGui::Text("%s%s", has_unread?"* ":"", (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name.c_str() : "<unk>"));
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
	return got_selected;
}

bool ChatGui4::renderContactListContactSmall(const Contact3 c, const bool selected) const {
	std::string label;

	label += (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name.c_str() : "<unk>");
	label += "###";
	label += std::to_string(entt::to_integral(c));

	return ImGui::Selectable(label.c_str(), selected);
}

bool ChatGui4::renderSubContactListContact(const Contact3 c, const bool selected) const {
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

	return ImGui::Selectable(label.c_str(), selected);
}

void ChatGui4::pasteFile(const char* mime_type) {
	size_t data_size = 0;
	void* data = SDL_GetClipboardData(mime_type, &data_size);

	// if image

	std::cout << "CG: pasted image of size " << data_size << " mime " << mime_type << "\n";

	_sip.sendMemory(
		static_cast<const uint8_t*>(data), data_size,
		[this](const auto& img_data, const auto file_ext) {
			// create file name
			// TODO: move this into sip
			std::ostringstream tmp_file_name {"tomato_Image_", std::ios_base::ate};
			{
				const auto now = std::chrono::system_clock::now();
				const auto ctime = std::chrono::system_clock::to_time_t(now);
				tmp_file_name
					<< std::put_time(std::localtime(&ctime), "%F_%H-%M-%S")
					<< "."
					<< std::setfill('0') << std::setw(3)
					<< std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())).count()
					<< file_ext
				;
			}

			std::cout << "tmp image path " << tmp_file_name.str() << "\n";

			const std::filesystem::path tmp_send_file_path = "tmp_send_files";
			std::filesystem::create_directories(tmp_send_file_path);
			const auto tmp_file_path = tmp_send_file_path / tmp_file_name.str();

			std::ofstream(tmp_file_path, std::ios_base::out | std::ios_base::binary)
				.write(reinterpret_cast<const char*>(img_data.data()), img_data.size());

			_rmm.sendFilePath(*_selected_contact, tmp_file_name.str(), tmp_file_path.u8string());
		},
		[](){}
	);
	SDL_free(data); // free data
}

