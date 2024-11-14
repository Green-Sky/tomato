#pragma once

class ToxClient;
struct ConfigModelI;
struct ToxPrivateI;

class ToxUIUtils {
	bool _show_add_friend_window {false};
	bool _show_add_group_window {false};

	ToxClient& _tc;
	ConfigModelI& _conf;
	ToxPrivateI* _tp{nullptr};

	public:
		ToxUIUtils(
			ToxClient& tc,
			ConfigModelI& conf,
			ToxPrivateI* tp = nullptr
		);

		void render(void);
};

