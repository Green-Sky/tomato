#include "./chat_gui4.hpp"

#include "./file_selector.hpp"

#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/components.hpp>
#include <solanaceae/contact/components.hpp>
#include <solanaceae/util/utils.hpp>

// HACK: remove them
#include <solanaceae/tox_contacts/components.hpp>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <SDL3/SDL.h>

#include "./media_meta_info_loader.hpp"
#include "./sdl_clipboard_utils.hpp"

#include <cctype>
#include <ctime>
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <variant>

namespace Components {

	struct UnreadFade {
		// fades form 1 to 0
		float fade {1.f};
	};

	struct ConvertedTimeCache {
		// calling localtime is expensive af
		int tm_year {0};
		int tm_yday {0};
		int tm_mon {0};
		int tm_mday {0};
		int tm_hour {0};
		int tm_min {0};
	};

} // Components

namespace Context {

	// TODO: move back to chat log window and keep per window instead of per contact
	struct CGView {
		// set to the ts of the newest rendered msg
		Message3Handle begin{};
		// set to the ts of the oldest rendered msg
		Message3Handle end{};
	};

} // Context

static constexpr float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

static std::string file_path_url_escape(const std::string&& value) {
	std::ostringstream escaped;

	escaped << std::hex;
	escaped.fill('0');

	for (const char c : value) {
		if (
			c == '-' || c == '_' || c == '.' || c == '~' || // normal allowed url chars
			std::isalnum(static_cast<unsigned char>(c)) || // more normal
			c == '/' // special bc its a file://
		) {
			escaped << c;
		} else {
			escaped
				<< std::uppercase
				<< '%' <<
				std::setw(2) << static_cast<int>((static_cast<unsigned char>(c)))
				<< std::nouppercase
			;
		}
	}

	return escaped.str();
}

const void* clipboard_callback(void* userdata, const char* mime_type, size_t* size) {
	if (mime_type == nullptr) {
		// cleared or new data is set
		return nullptr;
	}

	if (userdata == nullptr) {
		// error
		return nullptr;
	}

	auto* cg = static_cast<ChatGui4*>(userdata);
	std::lock_guard lg{cg->_set_clipboard_data_mutex};
	if (!cg->_set_clipboard_data.count(mime_type)) {
		return nullptr;
	}

	const auto& sh_vec = cg->_set_clipboard_data.at(mime_type);
	if (!static_cast<bool>(sh_vec)) {
		// error, empty shared pointer
		return nullptr;
	}

	*size = sh_vec->size();

	return sh_vec->data();
}

void ChatGui4::setClipboardData(std::vector<std::string> mime_types, std::shared_ptr<std::vector<uint8_t>>&& data) {
	if (!static_cast<bool>(data)) {
		std::cerr << "CG error: tried to set clipboard with empty shp\n";
		return;
	}

	if (data->empty()) {
		std::cerr << "CG error: tried to set clipboard with empty data\n";
		return;
	}

	std::vector<const char*> tmp_mimetype_list;

	std::lock_guard lg{_set_clipboard_data_mutex};
	for (const auto& mime_type : mime_types) {
		tmp_mimetype_list.push_back(mime_type.data());
		_set_clipboard_data[mime_type] = data;
	}

	SDL_SetClipboardData(clipboard_callback, nullptr, this, tmp_mimetype_list.data(), tmp_mimetype_list.size());
}

ChatGui4::ChatGui4(
	ConfigModelI& conf,
	RegistryMessageModel& rmm,
	Contact3Registry& cr,
	TextureUploaderI& tu,
	ContactTextureCache& contact_tc,
	MessageTextureCache& msg_tc
) : _conf(conf), _rmm(rmm), _cr(cr), _contact_tc(contact_tc), _msg_tc(msg_tc), _sip(tu) {
}

ChatGui4::~ChatGui4(void) {
	// TODO: this is bs
	SDL_ClearClipboardData();

	// this might be better, need to see if this works (docs needs improving)
	//for (const auto& [k, _] : _set_clipboard_data) {
		//const auto* tmp_mime_type = k.c_str();
		//SDL_SetClipboardData(nullptr, nullptr, nullptr, &tmp_mime_type, 1);
	//}
}

