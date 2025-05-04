#pragma once

#include <solanaceae/object_store/object_store.hpp>

namespace Backends {

struct STDFS : public StorageBackendIMeta, public StorageBackendIFile2 {
	ObjectStore2& _os;

	STDFS(
		ObjectStore2& os
	);
	~STDFS(void);

	ObjectHandle newObject(ByteSpan id, bool throw_construct = true) override;

	// TODO: interface?
	bool attach(Object ov);

	std::unique_ptr<File2I> file2(Object o, FILE2_FLAGS flags) override;
};

} // Backends

