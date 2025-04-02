#include "./sdl_video_input_service.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include "../stream_manager.hpp"
#include "./sdl_video_frame_stream2.hpp"

#include <iostream>

namespace Components {

	struct SDLVideoDevice {
		SDL_CameraID device;
	};

} // Components

bool SDLVideoInputService::addDevice(SDL_CameraID device) {
	const char *name = SDL_GetCameraName(device);
	std::cout << "  - Camera #" << device << ": " << name << "\n";

	int speccount {0};
	SDL_CameraSpec** specs = SDL_GetCameraSupportedFormats(device, &speccount);
	if (specs == nullptr) {
		std::cout << "    - no supported spec\n";
		return false;
	} else {
		for (int spec_i = 0; spec_i < speccount; spec_i++) {
			std::cout << "    - " << specs[spec_i]->width << "x" << specs[spec_i]->height << "@" << float(specs[spec_i]->framerate_numerator)/specs[spec_i]->framerate_denominator << "fps " << SDL_GetPixelFormatName(specs[spec_i]->format) << "\n";

		}
		SDL_free(specs);

		// create sink for device
		ObjectHandle vsrc {_os.registry(), _os.registry().create()};
		try {
			vsrc.emplace<Components::SDLVideoDevice>(device);

			vsrc.emplace<Components::FrameStream2Source<SDLVideoFrame>>(
				std::make_unique<SDLVideo2InputDevice>(device)
			);

			vsrc.emplace<Components::StreamSource>(Components::StreamSource::create<SDLVideoFrame>("SDL Video '" + std::string(name) + "'"));
			//vsrc.emplace<Components::TagDefaultTarget>();

			_os.throwEventConstruct(vsrc);
			//last_src = vsrc;
		} catch (...) {
			std::cerr << "SDLVIS error: failed constructing video input source\n";
			_os.registry().destroy(vsrc);
			return false;
		}
	}
	return true;
}

bool SDLVideoInputService::removeDevice(SDL_CameraID device) {
	for (const auto&& [ov, svd] : _os.registry().view<Components::SDLVideoDevice>().each()) {
		if (svd.device == device) {
			_os.throwEventDestroy(ov);
			_os.registry().destroy(ov);
			return true;
		}
	}
	return false; // not found ??
}

SDLVideoInputService::SDLVideoInputService(ObjectStore2& os) :
	_os(os)
{
	if (!SDL_InitSubSystem(SDL_INIT_CAMERA)) {
		std::cerr << "SDLVIS warning: no sdl camera: " << SDL_GetError() << "\n";
		return;
	}
	_subsystem_init = true;
	std::cout << "SDLVIS: SDL Camera Driver: " << SDL_GetCurrentCameraDriver() << "\n";

	// sdl throws all cameras as added events anyway
#if 0
	int devcount {0};
	SDL_CameraID *devices = SDL_GetCameras(&devcount);

	std::cout << "SDLVIS: found cameras:\n";
	if (devices != nullptr) {
		ObjectHandle last_src{};
		for (int i = 0; i < devcount; i++) {
			addDevice(devices[i]);
		}

		SDL_free(devices);
	} else {
		std::cout << "  none\n";
	}
#endif
}

SDLVideoInputService::~SDLVideoInputService(void) {
	if (_subsystem_init) {
		SDL_QuitSubSystem(SDL_INIT_CAMERA);
	}
}

bool SDLVideoInputService::handleEvent(SDL_Event& e) {
	if (e.type == SDL_EVENT_CAMERA_DEVICE_ADDED) {
		std::cout << "SDLVIS: camera added:\n";
		return addDevice(e.cdevice.which);
	} else if (e.type == SDL_EVENT_CAMERA_DEVICE_REMOVED) {
		std::cout << "SDLVIS: camera removed\n";
		return removeDevice(e.cdevice.which);
	}

	return false;
}

