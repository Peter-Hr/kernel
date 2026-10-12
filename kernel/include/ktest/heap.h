// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _KTEST_HEAP_H
#define _KTEST_HEAP_H

#include <adt/list.h>
#include <mm/heap.h>
#include <mm/self.h>

#ifdef KERNEL_TEST_PROBE_MEMORY_MAINMEM_SIZE_KB
#define KTEST_MAX_HEAP_SIZE ((KERNEL_TEST_PROBE_MEMORY_MAINMEM_SIZE_KB) * (1024))
#else
#define KTEST_MAX_HEAP_SIZE (512 * 1024 * 1024)
#endif

#define ktest_check_kmalloc_total(allocated_total) \
    do { \
        ktest_assert(allocated_total < (KTEST_MAX_HEAP_SIZE), "allocated too much memory: %d bytes (max at %d)", allocated_total, KTEST_MAX_HEAP_SIZE); \
    } while (0)

#define ktest_check_kmalloc_result(addr_ptr, size) \
    do { \
        uintptr_t __addr = (uintptr_t)addr_ptr; \
        uintptr_t __addr_end_of_kernel_image = kernel_get_image_end(); \
        unsigned long long __addr_end = __addr + (size); \
        ktest_assert((__addr >= 0x80000000) && (__addr < 0xA0000000), \
                #addr_ptr " (0x%x) not in kernel segment [0x80000000, 0xA0000000)", __addr); \
        ktest_assert((__addr_end >= 0x80000000) && (__addr_end < 0xA0000000), \
                #addr_ptr " + " #size " (0x%x + %dB) not in kernel segment [0x80000000, 0xA0000000)", __addr, (size)); \
        ktest_assert((__addr % 4) == 0, "bad alignment on 0x%x", __addr); \
        ktest_assert(__addr >= __addr_end_of_kernel_image, \
                #addr_ptr " (0x%x) is inside kernel image (ends at 0x%x)", \
                __addr, __addr_end_of_kernel_image); \
    } while (0)

#define ktest_check_kmalloc_writable(addr_ptr) \
    do { \
        volatile uint8_t* __ptr = (uint8_t*)addr_ptr; \
        __ptr[0] = 0xAA; \
        __ptr[1] = 0x55; \
        ktest_assert(__ptr[0] == 0xAA, "expected 0xAA, got 0x%x at %p", __ptr[0], addr_ptr); \
        ktest_assert(__ptr[1] == 0x55, "expected 0x55, got 0x%x at %p", __ptr[1], addr_ptr); \
    } while (0)

#define ktest_check_kmalloc_block(addr_ptr, size) \
    do { \
        ktest_check_kmalloc_result(addr_ptr, size); \
        ktest_check_kmalloc_writable(addr_ptr); \
    } while (0)

typedef struct {
    link_t link;
} ktest_allocated_heap_block_t;

static inline ktest_allocated_heap_block_t* ktest_allocated_heap_block_create(size_t size) {
    ktest_allocated_heap_block_t* result = kmalloc(size);
    dprintk("kmalloc(%u) = %pA\n", size, result);

    if (result != NULL) {
        link_init(&result->link);
    }

    return result;
}

static inline void ktest_allocated_heap_block_free_list(list_t* block_list) {
    while (!list_is_empty(block_list)) {
        link_t* first = list_pop(block_list);
        ktest_allocated_heap_block_t* block = list_item(first, ktest_allocated_heap_block_t, link);
        dprintk("kfree(%pA)\n", block);
        kfree(block);
    }
}

#endif
