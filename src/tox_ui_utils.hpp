#pragma once

class ToxClient;
struct ConfigModelI;

class ToxUIUtils {
	bool _show_add_friend_window {false};
	bool _show_add_group_window {false};

	ToxClient& _tc;
	ConfigModelI& _conf;

	public:
		ToxUIUtils(
			ToxClient& tc,
			ConfigModelI& conf
		);

		void render(void);
};

