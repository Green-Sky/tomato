#include "./file2_stack.hpp"

#include <solanaceae/file/file2_std.hpp>
#include "./file2_zstd.hpp"

#include <cassert>

#include <iostream>

// add enc and comp file layers
// assumes a file is already in the stack
bool buildStackRead(std::stack<std::unique_ptr<File2I>>& file_stack, Encryption encryption, Compression compression) {
	assert(!file_stack.empty());

	// TODO: decrypt here
	assert(encryption == Encryption::NONE);

	// add layer based on enum
	if (compression == Compression::ZSTD) {
		file_stack.push(std::make_unique<File2ZSTDR>(*file_stack.top().get()));
	} else {
		// TODO: make error instead
		assert(compression == Compression::NONE);
	}

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: file failed to add " << (int)compression << " decompression layer\n";
		return false;
	}

	return true;
}

// do i need this?
std::stack<std::unique_ptr<File2I>> buildFileStackRead(std::string_view file_path, Encryption encryption, Compression compression) {
	std::stack<std::unique_ptr<File2I>> file_stack;
	file_stack.push(std::make_unique<File2RFile>(file_path));

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: opening file for reading '" << file_path << "'\n";
		return {};
	}

	if (!buildStackRead(file_stack, encryption, compression)) {
		std::cerr << "FS error: file failed to add layers for '" << file_path << "'\n";
		return {};
	}

	return file_stack;
}

// add enc and comp file layers
// assumes a file is already in the stack
bool buildStackWrite(std::stack<std::unique_ptr<File2I>>& file_stack, Encryption encryption, Compression compression) {
	assert(!file_stack.empty());

	// TODO: encrypt here
	assert(encryption == Encryption::NONE);

	// add layer based on enum
	if (compression == Compression::ZSTD) {
		file_stack.push(std::make_unique<File2ZSTDW>(*file_stack.top().get()));
	} else {
		// TODO: make error instead
		assert(compression == Compression::NONE);
	}

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: file failed to add " << (int)compression << " compression layer\n";
		return false;
	}

	return true;
}

// do i need this?
std::stack<std::unique_ptr<File2I>> buildFileStackWrite(std::string_view file_path, Encryption encryption, Compression compression) {
	std::stack<std::unique_ptr<File2I>> file_stack;
	file_stack.push(std::make_unique<File2WFile>(file_path));

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: opening file for writing '" << file_path << "'\n";
		return {};
	}

	if (!buildStackWrite(file_stack, encryption, compression)) {
		std::cerr << "FS error: file failed to add layers for '" << file_path << "'\n";
		return {};
	}

	return file_stack;
}

