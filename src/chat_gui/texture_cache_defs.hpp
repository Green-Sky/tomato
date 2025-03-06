#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "../texture_cache.hpp"
#include "../tox_avatar_loader.hpp"
#include "../message_image_loader.hpp"
#include "../bitset_image_loader.hpp"

using ContactTextureCache = TextureCache<uint64_t, Contact4, ToxAvatarLoader>;
using MessageTextureCache = TextureCache<uint64_t, Message3Handle, MessageImageLoader>;
using BitsetTextureCache = TextureCache<uint64_t, ObjectHandle, BitsetImageLoader>;

