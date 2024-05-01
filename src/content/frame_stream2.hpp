#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <mutex>

#include "./SPSCQueue.h"

// Frames ofen consist of:
// - seq id // incremental sequential id, gaps in ids can be used to detect loss
// - data // the frame data
// eg:
//struct ExampleFrame {
	//int64_t seq_id {0};
	//std::vector<uint8_t> data;
//};

template<typename FrameType>
struct FrameStream2 {
	// get number of available frames
	[[nodiscard]] virtual int32_t size(void) = 0;

	// get next frame
	// TODO: optional instead?
	// data sharing? -> no, data is copied for each fsr, if concurency supported
	[[nodiscard]] virtual std::optional<FrameType> pop(void) = 0;

	// returns true if there are readers (or we dont know)
	virtual bool push(const FrameType& value) = 0;
};

// needs count frames queue size
// having ~1-2sec buffer size is often sufficent
template<typename FrameType>
struct QueuedFrameStream2 : public FrameStream2<FrameType> {
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

template<typename FrameType>
struct QueuedFrameStream2Multiplexer : public FrameStream2<FrameType> {
	using ReaderType = QueuedFrameStream2<FrameType>;

	// pointer stability
	std::vector<std::unique_ptr<ReaderType>> _readers;
	std::mutex _readers_lock; // accessing the _readers array needs to be exclusive
	// a simple lock here is ok, since this tends to be a rare operation,
	// except for the push, which is always on the same thread

	ReaderType* aquireReader(size_t queue_size = 10, bool lossy = true) {
		std::lock_guard lg{_readers_lock};
		return _readers.emplace_back(std::make_unique<ReaderType>(queue_size, lossy)).get();
	}

	void releaseReader(ReaderType* reader) {
		std::lock_guard lg{_readers_lock};
		for (auto it = _readers.begin(); it != _readers.end(); it++) {
			if (it->get() == reader) {
				_readers.erase(it);
				break;
			}
		}
	}

	// stream interface

	int32_t size(void) override {
		// TODO: return something sensible?
		return -1;
	}

	std::optional<FrameType> pop(void) override {
		assert(false && "tried to pop from a multiplexer");
		return std::nullopt;
	}

	// returns true if there are readers
	bool push(const FrameType& value) override {
		std::lock_guard lg{_readers_lock};
		bool have_readers{false};
		for (auto& it : _readers) {
			[[maybe_unused]] auto _ = it->push(value);
			have_readers = true; // even if queue full, we still continue believing in them
			// maybe consider push return value?
		}
		return have_readers;
	}
};

