#pragma once

class ToxClient;
struct ContactStore4I;
class ToxContactModel2;
struct ConfigModelI;
struct ToxPrivateI;

class ToxUIUtils {
	bool _show_add_friend_window {false};
	bool _show_add_group_window {false};
	bool _show_new_group_window {false};
	bool _show_dht_connect_node {false};

	ToxClient& _tc;
	ContactStore4I& _cs;
	ToxContactModel2& _tcm;
	ConfigModelI& _conf;
	ToxPrivateI* _tp {nullptr};

	char _chat_id[32*2+1] {};

	public:
		ToxUIUtils(
			ToxClient& tc,
			ContactStore4I& cs,
			ToxContactModel2& tcm,
			ConfigModelI& conf,
			ToxPrivateI* tp = nullptr
		);
		~ToxUIUtils(void);

		void render(void);
};

