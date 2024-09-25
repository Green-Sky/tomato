#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <mutex>

#include <SPSCQueue.h>

// Frames ofen consist of:
// - seq id // incremental sequential id, gaps in ids can be used to detect loss
// - data // the frame data
// eg:
//struct ExampleFrame {
	//int64_t seq_id {0};
	//std::vector<uint8_t> data;
//};

template<typename FrameType>
struct FrameStream2I {
	virtual ~FrameStream2I(void) {}

	// get number of available frames
	[[nodiscard]] virtual int32_t size(void) = 0;

	// get next frame
	// TODO: optional instead?
	// data sharing? -> no, data is copied for each fsr, if concurency supported
	[[nodiscard]] virtual std::optional<FrameType> pop(void) = 0;

	// returns true if there are readers (or we dont know)
	virtual bool push(const FrameType& value) = 0;
};

template<typename FrameType>
struct FrameStream2SourceI {
	virtual ~FrameStream2SourceI(void) {}
	[[nodiscard]] virtual std::shared_ptr<FrameStream2I<FrameType>> subscribe(void) = 0;
	virtual bool unsubscribe(const std::shared_ptr<FrameStream2I<FrameType>>& sub) = 0;
};

template<typename FrameType>
struct FrameStream2SinkI {
	virtual ~FrameStream2SinkI(void) {}
	[[nodiscard]] virtual std::shared_ptr<FrameStream2I<FrameType>> subscribe(void) = 0;
	virtual bool unsubscribe(const std::shared_ptr<FrameStream2I<FrameType>>& sub) = 0;
};

// needs count frames queue size
// having ~1-2sec buffer size is often sufficent
template<typename FrameType>
struct QueuedFrameStream2 : public FrameStream2I<FrameType> {
	using frame_type = FrameType;

	rigtorp::SPSCQueue<FrameType> _queue;

	// discard values if queue full
	// will block if not lossy and full on push
	const bool _lossy {true};

	explicit QueuedFrameStream2(size_t queue_size, bool lossy = true) : _queue(queue_size), _lossy(lossy) {}

	int32_t size(void) override {
		return _queue.size();
	}

	std::optional<FrameType> pop(void) override {
		auto* ret_ptr = _queue.front();
		if (ret_ptr == nullptr) {
			return std::nullopt;
		}

		// move away
		FrameType ret = std::move(*ret_ptr);

		_queue.pop();

		return ret;
	}

	bool push(const FrameType& value) override {
		if (_lossy) {
			[[maybe_unused]] auto _ = _queue.try_emplace(value);
			// TODO: maybe return ?
		} else {
			_queue.push(value);
		}
		return true;
	}
};

// implements a stream that pushes to all sub streams
// release all streams before destructing! // TODO: improve lifetime here, maybe some shared semaphore?
template<typename FrameType, typename SubStreamType = QueuedFrameStream2<FrameType>>
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
		// TODO: args???
		size_t queue_size = 8;
		bool lossy = true;

		std::lock_guard lg{_sub_stream_lock};
		return _sub_streams.emplace_back(std::make_unique<SubStreamType>(queue_size, lossy));
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

