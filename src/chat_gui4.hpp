#pragma once

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/util/config_model.hpp>

#include "./chat_gui/theme.hpp"
#include "./chat_gui/texture_cache_defs.hpp"

#include "./texture_uploader.hpp"
#include "./texture_cache.hpp"
#include "./tox_avatar_loader.hpp"
#include "./message_image_loader.hpp"
#include "./bitset_image_loader.hpp"
#include "./chat_gui/file_selector.hpp"
#include "./chat_gui/send_image_popup.hpp"
#include "./chat_gui/image_viewer_popup.hpp"

#include <entt/container/dense_map.hpp>

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <memory>

class ChatGui4 : public ObjectStoreEventI {
	ConfigModelI& _conf;
	ObjectStore2& _os;
	ObjectStoreEventProviderI::SubscriptionReference _os_sr;
	RegistryMessageModelI& _rmm;
	ContactStore4I& _cs;

	ContactTextureCache& _contact_tc;
	MessageTextureCache& _msg_tc;
	BitsetImageLoader _bil;
	BitsetTextureCache _b_tc;

	Theme& _theme;

	FileSelector _fss;
	SendImagePopup _sip;
	ImageViewerPopup _ivp;

	// set to true if not hovered
	// TODO: add timer?
	bool _contact_list_sortable {false};

	// TODO: refactor this to allow multiple open contacts
	std::optional<Contact4> _selected_contact;

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
			RegistryMessageModelI& rmm,
			ContactStore4I& cs,
			TextureUploaderI& tu,
			ContactTextureCache& contact_tc,
			MessageTextureCache& msg_tc,
			Theme& theme
		);
		~ChatGui4(void);

	public:
		float render(float time_delta, bool window_hidden, bool window_focused);

	public:
		void sendFilePath(Contact4 c, std::string_view file_path);
		void sendFileList(Contact4 c, const std::vector<std::string_view>& list);

		void sendFilePath(std::string_view file_path);
		void sendFileList(const std::vector<std::string_view>& list);

	private:
		void renderMessageBodyText(Message3Registry& reg, const Message3 e);
		void renderMessageBodyFile(Message3Registry& reg, const Message3 e);
		void renderMessageExtra(Message3Registry& reg, const Message3 e);

		void renderContactList(void);
		bool renderContactListContactSmall(const Contact4 c, const bool selected) const;
		//bool renderSubContactListContact(const Contact4 c, const bool selected) const;

		void pasteFile(const char* mime_type);

	protected:
		bool onEvent(const ObjectStore::Events::ObjectUpdate&) override;
};


