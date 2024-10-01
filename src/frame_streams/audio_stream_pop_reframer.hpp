#pragma once

#include "./audio_stream2.hpp"

// reframes audio frames to a specified size in ms
// TODO: use absolute sample count instead??
template<typename RealAudioStream>
struct AudioStreamPopReFramer : public FrameStream2I<AudioFrame2> {
	uint32_t _frame_length_ms {20};

	// gotta be careful of the multithreaded nature
	// and false(true) sharing
	uint64_t _pad0{};
	RealAudioStream _stream;
	uint64_t _pad1{};

	// dequeue?
	std::vector<int16_t> _buffer;

	uint32_t _sample_rate {48'000};
	size_t _channels {0};

	AudioStreamPopReFramer(uint32_t frame_length_ms = 20)
	: _frame_length_ms(frame_length_ms) {
	}

	AudioStreamPopReFramer(uint32_t frame_length_ms, FrameStream2I<AudioFrame2>&& stream)
	: _frame_length_ms(frame_length_ms), _stream(std::move(stream)) {
	}

	~AudioStreamPopReFramer(void) {}

	int32_t size(void) override { return -1; }

	std::optional<AudioFrame2> pop(void) override {
		auto new_in = _stream.pop();
		if (new_in.has_value()) {
			auto& new_value = new_in.value();

			// changed
			if (_sample_rate != new_value.sample_rate || _channels != new_value.channels) {
				if (_channels != 0) {
					//std::cerr << "ReFramer warning: reconfiguring, dropping buffer\n";
				}
				_sample_rate = new_value.sample_rate;
				_channels = new_value.channels;

				// buffer does not exist or config changed and we discard
				_buffer = {};
			}


			//std::cout << "new incoming frame is " << new_value.getSpan().size/new_value.channels*1000/new_value.sample_rate << "ms\n";

			auto new_span = new_value.getSpan();

			if (_buffer.empty()) {
				_buffer = {new_span.cbegin(), new_span.cend()};
			} else {
				_buffer.insert(_buffer.cend(), new_span.cbegin(), new_span.cend());
			}
		} else if (_buffer.empty()) {
			// first pop might result in invalid state
			return std::nullopt;
		}

		const size_t desired_size {_frame_length_ms * _sample_rate * _channels / 1000};

		// > threshold?
		if (_buffer.size() < desired_size) {
			return std::nullopt;
		}

		// copy data
		std::vector<int16_t> return_buffer(_buffer.cbegin(), _buffer.cbegin()+desired_size);

		// now crop buffer (meh)
		// move data from back to front
		_buffer.erase(_buffer.cbegin(), _buffer.cbegin() + desired_size);

		return AudioFrame2{
			_sample_rate,
			_channels,
			std::move(return_buffer),
		};
	}

	bool push(const AudioFrame2& value) override {
		// might be worth it to instead do the work on push
		//assert(false && "push reframing not implemented");
		// passthrough
		return _stream.push(value);
	}
};

