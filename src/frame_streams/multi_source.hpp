#pragma once

#include "./locked_frame_stream.hpp"

#include <cassert>

// implements a stream that pushes to all sub streams
template<typename FrameType, typename SubStreamType = LockedFrameStream2<FrameType>>
struct FrameStream2MultiSource : public FrameStream2SourceI<FrameType>, public FrameStream2I<FrameType> {
	using sub_stream_type_t = SubStreamType;

	// pointer stability
	std::vector<std::shared_ptr<SubStreamType>> _sub_streams;
	std::mutex _sub_stream_lock; // accessing the _sub_streams array needs to be exclusive
	// a simple lock here is ok, since this tends to be a rare operation,
	// except for the push, which is always on the same thread

	virtual ~FrameStream2MultiSource(void) {}

	// TODO: forward args instead
	std::shared_ptr<FrameStream2I<FrameType>> subscribe(void) override {
		std::lock_guard lg{_sub_stream_lock};
		return _sub_streams.emplace_back(std::make_unique<SubStreamType>());
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<FrameType>>& sub) override {
		std::lock_guard lg{_sub_stream_lock};
		for (auto it = _sub_streams.begin(); it != _sub_streams.end(); it++) {
			if (*it == sub) {
				_sub_streams.erase(it);
				return true;
			}
		}
		return false; // ?
	}

	// stream interface

	int32_t size(void) override {
		// TODO: return something sensible?
		return -1;
	}

	std::optional<FrameType> pop(void) override {
		// nope
		assert(false && "this logic is very frame type specific, provide an impl");
		return std::nullopt;
	}

	// returns true if there are readers
	bool push(const FrameType& value) override {
		std::lock_guard lg{_sub_stream_lock};
		bool have_readers{false};
		for (auto& it : _sub_streams) {
			[[maybe_unused]] auto _ = it->push(value);
			have_readers = true; // even if queue full, we still continue believing in them
			// maybe consider push return value?
		}
		return have_readers;
	}
};

