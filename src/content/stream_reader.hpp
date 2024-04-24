#pragma once

#include <solanaceae/util/span.hpp>

// most media that can be counted as "stream" comes in packets/frames/messages
// so this class provides an interface for ideal async fetching of frames
struct RawFrameStreamReaderI {
	// return the number of ready frames in cache
	// returns -1 if unknown
	virtual int64_t have(void) = 0;

	// get next frame, empty if none
	virtual ByteSpan getNext(void) = 0;
};

