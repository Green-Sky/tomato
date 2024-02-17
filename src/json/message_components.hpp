#pragma once

#include <solanaceae/util/utils.hpp>

#include <solanaceae/message3/components.hpp>

#include <nlohmann/json.hpp>

namespace Message::Components {

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Timestamp, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TimestampProcessed, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TimestampWritten, ts)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ContactFrom, c)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ContactTo, c)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Read, ts)
	// TODO: SyncedBy
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessageText, text)

	namespace Transfer {
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileInfo::FileDirEntry, file_name, file_size)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileInfo, file_list, total_size)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileInfoLocal, file_list)
	} // Transfer

} // Message::Components

