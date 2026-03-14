#include "./contact_info_window.hpp"

#include "./contact_info.hpp"

#include <solanaceae/contact/contact_store_impl.hpp>

#include <entt/entity/entity.hpp>
#include <entt/entity/handle.hpp>

#include <imgui.h>

#include <algorithm>

ContactInfoWindows::ContactInfoWindows(ContactStore4Impl& cs) : _cs(cs) {
}

ContactInfoWindows::~ContactInfoWindows(void) {
}

void ContactInfoWindows::open(Contact4 c) {
	const auto it = std::find_if(_windows.cbegin(), _windows.cend(), [c](const auto& it){ return it.c == c; });
	if (it == _windows.cend()) {
		_windows.push_back({c});
	}
}

void ContactInfoWindows::close(Contact4 c) {
	_windows.erase(
		std::remove_if(
			_windows.begin(),
			_windows.end(),
			[c](const auto& it){ return it.c == c; }
		),
		_windows.end()
	);
}

void ContactInfoWindows::render(void) {
	for (auto it = _windows.begin(); it != _windows.end();) {
		bool open = true;
		std::string title = "ContactInfo (" + std::to_string(entt::to_entity(it->c)) + ")";
		if (ImGui::Begin(title.c_str(), &open)) {
			ImGui::Checkbox("advanced", &it->advanced);
			ImGui::SameLine();
			ImGui::Checkbox("verbose", &it->verbose);

			renderContactInfo(_cs, _cs.contactHandle(it->c), it->advanced, it->verbose);
		}
		ImGui::End();

		if (open) {
			it++;
		} else {
			it = _windows.erase(it);
		}
	}
}
