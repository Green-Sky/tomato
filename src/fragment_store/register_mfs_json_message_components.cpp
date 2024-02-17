#include "./register_mfs_json_message_components.hpp"

#include "./message_serializer.hpp"
#include "../json/message_components.hpp"

void registerMFSJsonMessageComponents(MessageSerializerCallbacks& msc) {
	msc.registerSerializerJson<Message::Components::Timestamp>();
	msc.registerDeSerializerJson<Message::Components::Timestamp>();
	msc.registerSerializerJson<Message::Components::TimestampProcessed>();
	msc.registerDeSerializerJson<Message::Components::TimestampProcessed>();
	msc.registerSerializerJson<Message::Components::TimestampWritten>();
	msc.registerDeSerializerJson<Message::Components::TimestampWritten>();
	msc.registerSerializerJson<Message::Components::ContactFrom>();
	msc.registerDeSerializerJson<Message::Components::ContactFrom>();
	msc.registerSerializerJson<Message::Components::ContactTo>();
	msc.registerDeSerializerJson<Message::Components::ContactTo>();
	msc.registerSerializerJson<Message::Components::TagUnread>();
	msc.registerDeSerializerJson<Message::Components::TagUnread>();
	msc.registerSerializerJson<Message::Components::Read>();
	msc.registerDeSerializerJson<Message::Components::Read>();
	msc.registerSerializerJson<Message::Components::MessageText>();
	msc.registerDeSerializerJson<Message::Components::MessageText>();
	msc.registerSerializerJson<Message::Components::TagMessageIsAction>();
	msc.registerDeSerializerJson<Message::Components::TagMessageIsAction>();

	// files
	//_sc.registerSerializerJson<Message::Components::Transfer::FileID>()
	//_sc.registerSerializerJson<Message::Components::Transfer::FileInfo>();
	//_sc.registerDeSerializerJson<Message::Components::Transfer::FileInfo>();
	//_sc.registerSerializerJson<Message::Components::Transfer::FileInfoLocal>();
	//_sc.registerDeSerializerJson<Message::Components::Transfer::FileInfoLocal>();
	//_sc.registerSerializerJson<Message::Components::Transfer::TagHaveAll>();
	//_sc.registerDeSerializerJson<Message::Components::Transfer::TagHaveAll>();
}

