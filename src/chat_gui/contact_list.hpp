#pragma once

#include "./texture_cache_defs.hpp"

#include "./theme.hpp"

#include <solanaceae/contact/contact_model3.hpp>

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
};

void renderAvatar(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	ImVec2 box
);

// returns true if clicked, if selectable, will highlight on hover and respect selected
// TODO: refine
// +------+
// |	  | *Name (Alias?)            [v]
// |Avatar| Satus Message <-- richpresence interface?
// |	  | user status (online/away/busy)-direct/relayed / offline <-- last text?
// +------+
bool renderContactBig(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	int line_height = 3,
	const bool unread = false,
	const bool selectable = false,
	const bool selected = false
);

