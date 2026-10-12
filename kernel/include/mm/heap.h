// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include <types.h>

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
