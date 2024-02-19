#pragma once

#include <cstdint>

enum class Encryption : uint8_t {
	NONE = 0x00,
};
enum class Compression : uint8_t {
	NONE = 0x00,
	ZSTD = 0x01,
	// TODO: zstd without magic
	// TODO: zstd meta dict
	// TODO: zstd data(message) dict
};
enum class MetaFileType : uint8_t {
	TEXT_JSON,
	BINARY_MSGPACK, // msgpacked msgpack
};

