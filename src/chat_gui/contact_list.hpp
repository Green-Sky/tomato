#pragma once

#include "./texture_cache_defs.hpp"

#include "./theme.hpp"
#include "./contact_info_window.hpp" // tmp

#include <solanaceae/contact/fwd.hpp>

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
	ContactStore4Impl& cs,
	ContactRegistry4& cr,
	RegistryMessageModelI& rmm,
	const Theme& th,
	ContactTextureCache& contact_tc,
	const contact_const_runtime_view& view,
	ContactInfoWindows& ciw,

	// in/out
	ContactHandle4& selected_c
);

