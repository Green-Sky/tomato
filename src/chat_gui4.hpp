#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/util/config_model.hpp>

#include "./chat_gui/theme.hpp"

#include "./texture_uploader.hpp"
#include "./texture_cache.hpp"
#include "./tox_avatar_loader.hpp"
#include "./message_image_loader.hpp"
#include "./chat_gui/file_selector.hpp"
#include "./chat_gui/send_image_popup.hpp"

#include <entt/container/dense_map.hpp>

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <memory>

using ContactTextureCache = TextureCache<void*, Contact3, ToxAvatarLoader>;
using MessageTextureCache = TextureCache<void*, Message3Handle, MessageImageLoader>;

class ChatGui4 {
	ConfigModelI& _conf;
	ObjectStore2& _os;
	RegistryMessageModel& _rmm;
	Contact3Registry& _cr;

	ContactTextureCache& _contact_tc;
	MessageTextureCache& _msg_tc;

	Theme& _theme;

	FileSelector _fss;
	SendImagePopup _sip;

	// TODO: refactor this to allow multiple open contacts
	std::optional<Contact3> _selected_contact;

	// TODO: per contact
	std::string _text_input_buffer;

	bool _show_chat_extra_info {false};
	bool _show_chat_avatar_tf {false};

	float TEXT_BASE_WIDTH {1};
	float TEXT_BASE_HEIGHT {1};

	// mimetype -> data
	entt::dense_map<std::string, std::shared_ptr<std::vector<uint8_t>>> _set_clipboard_data;
	std::mutex _set_clipboard_data_mutex; // might be called out of order
	friend const void* clipboard_callback(void* userdata, const char* mime_type, size_t* size);
	void setClipboardData(std::vector<std::string> mime_types, std::shared_ptr<std::vector<uint8_t>>&& data);

	public:
		ChatGui4(
			ConfigModelI& conf,
			ObjectStore2& os,
			RegistryMessageModel& rmm,
			Contact3Registry& cr,
			TextureUploaderI& tu,
			ContactTextureCache& contact_tc,
			MessageTextureCache& msg_tc,
			Theme& theme
		);
		~ChatGui4(void);

	public:
		float render(float time_delta);

	public:
		void sendFilePath(const char* file_path);

	private:
		void renderMessageBodyText(Message3Registry& reg, const Message3 e);
		void renderMessageBodyFile(Message3Registry& reg, const Message3 e);
		void renderMessageExtra(Message3Registry& reg, const Message3 e);

		void renderContactList(void);
		bool renderContactListContactSmall(const Contact3 c, const bool selected) const;
		//bool renderSubContactListContact(const Contact3 c, const bool selected) const;

		void pasteFile(const char* mime_type);
};


