#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

bool image_scale(uint8_t* dst, const int dst_w, const int dst_h, uint8_t* src, const int src_w, const int src_h);

bool image_scale(SDL_Surface* dst, SDL_Surface* src);

