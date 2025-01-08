#pragma once

#include <SDL3/SDL.h>

#include <memory>

namespace IconGenerator {

// remember to SDL_DestroySurface()

// tomato icon
SDL_Surface* base(void);

// tomato icon with colored indicator
SDL_Surface* colorIndicator(float r, float g, float b, float a);

} // IconGenerator

