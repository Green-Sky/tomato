#include "./file_selector.hpp"

#include <imgui/imgui.h>

#include <chrono>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

void FileSelector::reset(void) {
	_is_valid = [](auto){ return true; };
	_on_choose = [](auto){};
	_on_cancel = [](){};
}

FileSelector::FileSelector(void) {
	reset();
}

void FileSelector::requestFile(
	std::function<bool(std::filesystem::path& path)>&& is_valid,
	std::function<void(const std::filesystem::path& path)>&& on_choose,
	std::function<void(void)>&& on_cancel
) {
	_open_popup = true;

	_is_valid = std::move(is_valid);
	_on_choose = std::move(on_choose);
	_on_cancel = std::move(on_cancel);
}

void FileSelector::render(void) {
	if (_open_popup) {
		_open_popup = false;
		ImGui::OpenPopup("file picker##FileSelector");

		const auto TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
		const auto TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		ImGui::SetNextWindowSize({TEXT_BASE_WIDTH*100, TEXT_BASE_HEIGHT*30});
	}

	if (ImGui::BeginPopupModal("file picker##FileSelector", nullptr/*, ImGuiWindowFlags_NoDecoration*/)) {
		const auto TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
		const auto TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		std::filesystem::path current_path = _current_file_path;
		current_path.remove_filename();

		ImGui::Text("path: %s", _current_file_path.generic_u8string().c_str());

		// begin table with selectables
		constexpr ImGuiTableFlags table_flags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_Sortable
		;
		if (ImGui::BeginTable("dir listing", 4, table_flags, {0, -TEXT_BASE_HEIGHT * 2.5f})) {
			enum class SortID : ImGuiID {
				name = 1,
				size,
				date
			};
			ImGui::TableSetupColumn("type", 0, TEXT_BASE_WIDTH);
			ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 0.f, static_cast<ImGuiID>(SortID::name));
			ImGui::TableSetupColumn("size", 0, 0.f, static_cast<ImGuiID>(SortID::size));
			ImGui::TableSetupColumn("dd.mm.yyyy - hh:mm", 0, 0.f, static_cast<ImGuiID>(SortID::date));

			ImGui::TableSetupScrollFreeze(0, 2);

			ImGui::TableHeadersRow();

			//ImGui::TableNextRow(0, TEXT_BASE_HEIGHT);

			if (current_path.has_parent_path()) {
				if (ImGui::TableNextColumn()) {
					if (ImGui::Selectable("D##..", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
						// the first "parent_path()" only removes the filename and the ending "/"
						_current_file_path = _current_file_path.parent_path().parent_path() / "";
						//_current_file_path = _current_file_path.remove_filename().parent_path() / "";
					}
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextUnformatted("..");
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextDisabled("---");
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextDisabled("---");
				}
			}
			size_t tmp_id = 0;
			// dirs
			static const ImU32 dir_bg0_color = ImGui::GetColorU32(ImVec4(0.6, 0.6, 0.1, 0.15));
			static const ImU32 dir_bg1_color = ImGui::GetColorU32(ImVec4(0.7, 0.7, 0.2, 0.15));

			std::vector<std::filesystem::directory_entry> dirs;
			std::vector<std::filesystem::directory_entry> files;
			for (auto const& dir_entry : std::filesystem::directory_iterator(current_path)) {
				if (dir_entry.is_directory()) {
					dirs.push_back(dir_entry);
				} else if (dir_entry.is_regular_file()) {
					files.push_back(dir_entry);
				}
			}

			// do sorting here
			// TODO: cache the result (lol)
			if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs(); sorts_specs != nullptr && sorts_specs->SpecsCount >= 1) {
				switch (static_cast<SortID>(sorts_specs->Specs->ColumnUserID)) {
					break; case SortID::name:
						if (sorts_specs->Specs->SortDirection == ImGuiSortDirection_Descending) {
							std::sort(dirs.begin(), dirs.end(), [](const auto& a, const auto& b) -> bool {
								return a.path() < b.path();
							});
							std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
								return a.path().filename() < b.path().filename();
							});
						} else {
							std::sort(dirs.begin(), dirs.end(), [](const auto& a, const auto& b) -> bool {
								return a.path() > b.path();
							});
							std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
								return a.path().filename() > b.path().filename();
							});
						}
					break; case SortID::size:
						if (sorts_specs->Specs->SortDirection == ImGuiSortDirection_Descending) {
							// TODO: sort dirs?
							std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
								return a.file_size() < b.file_size();
							});
						} else {
							// TODO: sort dirs?
							std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
								return a.file_size() > b.file_size();
							});
						}
					break; case SortID::date:
						if (sorts_specs->Specs->SortDirection == ImGuiSortDirection_Descending) {
							std::sort(dirs.begin(), dirs.end(), [](const auto& a, const auto& b) -> bool {
								return a.last_write_time() < b.last_write_time();
							});
							std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
								return a.last_write_time() < b.last_write_time();
							});
						} else {
							std::sort(dirs.begin(), dirs.end(), [](const auto& a, const auto& b) -> bool {
								return a.last_write_time() > b.last_write_time();
							});
							std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
								return a.last_write_time() > b.last_write_time();
							});
						}
					break; default: ;
				}
			}

			for (auto const& dir_entry : dirs) {
				if (ImGui::TableNextColumn()) {
					if (tmp_id & 0x01) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, dir_bg0_color);
					} else {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, dir_bg1_color);
					}
					ImGui::PushID(tmp_id++);
					if (ImGui::Selectable("D", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
						_current_file_path = dir_entry.path() / "";
					}
					ImGui::PopID();
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextUnformatted((dir_entry.path().filename().generic_u8string() + "/").c_str());
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextDisabled("---");
				}

				if (ImGui::TableNextColumn()) {
					const auto file_time_converted = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
						dir_entry.last_write_time()
						- decltype(dir_entry.last_write_time())::clock::now()
						+ std::chrono::system_clock::now()
					);
					const auto ctime = std::chrono::system_clock::to_time_t(file_time_converted);

					const auto ltime = std::localtime(&ctime);
					ImGui::TextDisabled("%2d.%2d.%2d - %2d:%2d", ltime->tm_mday, ltime->tm_mon, ltime->tm_year + 1900, ltime->tm_hour, ltime->tm_min);
				}
			}

			// files
			for (auto const& dir_entry : files) {
				if (ImGui::TableNextColumn()) {
					ImGui::PushID(tmp_id++);
					if (ImGui::Selectable("F", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
						_current_file_path = dir_entry.path();
					}
					ImGui::PopID();
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextUnformatted(dir_entry.path().filename().generic_u8string().c_str());
				}

				if (ImGui::TableNextColumn()) {
					ImGui::TextDisabled("%s", std::to_string(dir_entry.file_size()).c_str());
				}

				if (ImGui::TableNextColumn()) {
					const auto file_time_converted = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
						dir_entry.last_write_time()
						- decltype(dir_entry.last_write_time())::clock::now()
						+ std::chrono::system_clock::now()
					);
					const auto ctime = std::chrono::system_clock::to_time_t(file_time_converted);

					const auto ltime = std::localtime(&ctime);
					ImGui::TextDisabled("%2d.%2d.%2d - %2d:%2d", ltime->tm_mday, ltime->tm_mon, ltime->tm_year + 1900, ltime->tm_hour, ltime->tm_min);
				}
			}

			ImGui::EndTable();
		}

		//if (ImGui::Button("X cancel", {ImGui::GetWindowContentRegionWidth()/2.f, TEXT_BASE_HEIGHT*2})) {
		if (ImGui::Button("X cancel", {ImGui::GetContentRegionAvail().x/2.f, TEXT_BASE_HEIGHT*2})) {
			ImGui::CloseCurrentPopup();
			_on_cancel();
			reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("choose ->", {-FLT_MIN, TEXT_BASE_HEIGHT*2})) {
			if (_is_valid(_current_file_path)) {
				ImGui::CloseCurrentPopup();
				_on_choose(_current_file_path);
				reset();
			}
		}
		ImGui::EndPopup();
	}
}

