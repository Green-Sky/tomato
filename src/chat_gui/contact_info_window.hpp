#pragma once

#include <solanaceae/contact/fwd.hpp>

// fwd
struct ContactStore4Impl;

#include <vector>

struct ContactInfoWindows {
	ContactStore4Impl& _cs;

	struct Window {
		Contact4 c;
		bool advanced {false};
		bool verbose {false};

		Window(Contact4 _c) : c(_c) {}
	};
	std::vector<Window> _windows;

	public:
		ContactInfoWindows(ContactStore4Impl& cs);
		~ContactInfoWindows(void);

		// opens contact info window, if not already open
		void open(Contact4 c);

		void close(Contact4 c);

		// call this each frame
		void render(void);
};
