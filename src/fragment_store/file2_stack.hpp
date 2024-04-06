#pragma once

#include "./types.hpp"

#include <solanaceae/file/file2.hpp>

#include <stack>
#include <memory>
#include <string_view>

// add enc and comp file layers
// assumes a file is already in the stack
[[nodiscard]] bool buildStackRead(std::stack<std::unique_ptr<File2I>>& file_stack, Encryption encryption, Compression compression);

// do i need this?
[[nodiscard]] std::stack<std::unique_ptr<File2I>> buildFileStackRead(std::string_view file_path, Encryption encryption, Compression compression);

// add enc and comp file layers
// assumes a file is already in the stack
[[nodiscard]] bool buildStackWrite(std::stack<std::unique_ptr<File2I>>& file_stack, Encryption encryption, Compression compression);

// do i need this?
[[nodiscard]] std::stack<std::unique_ptr<File2I>> buildFileStackWrite(std::string_view file_path, Encryption encryption, Compression compression);
