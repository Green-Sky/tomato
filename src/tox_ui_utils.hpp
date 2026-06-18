#pragma once

#include <solanaceae/contact/fwd.hpp>

#include <string>

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

	bool _open_group_self_name {false};
	bool _open_group_password {false};
	bool _open_group_topic {false};

	ToxClient& _tc;
	ContactStore4I& _cs;
	ToxContactModel2& _tcm;
	ConfigModelI& _conf;
	ToxPrivateI* _tp {nullptr};

	char _chat_id[32*2+1] {};

	std::string _gsn_name;
	Contact4 _gsn_group;
	Contact4 _gsn_self;

	std::string _gp_password;
	bool _gp_show_password {false};
	Contact4 _gp_group;

	std::string _gt_topic;
	Contact4 _gt_group;

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

		void openGroupSelfName(Contact4 group_v, Contact4 self_v);
		void openGroupPassword(Contact4 group_v);
		void openGroupTopic(Contact4 group_v);
};

