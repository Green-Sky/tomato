#include "./contact_window.hpp"

#include <solanaceae/contact/contact_store_impl.hpp>
#include <solanaceae/contact/contact_model4.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/contact_components.hpp>

#include <solanaceae/message3/components.hpp>

#include "./contact_info.hpp"
#include "./contact_list.hpp"
#include "./file_selector.hpp"

#include "../sdl_clipboard_utils.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h> // needed for tab bar shenanigans

#include <SDL3/SDL.h>

#include <deque>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iostream>

ContactWindow::ContactWindow(
	ContactStore4Impl& cs,
	RegistryMessageModelI& rmm,
	ObjectStore2& os,
	Theme& theme,
	ContactTextureCache& contact_tc,
	MessageTextureCache& msg_tc,
	BitsetTextureCache& b_tc,
	ContactInfoWindows& ciw,
	FileSelector& fss,
	ImageViewerPopup& ivp,
	Clipboard& cb,
	TextureUploaderI& tu,
	std::function<void(ContactHandle4)>&& open_chat,
	ContactHandle4 c_
) : _cs(cs), _rmm(rmm), _os(os),
	_theme(theme), _contact_tc(contact_tc),
	_ciw(ciw),
	_fss(fss),
	_open_chat(std::move(open_chat)),
	_text_input_buffer(),
	c(c_),
	_ccl(cs, rmm, os, theme, contact_tc, msg_tc, b_tc, fss, ivp, cb, _text_input_buffer, c),
	_sip(tu, theme)
{
}

