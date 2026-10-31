// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include <adt/list.h>
#include <types.h>

/** Header placed right before every heap block. */
typedef struct {
    link_t link; // all blocks, address-ordered
    size_t size; // payload size (without header), multiple of 8
    bool used;
} heap_block_t;

_CHECK_TYPE_SIZE(heap_block_t, 16);

bool heap_contains(uintptr_t addr); // getters prototype

/** Allocator selection
 *
 * 1 = bump-pointer allocator (kfree does nothing)
 * 0 = block allocator with headers and coalescing
 */
#ifndef HEAP_USE_BUMP
#define HEAP_USE_BUMP 0
#endif

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
