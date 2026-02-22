#include <solanaceae/contact/contact_store_impl.hpp>

#include <imgui.h>

#include <entt/entity/handle.hpp>

void renderContactInfo(
	ContactStore4Impl& cs,
	ContactHandle4 c,
	bool advanced = false,
	bool verbose = false
) {
	const auto c2s_list = cs.compsToString(c);

	std::string last_cat;

	for (const auto& it : c2s_list) {
		if (it.advanced != advanced) {
			continue;
		}
		if (it.category != last_cat) {
			last_cat = it.category;
			ImGui::SeparatorText(last_cat.c_str());
		}

		ImGui::TextUnformatted(it.name.data(), it.name.data()+it.name.size());

		const std::string str = it.fn(c, verbose);
		if (!str.empty()) {
			ImGui::SameLine();
			ImGui::TextUnformatted(str.data(), str.data()+str.size());
		}
	}
}

