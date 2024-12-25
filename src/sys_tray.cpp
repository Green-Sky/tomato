#include "./sys_tray.hpp"

#include "./image_loader_sdl_image.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <memory>
#include <iostream>

SystemTray::SystemTray(void) {
	std::cout << "adding sys tray\n";

	std::ifstream file("res/icon/tomato_v1_256.png");
	file.seekg(0, std::ios::end);
	size_t file_size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<uint8_t> data(file_size);
	file.read(reinterpret_cast<char*>(data.data()), file_size);
	file.close();

	std::cout << "tray icon file size: " << file_size << "\n";

	ImageLoaderSDLImage il;
	auto image = il.loadFromMemoryRGBA(data.data(), data.size());
	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surf = {
		SDL_CreateSurfaceFrom(
			image.width, image.height,
			SDL_PIXELFORMAT_RGBA32,
			(void*)image.frames.at(0).data.data(),
			4*image.width
		),
		&SDL_DestroySurface
	};

	std::cout << "tray dims " << image.width << "x" << image.height << "\n";


	// different icons?
	_tray = SDL_CreateTray(surf.get(), "tomato");
	if (_tray == nullptr) {
		std::cerr << "failed to create SystemTray: " << SDL_GetError() << "\n";
	}
}

SystemTray::~SystemTray(void) {
	if (_tray != nullptr) {
		SDL_DestroyTray(_tray);
		_tray = nullptr;
	}
}

