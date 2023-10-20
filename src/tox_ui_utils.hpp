#pragma once

struct ToxI;
struct ConfigModelI;

class ToxUIUtils {
	bool _show_add_friend_window {false};
	bool _show_add_group_window {false};

	ToxI& _t;
	ConfigModelI& _conf;

	public:
		ToxUIUtils(
			ToxI& t,
			ConfigModelI& conf
		);

		void render(void);
};

