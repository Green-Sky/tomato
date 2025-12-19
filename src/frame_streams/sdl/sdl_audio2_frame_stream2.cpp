#include "./sdl_audio2_frame_stream2.hpp"

#include <cassert>
#include <iostream>
#include <optional>

#include "../audio_stream_pop_reframer.hpp"

// "thin" wrapper around sdl audio streams
// we dont needs to get fancy, as they already provide everything we need
struct SDLAudio2StreamReader : public AudioFrame2Stream2I {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	uint32_t _sample_rate {48'000};
	size_t _channels {0};

	// buffer gets reused!
	std::vector<int16_t> _buffer;

	SDLAudio2StreamReader(void) : _stream(nullptr, nullptr) {}
	SDLAudio2StreamReader(SDLAudio2StreamReader&& other) :
		_stream(std::move(other._stream)),
		_sample_rate(other._sample_rate),
		_channels(other._channels)
	{
		const size_t buffer_size {960*_channels};
		_buffer.resize(buffer_size);
	}

	~SDLAudio2StreamReader(void) {
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

	std::optional<AudioFrame2> pop(void) override {
		assert(_stream);
		if (!_stream) {
			return std::nullopt;
		}

		const size_t buffer_size {960*_channels};
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

		return AudioFrame2{
			_sample_rate, _channels,
			Span<int16_t>(_buffer.data(), read_bytes/sizeof(int16_t)),
		};
	}

	bool push(const AudioFrame2&) override {
		// TODO: make universal sdl stream wrapper (combine with SDLAudioOutputDeviceDefaultInstance)
		assert(false && "read only");
		return false;
	}
};

SDLAudio2InputDevice::SDLAudio2InputDevice(void) : SDLAudio2InputDevice(SDL_AUDIO_DEVICE_DEFAULT_RECORDING) {
}

SDLAudio2InputDevice::SDLAudio2InputDevice(SDL_AudioDeviceID conf_device_id) : _configured_device_id(conf_device_id) {
	if (_configured_device_id == 0) {
		// TODO: proper error handling
		throw int(1);
	}
}

SDLAudio2InputDevice::~SDLAudio2InputDevice(void) {
	_streams.clear();

	if (_virtual_device_id != 0) {
		SDL_CloseAudioDevice(_virtual_device_id);
		_virtual_device_id = 0;
	}
}

std::shared_ptr<FrameStream2I<AudioFrame2>> SDLAudio2InputDevice::subscribe(void) {
	if (_virtual_device_id == 0) {
		// first stream, open device
		// this spec is more like a hint to the hardware
		SDL_AudioSpec spec {
			SDL_AUDIO_S16,
			1, // TODO: conf
			48'000,
		};
		_virtual_device_id = SDL_OpenAudioDevice(_configured_device_id, &spec);
	}

	if (_virtual_device_id == 0) {
		std::cerr << "SDLAID error: failed opening device " << _configured_device_id << "\n";
		return nullptr;
	}

	SDL_AudioSpec spec {
		SDL_AUDIO_S16, // required, as AudioFrame2 only supports s16
		1, // TODO: conf
		48'000,
	};

	SDL_AudioSpec device_spec {
		SDL_AUDIO_S16,
		1, // TODO: conf
		48'000,
	};
	// TODO: error check
	SDL_GetAudioDeviceFormat(_virtual_device_id, &device_spec, nullptr);

	// error check
	auto* sdl_stream = SDL_CreateAudioStream(&device_spec, &spec);

	// error check
	SDL_BindAudioStream(_virtual_device_id, sdl_stream);

	//auto new_stream = std::make_shared<SDLAudio2StreamReader>();
	//// TODO: move to ctr
	//new_stream->_stream = {sdl_stream, &SDL_DestroyAudioStream};
	//new_stream->_sample_rate = spec.freq;
	//new_stream->_channels = spec.channels;

	auto new_stream = std::make_shared<AudioStreamPopReFramer<SDLAudio2StreamReader>>();
	new_stream->_stream._stream = {sdl_stream, &SDL_DestroyAudioStream};
	new_stream->_stream._sample_rate = spec.freq;
	new_stream->_stream._channels = spec.channels;
	new_stream->_frame_length_ms = 5; // WHY DOES THIS FIX MY ISSUE !!!

	_streams.emplace_back(new_stream);

	return new_stream;
}

bool SDLAudio2InputDevice::unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame2>>& sub) {
	for (auto it = _streams.cbegin(); it != _streams.cend(); it++) {
		if (*it == sub) {
			_streams.erase(it);
			if (_streams.empty()) {
				// last stream, close
				// TODO: make sure no shared ptr still exists???
				SDL_CloseAudioDevice(_virtual_device_id);
				std::cout << "SDLAID: closing device " << _virtual_device_id << " (" << _configured_device_id << ")\n";
				_virtual_device_id = 0;
			}
			return true;
		}
	}

	return false;
}

// does not need to be visible in the header
struct SDLAudio2OutputDeviceDefaultInstance : public AudioFrame2Stream2I {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	uint32_t _last_sample_rate {48'000};
	size_t _last_channels {0};

