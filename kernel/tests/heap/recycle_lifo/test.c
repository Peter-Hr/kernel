// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <ktest.h>
#include <ktest/heap.h>
#include <mm/heap.h>
#include <types.h>

#define MIN_SIZE 16
#define START_SIZE (MIN_SIZE << 14)
#define LOOPS 4

/*
 * Tests that repeated allocation and deallocation from the heap
 * does not cause memory to disappear (e.g. by some internal
 * fragmentation or similar).
 *
 * The test repeatedly allocates all available memory using
 * kmalloc() and then frees it all via kfree(). A properly implemented
 * allocator must be able to repeat such an operation without
 * "losing" memory due to fragmentation.
 *
 * This test frees the memory in LIFO manner, i.e. smallest allocations
 * (at the end of the heap) are freed first. This test can be passed
 * even by bump-pointer implementation that unbumps the pointer if
 * user returns the last allocated block.
 */

static size_t exhaust_and_free(void) {
    printk("Starting exhaustive allocation ...\n");

    list_t allocated_blocks;
    list_init(&allocated_blocks);

    size_t block_count = 0;
    size_t allocated_total = 0;

    size_t allocation_size = START_SIZE;
    while (allocation_size >= MIN_SIZE) {
        // Stress proper alignment a little bit ;-)
        size_t unaligned_size = allocation_size - 1;
        ktest_allocated_heap_block_t* block = ktest_allocated_heap_block_create(unaligned_size);
        if (block == NULL) {
            allocation_size = allocation_size / 2;
            continue;
        }

        ktest_check_kmalloc_block(block, unaligned_size);

        // No need to be overly conservative.
        allocated_total += allocation_size;
        ktest_check_kmalloc_total(allocated_total);

        list_prepend(&allocated_blocks, &block->link);
        block_count++;
    }

    printk(" ... and returning everything back (%d blocks) ...\n", block_count);
    ktest_allocated_heap_block_free_list(&allocated_blocks);

    return block_count;
}

void kernel_test(void) {
    ktest_start("heap/recycle_lifo");

    size_t expected_count = exhaust_and_free();
    printk(" ... allocated %d blocks (loop 0).\n", expected_count);
    ktest_assert(expected_count > 0, "no allocation succeeded");

    for (int i = 0; i < LOOPS; i++) {
        size_t count = exhaust_and_free();
        printk(" ... allocated %d blocks (loop %d).\n", count, i + 1);
        ktest_assert(count == expected_count,
                "allocation counts differ (%u => %u)", expected_count, count);
    }

    ktest_passed();
}
