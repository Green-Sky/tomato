#pragma once

#include <string>

class ToxClient;
struct ConfigModelI;
struct ToxPrivateI;
class ToxContactModel2;

class ToxUIUtils {
	bool _show_add_friend_window {false};
	bool _show_add_group_window {false};
	bool _show_new_group_window {false};

	ToxClient& _tc;
	ToxContactModel2& _tcm;
	ConfigModelI& _conf;
	ToxPrivateI* _tp {nullptr};

	char _chat_id[32*2+1] {};

	public:
		ToxUIUtils(
			ToxClient& tc,
			ToxContactModel2& tcm,
			ConfigModelI& conf,
			ToxPrivateI* tp = nullptr
		);

		void render(void);
};