	// TODO: audio device
	SDLAudio2OutputDeviceDefaultInstance(void);
	SDLAudio2OutputDeviceDefaultInstance(SDLAudio2OutputDeviceDefaultInstance&& other);

	~SDLAudio2OutputDeviceDefaultInstance(void);

	int32_t size(void) override;
	std::optional<AudioFrame2> pop(void) override;
	bool push(const AudioFrame2& value) override;
};

SDLAudio2OutputDeviceDefaultInstance::SDLAudio2OutputDeviceDefaultInstance(void) : _stream(nullptr, nullptr) {
}

SDLAudio2OutputDeviceDefaultInstance::SDLAudio2OutputDeviceDefaultInstance(SDLAudio2OutputDeviceDefaultInstance&& other) : _stream(std::move(other._stream)) {
}

SDLAudio2OutputDeviceDefaultInstance::~SDLAudio2OutputDeviceDefaultInstance(void) {
}
int32_t SDLAudio2OutputDeviceDefaultInstance::size(void) {
	return -1;
}

std::optional<AudioFrame2> SDLAudio2OutputDeviceDefaultInstance::pop(void) {
	assert(false);
	// this is an output device, there is no data to pop
	return std::nullopt;
}

bool SDLAudio2OutputDeviceDefaultInstance::push(const AudioFrame2& value) {
	if (!static_cast<bool>(_stream)) {
		return false;
	}

	// verify here the fame has the same channel count and sample freq
	// if something changed, we need to either use a temporary stream, just for conversion, or reopen _stream with the new params
	// because of data temporality, the second options looks like a better candidate
	if (
		value.sample_rate != _last_sample_rate ||
		value.channels != _last_channels
	) {
		const SDL_AudioSpec spec = {
			static_cast<SDL_AudioFormat>(SDL_AUDIO_S16),
			static_cast<int>(value.channels),
			static_cast<int>(value.sample_rate)
		};

		SDL_SetAudioStreamFormat(_stream.get(), &spec, nullptr);

		std::cerr << "SDLAOD: audio format changed\n";
	}

	auto data = value.getSpan();

	if (data.size == 0) {
		std::cerr << "empty audio frame??\n";
	}

	if (!SDL_PutAudioStreamData(_stream.get(), data.ptr, data.size * sizeof(int16_t))) {
		std::cerr << "put data error\n";
		return false; // return true?
	}

	_last_sample_rate = value.sample_rate;
	_last_channels = value.channels;

	return true;
}


SDLAudio2OutputDeviceDefaultSink::~SDLAudio2OutputDeviceDefaultSink(void) {
	// TODO: pause and close device?
}

std::shared_ptr<FrameStream2I<AudioFrame2>> SDLAudio2OutputDeviceDefaultSink::subscribe(void) {
	auto new_instance = std::make_shared<SDLAudio2OutputDeviceDefaultInstance>();

	constexpr SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 48000 };

	new_instance->_stream = {
		SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr),
		&SDL_DestroyAudioStream
	};
	new_instance->_last_sample_rate = spec.freq;
	new_instance->_last_channels = spec.channels;

	if (!static_cast<bool>(new_instance->_stream)) {
		std::cerr << "SDL open audio device failed!\n";
		return nullptr;
	}

	const auto audio_device_id = SDL_GetAudioStreamDevice(new_instance->_stream.get());
	SDL_ResumeAudioDevice(audio_device_id);

	return new_instance;
}

bool SDLAudio2OutputDeviceDefaultSink::unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame2>>& sub) {
	// TODO: i think we should keep track of them
	if (!sub) {
		return false;
	}

	return true;
}

