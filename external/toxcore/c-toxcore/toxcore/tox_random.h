/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_RANDOM_H
#define C_TOXCORE_TOXCORE_TOX_RANDOM_H

#include <stdbool.h>
#include <stdint.h>

#include "tox_attributes.h"
#include "tox_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tox_Random_Funcs Tox_Random_Funcs;

typedef struct Tox_Random Tox_Random;

Tox_Random *_Nullable tox_random_new(const Tox_Random_Funcs *_Nonnull funcs, void *_Nullable user_data, const Tox_Memory *_Nonnull mem);

void tox_random_free(Tox_Random *_Nullable rng);

void tox_random_bytes(const Tox_Random *_Nonnull rng, uint8_t *_Nonnull bytes, uint32_t length);
uint32_t tox_random_uniform(const Tox_Random *_Nonnull rng, uint32_t upper_bound);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_RANDOM_H */
