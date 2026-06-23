#pragma once

#include "./texture_cache_defs.hpp"

#include "./contact_chat_log.hpp"
#include "./send_image_popup.hpp"

#include <functional>

// fwd
struct ContactStore4Impl;
struct ContactInfoWindows;
struct Theme;
struct FileSelector;

// there can be multiple at the same time
struct ContactWindow {
	ContactStore4Impl& _cs;
	RegistryMessageModelI& _rmm;
	ObjectStore2& _os;
	Theme& _theme;
	ContactTextureCache& _contact_tc;
	ContactInfoWindows& _ciw;
	FileSelector& _fss;
	std::function<void(ContactHandle4)> _open_chat;

	std::string _text_input_buffer;
	ContactHandle4 c;
	ContactChatLog _ccl/*{_cs, _rmm, c}*/;
	SendImagePopup _sip;


	float TEXT_BASE_WIDTH {1};
	float TEXT_BASE_HEIGHT {1};

	ContactWindow(
		ContactStore4Impl& cs,
		RegistryMessageModelI& rmm,
		ObjectStore2& os,
		Theme& theme,
		ContactTextureCache& contact_tc,
		MessageTextureCache& msg_tc,
		BitsetTextureCache& b_tc,
		ContactInfoWindows& ciw,
		FileSelector& fss,
		ImageViewerPopup& ivp,
		Clipboard& cb,
		TextureUploaderI& tu,
		std::function<void(ContactHandle4)>&& open_chat,
		ContactHandle4 c_
	);

	ContactWindow(ContactWindow&&) = delete;
	ContactWindow(const ContactWindow&&) = delete;
	ContactWindow(ContactWindow&) = delete;
	ContactWindow(const ContactWindow&) = delete;

	float render(const bool window_focused, const float time_delta, const bool child = true);

	private:
		// true if shown
		bool renderSubList(const std::vector<Contact4>* sub_contacts);
		bool renderRequest(void);

		void pasteFile(const char* mime_type);

	public: // handed down
		void sendFilePath(std::string_view file_path);
		void sendFileList(const std::vector<std::string_view>& list);
};
