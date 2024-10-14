#pragma once

#include <solanaceae/contact/contact_model3.hpp>

#include "./texture_cache_defs.hpp"

// fwd
struct Theme;

class ContactTreeWindow {
	bool _show_window {true};

	Contact3Registry& _cr;

	Theme& _theme;
	ContactTextureCache& _ctc;

	public:
		ContactTreeWindow(Contact3Registry& cr, Theme& theme, ContactTextureCache& ctc);

		void render(void);
};

