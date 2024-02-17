#include "./register_mfs_json_message_components.hpp"

#include "./message_serializer.hpp"
#include "../json/tox_message_components.hpp"

void registerMFSJsonToxMessageComponents(MessageSerializerCallbacks& msc) {
	msc.registerSerializerJson<Message::Components::ToxGroupMessageID>();
	msc.registerDeSerializerJson<Message::Components::ToxGroupMessageID>();
}

