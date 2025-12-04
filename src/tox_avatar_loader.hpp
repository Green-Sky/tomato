#pragma once

#include <solanaceae/contact/fwd.hpp>
#include <solanaceae/object_store/fwd.hpp>

#include <solanaceae/file/file2.hpp>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

class ToxAvatarLoader {
	ContactStore4I& _cs;
	ObjectStore2& _os;

	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	ByteSpanWithOwnership loadDataFromObj(Contact4 cv);
	ByteSpanWithOwnership loadData(Contact4 cv);

	public:
		ToxAvatarLoader(ContactStore4I& cs, ObjectStore2& os);
		TextureLoaderResult load(TextureUploaderI& tu, Contact4 c, uint32_t w, uint32_t h);
};

