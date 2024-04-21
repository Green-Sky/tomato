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

	t.setColor<ThemeCol_Contact::request_incoming>({0.98f, 0.41f, 0.26f, 0.52f});
	t.setColor<ThemeCol_Contact::request_outgoing>({0.98f, 0.26f, 0.41f, 0.52f});

	t.setColor<ThemeCol_Contact::avatar_online_direct>({0.3, 1, 0, 1});
	t.setColor<ThemeCol_Contact::avatar_online_cloud>({0, 1, 0.8, 1});
	t.setColor<ThemeCol_Contact::avatar_offline>({0.4, 0.4, 0.4, 1});

	return t;
}

Theme getDefaultThemeLight(void) {
	// HACK: inherit dark and only diff
	Theme t = getDefaultThemeDark();

	return t;
}

