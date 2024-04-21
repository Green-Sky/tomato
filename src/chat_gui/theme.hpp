#pragma once

#include <imgui/imgui.h>

#include <entt/container/dense_map.hpp>
#include <entt/core/type_info.hpp>

#include <string>
#include <type_traits>

// default is resolving to its value
template<typename T>
std::string typeValueName(T V) {
	return std::to_string(static_cast<std::underlying_type_t<T>>(V));
}

// stores theming values and colors not expressed by imgui directly
struct Theme {
	using key_type = entt::id_type;

	entt::dense_map<key_type, ImVec4> colors;
	entt::dense_map<key_type, std::string> colors_name;

	// TODO: spec out dependencies

	// TODO: what for
	entt::dense_map<key_type, float> single_values;

	std::string name; // theme name

	Theme(void);

	// call when any color changed, so dependencies can be resolved
	void update(void);

	template<auto V>
	void setColor(ImVec4 color) {
		constexpr auto key = entt::type_hash<entt::tag<V>>::value();
		colors[key] = color;

		if (!colors_name.contains(key)) {
			std::string key_name = static_cast<std::string>(
				entt::type_name<decltype(V)>::value()
			) + ":" + typeValueName(V);
			colors_name[key] = key_name;
		}
	}
	template<auto V>
	ImVec4 getColor(void) const {
		constexpr auto key = entt::type_hash<entt::tag<V>>::value();
		const auto it = colors.find(key);
		if (it != colors.end()) {
			return it->second;
		} else {
			return {}; // TODO: pink as default?
		}
	}

	template<auto V>
	std::string_view getColorName(void) const {
		constexpr auto key = entt::type_hash<entt::tag<V>>::value();
		if (colors_name.contains(key)) {
			return colors_name.at(key);
		} else {
			return "unk";
		}
	}

	// TODO: actually serialize from config?
	bool load(void);
	bool store(void);
};

Theme getDefaultThemeDark(void);
Theme getDefaultThemeLight(void);

