#include "./object_store.hpp"
#include "./backends/filesystem_storage.hpp"
#include "./meta_components.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>

#include <cassert>

int main(int argc, const char** argv) {
	if (argc != 3) {
		std::cerr << "wrong paramter count, do " << argv[0] << " <input_folder> <output_folder>\n";
		return 1;
	}

	if (!std::filesystem::is_directory(argv[1])) {
		std::cerr << "input folder is no folder\n";
	}

	std::filesystem::create_directories(argv[2]);

	// we are going to use 2 different OS for convineance, but could be done with 1 too
	ObjectStore2 os_src;
	ObjectStore2 os_dst;

	backend::FilesystemStorage fsb_src(os_src, argv[1]);
	backend::FilesystemStorage fsb_dst(os_dst, argv[2]);

	// add message fragment store too (adds meta?)

	// hookup events
	struct EventListener : public ObjectStoreEventI {
		ObjectStore2& _os_src;
		backend::FilesystemStorage& _fsb_src;

		ObjectStore2& _os_dst;
		backend::FilesystemStorage& _fsb_dst;

		EventListener(
			ObjectStore2& os_src,
			backend::FilesystemStorage& fsb_src,
			ObjectStore2& os_dst,
			backend::FilesystemStorage& fsb_dst
		) :
			_os_src(os_src),
			_fsb_src(fsb_src),
			_os_dst(os_dst),
			_fsb_dst(fsb_dst)
		{
			_os_src.subscribe(this, ObjectStore_Event::object_construct);
			_os_src.subscribe(this, ObjectStore_Event::object_update);
		}

		protected: // os
			bool onEvent(const ObjectStore::Events::ObjectConstruct& e) override {
				assert(e.e.all_of<ObjComp::Ephemeral::MetaFileType>());
				assert(e.e.all_of<ObjComp::ID>());
				auto oh = _fsb_dst.newObject(e.e.get<ObjComp::Ephemeral::MetaFileType>().type, ByteSpan{e.e.get<ObjComp::ID>().v});

				if (!static_cast<bool>(oh)) {
					// already exists
					return false;
				}

				{ // sync meta
					// some hardcoded ehpemeral (besides mft/id)
					oh.emplace_or_replace<ObjComp::Ephemeral::MetaEncryptionType>(e.e.get_or_emplace<ObjComp::Ephemeral::MetaEncryptionType>());
					oh.emplace_or_replace<ObjComp::Ephemeral::MetaCompressionType>(e.e.get_or_emplace<ObjComp::Ephemeral::MetaCompressionType>());

					// serializable
					for (const auto& [type, fn] : _fsb_src._sc._serl_json) {
						//if (!e.e.registry()->storage(type)->contains(e.e)) {
							//continue;
						//}

						// this is hacky but we serialize and then deserialize the component
						// raw copy might be better in the future
						nlohmann::json tmp_j;
						if (fn(e.e, tmp_j)) {
							_fsb_dst._sc._deserl_json.at(type)(oh, tmp_j);
						}
					}
				}

				// read src and write dst data
				static_cast<StorageBackendI&>(_fsb_dst).write(oh, ByteSpan{});

				return false;
			}

			bool onEvent(const ObjectStore::Events::ObjectUpdate&) override {
				std::cerr << "Update called\n";
				assert(false);
				return false;
			}
	} el {
		os_src,
		fsb_src,
		os_dst,
		fsb_dst,
	};

	// perform scan (which triggers events)
	fsb_dst.scanAsync(); // fill with existing?
	fsb_src.scanAsync(); // the scan

	// done
	return 0;
}

