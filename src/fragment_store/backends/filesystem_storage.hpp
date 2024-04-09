#pragma once

#include "../object_store.hpp"

namespace backend {

struct FilesystemStorage : public StorageBackendI {
	FilesystemStorage(ObjectStore2& os);
	~FilesystemStorage(void);

	bool write(Object o, std::function<write_to_storage_fetch_data_cb>& data_cb) override;
	bool read(Object o, std::function<read_from_storage_put_data_cb>& data_cb) override;

	//// convenience function
	//nlohmann::json loadFromStorageNJ(FragmentID fid);
};

} // backend
