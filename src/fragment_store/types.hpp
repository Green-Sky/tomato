#pragma once

#include <cstdint>

enum class Encryption : uint8_t {
	NONE = 0x00,
};
enum class Compression : uint8_t {
	NONE = 0x00,
};
enum class MetaFileType : uint8_t {
	TEXT_JSON,
	//BINARY_ARB,
	BINARY_MSGPACK,
};

