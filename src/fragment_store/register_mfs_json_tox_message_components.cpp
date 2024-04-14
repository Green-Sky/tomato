#include "./register_mfs_json_tox_message_components.hpp"

#include "../json/tox_message_components.hpp"
#include "solanaceae/message3/message_serializer.hpp"

void registerMFSJsonToxMessageComponents(MessageSerializerNJ& msnj) {
	msnj.registerSerializer<Message::Components::ToxGroupMessageID>();
	msnj.registerDeserializer<Message::Components::ToxGroupMessageID>();
}

