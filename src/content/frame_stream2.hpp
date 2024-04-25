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
struct FrameStream2Reader {
	// get number of available frames
	virtual int32_t getSize(void) = 0;

	// get next frame
	// TODO: optional instead?
	// data sharing? -> no, data is copied for each fsr, if concurency supported
	virtual std::optional<FrameType>  getNext(void) = 0;
};

// needs count frames queue size
// having ~1-2sec buffer size is often sufficent
template<typename FrameType>
struct QueuedFrameStream2Reader : public FrameStream2Reader<FrameType> {
	using frame_type = FrameType;

	rigtorp::SPSCQueue<FrameType> _queue;

	explicit QueuedFrameStream2Reader(size_t queue_size) : _queue(queue_size) {}

	[[nodiscard]] int32_t getSize(void) override {
		return _queue.size();
	}

	[[nodiscard]] std::optional<FrameType> getNext(void) override {
		auto* ret_ptr = _queue.front();
		if (ret_ptr == nullptr) {
			return std::nullopt;
		}

		// move away
		FrameType ret = std::move(*ret_ptr);

		_queue.pop();

		return ret;
	}
};

template<typename FrameType>
struct MultiplexedQueuedFrameStream2Writer {
	using ReaderType = QueuedFrameStream2Reader<FrameType>;

	// TODO: expose
	const size_t _queue_size {10};

	// pointer stability
	std::vector<std::unique_ptr<ReaderType>> _readers;
	std::mutex _readers_lock; // accessing the _readers array needs to be exclusive
	// a simple lock here is ok, since this tends to be a rare operation,
	// except for the push, which is always on the same thread

	ReaderType* aquireReader(void) {
		std::lock_guard lg{_readers_lock};
		return _readers.emplace_back(std::make_unique<ReaderType>(_queue_size)).get();
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

	// returns true if there are readers
	bool pushValue(const FrameType& value) {
		std::lock_guard lg{_readers_lock};
		bool have_readers{false};
		for (auto& it : _readers) {
			[[maybe_unused]] auto _ = it->_queue.try_emplace(value);
			have_readers = true; // even if queue full, we still continue believing in them
		}
		return have_readers;
	}
};

