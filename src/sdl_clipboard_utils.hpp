#pragma once

// returns the mimetype (c-string) of the image in the clipboard
// or nullptr, if there is none
const char* clipboardHasImage(void);
const char* clipboardHasFileList(void);

bool mimeIsImage(const char* mime_type);
bool mimeIsFileList(const char* mime_type);

// TODO: add is file

