#pragma once

#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/util/config_model.hpp>

#include "./texture_uploader.hpp"
#include "./texture_cache.hpp"
#include "./tox_avatar_loader.hpp"
#include "./message_image_loader.hpp"
#include "./file_selector.hpp"
#include "./send_image_popup.hpp"

#include <vector>
#include <set>

class ChatGui4 {
	ConfigModelI& _conf;
	RegistryMessageModel& _rmm;
	Contact3Registry& _cr;

	ToxAvatarLoader _tal;
	TextureCache<void*, Contact3, ToxAvatarLoader> _contact_tc;
	MessageImageLoader _mil;
	TextureCache<void*, Message3Handle, MessageImageLoader> _msg_tc;

	FileSelector _fss;
	SendImagePopup _sip;

	std::optional<Contact3> _selected_contact;

	// TODO: per contact
	std::string _text_input_buffer;

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

	public:
		bool any_unread {false};

	private:
		void renderMessageBodyText(Message3Registry& reg, const Message3 e);
		void renderMessageBodyFile(Message3Registry& reg, const Message3 e);
		void renderMessageExtra(Message3Registry& reg, const Message3 e);

		void renderContactList(void);
		bool renderContactListContactBig(const Contact3 c, const bool selected);
		bool renderContactListContactSmall(const Contact3 c, const bool selected) const;
		bool renderSubContactListContact(const Contact3 c, const bool selected) const;
};


