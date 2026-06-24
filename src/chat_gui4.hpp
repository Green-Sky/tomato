#pragma once

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/contact/contact_store_impl.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/util/config_model.hpp>

#include "./chat_gui/theme.hpp"
#include "./chat_gui/texture_cache_defs.hpp"

#include "./texture_uploader.hpp"
#include "./bitset_image_loader.hpp"
#include "./sdl_clipboard_utils.hpp"
#include "./chat_gui/file_selector.hpp"
#include "./chat_gui/image_viewer_popup.hpp"
#include "./chat_gui/contact_info_window.hpp"
#include "./chat_gui/contact_list_sorter.hpp"
#include "./chat_gui/contact_window.hpp"

#include <entt/container/dense_map.hpp>

#include <vector>
#include <memory>
#include <stack>

class ChatGui4 : public ObjectStoreEventI {
	ConfigModelI& _conf;
	ObjectStore2& _os;
	ObjectStoreEventProviderI::SubscriptionReference _os_sr;
	RegistryMessageModelI& _rmm;
	ContactStore4Impl& _cs;

	TextureUploaderI& _tu;
	ContactTextureCache& _contact_tc;
	MessageTextureCache& _msg_tc;
	BitsetImageLoader _bil;
	BitsetTextureCache _b_tc;

	Theme& _theme;

	FileSelector _fss;
	ImageViewerPopup _ivp;
	ContactInfoWindows _ciw;
	ContactListSorter _cls;
	Clipboard _cb;

	// set to true if not hovered
	bool _contact_list_sortable {false};

	// TODO: refactor this to allow multiple open contacts
	std::stack<std::unique_ptr<ContactWindow>> _contact_stack;
	ContactHandle4 _next_contact;

	float TEXT_BASE_WIDTH {1};
	float TEXT_BASE_HEIGHT {1};

	public:
		ChatGui4(
			ConfigModelI& conf,
			ObjectStore2& os,
			RegistryMessageModelI& rmm,
			ContactStore4Impl& cs,
			TextureUploaderI& tu,
			ContactTextureCache& contact_tc,
			MessageTextureCache& msg_tc,
			Theme& theme
		);
		~ChatGui4(void);

	public:
		float render(float time_delta, bool window_hidden, bool window_focused);

	public:
		void sendFilePath(std::string_view file_path);
		void sendFileList(const std::vector<std::string_view>& list);

	private:
		void renderChatFilesTab(Contact4 c);

		void renderContactList(void);
		bool renderContactListContactSmall(const Contact4 c, const bool selected) const;
		//bool renderSubContactListContact(const Contact4 c, const bool selected) const;

	protected:
		bool onEvent(const ObjectStore::Events::ObjectUpdate&) override;
};


