#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/util/bitset.hpp>

#include "./texture_cache.hpp"

#include <optional>

struct ObjectContactSub final {
	ObjectHandle o;
	Contact3 c{entt::null};
};

template<>
struct std::hash<ObjectContactSub> {
	std::size_t operator()(ObjectContactSub const& ocs) const noexcept {
		const std::size_t h1 = reinterpret_cast<std::size_t>(ocs.o.registry());
		const std::size_t h2 = entt::to_integral(ocs.o.entity());
		const std::size_t h3 = entt::to_integral(ocs.c);
		return (h1 << 3) ^ (h3 << 7) ^ (h2 * 11400714819323198485llu);
	}
};

class BitsetImageLoader {
	BitSet _tmp_bitset;

	std::optional<TextureEntry> haveToTexture(TextureUploaderI& tu, BitSet& have, ObjectHandle o);

	public:
		BitsetImageLoader(void);
		std::optional<TextureEntry> load(TextureUploaderI& tu, ObjectHandle o);
		std::optional<TextureEntry> load(TextureUploaderI& tu, ObjectContactSub ocs);
};

