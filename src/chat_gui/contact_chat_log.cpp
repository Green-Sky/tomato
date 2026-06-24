#include "./contact_chat_log.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/contact/components.hpp>

#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/msg_components.hpp> // TODO: remove

#include <solanaceae/object_store/meta_components_file.hpp>
#include <solanaceae/tox_messages/obj_components.hpp> // TODO: remove

#include <solanaceae/util/time.hpp>

#include "./contact_list.hpp"
#include "./file_selector.hpp"
#include "./image_viewer_popup.hpp"

#include "../string_formatter_utils.hpp"
#include "../sdl_clipboard_utils.hpp"

#include <imgui.h>
#include <imgui_internal.h> // TODO: remove, renderframe

#include <SDL3/SDL.h>

#include <filesystem>
#include <chrono>

#include <cassert>
#include <iostream> // TODO: replace with logging

// TODO: split into msg and c
namespace Components {

	struct UnreadFade {
		// fades from 1 to 0
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

		std::string h_m;
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

ContactChatLog::ContactChatLog(
	ContactStore4I& cs,
	RegistryMessageModelI& rmm,
	ObjectStore2& os,
	Theme& theme,
	ContactTextureCache& contact_tc,
	MessageTextureCache& msg_tc,
	BitsetTextureCache& b_tc,
	FileSelector& fss,
	ImageViewerPopup& ivp,
	Clipboard& cb,
	std::string& text_input_buffer,
	ContactHandle4 c_
) : _cs(cs), _rmm(rmm), _os(os), _theme(theme), _contact_tc(contact_tc), _msg_tc(msg_tc), _b_tc(b_tc), _fss(fss), _ivp(ivp), _cb(cb), _text_input_buffer(text_input_buffer), c(c_) {
}

float ContactChatLog::render(bool window_focused, float time_delta, const std::vector<Contact4>* sub_contacts) {
	msg_reg = _rmm.get(c);
	if (msg_reg == nullptr) {
		return 6000.f;
	}

	fadeSystem(window_focused, time_delta);

	// TODO: optimize
	TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
	TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	// TODO: background image?
	//auto p_min = ImGui::GetCursorScreenPos();
	//auto a_max = ImGui::GetContentRegionAvail();
	//ImGui::GetWindowDrawList()->AddImage(0, p_min, {p_min.x+a_max.x, p_min.y+a_max.y});

	const float scroll_amount {2.f * TEXT_BASE_HEIGHT};
	bool manually_scrolled {false};
	// TODO: replace with IsKeyPressed version in the future
	if (ImGui::Shortcut(ImGuiKey_J, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
		ImGui::SetScrollY(ImGui::GetScrollY() + scroll_amount);
		manually_scrolled = true;
	}
	if (ImGui::Shortcut(ImGuiKey_K, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
		ImGui::SetScrollY(ImGui::GetScrollY() - scroll_amount);
		manually_scrolled = true;
	}

	if (ImGui::Shortcut(ImGuiKey_PageDown, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
		// TODO: figure out page size
		// -> 20 lines
		ImGui::SetScrollY(ImGui::GetScrollY() + scroll_amount*10.f);
		manually_scrolled = true;
	}
	if (ImGui::Shortcut(ImGuiKey_PageUp, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
		ImGui::SetScrollY(ImGui::GetScrollY() - scroll_amount*10.f);
		manually_scrolled = true;
	}

	constexpr ImGuiTableFlags table_flags =
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingFixedFit
	;
	if (ImGui::BeginTable("chat_table", 5, table_flags)) {
		ImGui::TableSetupColumn("name", 0, TEXT_BASE_WIDTH * 16.f);
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

		const auto& cr = _cs.registry();

		const bool highlight_private {!c.all_of<Contact::Components::TagPrivate>()};

		//auto tmp_view = msg_reg.view<Message::Components::ContactFrom, Message::Components::ContactTo, Message::Components::Timestamp>();
		//tmp_view.use<Message::Components::Timestamp>();
		//tmp_view.each([&](const Message3 e, Message::Components::ContactFrom& c_from, Message::Components::ContactTo& c_to, Message::Components::Timestamp ts
		//) {
		//uint64_t prev_ts {0};
		Components::ConvertedTimeCache prev_time {};
		auto tmp_view = msg_reg->view<Message::Components::Timestamp>();
		for (auto view_it = tmp_view.rbegin(), view_last = tmp_view.rend(); view_it != view_last; view_it++) {
			const Message3 e = *view_it;

			const Message::Components::ContactFrom* c_from = msg_reg->try_get<Message::Components::ContactFrom>(e);
			const Message::Components::ContactTo* c_to = msg_reg->try_get<Message::Components::ContactTo>(e);

			// manually filter ("reverse" iteration <.<)
			if (c_from == nullptr || c_to == nullptr) {
				continue;
			}

			Message::Components::Timestamp ts = tmp_view.get<Message::Components::Timestamp>(e);

			// TODO: why?
			ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

			if (const auto* next_time = msg_reg->try_get<Components::ConvertedTimeCache>(e); next_time != nullptr) { // check if date changed
				// TODO: move conversion up?
				if (
					prev_time.tm_yday != next_time->tm_yday ||
					prev_time.tm_year != next_time->tm_year // making sure
				) {
					// name
					if (ImGui::TableNextColumn()) {
						//ImGui::TextDisabled("---");
					}
					// msg
					if (ImGui::TableNextColumn()) {
						ImGui::TextDisabled("DATE CHANGED from %d.%d.%d to %d.%d.%d",
							1900+prev_time.tm_year, 1+prev_time.tm_mon, prev_time.tm_mday,
							1900+next_time->tm_year, 1+next_time->tm_mon, next_time->tm_mday
						);
					}
					ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);
				}

				prev_time = *next_time;
			}

			ImGui::PushID(entt::to_integral(e));

			// name
			if (ImGui::TableNextColumn()) {
				const float img_y {TEXT_BASE_HEIGHT - ImGui::GetStyle().FramePadding.y*2};
				renderAvatar(_theme, _contact_tc, _cs.contactHandle(c_from->c), {img_y, img_y});
				ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x*0.5f);

				if (const auto* name_ptr = cr.try_get<Contact::Components::Name>(c_from->c)) {
					ImGui::TextUnformatted(name_ptr->name.c_str());
				} else {
					ImGui::TextUnformatted("<unk>");
				}

				// use username as visibility test
				if (ImGui::IsItemVisible()) {
					if (msg_reg->all_of<Message::Components::TagUnread>(e)) {
						if (!msg_reg->all_of<Components::UnreadFade>(e)) {
							if (msg_reg->all_of<Message::Components::Read>(e)) {
								// skip fade, we might get here by merging
								msg_reg->remove<Message::Components::TagUnread>(e);
							} else {
								// get time now
								const uint64_t ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
								msg_reg->emplace_or_replace<Message::Components::Read>(e, ts_now);
								msg_reg->emplace_or_replace<Components::UnreadFade>(e, 1.f);
							}
							_rmm.throwEventUpdate(*msg_reg, e);
						} else if (window_focused) {
							// remove unread early, when we focus the window
							msg_reg->remove<Message::Components::TagUnread>(e);
							_rmm.throwEventUpdate(*msg_reg, e);
						}
					}

					// track view
					if (!static_cast<bool>(message_view_oldest)) {
						message_view_oldest = {*msg_reg, e};
						message_view_newest = {*msg_reg, e};
					} else if (static_cast<bool>(message_view_newest)) {
						// update to latest
						message_view_newest = {*msg_reg, e};
					}
				}

				// highlight self
				if (cr.any_of<Contact::Components::TagSelfStrong, Contact::Components::TagSelfWeak>(c_from->c)) {
					const auto self_msg_hi_col = _theme.getColor<ThemeCol_Contact::message_highlight_self>();
					ImU32 cell_bg_color = ImGui::GetColorU32(self_msg_hi_col);
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
				} else {
					//based on power level?
					//ImU32 cell_bg_color = ImGui::GetColorU32(ImVec4(0.3f, 0.7f, 0.3f, 0.65f));
					//ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
				}

				std::optional<ImVec4> row_bg;

				// private group message
				if (highlight_private && cr.any_of<Contact::Components::TagSelfStrong, Contact::Components::TagSelfWeak, Contact::Components::TagPrivate>(c_to->c)) {
					const ImVec4 priv_msg_hi_col = _theme.getColor<ThemeCol_Contact::message_highlight_private>();
					ImU32 row_bg_color = ImGui::GetColorU32(priv_msg_hi_col);
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, row_bg_color);
					row_bg = priv_msg_hi_col;
				}

				// fade
				if (msg_reg->all_of<Components::UnreadFade>(e)) {
					ImVec4 hi_color = _theme.getColor<ThemeCol_Contact::unread>();
					hi_color.w = 0.8f;
					const ImVec4 orig_color = row_bg.value_or(ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg)); // imgui defaults to 0,0,0,0
					const float fade_frac = msg_reg->get<Components::UnreadFade>(e).fade;

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
			if (ImGui::TableNextColumn()) {
				bool main_cell_visible {true};
				if (const auto* txt_comp_ptr = msg_reg->try_get<Message::Components::MessageText>(e); txt_comp_ptr != nullptr) {
					main_cell_visible = renderMessageBodyText(*msg_reg, e, *txt_comp_ptr);
				} else if (const auto* file_comp_ptr = msg_reg->try_get<Message::Components::MessageFileObject>(e); file_comp_ptr != nullptr) {
					main_cell_visible = renderMessageBodyFile(*msg_reg, e, *file_comp_ptr);
				} else {
					ImGui::TextDisabled("---");
					main_cell_visible = ImGui::IsItemVisible();
				}
				if (!main_cell_visible) {
					ImGui::PopID(); // ent
					continue; // optimization
				}
			}

			// remote received and read state
			if (ImGui::TableNextColumn()) {
				if (!msg_reg->all_of<Message::Components::ReceivedBy>(e)) {
					// TODO: dedup?
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
					ImGui::TextUnformatted("_");
					ImGui::PopStyleColor();
				} else {
					const auto& list = msg_reg->get<Message::Components::ReceivedBy>(e).ts;
					// wrongly assumes contacts never get removed from a group
					if (sub_contacts != nullptr && list.size() < sub_contacts->size()) {
						// if partically delivered
						ImGui::PushStyleColor(ImGuiCol_Text, _theme.getColor<ThemeCol_Contact::delivery_partial>());
						ImGui::TextUnformatted("d");
						ImGui::PopStyleColor();
					} else {
						// if fully delivered
						ImGui::PushStyleColor(ImGuiCol_Text, _theme.getColor<ThemeCol_Contact::delivery_full>());
						ImGui::TextUnformatted("D");
						ImGui::PopStyleColor();
					}

					if (ImGui::BeginItemTooltip()) {
						std::string synced_by_text {"delivery confirmed by:"};
						const int64_t now_ts_s = int64_t(getTimeMS() / 1000u);

						size_t other_contacts {0};
						for (const auto& [cd, syned_ts] : list) {
							if (cr.all_of<Contact::Components::TagSelfStrong>(cd)) {
								//synced_by_text += "\n sself(!)"; // makes no sense
								continue;
							} else if (cr.all_of<Contact::Components::TagSelfWeak>(cd)) {
								synced_by_text += "\n wself"; // TODO: add name?
							} else {
								synced_by_text += "\n >" + (cr.all_of<Contact::Components::Name>(cd) ? cr.get<Contact::Components::Name>(cd).name : "<unk>");
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
				if (!msg_reg->all_of<Message::Components::ReadBy>(e)) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
					ImGui::TextUnformatted("_");
					ImGui::PopStyleColor();
				} else {
					const auto list = msg_reg->get<Message::Components::ReadBy>(e).ts;
					// wrongly assumes contacts never get removed from a group
					if (sub_contacts != nullptr && list.size() < sub_contacts->size()) {
						// if partially read
						ImGui::TextColored(_theme.getColor<ThemeCol_Contact::read_partial>(), "r");
					} else {
						// if fully read
						ImGui::TextColored(_theme.getColor<ThemeCol_Contact::read_full>(), "R");
					}

					if (ImGui::BeginItemTooltip()) {
						std::string synced_by_text {"read confirmed by:"};
						const int64_t now_ts_s = int64_t(getTimeMS() / 1000u);

						for (const auto& [cd, syned_ts] : list) {
							if (cr.all_of<Contact::Components::TagSelfStrong>(cd)) {
								//synced_by_text += "\n sself(!)"; // makes no sense
								continue;
							} else if (cr.all_of<Contact::Components::TagSelfWeak>(cd)) {
								synced_by_text += "\n wself";
							} else {
								synced_by_text += "\n >" + (cr.all_of<Contact::Components::Name>(cd) ? cr.get<Contact::Components::Name>(cd).name : "<unk>");
							}
							const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
							synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
						}

						ImGui::Text("%s", synced_by_text.c_str());

						ImGui::EndTooltip();
					}
				}
			}

			// ts
			if (ImGui::TableNextColumn()) {
				if (!msg_reg->all_of<Components::ConvertedTimeCache>(e)) {
					auto time = std::chrono::system_clock::to_time_t(
						std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>{std::chrono::milliseconds{ts.ts}}
					);
					auto localtime = std::localtime(&time);
					char h_m_buf[6]; // 2+1+2
					std::snprintf(h_m_buf, sizeof(h_m_buf), "%.2d:%.2d", localtime->tm_hour, localtime->tm_min);
					msg_reg->emplace<Components::ConvertedTimeCache>(
						e,
						localtime->tm_year,
						localtime->tm_yday,
						localtime->tm_mon,
						localtime->tm_mday,
						localtime->tm_hour,
						localtime->tm_min,
						h_m_buf
					);
				}
				const auto& ctc = msg_reg->get<Components::ConvertedTimeCache>(e);

				//ImGui::Text("%.2d:%.2d", ctc.tm_hour, ctc.tm_min);
				ImGui::TextUnformatted(ctc.h_m.c_str());
			}

			if (_show_chat_extra_info &&ImGui::TableNextColumn()) {
				renderMessageExtra(*msg_reg, e);
			}

			ImGui::PopID(); // ent
		}

		// fake empty placeholders
		//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);
		//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

		{ // update view cursers
			if (!msg_reg->ctx().contains<Context::CGView>()) {
				msg_reg->ctx().emplace<Context::CGView>();
			}

			auto& cg_view = msg_reg->ctx().get<Context::CGView>();

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
					cg_view.begin = {*msg_reg, msg_reg->create()};
					cg_view.end = {*msg_reg, msg_reg->create()};

					cg_view.begin.emplace_or_replace<Message::Components::ViewCurserBegin>(cg_view.end);
					cg_view.end.emplace_or_replace<Message::Components::ViewCurserEnd>(cg_view.begin);

					cg_view.begin.get_or_emplace<Message::Components::Timestamp>().ts = getTimeMS();
					cg_view.end.get_or_emplace<Message::Components::Timestamp>().ts = getTimeMS();

					std::cout << "CG: created view FRONT begin ts\n";
					_rmm.throwEventConstruct(cg_view.begin);
					std::cout << "CG: created view FRONT end ts\n";
					_rmm.throwEventConstruct(cg_view.end);
				} // else? we do nothing?
			} else {
				bool begincreated {false};
				if (!static_cast<bool>(cg_view.begin)) {
					cg_view.begin = {*msg_reg, msg_reg->create()};
					begincreated = true;
				}
				bool endcreated {false};
				if (!static_cast<bool>(cg_view.end)) {
					cg_view.end = {*msg_reg, msg_reg->create()};
					endcreated = true;
				}
				cg_view.begin.emplace_or_replace<Message::Components::ViewCurserBegin>(cg_view.end);
				cg_view.end.emplace_or_replace<Message::Components::ViewCurserEnd>(cg_view.begin);

				{
					auto& old_begin_ts = cg_view.begin.get_or_emplace<Message::Components::Timestamp>().ts;
					if (old_begin_ts != message_view_newest.get<Message::Components::Timestamp>().ts) {
						old_begin_ts = message_view_newest.get<Message::Components::Timestamp>().ts;
						if (begincreated) {
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
						if (endcreated) {
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

	if (ImGui::Shortcut(ImGuiKey_G | ImGuiMod_Shift, ImGuiInputFlags_RouteGlobal) || ImGui::Shortcut(ImGuiKey_End, ImGuiInputFlags_RouteGlobal)) {
		ImGui::SetScrollHereY(1.f);
		manually_scrolled = true;
	}

	// follow if at bottom (this is a frame delayed, but thats just how it works)
	if (!manually_scrolled && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.f);
	}

	return 2000.f;
}

void ContactChatLog::fadeSystem(bool window_focused, float time_delta) {
	assert(msg_reg != nullptr);

	if (window_focused) { // fade system
		std::vector<Message3> to_remove;
		msg_reg->view<Components::UnreadFade>().each([&to_remove, time_delta](const Message3 e, Components::UnreadFade& fade) {
			// TODO: configurable
			const float fade_duration = 5.f;
			fade.fade -= 1.f/fade_duration * std::min<float>(time_delta, 1.f/10.f); // fps but not below 10 for smooth-ish fade
			if (fade.fade <= 0.f) {
				to_remove.push_back(e);
			}
		});
		msg_reg->remove<Message::Components::TagUnread, Components::UnreadFade>(to_remove.cbegin(), to_remove.cend());
	}
}

bool ContactChatLog::renderMessageBodyText(Message3Registry&, const Message3, const Message::Components::MessageText& msgtext) {
#if 0
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
			//text_buffer.insert(0, (cr.all_of<Contact::Components::Name>(c) ? cr.get<Contact::Components::Name>(c).name : "<unk>") + ": ");
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
		if (ImGui::MenuItem("copy")) {
			ImGui::SetClipboardText(msgtext.c_str());
		}
		ImGui::EndPopup();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
#else

	ImGui::PushTextWrapPos(0.0f);
	std::string_view msgtext_sv{msgtext.text};
	size_t pos_prev {0};
	size_t pos_next {msgtext_sv.find_first_of('\n')};
	ImGui::BeginGroup();
	do {
		const auto current_line = msgtext_sv.substr(pos_prev, pos_next - pos_prev);
		if (!current_line.empty() && current_line.front() == '>') {
			ImGui::PushStyleColor(ImGuiCol_Text, _theme.getColor<ThemeCol_Contact::message_quoted_text>());
			ImGui::TextUnformatted(current_line.data(), current_line.data()+current_line.size());
			ImGui::PopStyleColor();
		} else {
			ImGui::TextUnformatted(current_line.data(), current_line.data()+current_line.size());
		}

		if (pos_next != msgtext_sv.npos) {
			pos_next += 1; // skip past
			if (pos_next < msgtext_sv.size()) {
				pos_prev = pos_next; // old end is new start
				pos_next = msgtext_sv.find_first_of('\n', pos_next);
			} else {
				pos_prev = msgtext_sv.npos;
				pos_next = msgtext_sv.npos;
			}
		} else {
			pos_prev = msgtext_sv.npos;
		}
	} while (pos_prev != msgtext_sv.npos);

	ImGui::EndGroup();
	const bool cell_visible = ImGui::IsItemVisible();
	ImGui::PopTextWrapPos();

	if (ImGui::BeginPopupContextItem("##text")) {
		// TODO: maybe hooks instead
		if (ImGui::MenuItem("quote")) {
			//text_buffer.insert(0, (cr.all_of<Contact::Components::Name>(c) ? cr.get<Contact::Components::Name>(c).name : "<unk>") + ": ");
			if (!_text_input_buffer.empty()) {
				_text_input_buffer += "\n";
			}

			_text_input_buffer += "> ";

			for (const char ch : msgtext.text) {
				_text_input_buffer += ch;

				if (ch == '\n') {
					_text_input_buffer += "> ";
				}
			}
		}
		if (ImGui::MenuItem("copy")) {
			ImGui::SetClipboardText(msgtext.text.c_str());
		}
		ImGui::EndPopup();
	}
#endif
	return cell_visible;
}

bool ContactChatLog::renderMessageBodyFile(Message3Registry& reg, const Message3 e, const Message::Components::MessageFileObject& o_comp) {
	const auto& o = o_comp.o;
	if (!o) {
		ImGui::TextDisabled("file message missing file object!");
		return ImGui::IsItemVisible();
	}

	if (
		!_show_chat_avatar_tf
		&& (
			o.all_of<ObjComp::Tox::FileKind>()
			&& o.get<ObjComp::Tox::FileKind>().kind == 1
		)
	) {
		// TODO: this looks ugly
		ImGui::TextDisabled("set avatar");
		return ImGui::IsItemVisible();
	}

	const bool local_have_all = o.all_of<ObjComp::F::TagLocalHaveAll>();

	ImGui::BeginGroup();

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
	if (o.all_of<ObjComp::Ephemeral::File::TagTransferPaused>()) {
		ImGui::TextUnformatted("paused");
	//} else if (reg.all_of<Message::Components::Transfer::TagReceiving, Message::Components::Transfer::TagHaveAll>(e)) {
	//    ImGui::TextUnformatted("done");
	} else {
		// TODO: missing other states
		ImGui::TextUnformatted("running");
	}
	if (local_have_all) {
		ImGui::SameLine();
		ImGui::TextUnformatted("(have all)");
	}

	// if in offered state
	// paused, never started
	if (
		!local_have_all &&
		//reg.all_of<Message::Components::Transfer::TagReceiving>(e) &&
		o.all_of<ObjComp::Ephemeral::File::TagTransferPaused>() &&
		// TODO: how does restarting a broken/incomplete transfer look like?
		!o.all_of<ObjComp::F::SingleInfoLocal>() &&
		!o.all_of<ObjComp::Ephemeral::File::ActionTransferAccept>()
	) {
		if (ImGui::Button("save to")) {
			_fss.requestFile(
				[](std::filesystem::path& path) -> bool {
					// remove file path
					path.remove_filename();
					return std::filesystem::is_directory(path);
				},
				[this, o](const auto& path) {
					if (static_cast<bool>(o)) { // still valid
						// TODO: trim file?
						o.emplace<ObjComp::Ephemeral::File::ActionTransferAccept>(path.generic_u8string());
						//_rmm.throwEventUpdate(reg, e);
						// TODO: block recursion
						_os.throwEventUpdate(o);
					}
				},
				[](){}
			);
		}
	}

	// hacky
	const auto* fts = o.try_get<ObjComp::Ephemeral::File::TransferStats>();
	if (fts != nullptr && o.any_of<ObjComp::F::SingleInfo, ObjComp::F::CollectionInfo>()) {
		const bool upload = local_have_all && fts->total_down <= 0;

		const int64_t total_size =
			o.all_of<ObjComp::F::SingleInfo>() ?
				o.get<ObjComp::F::SingleInfo>().file_size :
				o.get<ObjComp::F::CollectionInfo>().total_size
			;

		int64_t transfer_total {0u};
		float transfer_rate {0.f};
		if (upload) {
			// if have all AND no dl -> show upload progress
			ImGui::TextUnformatted("  up");
			transfer_total = fts->total_up;
			transfer_rate = fts->rate_up;
		} else {
			// else show download progress
			ImGui::TextUnformatted("down");
			transfer_total = fts->total_down;
			transfer_rate = fts->rate_down;
		}
		ImGui::SameLine();

		float fraction{0.f};
		if (total_size > 0) {
			fraction = float(transfer_total) / total_size;
		} else if (o.all_of<ObjComp::F::TagLocalHaveAll>()) {
			fraction = 1.f;
		}

		char overlay_buf[128];
		if (transfer_rate > 0.000001f) {
			const char* byte_suffix = "???";
			int64_t byte_divider = sizeToHumanReadable(transfer_rate, byte_suffix);
			int64_t ms_remaining = (total_size - transfer_total) / (transfer_rate/1000.f);
			if (ms_remaining > 0) {
				const char* duration_suffix = "???";
				int64_t duration_divider = durationToHumanReadable(ms_remaining, duration_suffix);
				std::snprintf(
					overlay_buf, sizeof(overlay_buf),
					"%.1f%% @ %.1f%s/s %.1f%s",
					fraction * 100 + 0.01f,

					transfer_rate/byte_divider,
					byte_suffix,

					double(ms_remaining)/duration_divider,
					duration_suffix
				);
			} else {
				std::snprintf(overlay_buf, sizeof(overlay_buf), "%.1f%% @ %.1f%s/s", fraction * 100 + 0.01f, transfer_rate/byte_divider, byte_suffix);
			}
		} else {
			std::snprintf(overlay_buf, sizeof(overlay_buf), "%.1f%%", fraction * 100 + 0.01f);
		}

		const auto plot_color = local_have_all ? _theme.getColor<ThemeCol_Contact::ft_have_all>() : _theme.getColor<ThemeCol_Contact::ft_base>();
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, plot_color);
		if (
			(!upload && !local_have_all && o.all_of<ObjComp::F::LocalHaveBitset>()) ||
			(upload && o.all_of<ObjComp::F::RemoteHaveBitset>())
		) {
			ImGui::BeginGroup();

			// TODO: hights are all off

			ImGui::ProgressBar(
				fraction,
				{-FLT_MIN, TEXT_BASE_HEIGHT*0.66f},
				overlay_buf
			);

			ImVec2 orig_curser_pos = ImGui::GetCursorPos();
			const ImVec2 bar_size{ImGui::GetContentRegionAvail().x, TEXT_BASE_HEIGHT*0.15f};
			// deploy dummy and check visibility
			ImGui::Dummy(bar_size);
			if (ImGui::IsItemVisible()) {
				ImGui::SetCursorPos(orig_curser_pos); // reset before dummy

				auto const cursor_start_vec = ImGui::GetCursorScreenPos();
				// TODO: replace with own version, so we dont have to internal
				ImGui::RenderFrame(
					cursor_start_vec,
					{
						cursor_start_vec.x + bar_size.x,
						cursor_start_vec.y + bar_size.y
					},
					ImGui::GetColorU32(ImGuiCol_FrameBg),
					false
				);

				auto [id, img_width, img_height] = _b_tc.get(o);
				ImGui::ImageWithBg(
					id,
					bar_size,
					{0.f, 0.f}, // default
					{1.f, 1.f}, // default
					{0.f, 0.f, 0.f, 0.f}, // sadly bg was moved before tint (???)
					plot_color
				);
			}

			ImGui::EndGroup();
		} else {
			ImGui::ProgressBar(
				fraction,
				{-FLT_MIN, TEXT_BASE_HEIGHT},
				overlay_buf
			);
		}
		ImGui::PopStyleColor();
	} else {
		// infinite scrolling progressbar fallback
		ImGui::TextUnformatted("  ??");
		ImGui::SameLine();
		ImGui::ProgressBar(
			-0.333f * ImGui::GetTime(),
			{-FLT_MIN, TEXT_BASE_HEIGHT},
			"?%"
		);
	}

	if (o.all_of<ObjComp::F::FrameDims>()) {
		const auto& frame_dims = o.get<ObjComp::F::FrameDims>();

		// TODO: config
		const auto max_inline_height = 10*TEXT_BASE_HEIGHT;

		float width = frame_dims.w;
		float height = frame_dims.h;

		if (height > max_inline_height) {
			const float scale = max_inline_height / height;
			height = max_inline_height;
			width *= scale;
		}

		ImVec2 orig_curser_pos = ImGui::GetCursorPos();
		// deploy dummy of framedim size and check visibility
		// +2 for border
		ImGui::Dummy(ImVec2{width+2, height+2});
		if (ImGui::IsItemVisible() && o.all_of<ObjComp::F::TagLocalHaveAll, ObjComp::F::SingleInfo, ObjComp::Ephemeral::BackendFile2>()) {
			ImGui::SetCursorPos(orig_curser_pos); // reset for actual img

			auto [id, img_width, img_height] = _msg_tc.get(Message3Handle{reg, e}, width, height);

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

			ImGui::Image(
				id,
				ImVec2{width, height},
				{0.f, 0.f}, // default
				{1.f, 1.f}, // default
				{1.f, 1.f, 1.f, 1.f}, // default
				{0.5f, 0.5f, 0.5f, 0.8f} // border
			);

			if (ImGui::IsItemClicked()) {
				_ivp.view(Message3Handle{reg, e});
			}
		}
	} else if (o.all_of<ObjComp::F::SingleInfo>()) { // only show info if not inlined image
		// just filename
		const auto& si = o.get<ObjComp::F::SingleInfo>();
		const char* byte_suffix = "???";
		int64_t byte_divider = sizeToHumanReadable(si.file_size, byte_suffix);
		ImGui::Text("%s (%.2lf %s)", si.file_name.c_str(), double(si.file_size)/byte_divider, byte_suffix);
	} else if (o.all_of<ObjComp::F::CollectionInfo>()) {
		// same old bulletpoint list
		const auto& file_list = o.get<ObjComp::F::CollectionInfo>().file_list;

		// if has local, display save base path?, do we have base save path?

		for (size_t i = 0; i < file_list.size(); i++) {
			ImGui::PushID(i);

			const char* byte_suffix = "???";
			int64_t byte_divider = sizeToHumanReadable(file_list[i].file_size, byte_suffix);

			// TODO: selectable text widget ?
			ImGui::Bullet(); ImGui::Text("%s (%.2lf %s)", file_list[i].file_name.c_str(), double(file_list[i].file_size)/byte_divider, byte_suffix);

			if (o.all_of<ObjComp::F::CollectionInfoLocal>()) {
				const auto& local_info = o.get<ObjComp::F::CollectionInfoLocal>();
				if (local_info.file_list.size() > i && ImGui::BeginPopupContextItem("##file_c")) {

					if (ImGui::MenuItem("open")) {
						const std::string url {file_path_to_file_url(local_info.file_list.at(i).file_path)};
						std::cout << "opening file '" << url << "'\n";
						SDL_OpenURL(url.c_str());
					}
					if (ImGui::MenuItem("copy file")) {
						const std::string url {file_path_to_file_url(local_info.file_list.at(i).file_path)};
						//ImGui::SetClipboardText(url.c_str());
						_cb.setClipboardData({"text/uri-list", "text/x-moz-url"}, std::make_shared<std::vector<uint8_t>>(url.begin(), url.end()));
					}
					if (ImGui::MenuItem("copy filepath")) {
						const auto file_path = std::filesystem::canonical(local_info.file_list.at(i).file_path).u8string(); //TODO: use generic over native?
						ImGui::SetClipboardText(file_path.c_str());
					}
					ImGui::EndPopup();
				}
			}

			ImGui::PopID();
		}
	} else {
		ImGui::TextDisabled("neither info available");
	}

	ImGui::EndGroup();
	const bool cell_visible = ImGui::IsItemVisible();

	if (o.all_of<ObjComp::F::SingleInfoLocal>()) {
		const auto& local_info = o.get<ObjComp::F::SingleInfoLocal>();
		if (!local_info.file_path.empty() && ImGui::BeginPopupContextItem("##file_c")) {
			if (ImGui::MenuItem("open")) {
				const std::string url {file_path_to_file_url(local_info.file_path)};
				std::cout << "opening file '" << url << "'\n";
				SDL_OpenURL(url.c_str());
			}

			ImGui::Separator();

			// TODO: better way to dif up/down
			if (!local_have_all) {
				if (ImGui::BeginMenu("dowload priority")) {
					using Priority = ObjComp::Ephemeral::File::DownloadPriority::Priority;
					auto& p_comp = o.get_or_emplace<ObjComp::Ephemeral::File::DownloadPriority>();

					bool updated {false};
					if (ImGui::MenuItem("highest", nullptr, p_comp.p == Priority::HIGHEST)) {
						p_comp.p = Priority::HIGHEST;
						updated = true;
					}
					if (ImGui::MenuItem("high", nullptr, p_comp.p == Priority::HIGH)) {
						p_comp.p = Priority::HIGH;
						updated = true;
					}
					if (ImGui::MenuItem("normal", nullptr, p_comp.p == Priority::NORMAL)) {
						p_comp.p = Priority::NORMAL;
						updated = true;
					}
					if (ImGui::MenuItem("low", nullptr, p_comp.p == Priority::LOW)) {
						p_comp.p = Priority::LOW;
						updated = true;
					}
					if (ImGui::MenuItem("lowest", nullptr, p_comp.p == Priority::LOWEST)) {
						p_comp.p = Priority::LOWEST;
						updated = true;
					}

					if (updated) {
						std::cout << "CG: updated download priority to " << int(p_comp.p) << "\n";
						// TODO: dont do it here
						_os.throwEventUpdate(o);
					}

					ImGui::EndMenu();
				}
				ImGui::Separator();
			}

			if (ImGui::BeginMenu("forward", local_have_all)) {
				auto& cr = _cs.registry();
				for (const auto& fc : cr.view<Contact::Components::TagBig>()) {
					// filter
					if (cr.any_of<Contact::Components::RequestIncoming, Contact::Components::TagRequestOutgoing>(fc)) {
						continue;
					}
					// TODO: check for contact capability
					// or just error popup?/noti/toast

					if (renderContactBig(_theme, _contact_tc, {cr, fc}, 1, false, true, false)) {
						// TODO: try object interface first instead, then fall back to send with SingleInfoLocal
						//_rmm.sendFileObj(fc, o);
						std::filesystem::path path = o.get<ObjComp::F::SingleInfoLocal>().file_path;
						_rmm.sendFilePath(fc, path.filename().generic_u8string(), path.generic_u8string());
					}
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("copy file")) {
				const std::string url {file_path_to_file_url(local_info.file_path)};
				_cb.setClipboardData({"text/uri-list", "text/x-moz-url"}, std::make_shared<std::vector<uint8_t>>(url.begin(), url.end()));
			}
			if (ImGui::MenuItem("copy filepath")) {
				const auto file_path = std::filesystem::canonical(local_info.file_path).u8string(); //TODO: use generic over native?
				ImGui::SetClipboardText(file_path.c_str());
			}
			ImGui::EndPopup();
		}
	}
	// TODO: how to collections?

	if (ImGui::BeginItemTooltip()) {
		if (o.all_of<ObjComp::F::SingleInfo>()) {
			ImGui::SeparatorText("single info");
			const auto& si = o.get<ObjComp::F::SingleInfo>();
			ImGui::Text("file name: '%s'", si.file_name.c_str());

			const char* byte_suffix = "???";
			int64_t byte_divider = sizeToHumanReadable(si.file_size, byte_suffix);
			ImGui::Text("file size: %.2lf %s (%lu Bytes)", double(si.file_size)/byte_divider, byte_suffix, si.file_size);
			if (o.all_of<ObjComp::F::SingleInfoLocal>()) {
				ImGui::Text("local path: '%s'", o.get<ObjComp::F::SingleInfoLocal>().file_path.c_str());
			}
		} else if (o.all_of<ObjComp::F::CollectionInfo>()) {
			ImGui::SeparatorText("collection info");
			const auto& ci = o.get<ObjComp::F::CollectionInfo>();
			const char* byte_suffix = "???";
			int64_t byte_divider = sizeToHumanReadable(ci.total_size, byte_suffix);
			ImGui::Text("total size: %.2lf %s (%lu Bytes)", double(ci.total_size)/byte_divider, byte_suffix, ci.total_size);
		}

		if (fts != nullptr) {
			ImGui::SeparatorText("transfer stats");

			{
				const char* byte_suffix = "???";
				int64_t byte_divider = sizeToHumanReadable(fts->rate_up, byte_suffix);
				ImGui::Text("rate up   : %.2f %s/s", fts->rate_up/byte_divider, byte_suffix);
			}
			{
				const char* byte_suffix = "???";
				int64_t byte_divider = sizeToHumanReadable(fts->rate_down, byte_suffix);
				ImGui::Text("rate down : %.2f %s/s", fts->rate_down/byte_divider, byte_suffix);
			}
			{
				const char* byte_suffix = "???";
				int64_t byte_divider = sizeToHumanReadable(fts->total_up, byte_suffix);
				ImGui::Text("total up  : %.3f %s", double(fts->total_up)/byte_divider, byte_suffix);
			}
			{
				const char* byte_suffix = "???";
				int64_t byte_divider = sizeToHumanReadable(fts->total_down, byte_suffix);
				ImGui::Text("total down: %.3f %s", double(fts->total_down)/byte_divider, byte_suffix);
			}
		}

		ImGui::EndTooltip();
	}

	return cell_visible;
}

void ContactChatLog::renderMessageExtra(Message3Registry& reg, const Message3 e) {
	if (reg.all_of<Message::Components::MessageFileObject>(e)) {
		const auto o = reg.get<Message::Components::MessageFileObject>(e).o;

		ImGui::TextDisabled("o:%u", entt::to_integral(o.entity()));

		if (o.all_of<ObjComp::Tox::FileKind>()) {
			ImGui::TextDisabled("fk:%lu", o.get<ObjComp::Tox::FileKind>().kind);
		}
		if (o.all_of<ObjComp::Ephemeral::ToxTransferFriend>()) {
			ImGui::TextDisabled("ttf:%u", o.get<ObjComp::Ephemeral::ToxTransferFriend>().transfer_number);
		}
	}

	if (reg.all_of<Message::Components::ToxGroupMessageID>(e)) {
		ImGui::TextDisabled("msgid:%u", reg.get<Message::Components::ToxGroupMessageID>(e).id);
	}

	const auto& cr = _cs.registry();

	if (reg.all_of<Message::Components::SyncedBy>(e)) {
		std::string synced_by_text {"syncedBy:"};
		const int64_t now_ts_s = int64_t(getTimeMS() / 1000u);

		for (const auto& [cc, syned_ts] : reg.get<Message::Components::SyncedBy>(e).ts) {
			if (cr.all_of<Contact::Components::TagSelfStrong>(cc)) {
				synced_by_text += "\n sself";
			} else if (cr.all_of<Contact::Components::TagSelfWeak>(cc)) {
				synced_by_text += "\n wself";
			} else {
				synced_by_text += "\n >" + (cr.all_of<Contact::Components::Name>(cc) ? cr.get<Contact::Components::Name>(cc).name : "<unk>");
			}
			const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
			synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
		}

		ImGui::TextDisabled("%s", synced_by_text.c_str());
	}

	// TODO: remove?
	if (reg.all_of<Message::Components::ReceivedBy>(e)) {
		std::string synced_by_text {"receivedBy:"};
		const int64_t now_ts_s = int64_t(getTimeMS() / 1000u);

		for (const auto& [cc, syned_ts] : reg.get<Message::Components::ReceivedBy>(e).ts) {
			if (cr.all_of<Contact::Components::TagSelfStrong>(cc)) {
				synced_by_text += "\n sself"; // required (except when synced externally)
			} else if (cr.all_of<Contact::Components::TagSelfWeak>(cc)) {
				synced_by_text += "\n wself";
			} else {
				synced_by_text += "\n >" + (cr.all_of<Contact::Components::Name>(cc) ? cr.get<Contact::Components::Name>(cc).name : "<unk>");
			}
			const int64_t seconds_ago = (int64_t(syned_ts / 1000u) - now_ts_s) * -1;
			synced_by_text += " (" + std::to_string(seconds_ago) + "sec ago)";
		}

		ImGui::TextDisabled("%s", synced_by_text.c_str());
	}
}