float ChatGui4::render(float time_delta) {
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

			const std::vector<Contact3>* sub_contacts = nullptr;
			if (_cr.all_of<Contact::Components::ParentOf>(*_selected_contact)) {
				sub_contacts = &_cr.get<Contact::Components::ParentOf>(*_selected_contact).subs;
			}

			const bool highlight_private {!_cr.all_of<Contact::Components::TagPrivate>(*_selected_contact)};

			if (ImGui::BeginChild(chat_label.c_str(), {0, 0}, true)) {
				if (sub_contacts != nullptr && !_cr.all_of<Contact::Components::TagPrivate>(*_selected_contact) && _cr.all_of<Contact::Components::TagGroup>(*_selected_contact)) {
					if (!sub_contacts->empty()) {
						if (ImGui::BeginChild("subcontacts", {150, -100}, true)) {
							ImGui::Text("subs: %zu", sub_contacts->size());
							ImGui::Separator();
							for (const auto& c : *sub_contacts) {
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

							// TODO: cheese it and rename to copy id?
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

					constexpr ImGuiTableFlags table_flags =
						ImGuiTableFlags_BordersInnerV |
						ImGuiTableFlags_RowBg |
						ImGuiTableFlags_SizingFixedFit
					;
					if (msg_reg_ptr != nullptr && ImGui::BeginTable("chat_table", 5, table_flags)) {
						ImGui::TableSetupColumn("name", 0, TEXT_BASE_WIDTH * 15.f);
						ImGui::TableSetupColumn("message", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("delivered/read");
						ImGui::TableSetupColumn("timestamp");
						ImGui::TableSetupColumn("extra_info", _show_chat_extra_info ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_Disabled);

						Message3Handle message_view_oldest; // oldest visible message
						Message3Handle message_view_newest; // last visible message

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
							msg_reg.view<Components::UnreadFade>().each([&to_remove, time_delta](const Message3 e, Components::UnreadFade& fade) {
								// TODO: configurable
								const float fade_duration = 7.5f;
								fade.fade -= 1.f/fade_duration * std::min<float>(time_delta, 1.f/10.f); // fps but not below 10 for smooth fade
								if (fade.fade <= 0.f) {
									to_remove.push_back(e);
								}
							});
							msg_reg.remove<Components::UnreadFade>(to_remove.cbegin(), to_remove.cend());
						}

						//auto tmp_view = msg_reg.view<Message::Components::ContactFrom, Message::Components::ContactTo, Message::Components::Timestamp>();
						//tmp_view.use<Message::Components::Timestamp>();
						//tmp_view.each([&](const Message3 e, Message::Components::ContactFrom& c_from, Message::Components::ContactTo& c_to, Message::Components::Timestamp ts
						//) {
						//uint64_t prev_ts {0};
						Components::ConvertedTimeCache prev_time {};
						auto tmp_view = msg_reg.view<Message::Components::Timestamp>();
						for (auto view_it = tmp_view.rbegin(), view_last = tmp_view.rend(); view_it != view_last; view_it++) {
							const Message3 e = *view_it;

							// manually filter ("reverse" iteration <.<)
							if (!msg_reg.all_of<Message::Components::ContactFrom, Message::Components::ContactTo>(e)) {
								continue;
							}

							Message::Components::ContactFrom& c_from = msg_reg.get<Message::Components::ContactFrom>(e);
							Message::Components::ContactTo& c_to = msg_reg.get<Message::Components::ContactTo>(e);
							Message::Components::Timestamp ts = tmp_view.get<Message::Components::Timestamp>(e);


							// TODO: why?
							ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

							if (msg_reg.all_of<Components::ConvertedTimeCache>(e)) { // check if date changed
								// TODO: move conversion up?
								const auto& next_time = msg_reg.get<Components::ConvertedTimeCache>(e);
								if (
									prev_time.tm_yday != next_time.tm_yday ||
									prev_time.tm_year != next_time.tm_year // making sure
								) {
									// name
									if (ImGui::TableNextColumn()) {
										//ImGui::TextDisabled("---");
									}
									// msg
									if (ImGui::TableNextColumn()) {
										ImGui::TextDisabled("DATE CHANGED from %d.%d.%d to %d.%d.%d",
											1900+prev_time.tm_year, 1+prev_time.tm_mon, prev_time.tm_mday,
											1900+next_time.tm_year, 1+next_time.tm_mon, next_time.tm_mday
										);
									}
									ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);
								}

								prev_time = next_time;
							}


							ImGui::PushID(entt::to_integral(e));

							// name
							if (ImGui::TableNextColumn()) {
								if (_cr.all_of<Contact::Components::Name>(c_from.c)) {
									ImGui::TextUnformatted(_cr.get<Contact::Components::Name>(c_from.c).name.c_str());
								} else {
									ImGui::TextUnformatted("<unk>");
								}

								// use username as visibility test
								if (ImGui::IsItemVisible()) {
									if (msg_reg.all_of<Message::Components::TagUnread>(e)) {
										// get time now
										const uint64_t ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
										msg_reg.emplace_or_replace<Message::Components::Read>(e, ts_now);
										msg_reg.remove<Message::Components::TagUnread>(e);
										msg_reg.emplace_or_replace<Components::UnreadFade>(e, 1.f);

										// we remove the unread tag here
										_rmm.throwEventUpdate(msg_reg, e);
									}

									// track view
									if (!static_cast<bool>(message_view_oldest)) {
										message_view_oldest = {msg_reg, e};
										message_view_newest = {msg_reg, e};
									} else if (static_cast<bool>(message_view_newest)) {
										// update to latest
										message_view_newest = {msg_reg, e};
									}
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
								if (highlight_private && _cr.any_of<Contact::Components::TagSelfWeak, Contact::Components::TagSelfStrong>(c_to.c)) {
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

							// remote received and read state
							if (ImGui::TableNextColumn()) {
								// TODO: theming for hardcoded values

								if (!msg_reg.all_of<Message::Components::Remote::TimestampReceived>(e)) {
									// TODO: dedup?
									ImGui::TextDisabled("_");
								} else {
									const auto list = msg_reg.get<Message::Components::Remote::TimestampReceived>(e).ts;
									// wrongly assumes contacts never get removed from a group
									if (sub_contacts != nullptr && list.size() < sub_contacts->size()) {
										// if partically delivered
										ImGui::TextColored(ImVec4{0.8f, 0.8f, 0.1f, 0.7f}, "d");
									} else {
										// if fully delivered
										ImGui::TextColored(ImVec4{0.1f, 0.8f, 0.1f, 0.7f}, "D");
									}

									if (ImGui::BeginItemTooltip()) {
										std::string synced_by_text {"delivery confirmed by:"};
										const int64_t now_ts_s = int64_t(Message::getTimeMS() / 1000u);

										size_t other_contacts {0};
										for (const auto& [c, syned_ts] : list) {
											if (_cr.all_of<Contact::Components::TagSelfStrong>(c)) {
												//synced_by_text += "\n sself(!)"; // makes no sense
												continue;
											} else if (_cr.all_of<Contact::Components::TagSelfWeak>(c)) {
												synced_by_text += "\n wself"; // TODO: add name?
											} else {
												synced_by_text += "\n >" + (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>");
											}
											other_contacts += 1;
											const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
											synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
										}

										if (other_contacts > 0) {
											ImGui::Text("%s", synced_by_text.c_str());
										} else {
											ImGui::TextUnformatted("no delivery confirmation");
										}

										ImGui::EndTooltip();
									}
								}

								ImGui::SameLine();

								// TODO: dedup
								if (msg_reg.all_of<Message::Components::Remote::TimestampRead>(e)) {
									const auto list = msg_reg.get<Message::Components::Remote::TimestampRead>(e).ts;
									// wrongly assumes contacts never get removed from a group
									if (sub_contacts != nullptr && list.size() < sub_contacts->size()) {
										// if partially read
										ImGui::TextColored(ImVec4{0.8f, 0.8f, 0.1f, 0.7f}, "r");
									} else {
										// if fully read
										ImGui::TextColored(ImVec4{0.1f, 0.8f, 0.1f, 0.7f}, "R");
									}

									if (ImGui::BeginItemTooltip()) {
										std::string synced_by_text {"read confirmed by:"};
										const int64_t now_ts_s = int64_t(Message::getTimeMS() / 1000u);

										for (const auto& [c, syned_ts] : list) {
											if (_cr.all_of<Contact::Components::TagSelfStrong>(c)) {
												//synced_by_text += "\n sself(!)"; // makes no sense
												continue;
											} else if (_cr.all_of<Contact::Components::TagSelfWeak>(c)) {
												synced_by_text += "\n wself";
											} else {
												synced_by_text += "\n >" + (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>");
											}
											const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
											synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
										}

										ImGui::Text("%s", synced_by_text.c_str());

										ImGui::EndTooltip();
									}
								} else {
									ImGui::TextDisabled("_");
								}
							}

							// ts
							if (ImGui::TableNextColumn()) {
								if (!msg_reg.all_of<Components::ConvertedTimeCache>(e)) {
									auto time = std::chrono::system_clock::to_time_t(
										std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>{std::chrono::milliseconds{ts.ts}}
									);
									auto localtime = std::localtime(&time);
									msg_reg.emplace<Components::ConvertedTimeCache>(
										e,
										localtime->tm_year,
										localtime->tm_yday,
										localtime->tm_mon,
										localtime->tm_mday,
										localtime->tm_hour,
										localtime->tm_min
									);
								}
								const auto& ctc = msg_reg.get<Components::ConvertedTimeCache>(e);

								ImGui::Text("%.2d:%.2d", ctc.tm_hour, ctc.tm_min);
							}

							// extra
							if (ImGui::TableNextColumn()) {
								renderMessageExtra(msg_reg, e);
							}

							ImGui::PopID(); // ent
						}

						// fake empty placeholders
						//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);
						//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

						{ // update view cursers
							if (!msg_reg.ctx().contains<Context::CGView>()) {
								msg_reg.ctx().emplace<Context::CGView>();
							}

							auto& cg_view = msg_reg.ctx().get<Context::CGView>();

							// any message in view
							if (!static_cast<bool>(message_view_oldest)) {
								// no message in view, we setup a view at current time, so the next frags are loaded
								if (!static_cast<bool>(cg_view.begin) || !static_cast<bool>(cg_view.end)) {
									// fix invalid state
									if (static_cast<bool>(cg_view.begin)) {
										cg_view.begin.destroy();
										_rmm.throwEventDestroy(cg_view.begin);
									}
									if (static_cast<bool>(cg_view.end)) {
										cg_view.end.destroy();
										_rmm.throwEventDestroy(cg_view.end);
									}

									// create new
									cg_view.begin = {msg_reg, msg_reg.create()};
									cg_view.end = {msg_reg, msg_reg.create()};

									cg_view.begin.emplace_or_replace<Message::Components::ViewCurserBegin>(cg_view.end);
									cg_view.end.emplace_or_replace<Message::Components::ViewCurserEnd>(cg_view.begin);

									cg_view.begin.get_or_emplace<Message::Components::Timestamp>().ts = Message::getTimeMS();
									cg_view.end.get_or_emplace<Message::Components::Timestamp>().ts = Message::getTimeMS();

									std::cout << "CG: created view FRONT begin ts\n";
									_rmm.throwEventConstruct(cg_view.begin);
									std::cout << "CG: created view FRONT end ts\n";
									_rmm.throwEventConstruct(cg_view.end);
								} // else? we do nothing?
							} else {
								bool begin_created {false};
								if (!static_cast<bool>(cg_view.begin)) {
									cg_view.begin = {msg_reg, msg_reg.create()};
									begin_created = true;
								}
								bool end_created {false};
								if (!static_cast<bool>(cg_view.end)) {
									cg_view.end = {msg_reg, msg_reg.create()};
									end_created = true;
								}
								cg_view.begin.emplace_or_replace<Message::Components::ViewCurserBegin>(cg_view.end);
								cg_view.end.emplace_or_replace<Message::Components::ViewCurserEnd>(cg_view.begin);

								{
									auto& old_begin_ts = cg_view.begin.get_or_emplace<Message::Components::Timestamp>().ts;
									if (old_begin_ts != message_view_newest.get<Message::Components::Timestamp>().ts) {
										old_begin_ts = message_view_newest.get<Message::Components::Timestamp>().ts;
										if (begin_created) {
											std::cout << "CG: created view begin ts with " << old_begin_ts << "\n";
											_rmm.throwEventConstruct(cg_view.begin);
										} else {
											//std::cout << "CG: updated view begin ts to " << old_begin_ts << "\n";
											_rmm.throwEventUpdate(cg_view.begin);
										}
									}
								}

								{
									auto& old_end_ts = cg_view.end.get_or_emplace<Message::Components::Timestamp>().ts;
									if (old_end_ts != message_view_oldest.get<Message::Components::Timestamp>().ts) {
										old_end_ts = message_view_oldest.get<Message::Components::Timestamp>().ts;
										if (end_created) {
											std::cout << "CG: created view end ts with " << old_end_ts << "\n";
											_rmm.throwEventConstruct(cg_view.end);
										} else {
											//std::cout << "CG: updated view end ts to " << old_end_ts << "\n";
											_rmm.throwEventUpdate(cg_view.end);
										}
									}
								}
							}
						}

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
								_rmm.sendFilePath(*_selected_contact, path.filename().generic_u8string(), path.generic_u8string());
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

	return 1000.f; // TODO: higher min fps?
}

void ChatGui4::sendFilePath(const char* file_path) {
	if (_selected_contact && std::filesystem::is_regular_file(file_path)) {
		_rmm.sendFilePath(*_selected_contact, std::filesystem::path(file_path).filename().generic_u8string(), file_path);
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
					//_rmm.sendFilePath(*_selected_contact, path.filename().generic_u8string(), path.generic_u8string());
					const auto& fil = reg.get<Message::Components::Transfer::FileInfoLocal>(e);
					for (const auto& path : fil.file_list) {
						_rmm.sendFilePath(c, std::filesystem::path{path}.filename().generic_u8string(), path);
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
				[](std::filesystem::path& path) -> bool {
					// remove file path
					path.remove_filename();
					return std::filesystem::is_directory(path);
				},
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
					const std::string url{"file://" + file_path_url_escape(std::filesystem::canonical(local_info.file_list.at(i)).generic_u8string())};
					std::cout << "opening file '" << url << "'\n";
					SDL_OpenURL(url.c_str());
				}
				if (ImGui::MenuItem("copy file")) {
					const std::string url{"file://" + file_path_url_escape(std::filesystem::canonical(local_info.file_list.at(i)).generic_u8string())};
					//ImGui::SetClipboardText(url.c_str());
					setClipboardData({"text/uri-list", "text/x-moz-url"}, std::make_shared<std::vector<uint8_t>>(url.begin(), url.end()));
				}
				if (ImGui::MenuItem("copy filepath")) {
					const auto file_path = std::filesystem::canonical(local_info.file_list.at(i)).u8string(); //TODO: use generic over native?
					ImGui::SetClipboardText(file_path.c_str());
				}
				ImGui::EndPopup();
			}
		}

		ImGui::PopID();
	}

	if (reg.all_of<Message::Components::FrameDims>(e)) {
		const auto& frame_dims = reg.get<Message::Components::FrameDims>(e);

		// TODO: config
		const auto max_inline_height = 10*TEXT_BASE_HEIGHT;

		float height = frame_dims.height;
		float width = frame_dims.width;

		if (height > max_inline_height) {
			const float scale = max_inline_height / height;
			height = max_inline_height;
			width *= scale;
		}

		ImVec2 orig_curser_pos = ImGui::GetCursorPos();
		ImGui::Dummy(ImVec2{width, height});

		// deploy dummy of framedim size and check visibility
		bool image_preview_visible = ImGui::IsItemVisible();


		if (image_preview_visible && file_list.size() == 1 && reg.all_of<Message::Components::Transfer::FileInfoLocal>(e)) {
			ImGui::SetCursorPos(orig_curser_pos); // reset for actual img

			auto [id, img_width, img_height] = _msg_tc.get(Message3Handle{reg, e});

			// if cache gives 0s, fall back to frame dims (eg if pic not loaded yet)
			//if (img_width == 0 || img_height == 0) {
				//width = frame_dims.width;
				//height = frame_dims.height;
			//}

			//if (height > max_inline_height) {
				//const float scale = max_inline_height / height;
				//height = max_inline_height;
				//width *= scale;
			//}

			ImGui::Image(id, ImVec2{width, height});

			// TODO: clickable to open in internal image viewer
		}
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
		const int64_t now_ts_s = int64_t(Message::getTimeMS() / 1000u);

		for (const auto& [c, syned_ts] : reg.get<Message::Components::SyncedBy>(e).ts) {
			if (_cr.all_of<Contact::Components::TagSelfStrong>(c)) {
				synced_by_text += "\n sself";
			} else if (_cr.all_of<Contact::Components::TagSelfWeak>(c)) {
				synced_by_text += "\n wself";
			} else {
				synced_by_text += "\n >" + (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>");
			}
			const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
			synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
		}

		ImGui::TextDisabled("%s", synced_by_text.c_str());
	}

	// TODO: remove?
	if (reg.all_of<Message::Components::Remote::TimestampReceived>(e)) {
		std::string synced_by_text {"receivedBy:"};
		const int64_t now_ts_s = int64_t(Message::getTimeMS() / 1000u);

		for (const auto& [c, syned_ts] : reg.get<Message::Components::Remote::TimestampReceived>(e).ts) {
			if (_cr.all_of<Contact::Components::TagSelfStrong>(c)) {
				synced_by_text += "\n sself"; // required (except when synced externally)
			} else if (_cr.all_of<Contact::Components::TagSelfWeak>(c)) {
				synced_by_text += "\n wself";
			} else {
				synced_by_text += "\n >" + (_cr.all_of<Contact::Components::Name>(c) ? _cr.get<Contact::Components::Name>(c).name : "<unk>");
			}
			const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
			synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
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

			_rmm.sendFilePath(*_selected_contact, tmp_file_name.str(), tmp_file_path.generic_u8string());
		},
		[](){}
	);
	SDL_free(data); // free data
}

