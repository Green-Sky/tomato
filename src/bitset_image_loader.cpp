#include "./bitset_image_loader.hpp"

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>

#include "./os_comps.hpp"

#include <entt/entity/entity.hpp>

#include <SDL3/SDL.h>

#include <iostream>

std::optional<TextureEntry> BitsetImageLoader::haveToTexture(TextureUploaderI& tu, BitSet& have, ObjectHandle o) {
	assert(have.size_bits() > 0);

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
	new_entry.timestamp_last_rendered = getTimeMS();
	new_entry.width = have.size_bits();
	new_entry.height = 1;

	const auto n_t = tu.upload(static_cast<uint8_t*>(conv_surf->pixels), conv_surf->w, conv_surf->h, TextureUploaderI::RGBA);
	assert(n_t != 0);
	new_entry.textures.push_back(n_t);
	new_entry.frame_duration.push_back(1);

	std::cout << "BIL: genereated bitset image o:" << entt::to_integral(o.entity()) << "\n";

	SDL_UnlockSurface(conv_surf);
	SDL_DestroySurface(conv_surf);

	return new_entry;
}

BitsetImageLoader::BitsetImageLoader(void) {
}

TextureLoaderResult BitsetImageLoader::load(TextureUploaderI& tu, ObjectHandle o, uint32_t /*w*/, uint32_t /*h*/) {
	if (!static_cast<bool>(o)) {
		std::cerr << "BIL error: trying to load invalid object\n";
		return {};
	}

	if (!o.any_of<ObjComp::F::LocalHaveBitset, ObjComp::F::RemoteHaveBitset>()) {
		// after completion, this is called until the texture times out
		//std::cout << "BIL: no have bitset\n";
		return {};
	}

	if (o.all_of<ObjComp::F::LocalHaveBitset>()) {
		auto& have = o.get<ObjComp::F::LocalHaveBitset>().have;
		assert(have.size_bits() > 0);
		return {haveToTexture(tu, have, o)};
	} else if (o.all_of<ObjComp::F::RemoteHaveBitset>()) {
		auto& list = o.get<ObjComp::F::RemoteHaveBitset>().others;
		if (list.empty()) {
			std::cout << "BIL: remote set list empty\n";
			_tmp_bitset = {8};
			return {haveToTexture(tu, _tmp_bitset, o)};
		}
		const auto& first_entry = list.begin()->second;

		if (first_entry.have_all) {
			_tmp_bitset = {8};
			_tmp_bitset.invert();
			std::cout << "BIL: remote first have all\n";
		} else {
			_tmp_bitset = first_entry.have;
			assert(_tmp_bitset.size_bits() == first_entry.have.size_bits());

			for (auto it = list.begin()+1; it != list.end(); it++) {
				if (it->second.have_all) {
					_tmp_bitset = {8};
					_tmp_bitset.invert();
					std::cout << "BIL: remote have all\n";
					break;
				}

				_tmp_bitset.merge(it->second.have);
			}
		}

		return {haveToTexture(tu, _tmp_bitset, o)};
	}

	return {};
}

std::optional<TextureEntry> BitsetImageLoader::load(TextureUploaderI& tu, ObjectContactSub ocs) {
	if (!static_cast<bool>(ocs.o)) {
		std::cerr << "BIL error: trying to load invalid object\n";
		return std::nullopt;
	}

	if (!ocs.o.all_of<ObjComp::F::RemoteHaveBitset>()) {
		// after completion, this is called until the texture times out
		return std::nullopt;
	}

	auto& map = ocs.o.get<ObjComp::F::RemoteHaveBitset>().others;
	auto it = map.find(ocs.c);
	if (it == map.end()) {
		// contact not found
		return std::nullopt;
	}

	if (it->second.have_all) {
		BitSet tmp{8}; // or 1?
		tmp.invert();
		return haveToTexture(tu, tmp, ocs.o);
	} else if (it->second.have.size_bits() == 0) {
		BitSet tmp{8}; // or 1?
		return haveToTexture(tu, tmp, ocs.o);
	} else {
		return haveToTexture(tu, it->second.have, ocs.o);
	}
}

