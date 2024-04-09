#include "./object_store.hpp"
#include "./backends/filesystem_storage.hpp"

#include <filesystem>
#include <iostream>

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
	// perform scan (which triggers events)
	fsb_dst.scanAsync(); // fill with existing?
	fsb_src.scanAsync(); // the scan

	// done
	return 0;
}

