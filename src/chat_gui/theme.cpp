#include "./theme.hpp"

// HACK: includes everything and sets theme defaults
#include "./contact_list.hpp"

//#include <iostream>

//enum class TestThemeSet {
	//Value1,
//};

//// specialization
////template<>
////std::string typeValueName(TestThemeSet v) {
	////switch (v) {
		////case TestThemeSet::Value1: return "Value1";
		////default: return "unk";
	////}
////}

Theme::Theme(void) {
	load();
}

void Theme::update(void) {
}

bool Theme::load(void) {
	name = "Default";

	//setColor<TestThemeSet::Value1>(ImVec4{});
	//std::cout << "test value name: " << getColorName<TestThemeSet::Value1>() << "\n";

	return true;
}

bool Theme::store(void) {
	return true;
}

Theme getDefaultThemeDark(void) {
	Theme t;

	t.setColor<ThemeCol_Contact::request_incoming		>({0.98f, 0.41f, 0.26f, 0.52f});
	t.setColor<ThemeCol_Contact::request_outgoing		>({0.98f, 0.26f, 0.41f, 0.52f});

	t.setColor<ThemeCol_Contact::avatar_online_direct	>({0.3f, 1.0f, 0.0f, 1.0f});
	t.setColor<ThemeCol_Contact::avatar_online_cloud	>({0.0f, 1.0f, 0.8f, 1.0f});
	t.setColor<ThemeCol_Contact::avatar_offline			>({0.4f, 0.4f, 0.4f, 1.0f});

	t.setColor<ThemeCol_Contact::unread					>(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogramHovered));
	t.setColor<ThemeCol_Contact::unread_muted			>({0.6f, 0.6f, 0.6f, 0.9f});

	t.setColor<ThemeCol_Contact::icon_backdrop			>({0.0f, 0.0f, 0.0f, 0.4f});

	return t;
}

Theme getDefaultThemeLight(void) {
	// HACK: inherit dark and only diff
	Theme t = getDefaultThemeDark();

	return t;
}

