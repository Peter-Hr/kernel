// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <ktest.h>
#include <ktest/heap.h>
#include <mm/heap.h>
#include <types.h>

#define BLOCK_SIZE 64
#define BLOCK_SIZE_2 124
#define BLOCK_SIZE_5 300

/*
 * Tests that coalescing in the heap allocator works.
 *
 * This test is useful for debugging the allocator as it
 * operates on five blocks only. It frees them in well-defined
 * order to test coalescing to both sides (assuming the
 * traditional heap allocator).
 */

static size_t exhaust_memory(size_t allocation_size, size_t allocation_size_min) {
    size_t allocated_total = 0;

    while (allocation_size >= allocation_size_min) {
        uint8_t* ptr = kmalloc(allocation_size);
        dprintk("kmalloc(%u) = %pA\n", allocation_size, ptr);
        if (ptr == NULL) {
            allocation_size = allocation_size / 2;
            continue;
        }

        ktest_check_kmalloc_block(ptr, allocation_size);

        allocated_total += allocation_size;
        ktest_check_kmalloc_total(allocated_total);
    }

    return allocated_total;
}

void kernel_test(void) {
    ktest_start("heap/coalesce1");

    /*
     * The test assumes that blocks are allocated one after
     * another without gaps. That should hold for most of the
     * allocators that are out there :-).
     */
    void* p1 = kmalloc(BLOCK_SIZE);
    printk("p1 = %pA\n", p1);
    ktest_assert(p1 != NULL, "out of memory");
    ktest_check_kmalloc_result(p1, BLOCK_SIZE);

    void* p2 = kmalloc(BLOCK_SIZE);
    printk("p2 = %pA\n", p2);
    ktest_assert(p2 != NULL, "out of memory");
    ktest_check_kmalloc_result(p2, BLOCK_SIZE);

    void* p3 = kmalloc(BLOCK_SIZE);
    printk("p3 = %pA\n", p3);
    ktest_assert(p3 != NULL, "out of memory");
    ktest_check_kmalloc_result(p3, BLOCK_SIZE);

    void* p4 = kmalloc(BLOCK_SIZE);
    printk("p4 = %pA\n", p4);
    ktest_assert(p4 != NULL, "out of memory");
    ktest_check_kmalloc_result(p4, BLOCK_SIZE);

    void* p5 = kmalloc(BLOCK_SIZE);
    printk("p5 = %pA\n", p5);
    ktest_assert(p5 != NULL, "out of memory");
    ktest_check_kmalloc_result(p5, BLOCK_SIZE);

    /*
     * Exhaust the rest of available memory.
     * That should take care of any next-fit-style allocators
     * and ensure that we only operate on memory behind p1-p5.
     */
    (void)exhaust_memory(1 << 16, 32);

    /*
     * Test coalescing with following block.
     */
    kfree(p2);
    kfree(p1);

    void* q1 = kmalloc(BLOCK_SIZE_2);
    printk("q1 = %pA\n", q1);
    ktest_assert(q1 != NULL, "coalescing with following free block failed");

    /*
     * Test coalescing with previous free block.
     */
    kfree(p4);
    kfree(p5);

    void* q2 = kmalloc(BLOCK_SIZE_2);
    printk("q2 = %pA\n", q2);
    ktest_assert(q2 != NULL, "coalescing with previous free block failed");

    /*
     * Test coalescing when both surrounding blocks are free.
     */
    kfree(q1);
    kfree(q2);
    kfree(p3);

    void* r1 = kmalloc(BLOCK_SIZE_5);
    printk("r1 = %pA\n", r1);
    ktest_assert(r1 != NULL, "coalescing with surrounding free blocks failed");

    /*
     * Basic coalescing works.
     */
    ktest_passed();
}
