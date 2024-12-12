#include "./bitset_image_loader.hpp"

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>

#include "./os_comps.hpp"

#include <entt/entity/entity.hpp>

#include <SDL3/SDL.h>

#include <iostream>

// fwd
namespace Message {
uint64_t getTimeMS(void);
}

BitsetImageLoader::BitsetImageLoader(void) {
}

std::optional<TextureEntry> BitsetImageLoader::load(TextureUploaderI& tu, ObjectHandle o) {
	if (!static_cast<bool>(o)) {
		std::cerr << "BIL error: trying to load invalid object\n";
		return std::nullopt;
	}

	if (!o.all_of<ObjComp::F::LocalHaveBitset>()) {
		// after completion, this is called until the texture times out
		//std::cout << "BIL: no local have bitset\n";
		return std::nullopt;
	}

	// TODO: const
	auto& have = o.get<ObjComp::F::LocalHaveBitset>().have;

	auto* surf = SDL_CreateSurfaceFrom(
		have.size_bits(), 1,
		SDL_PIXELFORMAT_INDEX1MSB, // LSB ?
		have.data(), have.size_bytes()
	);
	if (surf == nullptr) {
		std::cerr << "BIL error: bitset to 1bit surface creationg failed o:" << entt::to_integral(o.entity()) << "\n";
		return std::nullopt;
	}

	SDL_Color colors[] {
		{0, 0, 0, 0},
		{255, 255, 255, 255},
	};

	SDL_Palette* palette = SDL_CreatePalette(2);
	SDL_SetPaletteColors(palette, colors, 0, 2);
	SDL_SetSurfacePalette(surf, palette);
	auto* conv_surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);

	SDL_DestroySurface(surf);
	SDL_DestroyPalette(palette);

	if (conv_surf == nullptr) {
		std::cerr << "BIL error: surface conversion failed o:" << entt::to_integral(o.entity()) << " : " << SDL_GetError() << "\n";
		return std::nullopt;
	}

	SDL_LockSurface(conv_surf);

	TextureEntry new_entry;
	new_entry.timestamp_last_rendered = Message::getTimeMS();
	new_entry.current_texture = 0;
	new_entry.width = have.size_bits();
	new_entry.height = 1;

	const auto n_t = tu.upload(static_cast<uint8_t*>(conv_surf->pixels), conv_surf->w, conv_surf->h, TextureUploaderI::RGBA);
	assert(n_t != 0);
	new_entry.textures.push_back(n_t);
	new_entry.frame_duration.push_back(0);

	std::cout << "BIL: genereated bitset image o:" << entt::to_integral(o.entity()) << "\n";

	SDL_UnlockSurface(conv_surf);
	SDL_DestroySurface(conv_surf);

	return new_entry;
}

