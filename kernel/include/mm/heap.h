// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include <types.h>

/** Allocator selection
 *
 * 1 = bump-pointer allocator (kfree does nothing)
 * 0 = block allocator with headers and coalescing
 */
#ifndef HEAP_USE_BUMP
#define HEAP_USE_BUMP 1
#endif

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
