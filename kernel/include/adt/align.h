// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _ADT_ALIGN_H
#define _ADT_ALIGN_H

#include <types.h>

/** Align given address up to the given power-of-two alignment. */
static inline uintptr_t uintptr_align_up(uintptr_t addr, unative_t align) {
    return (addr + align - 1) & ~(align - 1);
}

/** Align given address down to the given power-of-two alignment. */
static inline uintptr_t uintptr_align_down(uintptr_t addr, unative_t align) {
    return (addr) & ~(align - 1);
}

/** Align given size up to the given power-of-two alignment. */
static inline size_t size_align_up(size_t size, unative_t align) {
    return (size + align - 1) & ~(align - 1);
}

#endif
