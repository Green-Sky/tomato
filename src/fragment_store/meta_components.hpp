#pragma once

#include "./types.hpp"
#include "./object_store.hpp"

#include <vector>
#include <string>
#include <cstdint>

namespace ObjectStore::Components {

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

		// TODO: move, backend specific
		struct MetaFileType {
			::MetaFileType type {::MetaFileType::TEXT_JSON};
		};

		struct MetaEncryptionType {
			Encryption enc {Encryption::NONE};
		};

		struct MetaCompressionType {
			Compression comp {Compression::NONE};
		};

		struct Backend {
			// TODO: shared_ptr instead??
			StorageBackendI* ptr;
		};

		// excluded from file meta
		// TODO: move to backend specific
		struct FilePath {
			// contains store path, if any
			std::string path;
		};

		// TODO: seperate into remote and local?
		// (remote meaning eg. the file on disk was changed by another program)
		struct DirtyTag {};

	} // Ephemeral

} // Components

// shortened to save bytes (until I find a way to save by ID in msgpack)
namespace ObjComp = ObjectStore::Components;

// old names from frag era
namespace Fragment::Components {
	//struct ID {};
	//struct DataEncryptionType {};
	//struct DataCompressionType {};
	struct ID : public ObjComp::ID {};
	struct DataEncryptionType : public ObjComp::DataEncryptionType {};
	struct DataCompressionType : public ObjComp::DataCompressionType {};
}
namespace FragComp = Fragment::Components;

#include "./meta_components_id.inl"

