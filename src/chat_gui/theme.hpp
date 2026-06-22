#pragma once

#include <imgui.h>

#include <entt/container/dense_map.hpp>
#include <entt/core/type_info.hpp>

#include <string>
#include <type_traits>

// default is resolving to its value
template<typename T>
std::string typeValueName(T V) {
	return std::to_string(static_cast<std::underlying_type_t<T>>(V));
}

enum class ThemeCol_Contact {
	request_incoming,
	request_outgoing,

	avatar_online_direct,
	avatar_online_cloud,
	avatar_offline,

	unread,
	unread_muted,

	icon_backdrop,

	ft_have_all,
	ft_base,

	request_panel_background,
	message_warning_text,
	message_quoted_text,
	message_highlight_self,
	message_highlight_private,

	delivery_partial,
	delivery_full,
	read_partial,
	read_full,

	crop_button,
	crop_button_active,

	directory_background_even,
	directory_background_odd,

	stream_source_connection,
	stream_sink_connection,
	stream_default_connection,
};

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
		constexpr auto key = entt::type_hash<entt::tag<static_cast<entt::id_type>(V)>>::value();
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
		constexpr auto key = entt::type_hash<entt::tag<static_cast<entt::id_type>(V)>>::value();
		const auto it = colors.find(key);
		if (it != colors.end()) {
			return it->second;
		} else {
			return {}; // TODO: pink as default?
		}
	}

	template<auto V>
	std::string_view getColorName(void) const {
		constexpr auto key = entt::type_hash<entt::tag<static_cast<entt::id_type>(V)>>::value();
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