float ContactWindow::render(const bool window_focused, const float time_delta, const bool child, const bool sub_contact_list) {
	ImGui::PushID(entt::to_integral(c.entity()));

	_sip.render(time_delta);

	const std::string chat_label = "chat " + std::to_string(entt::to_integral(c.entity()));

	if (
		(child && ImGui::BeginChild(chat_label.c_str(), {0, 0}, ImGuiChildFlags_Borders, ImGuiWindowFlags_MenuBar)) ||
		(!child && ImGui::Begin(chat_label.c_str(), nullptr, ImGuiWindowFlags_MenuBar))
	) {
		// TODO: optimize
		TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
		TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("ContactInfo")) {
				static bool advanced {false};
				static bool verbose {false};
				ImGui::Checkbox("advanced", &advanced);
				ImGui::SameLine();
				ImGui::Checkbox("verbose", &verbose);
				renderContactInfo(_cs, c, advanced, verbose);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("debug")) {
				ImGui::Checkbox("show extra info", &_ccl._show_chat_extra_info);
				ImGui::Checkbox("show avatar transfers", &_ccl._show_chat_avatar_tf);
				ImGui::EndMenu();
			}

			{
				const auto ctx_list = _cs.getImGuiContext(c);

				if (!ctx_list.empty()) {
					ImGui::Separator();

					for (const auto it : ctx_list) {
						it.fn(c);
					}
				}
			}

			ImGui::EndMenuBar();
		}

		const std::vector<Contact4>* sub_contacts = nullptr;
		if (const auto* po_sub = c.try_get<Contact::Components::ParentOf>(); po_sub) {
			sub_contacts = &po_sub->subs;
		}

		renderContactBig(_theme, _contact_tc, c, 3, false, false, false);
		ImGui::Separator();

		if (!sub_contact_list && renderSubListChild(sub_contacts)) {
			ImGui::SameLine();
		}

		renderRequest();

		if (ImGui::BeginChild("chat_main", {0, -TEXT_BASE_HEIGHT*4.5f}, ImGuiChildFlags_None)) {
			const auto tab_cb_list = _cs.getImGuiChatTab(c);
			const bool show_sub_contacts =
				sub_contact_list &&
				sub_contacts != nullptr && !sub_contacts->empty() &&
				!c.all_of<Contact::Components::TagPrivate>() && c.all_of<Contact::Components::TagGroup>()
			;
			const bool has_other_tabs = !tab_cb_list.empty();

			if (show_sub_contacts || has_other_tabs) {
				if (ImGui::BeginTabBar("chat_tab_bar")) {
					bool has_unread = false;
					{ // has unread // TODO: extract?
						auto* msg_reg_ptr = _rmm.get(c);
						if (msg_reg_ptr != nullptr) {
							const auto* unread_storage = static_cast<const Message3Registry*>(msg_reg_ptr)->storage<Message::Components::TagUnread>();
							if (unread_storage != nullptr) {
								has_unread = !unread_storage->empty();
							}
						}
					}

					if (ImGui::BeginTabItem("MessageLog", nullptr, (has_unread ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None))) {
						if (ImGui::BeginChild("tab_content")) {
							_ccl.render(window_focused, time_delta, sub_contacts);
						}
						ImGui::EndChild();
						ImGui::EndTabItem();
					}

					if (show_sub_contacts && ImGui::BeginTabItem("SubContacts")) {
						renderSubList(sub_contacts);
						ImGui::EndTabItem();
					}

					// TODO: think more about fs here
#if 0
					if (ImGui::BeginTabItem("Filetransfers")) {
						if (ImGui::BeginChild("tab_content")) {
							renderChatFilesTab(c);
						}
						ImGui::EndChild();
						ImGui::EndTabItem();
					}
#endif

					for (const auto& it : tab_cb_list) {
						it.fn(c);
					}

					constexpr auto get_sel_id = [](ImGuiTabBar* tb) -> int {
						// find selected tab by order id
						for (int i = 0; i < tb->Tabs.Size; i++) {
							const auto* tab = ImGui::TabBarFindTabByOrder(tb, i);
							if (tab == nullptr) {
								continue;
							}

							if (tab->ID == tb->SelectedTabId) {
								return i;
							}
						}
						return 0; // TODO: error, like -1 ?
					};

					if (ImGui::Shortcut(ImGuiKey_H, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
						// go left
						if (auto* tb = ImGui::GetCurrentTabBar(); tb != nullptr) {
							const int sel_id = get_sel_id(tb);
							if (sel_id > 0) {
								ImGui::TabBarQueueFocus(tb, ImGui::TabBarFindTabByOrder(tb, sel_id-1));
							}
						}
					} else if (ImGui::Shortcut(ImGuiKey_L, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
						// go right
						if (auto* tb = ImGui::GetCurrentTabBar(); tb != nullptr) {
							const int sel_id = get_sel_id(tb);
							if (sel_id+1 < tb->Tabs.Size) {
								ImGui::TabBarQueueFocus(tb, ImGui::TabBarFindTabByOrder(tb, sel_id+1));
							}
						}
					}

					ImGui::EndTabBar();
				}
			} else {
				_ccl.render(window_focused, time_delta, sub_contacts);
			}
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("text_input", {-150, 0})) {
			ImGui::SetNextItemShortcut(ImGuiKey_I, ImGuiInputFlags_RouteGlobal);

			constexpr ImGuiInputTextFlags input_flags =
				//ImGuiInputTextFlags_AllowTabInput |
				ImGuiInputTextFlags_NoHorizontalScroll |
				ImGuiInputTextFlags_CallbackCharFilter |
				ImGuiInputTextFlags_WordWrap;

			bool text_input_validate {false};
			ImGui::InputTextMultiline(
				"##text_input",
				&_text_input_buffer,
				{-0.001f, -0.001f},
				input_flags,
				+[](ImGuiInputTextCallbackData* data) -> int {
					// ignore unrelated callbacks
					if ((data->EventFlag & ImGuiInputTextFlags_CallbackCharFilter) == 0) {
						return 0;
					}

					// we let everything through, except enter without shift, in which case we signal outside
					if (
						data->EventChar == '\n' &&
						!ImGui::GetIO().KeyShift &&
						ImGui::IsKeyPressed(ImGuiKey_Enter) // also needs to be a key press, not a paste
					) {
						*reinterpret_cast<bool*>(data->UserData) = true;
						return 1;
					}

					return 0;
				},
				&text_input_validate
			);
			if (text_input_validate) {
				_rmm.sendText(c, _text_input_buffer);
				_text_input_buffer = "";
				if (ImGuiInputTextState* input_state = ImGui::GetInputTextState(ImGui::GetItemID())) {
					//input_state->ReloadUserBufAndSelectAll();
					input_state->ReloadUserBufAndMoveToEnd();
				}
				ImGui::SetKeyboardFocusHere(-1);
			}
			if (
				const auto* ml = c.try_get<Contact::Components::MessageLengths>();
				ml != nullptr && _text_input_buffer.size() > ml->max_extended
			) {
				ImGui::PushStyleColor(ImGuiCol_Text, _theme.getColor<ThemeCol_Contact::message_warning_text>());
				ImGui::SetTooltip("!! exceeded max message length of %lu bytes !!", ml->max_extended);
				ImGui::PopStyleColor();
			}

			// welcome to linux
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
				if (!ImGui::IsItemFocused()) {
					ImGui::SetKeyboardFocusHere(-1);
				}
				char* primary_text = SDL_GetPrimarySelectionText();
				if (primary_text != nullptr) {
					ImGui::GetIO().AddInputCharactersUTF8(primary_text);
					SDL_free(primary_text);
				}
			}
		}
		ImGui::EndChild();
		ImGui::SameLine();

		if (ImGui::BeginChild("buttons")) {
			if (ImGui::Button("send\nfile", {-FLT_MIN, 0})) {
				_fss.requestFile(
					[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
					[this](const auto& path){
						_rmm.sendFilePath(c, path.filename().generic_u8string(), path.generic_u8string());
					},
					[](){}
				);
			}

			// TODO: add support for more than images
			// !!! polling each frame can be VERY expensive !!!
			//const auto* mime_type = clipboardHasImage();
			//ImGui::BeginDisabled(mime_type == nullptr);
			if (ImGui::Button("paste\nfile", {-FLT_MIN, 0})) {
				if (const auto* imt = clipboardHasImage(); imt != nullptr) { // making sure
					pasteFile(imt);
				} else if (const auto* fpmt = clipboardHasFileList(); fpmt != nullptr) {
					pasteFile(fpmt);
				}
			} else if (ImGui::BeginPopupContextItem(nullptr, ImGuiMouseButton_Right)) {
				// TODO: use list instead
				const static std::vector<const char*> image_mime_types {
					// add apng?
					"image/png",
					"image/webp",
					"image/gif",
					"image/jpeg",
					"image/bmp",
					"image/qoi",
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
		ImGui::EndChild();

#if 0
		// if preview window not open?
		if (ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::IsKeyPressed(ImGuiMod_Shortcut, false)) {
			std::cout << "CW: paste?\n";
			if (const auto* mime_type = clipboardHasImage(); mime_type != nullptr) {
				size_t data_size = 0;
				const auto* data = SDL_GetClipboardData(mime_type, &data_size);
				// open file send preview.rawpixels
				std::cout << "CW: pasted image of size " << data_size << " mime " << mime_type << "\n";
			}
		}
#endif
	}
	if (child) {
		ImGui::EndChild();
	} else {
		ImGui::End();
	}

	ImGui::PopID();

	return 2000.f;
}

struct ContactWRole {
	Contact4 cv;
	// best role, lowest numerically
	uint64_t role{std::numeric_limits<uint64_t>::max()};
};
static std::deque<ContactWRole> sortSubsByRole(ContactStore4I& cs, const std::vector<Contact4>& sub_contacts) {
	std::deque<ContactWRole> sorted_subs;

	for (const auto& c_sub : sub_contacts) {
		auto c = cs.contactHandle(c_sub);
		// we sort by lowest number role

		uint64_t role{std::numeric_limits<uint64_t>::max()};
		const auto* role_comp_ptr = c.try_get<Contact::Components::Roles>();
		if (role_comp_ptr != nullptr && !role_comp_ptr->rs.empty()) {
			const auto& role_vec = role_comp_ptr->rs;
			role = role_vec.front();
			// TODO: remove this ape dance, enforce sorted roles list?
			for (size_t i = 1; i < role_vec.size(); i++) {
				if (role_vec[i] < role) {
					role = role_vec[i];
				}
			}
		}

		// TODO: use algo find
		// find lower prio contact to insert before
		for (auto it = sorted_subs.begin(); ; it++) {
			if (it == sorted_subs.end()) {
				sorted_subs.emplace_back(ContactWRole{c, role});
				break;
			}

			if (it->role > role) {
				sorted_subs.insert(it, ContactWRole{c, role});
				break;
			}
		}
	}

	return sorted_subs;
}

void ContactWindow::renderSubList(const std::vector<Contact4>* sub_contacts) {
	auto& cr = _cs.registry();

	ImGui::Text("subs: %zu", sub_contacts->size());
	ImGui::Separator();

	// HACK: ugly in place sort by role, needs caching
	const auto sorted_subs = sortSubsByRole(_cs, *sub_contacts);
	const auto* role_map_comp = c.try_get<Contact::Components::RoleMap>();
	uint64_t last_role = std::numeric_limits<uint64_t>::max();

	for (const auto& [sub_cv, role] : sorted_subs) {
		if (last_role != role) {
			if (role == std::numeric_limits<uint64_t>::max()) {
				ImGui::SeparatorText("without Role");
			} else if (role_map_comp != nullptr && role_map_comp->map.count(role)) {
				ImGui::SeparatorText(role_map_comp->map.at(role).c_str());
			} else {
				const std::string role_text = "unk Role " + std::to_string(role);
				ImGui::SeparatorText(role_text.c_str());
			}
			last_role = role;
		}

		ImGui::PushID(entt::to_integral(sub_cv));

		ContactHandle4 sub_c{cr, sub_cv};

		// TODO: can a sub be selected? no
		//if (renderSubContactListContact(c_sub, _selected_contact.has_value() && c == c_sub)) {
		if (renderContactBig(_theme, _contact_tc, sub_c, 1)) {
			_text_input_buffer.insert(0, (sub_c.all_of<Contact::Components::Name>() ? sub_c.get<Contact::Components::Name>().name : "<unk>") + ": ");
		}
		renderSubContactContext(sub_c, sub_cv);

		ImGui::PopID();
	}
}

bool ContactWindow::renderSubListChild(const std::vector<Contact4>* sub_contacts) {
	if (sub_contacts == nullptr || c.all_of<Contact::Components::TagPrivate>() || !c.all_of<Contact::Components::TagGroup>() || sub_contacts->empty()) {
		// no list
		return false;
	}

	if (!ImGui::BeginChild("subcontacts", {TEXT_BASE_WIDTH * 18.f, -TEXT_BASE_HEIGHT*4.5f}, true)) {
		ImGui::EndChild();
		return true; // there is an empty child
	}

	renderSubList(sub_contacts);

	ImGui::EndChild();
	return true;
}

void ContactWindow::renderSubContactContext(ContactHandle4 sub_c, const Contact4 sub_cv) {
	if (ImGui::BeginPopupContextItem("sub_contact_context")) {
		if (!sub_c.all_of<Contact::Components::TagSelfStrong>() && ImGui::MenuItem("open chat")) {
			_open_chat(sub_c);
		}

		if (ImGui::MenuItem("open contact info")) {
			_ciw.open(sub_cv);
		}

		const auto ctx_list = _cs.getImGuiContext(sub_c);

		if (!ctx_list.empty()) {
			ImGui::Separator();

			for (const auto it : ctx_list) {
				it.fn(sub_c);
			}
		}

		ImGui::EndPopup();
	}
}

bool ContactWindow::renderRequest(void) {
	const bool request_incoming = c.all_of<Contact::Components::RequestIncoming>();
	const bool request_outgoing = c.all_of<Contact::Components::TagRequestOutgoing>();
	if (!request_incoming && !request_outgoing) {
		return false;
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, _theme.getColor<ThemeCol_Contact::request_panel_background>());

	if (ImGui::BeginChild("request", {0, TEXT_BASE_HEIGHT*6.1f}, true, ImGuiWindowFlags_NoScrollbar)) {
		if (request_incoming) {
			const auto& ri = c.get<Contact::Components::RequestIncoming>();
			ImGui::TextUnformatted("You got a request to add this contact.");

			// TODO: remove this hack
			// move to member var maybe, init on construction?
			//static std::string self_name = _conf.get_string("tox", "name").value_or("default_tomato");
			static std::string self_name = [&]() -> std::string {
				const auto* self_comp = c.try_get<Contact::Components::Self>();
				if (self_comp == nullptr) {
					// fall back to parent
					if (const auto* p = c.try_get<Contact::Components::Parent>(); p != nullptr) {
						const auto parent_c = _cs.contactHandle(p->parent);
						if (static_cast<bool>(parent_c)) {
							self_comp = parent_c.try_get<Contact::Components::Self>();
						}
					}
				}
				if (self_comp == nullptr) {
					// fall back to root
					if (const auto* r = c.try_get<Contact::Components::Root>(); r != nullptr) {
						const auto root_c = _cs.contactHandle(r->root);
						if (static_cast<bool>(root_c)) {
							self_comp = root_c.try_get<Contact::Components::Self>();
						}
					}
				}
				if (self_comp != nullptr) {
					const auto self_c = _cs.contactHandle(self_comp->self);
					if (static_cast<bool>(self_c)) {
						const auto* self_name_comp = self_c.try_get<Contact::Components::Name>();
						if (self_name_comp != nullptr && !self_name_comp->name.empty()) {
							return self_name_comp->name;
						}
					}
				}
				return "default_tomato";
			}();

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
				c.get<Contact::Components::ContactModel>()->acceptRequest(c, self_name, password);
				password.clear();
			}
			ImGui::SameLine();
			if (ImGui::Button("Decline")) {
			}
		} else {
			ImGui::TextUnformatted("You sent a request to add this contact.");
		}
	}

	ImGui::PopStyleColor();
	ImGui::EndChild();
	return true;
}

void ContactWindow::pasteFile(const char* mime_type) {
	if (mimeIsImage(mime_type)) {
		size_t data_size = 0;
		void* data = SDL_GetClipboardData(mime_type, &data_size);

		std::cout << "CW: pasted image of size: " << data_size << " mimetype: " << mime_type << "\n";

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

				//std::cout << "tmp image path " << tmp_file_name.str() << "\n";

				const std::filesystem::path tmp_send_file_path = "tmp_send_files";
				std::filesystem::create_directories(tmp_send_file_path);
				const auto tmp_file_path = tmp_send_file_path / tmp_file_name.str();

				std::ofstream(tmp_file_path, std::ios_base::out | std::ios_base::binary)
					.write(reinterpret_cast<const char*>(img_data.data()), img_data.size());

				_rmm.sendFilePath(c, tmp_file_name.str(), tmp_file_path.generic_u8string());
			},
			[](){}
		);
		SDL_free(data); // free data
	} else if (mimeIsFileList(mime_type)) {
		size_t data_size = 0;
		void* data = SDL_GetClipboardData(mime_type, &data_size);

		std::cout << "CW: pasted file list of size " << data_size << " mime " << mime_type << "\n";

		std::vector<std::string_view> list;
		if (mime_type == std::string_view{"text/uri-list"}) {
			// lines starting with # are comments
			// every line is a link
			// line sep is CRLF
			std::string_view list_body{reinterpret_cast<const char*>(data), data_size};
			size_t start {0};
			bool comment {false};
			for (size_t i = 0; i < data_size; i++) {
				if (list_body[i] == '\r' || list_body[i] == '\n') {
					if (!comment && i - start > 0) {
						list.push_back(list_body.substr(start, i - start));
					}
					start = i+1;
					comment = false;
				} else if (i == start && list_body[i] == '#') {
					comment = true;
				}
			}
			if (!comment && start+1 < data_size) {
				list.push_back(list_body.substr(start));
			}
		} else if (mime_type == std::string_view{"text/x-moz-url"}) {
			assert(false && "implement me");
			// every link line is followed by link-title line (for the prev link)
			// line sep unclear ("text/*" should always be CRLF, but its not explicitly specified)
			// (does not matter, we can account for both)
		}

		// TODO: remove debug log
		std::cout << "preprocessing:\n";
		for (const auto it : list) {
			std::cout << "  >" << it << "\n";
		}

		// now we need to potentially convert file uris to file paths

		for (auto it = list.begin(); it != list.end();) {
			constexpr auto size_of_file_uri_prefix = std::string_view{"file://"}.size();
			if (it->size() > size_of_file_uri_prefix && it->substr(0, size_of_file_uri_prefix) == std::string_view{"file://"}) {
				it->remove_prefix(size_of_file_uri_prefix);
			}

			std::filesystem::path path(*it);
			if (!std::filesystem::is_regular_file(path)) {
				it = list.erase(it);
			} else {
				it++;
			}
		}

		std::cout << "postprocessing:\n";
		for (const auto it : list) {
			std::cout << "  >" << it << "\n";
		}

		sendFileList(list);

		SDL_free(data); // free data
	}
}

void ContactWindow::sendFilePath(std::string_view file_path) {
	const auto path = std::filesystem::path(file_path);
	if (!std::filesystem::is_regular_file(path)) {
		return;
	}

	_rmm.sendFilePath(c, path.filename().generic_u8string(), path.generic_u8string());
}

void ContactWindow::sendFileList(const std::vector<std::string_view>& list) {
	// TODO: file collection sip
	if (list.size() > 1) {
		for (const auto it : list) {
			sendFilePath(it);
		}
	} else if (list.size() == 1) {
		const auto path = std::filesystem::path(list.front());
		if (std::filesystem::is_regular_file(path)) {
			if (!_sip.sendFilePath(
				list.front(),
				[this](const auto& img_data, const auto file_ext) {
					// create file name
					// TODO: only create file if changed or from memory
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

					_rmm.sendFilePath(c, tmp_file_name.str(), tmp_file_path.generic_u8string());
				},
				[](){}
			)) {
				// if sip fails to open the file
				sendFilePath(list.front());
			}
		} else {
			// if not file (???)
		}
	}
}
