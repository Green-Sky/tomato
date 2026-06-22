#include "./theme.hpp"

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

	t.setColor<ThemeCol_Contact::ft_base				>(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogramHovered));
	t.setColor<ThemeCol_Contact::ft_have_all			>({0.35f, 0.84f, 0.22f, 1.0f});

	t.setColor<ThemeCol_Contact::request_panel_background	>({0.90f, 0.70f, 0.00f, 0.32f});
	t.setColor<ThemeCol_Contact::message_warning_text		>({1.00f, 0.30f, 0.30f, 1.00f});
	t.setColor<ThemeCol_Contact::message_quoted_text		>({0.30f, 0.90f, 0.10f, 1.00f});
	t.setColor<ThemeCol_Contact::message_highlight_private	>({0.50f, 0.20f, 0.50f, 0.35f});
	t.setColor<ThemeCol_Contact::message_highlight_self		>({0.30f, 0.70f, 0.30f, 0.20f});

	t.setColor<ThemeCol_Contact::delivery_partial		>({0.80f, 0.80f, 0.10f, 0.70f});
	t.setColor<ThemeCol_Contact::delivery_full			>({0.10f, 0.80f, 0.10f, 0.70f});
	t.setColor<ThemeCol_Contact::read_partial			>(t.getColor<ThemeCol_Contact::delivery_partial>());
	t.setColor<ThemeCol_Contact::read_full				>(t.getColor<ThemeCol_Contact::delivery_full>());

	t.setColor<ThemeCol_Contact::crop_button			>({0.50f, 0.50f, 0.50f, 0.20f});
	t.setColor<ThemeCol_Contact::crop_button_active		>({0.50f, 0.50f, 0.50f, 0.40f});

	t.setColor<ThemeCol_Contact::directory_background_even	>({0.60f, 0.60f, 0.10f, 0.15f});
	t.setColor<ThemeCol_Contact::directory_background_odd	>({0.70f, 0.70f, 0.20f, 0.15f});

	t.setColor<ThemeCol_Contact::stream_source_connection	>({0.60f, 0.00f, 0.60f, 0.25f});
	t.setColor<ThemeCol_Contact::stream_sink_connection		>({0.60f, 0.60f, 0.00f, 0.25f});
	t.setColor<ThemeCol_Contact::stream_default_connection	>({0.60f, 0.60f, 0.00f, 0.25f});

	return t;
}

Theme getDefaultThemeLight(void) {
	// HACK: inherit dark and only diff
	Theme t = getDefaultThemeDark();

	t.setColor<ThemeCol_Contact::icon_backdrop			>({0.1f, 0.1f, 0.1f, 0.3f});

	return t;
}

