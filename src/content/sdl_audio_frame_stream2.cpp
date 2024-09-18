#include "./sdl_audio_frame_stream2.hpp"

#include <iostream>
#include <vector>

SDLAudioInputDeviceDefault::SDLAudioInputDeviceDefault(void) : _stream{nullptr, &SDL_DestroyAudioStream} {
	constexpr SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 48000 };

	_stream = {
		SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, nullptr, nullptr),
		&SDL_DestroyAudioStream
	};

	if (!static_cast<bool>(_stream)) {
		std::cerr << "SDL open audio device failed!\n";
	}

	const auto audio_device_id = SDL_GetAudioStreamDevice(_stream.get());
	SDL_ResumeAudioDevice(audio_device_id);

	static constexpr size_t buffer_size {512*2}; // in samples
	const auto interval_ms {(buffer_size * 1000) / spec.freq};

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

SDLAudioInputDeviceDefault::~SDLAudioInputDeviceDefault(void) {
	// TODO: pause audio device?
	_thread_should_quit = true;
	_thread.join();
	// TODO: what to do if readers are still present?
}

int32_t SDLAudioOutputDeviceDefaultInstance::size(void) {
	return -1;
}

std::optional<AudioFrame> SDLAudioOutputDeviceDefaultInstance::pop(void) {
	assert(false);
	// this is an output device, there is no data to pop
	return std::nullopt;
}

bool SDLAudioOutputDeviceDefaultInstance::push(const AudioFrame& value) {
	if (!static_cast<bool>(_stream)) {
		return false;
	}

	// verify here the fame has the same sample type, channel count and sample freq
	// if something changed, we need to either use a temporary stream, just for conversion, or reopen _stream with the new params
	// because of data temporality, the second options looks like a better candidate
	if (
		value.sample_rate != _last_sample_rate ||
		value.channels != _last_channels ||
		(value.isF32() && _last_format != SDL_AUDIO_F32) ||
		(value.isS16() && _last_format != SDL_AUDIO_S16)
	) {
		const SDL_AudioSpec spec = {
			static_cast<SDL_AudioFormat>((value.isF32() ? SDL_AUDIO_F32 :  SDL_AUDIO_S16)),
			static_cast<int>(value.channels),
			static_cast<int>(value.sample_rate)
		};

		SDL_SetAudioStreamFormat(_stream.get(), &spec, nullptr);

		std::cerr << "SDLAOD: audio format changed\n";
	}

	if (value.isS16()) {
		auto data = value.getSpan<int16_t>();

		if (data.size == 0) {
			std::cerr << "empty audio frame??\n";
		}

		if (!SDL_PutAudioStreamData(_stream.get(), data.ptr, data.size * sizeof(int16_t))) {
			std::cerr << "put data error\n";
			return false; // return true?
		}
	} else if (value.isF32()) {
		auto data = value.getSpan<float>();

		if (data.size == 0) {
			std::cerr << "empty audio frame??\n";
		}

		if (!SDL_PutAudioStreamData(_stream.get(), data.ptr, data.size * sizeof(float))) {
			std::cerr << "put data error\n";
			return false; // return true?
		}
	}

	_last_sample_rate = value.sample_rate;
	_last_channels = value.channels;
	_last_format = value.isF32() ? SDL_AUDIO_F32 :  SDL_AUDIO_S16;

	return true;
}

SDLAudioOutputDeviceDefaultInstance::SDLAudioOutputDeviceDefaultInstance(void) : _stream(nullptr, nullptr) {
}

SDLAudioOutputDeviceDefaultInstance::SDLAudioOutputDeviceDefaultInstance(SDLAudioOutputDeviceDefaultInstance&& other) : _stream(std::move(other._stream)) {
}

SDLAudioOutputDeviceDefaultInstance::~SDLAudioOutputDeviceDefaultInstance(void) {
}


SDLAudioOutputDeviceDefaultSink::~SDLAudioOutputDeviceDefaultSink(void) {
	// TODO: pause and close device?
}

std::shared_ptr<FrameStream2I<AudioFrame>> SDLAudioOutputDeviceDefaultSink::subscribe(void) {
	auto new_instance = std::make_shared<SDLAudioOutputDeviceDefaultInstance>();

	constexpr SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 48000 };

	new_instance->_stream = {
		SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr),
		&SDL_DestroyAudioStream
	};
	new_instance->_last_sample_rate = spec.freq;
	new_instance->_last_channels = spec.channels;
	new_instance->_last_format = spec.format;

	if (!static_cast<bool>(new_instance->_stream)) {
		std::cerr << "SDL open audio device failed!\n";
		return nullptr;
	}

	const auto audio_device_id = SDL_GetAudioStreamDevice(new_instance->_stream.get());
	SDL_ResumeAudioDevice(audio_device_id);

	return new_instance;
}

bool SDLAudioOutputDeviceDefaultSink::unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame>>& sub) {
	if (!sub) {
		return false;
	}

	return true;
}

