#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "../texture_cache.hpp"
#include "../tox_avatar_loader.hpp"
#include "../message_image_loader.hpp"


using ContactTextureCache = TextureCache<void*, Contact3, ToxAvatarLoader>;
using MessageTextureCache = TextureCache<void*, Message3Handle, MessageImageLoader>;

