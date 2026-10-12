// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <ktest.h>
#include <ktest/heap.h>
#include <mm/heap.h>
#include <types.h>

#define BLOCK_SIZE_UNIT 1024

/*
 * Checks that heap allocator properly coalesces returned blocks.
 *
 * We check that after exhausting and freeing memory using blocks of size N,
 * we can do the same by allocating approximately half the number of blocks
 * of size 2*N. This is repeated for several multiples of N, with the expected
 * number of allocated blocks scaled accordingly.
 *
 * Because we free the allocated memory in a semi-random order, this should
 * test the coalescing of free blocks.
 */

static size_t exhaust_with_size_and_free(size_t allocation_size) {
    printk("Starting exhaustive allocation of %u bytes blocks ...\n", allocation_size);

    list_t allocated_blocks;
    list_init(&allocated_blocks);

    size_t block_count = 0;
    size_t allocated_total = 0;

    while (1) {
        ktest_allocated_heap_block_t* block = ktest_allocated_heap_block_create(allocation_size);
        if (block == NULL) {
            break;
        }

        ktest_check_kmalloc_block(block, allocation_size);

        allocated_total += allocation_size;
        ktest_check_kmalloc_total(allocated_total);

        list_rotate(&allocated_blocks);
        list_rotate(&allocated_blocks);
        list_append(&allocated_blocks, &block->link);

        block_count += 1;
    }

    printk(" ... and returning everything back (%d blocks) ...\n", block_count);
    ktest_allocated_heap_block_free_list(&allocated_blocks);

    return block_count;
}

void kernel_test(void) {
    ktest_start("heap/coalesce2");

    size_t allocations_unit = exhaust_with_size_and_free(BLOCK_SIZE_UNIT);
    printk("Blocks of %u bytes: %u\n", BLOCK_SIZE_UNIT, allocations_unit);

    size_t allocations_2_units = exhaust_with_size_and_free(BLOCK_SIZE_UNIT * 2);
    printk("Blocks of %u bytes: %u\n", BLOCK_SIZE_UNIT * 2, allocations_2_units);
    ktest_assert_in_range("we want about half of double blocks",
            allocations_2_units, allocations_unit / 2 - 2, allocations_unit);

    size_t allocations_3_units = exhaust_with_size_and_free(BLOCK_SIZE_UNIT * 3);
    printk("Blocks of %u bytes: %u\n", BLOCK_SIZE_UNIT * 3, allocations_3_units);
    ktest_assert_in_range("we want about third of triple blocks",
            allocations_3_units, allocations_unit / 3 - 2, allocations_unit);

    size_t allocations_4_units = exhaust_with_size_and_free(BLOCK_SIZE_UNIT * 4);
    printk("Blocks of %u bytes: %u\n", BLOCK_SIZE_UNIT * 4, allocations_4_units);
    ktest_assert_in_range("we want about quarter of quadruple blocks",
            allocations_4_units, allocations_unit / 4 - 2, allocations_unit);

    size_t allocations_unit_again = exhaust_with_size_and_free(BLOCK_SIZE_UNIT);
    printk("Blocks of %u bytes: %u\n", BLOCK_SIZE_UNIT, allocations_unit_again);
    ktest_assert(allocations_unit == allocations_unit_again,
            "same size allocations should not differ (%u => %u)",
            allocations_unit, allocations_unit_again);

    ktest_passed();
}
