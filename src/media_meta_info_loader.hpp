#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./image_loader.hpp"

namespace Message::Components {

	struct FrameDims {
		uint32_t width {0};
		uint32_t height {0};
	};

	struct TagNotImage {};

} // Message::Components

// adds metadata to file messages
class MediaMetaInfoLoader : public RegistryMessageModelEventI {
	protected:
		RegistryMessageModel& _rmm;

		std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	public:
		MediaMetaInfoLoader(RegistryMessageModel& rmm);
		virtual ~MediaMetaInfoLoader(void);

	protected: // rmm
		bool onEvent(const Message::Events::MessageConstruct& e) override;
		bool onEvent(const Message::Events::MessageUpdated& e) override;
};

