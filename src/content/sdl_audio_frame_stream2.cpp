#include "./sdl_audio_frame_stream2.hpp"
#include "SDL_audio.h"

#include <iostream>
#include <vector>

SDLAudioInputDevice::SDLAudioInputDevice(void) : _stream{nullptr, &SDL_DestroyAudioStream} {
	constexpr SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 48000 };

	_stream = {
		SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_CAPTURE, &spec, nullptr, nullptr),
		&SDL_DestroyAudioStream
	};

	if (!static_cast<bool>(_stream)) {
		std::cerr << "SDL open audio device failed!\n";
	}

	const auto audio_device_id = SDL_GetAudioStreamDevice(_stream.get());
	SDL_ResumeAudioDevice(audio_device_id);

	static constexpr size_t buffer_size {512}; // in samples
	const auto interval_ms {buffer_size/(spec.freq * 1000)};

	_thread = std::thread([this, interval_ms, spec](void) {
		while (!_thread_should_quit) {
			//static std::vector<int16_t> buffer(buffer_size);
			static AudioFrame tmp_frame {
				0, // TODO: seq
				spec.freq, spec.channels,
				std::vector<int16_t>(buffer_size)
			};

			auto& buffer = std::get<std::vector<int16_t>>(tmp_frame.buffer);
			buffer.resize(buffer_size);

			const auto read_bytes = SDL_GetAudioStreamData(
				_stream.get(),
				buffer.data(),
				buffer.size()*sizeof(int16_t)
			);
			//if (read_bytes != 0) {
				//std::cerr << "read " << read_bytes << "/" << buffer.size()*sizeof(int16_t) << " audio bytes\n";
			//}

			// no new frame yet, or error
			if (read_bytes <= 0) {
				// only sleep 1/5, we expected a frame
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval_ms/5)));
				continue;
			}

			buffer.resize(read_bytes/sizeof(int16_t)); // this might be costly?

			bool someone_listening {false};
			someone_listening = push(tmp_frame);

			if (someone_listening) {
				// double the interval on acquire
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval_ms/2)));
			} else {
				std::cerr << "i guess no one is listening\n";
				// we just sleep 32x as long, bc no one is listening
				// with the hardcoded settings, this is ~320ms
				// TODO: just hardcode something like 500ms?
				// TODO: suspend
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval_ms*32)));
			}
		}
	});
}

SDLAudioInputDevice::~SDLAudioInputDevice(void) {
	// TODO: pause audio device?
	_thread_should_quit = true;
	_thread.join();
	// TODO: what to do if readers are still present?
}

