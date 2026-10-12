// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <ktest.h>
#include <ktest/heap.h>
#include <main.h>
#include <mm/heap.h>
#include <types.h>

#ifndef KERNEL_TEST_PROBE_MEMORY_MAINMEM_SIZE_KB
#error Macro KERNEL_TEST_PROBE_MEMORY_MAINMEM_SIZE_KB not defined
#endif

/*
 * Tests the allocator overhead.
 *
 * When we exhaust all available memory, we expect to have
 * allocated an amount roughly corresponding to the size of
 * the RAM (minus allocator overhead).
 */

/**
 * This defines an acceptable overhead, with a generous safety
 * margin. The overhead of a reasonable implementation will be
 * typically less than one percent (for the block sizes the
 * test allocates).
 */
#define THRESHOLD_PERCENT 5

/**
 * Fixed amount of memory that we expect is already used by stacks
 * of initial threads etc.
 */
#define STARTUP_ALLOCATIONS_KB 16

#define MIN_SIZE 8
#define START_SIZE (MIN_SIZE << 12)

/** Convert bytes to kilobytes with rounding. */
static inline size_t b2kb(size_t bytes) {
    return (bytes + 512) / 1024;
}

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
    ktest_start("heap/overhead");

    size_t ram_size_kb = KERNEL_TEST_PROBE_MEMORY_MAINMEM_SIZE_KB;
    size_t threshold_kb = THRESHOLD_PERCENT * ram_size_kb / 100;
    size_t kernel_size_kb = b2kb(((uintptr_t)&_kernel_end) - 0x80000000);
    printk("MSIM has mainmem of %uKB, kernel has %uKB.\n",
            ram_size_kb, kernel_size_kb);

    size_t actual_size_kb = ram_size_kb - kernel_size_kb;
    size_t minimum_to_pass_kb = actual_size_kb;
    if (minimum_to_pass_kb > threshold_kb) {
        minimum_to_pass_kb -= threshold_kb;
    }
    if (minimum_to_pass_kb > STARTUP_ALLOCATIONS_KB) {
        minimum_to_pass_kb -= STARTUP_ALLOCATIONS_KB;
    }

    size_t detected_kb = b2kb(exhaust_memory(START_SIZE, MIN_SIZE));
    printk("Detected %uKB (checking against minimum %uKB).",
            detected_kb, minimum_to_pass_kb);

    ktest_assert(detected_kb > minimum_to_pass_kb, "heap allocator overhead is too high");

    ktest_passed();
}
