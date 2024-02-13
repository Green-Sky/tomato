#pragma once

#include "./types.hpp"

#include <vector>
#include <string>
#include <cstdint>

namespace Fragment::Components {

	// TODO: is this special and should this be saved to meta or not (its already in the file name on disk)
	struct ID {
		std::vector<uint8_t> v;
	};

	struct DataEncryptionType {
		Encryption enc {Encryption::NONE};
	};

	struct DataCompressionType {
		Compression comp {Compression::NONE};
	};


	// meta that is not written to (meta-)file
	namespace Ephemeral {

		// excluded from file meta
		struct FilePath {
			// contains store path, if any
			std::string path;
		};

		// TODO: seperate into remote and local?
		// (remote meaning eg. the file on disk was changed by another program)
		struct DirtyTag {};


		// type as comp
		struct MetaFileType {
			::MetaFileType type {::MetaFileType::TEXT_JSON};
		};

		struct MetaEncryptionType {
			Encryption enc {Encryption::NONE};
		};

		struct MetaCompressionType {
			Compression comp {Compression::NONE};
		};

	} // Ephemeral

} // Components

// shortened to save bytes (until I find a way to save by ID in msgpack)
namespace FragComp = Fragment::Components;

#include "./meta_components_id.inl"

