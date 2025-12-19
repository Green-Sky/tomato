#pragma once

#include "./frame_stream2.hpp"

#include <mutex>
#include <deque>

// threadsafe queue frame stream
// protected by a simple mutex lock
// prefer lockless queue implementations, when available
template<typename FrameType>
struct LockedFrameStream2 : public FrameStream2I<FrameType> {
	std::mutex _lock;

	std::deque<FrameType> _frames;

	~LockedFrameStream2(void) {}

	int32_t size(void) { return -1; }

	std::optional<FrameType> pop(void) {
		std::lock_guard lg{_lock};

		if (_frames.empty()) {
			return std::nullopt;
		}

		FrameType new_frame = std::move(_frames.front());
		_frames.pop_front();

		return new_frame;
	}

	bool push(const FrameType& value) {
		std::lock_guard lg{_lock};

		if (_frames.size() > 1024) {
			return false; // hard limit
		}

		_frames.push_back(value);

		return true;
	}
};

