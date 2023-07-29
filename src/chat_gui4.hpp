#pragma once

#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/util/config_model.hpp>

#include "./texture_uploader.hpp"
#include "./file_selector.hpp"

#include <vector>
#include <set>

class ChatGui4 {
	ConfigModelI& _conf;
	RegistryMessageModel& _rmm;
	Contact3Registry& _cr;

	FileSelector _fss;

	std::optional<Contact3> _selected_contact;

	bool _show_chat_extra_info {true};

	float TEXT_BASE_WIDTH {1};
	float TEXT_BASE_HEIGHT {1};

	public:
		ChatGui4(
			ConfigModelI& conf,
			RegistryMessageModel& rmm,
			Contact3Registry& cr,
			TextureUploaderI& tu
		);

	public:
		void render(void);

	private:
		void renderMessageBodyText(Message3Registry& reg, const Message3 e);
		void renderMessageBodyFile(Message3Registry& reg, const Message3 e);
		void renderMessageExtra(Message3Registry& reg, const Message3 e);

		void renderContactList(void);
		bool renderContactListContactBig(const Contact3 c);
		bool renderContactListContactSmall(const Contact3 c);
		bool renderSubContactListContact(const Contact3 c);
};


