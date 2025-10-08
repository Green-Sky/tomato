/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013 Tox project.
 */

/**
 * Memory allocation and deallocation functions.
 */
#ifndef C_TOXCORE_TOXCORE_MEM_H
#define C_TOXCORE_TOXCORE_MEM_H

#include <stdint.h>     // uint*_t

#include "attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *mem_malloc_cb(void *_Nullable obj, uint32_t size);
typedef void *mem_calloc_cb(void *_Nullable obj, uint32_t nmemb, uint32_t size);
typedef void *mem_realloc_cb(void *_Nullable obj, void *_Nullable ptr, uint32_t size);
typedef void mem_free_cb(void *_Nullable obj, void *_Nullable ptr);

/** @brief Functions wrapping standard C memory allocation functions. */
typedef struct Memory_Funcs {
    mem_malloc_cb *_Nullable malloc;
    mem_calloc_cb *_Nullable calloc;
    mem_realloc_cb *_Nullable realloc;
    mem_free_cb *_Nullable free;
} Memory_Funcs;

typedef struct Memory {
    const Memory_Funcs *_Nullable funcs;
    void *_Nullable obj;
} Memory;

const Memory *_Nullable os_memory(void);

/**
 * @brief Allocate an array of a given size for built-in types.
 *
 * The array will not be initialised. Supported built-in types are
 * `uint8_t`, `int8_t`, and `int16_t`.
 */
void *_Nullable mem_balloc(const Memory *_Nonnull mem, uint32_t size);

/**
 * @brief Resize an array of a given size for built-in types.
 *
 * If used for a type other than byte-sized types, `size` needs to be manually
 * multiplied by the element size.
 */
void *_Nullable mem_brealloc(const Memory *_Nonnull mem, void *_Nullable ptr, uint32_t size);

/**
 * @brief Allocate a single object.
 *
 * Always use as `(T *)mem_alloc(mem, sizeof(T))`.
 */
void *_Nullable mem_alloc(const Memory *_Nonnull mem, uint32_t size);

/**
 * @brief Allocate a vector (array) of objects.
 *
 * Always use as `(T *)mem_valloc(mem, N, sizeof(T))`.
 */
void *_Nullable mem_valloc(const Memory *_Nonnull mem, uint32_t nmemb, uint32_t size);

/**
 * @brief Resize an object vector.
 *
 * Changes the size of (and possibly moves) the memory block pointed to by
 * @p ptr to be large enough for an array of @p nmemb elements, each of which
 * is @p size bytes. It is similar to the call
 *
 * @code
 * realloc(ptr, nmemb * size);
 * @endcode
 *
 * However, unlike that `realloc()` call, `mem_vrealloc()` fails safely in the
 * case where the multiplication would overflow. If such an overflow occurs,
 * `mem_vrealloc()` returns `nullptr`.
 */
void *_Nullable mem_vrealloc(const Memory *_Nonnull mem, void *_Nullable ptr, uint32_t nmemb, uint32_t size);

/** @brief Free an array, object, or object vector. */
void mem_delete(const Memory *_Nonnull mem, void *_Nullable ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_MEM_H */
