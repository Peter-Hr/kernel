// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <ktest.h>
#include <ktest/heap.h>
#include <mm/heap.h>
#include <types.h>

#define MIN_SIZE 8
#define START_SIZE (MIN_SIZE << 12)

/*
 * Tests that kmalloc() eventually exhausts memory.
 */

void kernel_test(void) {
    ktest_start("heap/exhaust");

    size_t allocated_total = 0;

    size_t allocation_size = START_SIZE;
    while (allocation_size >= MIN_SIZE) {
        // Stress proper alignment a little bit ;-)
        size_t unaligned_size = allocation_size - 1;
        void* ptr = kmalloc(unaligned_size);
        dprintk("kmalloc(%u) = %pA\n", unaligned_size, ptr);
        if (ptr == NULL) {
            allocation_size = allocation_size / 2;
            continue;
        }

        ktest_check_kmalloc_block(ptr, unaligned_size);

        // No need to be overly conservative.
        allocated_total += allocation_size;
        ktest_check_kmalloc_total(allocated_total);
    }

    printk("allocated_total = %u\n", allocated_total);
    ktest_assert(allocated_total > 0, "no memory on heap");

    ktest_passed();
}
