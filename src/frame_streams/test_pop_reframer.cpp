#include <iostream>

#include "./audio_stream_pop_reframer.hpp"
#include "./locked_frame_stream.hpp"

#include <cassert>

int main(void) {
	{ // pump perfect
		AudioStreamPopReFramer<LockedFrameStream2<AudioFrame2>> stream;
		stream._frame_length_ms = 10;

		AudioFrame2 f1 {
			48'000,
			1,
			{},
		};
		f1.buffer = std::vector<int16_t>(
			// perfect size
			stream._frame_length_ms * f1.sample_rate * f1.channels / 1000,
			0
		);

		{ // fill with sequential value
			int16_t seq = 0;
			for (auto& v : std::get<std::vector<int16_t>>(f1.buffer)) {
				v = seq++;
			}
		}

		stream.push(f1);

		auto ret_opt = stream.pop();
		assert(ret_opt);

		auto& ret = ret_opt.value();
		assert(ret.sample_rate == f1.sample_rate);
		assert(ret.channels == f1.channels);
		assert(ret.getSpan().size == f1.getSpan().size);
		{
			int16_t seq = 0;
			for (const auto v : ret.getSpan()) {
				assert(v == seq++);
			}
		}
	}

	{ // pump half
		AudioStreamPopReFramer<LockedFrameStream2<AudioFrame2>> stream;
		stream._frame_length_ms = 10;

		AudioFrame2 f1 {
			48'000,
			1,
			{},
		};
		f1.buffer = std::vector<int16_t>(
			// perfect size
			(stream._frame_length_ms * f1.sample_rate * f1.channels / 1000) / 2,
			0
		);
		AudioFrame2 f2 {
			48'000,
			1,
			{},
		};
		f2.buffer = std::vector<int16_t>(
			// perfect size
			(stream._frame_length_ms * f1.sample_rate * f1.channels / 1000) / 2,
			0
		);

		{ // fill with sequential value
			int16_t seq = 0;
			for (auto& v : std::get<std::vector<int16_t>>(f1.buffer)) {
				v = seq++;
			}
			for (auto& v : std::get<std::vector<int16_t>>(f2.buffer)) {
				v = seq++;
			}
		}

		stream.push(f1);

		{
			auto ret_opt = stream.pop();
			assert(!ret_opt);
		}

		// push the other half
		stream.push(f2);

		auto ret_opt = stream.pop();
		assert(ret_opt);

		auto& ret = ret_opt.value();
		assert(ret.sample_rate == f1.sample_rate);
		assert(ret.channels == f1.channels);
		assert(ret.getSpan().size == stream._frame_length_ms * f1.sample_rate * f1.channels / 1000);
		{
			int16_t seq = 0;
			for (const auto v : ret.getSpan()) {
				assert(v == seq++);
			}
		}
	}

	{ // pump double
		AudioStreamPopReFramer<LockedFrameStream2<AudioFrame2>> stream;
		stream._frame_length_ms = 20;

		AudioFrame2 f1 {
			48'000,
			2,
			{},
		};
		f1.buffer = std::vector<int16_t>(
			// perfect size
			(stream._frame_length_ms * f1.sample_rate * f1.channels / 1000) * 2,
			0
		);
		{ // fill with sequential value
			int16_t seq = 0;
			for (auto& v : std::get<std::vector<int16_t>>(f1.buffer)) {
				v = seq++;
			}
		}

		stream.push(f1);

		// pop 2x
		int16_t seq = 0;
		{
			auto ret_opt = stream.pop();
			assert(ret_opt);

			auto& ret = ret_opt.value();
			assert(ret.sample_rate == f1.sample_rate);
			assert(ret.channels == f1.channels);
			assert(ret.getSpan().size == stream._frame_length_ms * f1.sample_rate * f1.channels / 1000);
			for (const auto v : ret.getSpan()) {
				assert(v == seq++);
			}
		}

		{
			auto ret_opt = stream.pop();
			assert(ret_opt);

			auto& ret = ret_opt.value();
			assert(ret.sample_rate == f1.sample_rate);
			assert(ret.channels == f1.channels);
			assert(ret.getSpan().size == stream._frame_length_ms * f1.sample_rate * f1.channels / 1000);
			for (const auto v : ret.getSpan()) {
				assert(v == seq++);
			}
		}
	}

	return 0;
}
