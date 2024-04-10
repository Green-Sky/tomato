#pragma once

#include "../types.hpp"
#include "../object_store.hpp"

namespace backend {

struct FilesystemStorage : public StorageBackendI {
	FilesystemStorage(ObjectStore2& os, std::string_view storage_path = "test_obj_store");
	~FilesystemStorage(void);

	// TODO: fix the path for this specific fs?
	// for now we assume a single storage path per backend (there can be multiple per type)
	std::string _storage_path;

	ObjectHandle newObject(MetaFileType mft, ByteSpan id);

	bool write(Object o, std::function<write_to_storage_fetch_data_cb>& data_cb) override;
	bool read(Object o, std::function<read_from_storage_put_data_cb>& data_cb) override;

	//// convenience function
	//nlohmann::json loadFromStorageNJ(FragmentID fid);

	void scanAsync(void);

	private:
		size_t scanPath(std::string_view path);
		void scanPathAsync(std::string path);

	private:
		// this thing needs to change and be facilitated over the OS
		// but the json serializer are specific to the backend
		SerializerCallbacks<Object> _sc;
};

} // backend
