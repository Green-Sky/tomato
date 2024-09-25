#include "./sdl_audio_frame_stream2.hpp"

#include <iostream>

// "thin" wrapper around sdl audio streams
// we dont needs to get fance, as they already provide everything we need
struct SDLAudioStreamReader : public AudioFrameStream2I {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	uint32_t _seq_counter {0};

	uint32_t _sample_rate {48'000};
	size_t _channels {0};
	SDL_AudioFormat _format {SDL_AUDIO_S16};

	std::vector<int16_t> _buffer;

	SDLAudioStreamReader(void) : _stream(nullptr, nullptr) {}
	SDLAudioStreamReader(SDLAudioStreamReader&& other) :
		_stream(std::move(other._stream)),
		_sample_rate(other._sample_rate),
		_channels(other._channels),
		_format(other._format)
	{
		static const size_t buffer_size {960*_channels};
		_buffer.resize(buffer_size);
	}

	~SDLAudioStreamReader(void) {
		if (_stream) {
			SDL_UnbindAudioStream(_stream.get());
		}
	}

	int32_t size(void) override {
		//assert(_stream);
		// returns bytes
		//SDL_GetAudioStreamAvailable(_stream.get());
		return -1;
	}

	std::optional<AudioFrame> pop(void) override {
		assert(_stream);
		assert(_format == SDL_AUDIO_S16);

		static const size_t buffer_size {960*_channels};
		_buffer.resize(buffer_size); // noop?

		const auto read_bytes = SDL_GetAudioStreamData(
			_stream.get(),
			_buffer.data(),
			_buffer.size()*sizeof(int16_t)
		);

		// no new frame yet, or error
		if (read_bytes <= 0) {
			return std::nullopt;
		}

		return AudioFrame {
			_seq_counter++,
			_sample_rate, _channels,
			Span<int16_t>(_buffer.data(), read_bytes/sizeof(int16_t)),
		};
	}

	bool push(const AudioFrame&) override {
		// TODO: make universal sdl stream wrapper (combine with SDLAudioOutputDeviceDefaultInstance)
		assert(false && "read only");
		return false;
	}
};

SDLAudioInputDevice::SDLAudioInputDevice(void) : SDLAudioInputDevice(SDL_AUDIO_DEVICE_DEFAULT_RECORDING) {
}

SDLAudioInputDevice::SDLAudioInputDevice(SDL_AudioDeviceID device_id) {
	// this spec is more like a hint to the hardware
	SDL_AudioSpec spec {
		SDL_AUDIO_S16,
		1, // configurable?
		48'000,
	};
	_device_id = SDL_OpenAudioDevice(device_id, &spec);

	if (_device_id == 0) {
		// TODO: proper error handling
		throw int(1);
	}
}

SDLAudioInputDevice::~SDLAudioInputDevice(void) {
	_streams.clear();

	SDL_CloseAudioDevice(_device_id);
}

std::shared_ptr<FrameStream2I<AudioFrame>> SDLAudioInputDevice::subscribe(void) {
	SDL_AudioSpec spec {
		SDL_AUDIO_S16,
		1,
		48'000,
	};

	SDL_AudioSpec device_spec {
		SDL_AUDIO_S16,
		1,
		48'000,
	};
	// TODO: error check
	SDL_GetAudioDeviceFormat(_device_id, &device_spec, nullptr);

	// error check
	auto* sdl_stream = SDL_CreateAudioStream(&device_spec, &spec);

	// error check
	SDL_BindAudioStream(_device_id, sdl_stream);

	auto new_stream = std::make_shared<SDLAudioStreamReader>();
	// TODO: move to ctr
	new_stream->_stream = {sdl_stream, &SDL_DestroyAudioStream};
	new_stream->_sample_rate = spec.freq;
	new_stream->_channels = spec.channels;
	new_stream->_format = spec.format;

	_streams.emplace_back(new_stream);

	return new_stream;
}

bool SDLAudioInputDevice::unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame>>& sub) {
	for (auto it = _streams.cbegin(); it != _streams.cend(); it++) {
		if (*it == sub) {
			_streams.erase(it);
			return true;
		}
	}

	return false;
}

// does not need to be visible in the header
struct SDLAudioOutputDeviceDefaultInstance : public AudioFrameStream2I {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	uint32_t _last_sample_rate {48'000};
	size_t _last_channels {0};
	SDL_AudioFormat _last_format {SDL_AUDIO_S16};

	// TODO: audio device
	SDLAudioOutputDeviceDefaultInstance(void);
	SDLAudioOutputDeviceDefaultInstance(SDLAudioOutputDeviceDefaultInstance&& other);

	~SDLAudioOutputDeviceDefaultInstance(void);

	int32_t size(void) override;
	std::optional<AudioFrame> pop(void) override;
	bool push(const AudioFrame& value) override;
};

SDLAudioOutputDeviceDefaultInstance::SDLAudioOutputDeviceDefaultInstance(void) : _stream(nullptr, nullptr) {
}

SDLAudioOutputDeviceDefaultInstance::SDLAudioOutputDeviceDefaultInstance(SDLAudioOutputDeviceDefaultInstance&& other) : _stream(std::move(other._stream)) {
}

SDLAudioOutputDeviceDefaultInstance::~SDLAudioOutputDeviceDefaultInstance(void) {
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

