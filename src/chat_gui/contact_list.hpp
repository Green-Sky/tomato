#pragma once

#include "./texture_cache_defs.hpp"

#include "./theme.hpp"

#include <solanaceae/contact/fwd.hpp>

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
	const ContactHandle4 c,
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
	const ContactHandle4 c,
	int line_height = 3,
	const bool unread = false,
	const bool selectable = false,
	const bool selected = false
);

using contact_sparse_set = entt::basic_sparse_set<Contact4>;
using contact_runtime_view = entt::basic_runtime_view<contact_sparse_set>;
using contact_const_runtime_view = entt::basic_runtime_view<const contact_sparse_set>;

// returns true if contact was selected
bool renderContactList(
	ContactRegistry4& cr,
	RegistryMessageModelI& rmm,
	const Theme& th,
	ContactTextureCache& contact_tc,
	const contact_const_runtime_view& view,

	// in/out
	ContactHandle4& selected_c
);

