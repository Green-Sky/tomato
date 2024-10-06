#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./image_loader.hpp"

namespace Message::Components {

	struct TagNotImage {};

} // Message::Components

// adds metadata to file messages
class MediaMetaInfoLoader : public RegistryMessageModelEventI {
	protected:
		RegistryMessageModelI& _rmm;

		std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

		void handleMessage(const Message3Handle& m);

	public:
		MediaMetaInfoLoader(RegistryMessageModelI& rmm);
		virtual ~MediaMetaInfoLoader(void);

	protected: // rmm
		bool onEvent(const Message::Events::MessageConstruct& e) override;
		bool onEvent(const Message::Events::MessageUpdated& e) override;

	//protected: // os
	//    bool onEvent(const ObjectStore::Events::ObjectConstruct& e) override;
	// should listen on update
	//    bool onEvent(const ObjectStore::Events::ObjectUpdate& e) override;
};

