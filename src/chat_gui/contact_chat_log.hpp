#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./texture_cache_defs.hpp"

// fwd
struct Theme;
struct FileSelector;
struct ImageViewerPopup;
struct Clipboard;

struct ContactChatLog {
	ContactStore4I& _cs;
	RegistryMessageModelI& _rmm;
	ObjectStore2& _os;
	Theme& _theme;
	ContactTextureCache& _contact_tc;
	MessageTextureCache& _msg_tc;
	BitsetTextureCache& _b_tc;
	FileSelector& _fss;
	ImageViewerPopup& _ivp;
	Clipboard& _cb;

	std::string& _text_input_buffer; // ref

	ContactHandle4 c;

	Message3Registry* msg_reg{nullptr};

	bool _show_chat_extra_info {false};
	bool _show_chat_avatar_tf {false};

	float TEXT_BASE_WIDTH {1};
	float TEXT_BASE_HEIGHT {1};

	ContactChatLog(
		ContactStore4I& cs,
		RegistryMessageModelI& rmm,
		ObjectStore2& os,
		Theme& theme,
		ContactTextureCache& contact_tc,
		MessageTextureCache& msg_tc,
		BitsetTextureCache& b_tc,
		FileSelector& fss,
		ImageViewerPopup& ivp,
		Clipboard& cb,
		std::string& text_input_buffer,
		ContactHandle4 c_
	);

	float render(bool window_focused, float time_delta, const std::vector<Contact4>* sub_contacts);

	private:
		void fadeSystem(bool window_focused, float time_delta);

		void renderMessageBodyText(Message3Registry& reg, const Message3 e);
		void renderMessageBodyFile(Message3Registry& reg, const Message3 e);
		void renderMessageExtra(Message3Registry& reg, const Message3 e);

};
