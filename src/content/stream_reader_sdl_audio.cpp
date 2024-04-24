#include "./stream_reader_sdl_audio.hpp"

SDLAudioFrameStreamReader::SDLAudioFrameStreamReader(int32_t buffer_size) : _buffer_size(buffer_size),  _stream{nullptr, &SDL_DestroyAudioStream} {
	_buffer.resize(_buffer_size);

	const SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 48000 };

	_stream = {
		SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_CAPTURE, &spec, nullptr, nullptr),
		&SDL_DestroyAudioStream
	};
}

Span<int16_t> SDLAudioFrameStreamReader::getNextAudioFrame(void) {
	const int32_t needed_bytes = (_buffer.size() - _remaining_size) * sizeof(int16_t);
	const auto read_bytes = SDL_GetAudioStreamData(_stream.get(), _buffer.data()+_remaining_size, needed_bytes);
	if (read_bytes < 0) {
		// error
		return {};
	}
	if (read_bytes < needed_bytes) {
		// HACK: we are just assuming here that sdl never gives us half a sample!
		_remaining_size += read_bytes / sizeof(int16_t);
		return {};
	}

	_remaining_size = 0;
	return Span<int16_t>{_buffer};
}

int64_t SDLAudioFrameStreamReader::have(void) {
	return -1;
}

ByteSpan SDLAudioFrameStreamReader::getNext(void) {
	auto next_frame_span = getNextAudioFrame();
	return ByteSpan{reinterpret_cast<const uint8_t*>(next_frame_span.ptr), next_frame_span.size};
}

