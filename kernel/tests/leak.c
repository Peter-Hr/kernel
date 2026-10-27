// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Charles University

#include <adt/align.h>
#include <ktest.h>
#include <ktest/leak.h>
#include <mm/heap.h>

#define MAX_RECORDED_OBJECTS 1024
#define STATIC_HEAP_SIZE (1024 * 128)

#define log_info(fmt, ...) printk("[leak-test  INFO] " fmt "\n", ##__VA_ARGS__)

#ifdef KERNEL_DEBUG
#undef KERNEL_DEBUG_LEAKS
#define KERNEL_DEBUG_LEAKS
#endif

#ifdef KERNEL_DEBUG_LEAKS
#define log_debug(fmt, ...) printk("[leak-test DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define log_debug(...) ((void)0)
#endif

#define GET_CALLER_ADDRESS() ((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))

typedef struct {
    uintptr_t addresses[MAX_RECORDED_OBJECTS];
    size_t first_free_index;
} allocated_objects_t;

#define LEAK_CHECK_STATUS_NONE 0
#define LEAK_CHECK_STATUS_COLLECTING 1
#define LEAK_CHECK_STATUS_INJECTING 2

static int leak_checking_status = LEAK_CHECK_STATUS_NONE;

static size_t loop_index = 0;
static size_t allocation_counter = 0;
static size_t allocation_collection_phase_counter = 0;

static uint8_t heap[STATIC_HEAP_SIZE];
static size_t heap_first_free_index = 0;
static allocated_objects_t heap_objects;

static void allocated_objects_clear_unsafe(allocated_objects_t* objects) {
    objects->first_free_index = 0;
}

static void allocated_objects_allocate_unsafe(allocated_objects_t* objects, uintptr_t address) {
    ktest_assert(objects->first_free_index < MAX_RECORDED_OBJECTS, "Too many allocated objects required, cannot continue.");
    ktest_assert(address != 0, "allocated object cannot be at address zero");
    objects->addresses[objects->first_free_index] = address;
    objects->first_free_index++;
}

static void allocated_objects_release_unsafe(allocated_objects_t* objects, uintptr_t address) {
    for (size_t i = 0; i < objects->first_free_index; i++) {
        if (objects->addresses[i] == address) {
            log_debug("Object 0x%x correctly released.", address);
            objects->addresses[i] = 0;
            return;
        }
    }
    ktest_assert(false, "Trying to release unknown object 0x%x", address);
}

static void allocated_objects_check_all_released_unsafe(allocated_objects_t* objects) {
    for (size_t i = 0; i < objects->first_free_index; i++) {
        ktest_assert(objects->addresses[i] == 0, "Object at 0x%x not released (index %u)", objects->addresses[i], i);
    }
}

static bool should_inject_oom_unsafe(void) {
    if (leak_checking_status != LEAK_CHECK_STATUS_INJECTING) {
        return false;
    }
    return allocation_counter == loop_index;
}

static void on_object_allocation_unsafe(allocated_objects_t* objects, uintptr_t address) {
    // Certainly increment allocation counter
    allocation_counter++;

    // Record how many allocations were necessary in a clean run
    if (leak_checking_status == LEAK_CHECK_STATUS_COLLECTING) {
        allocation_collection_phase_counter++;
    }

    // If we are inside an actual leak check, record everything that was allocated
    if (leak_checking_status == LEAK_CHECK_STATUS_INJECTING) {
        allocated_objects_allocate_unsafe(objects, address);
    }
}

void heap_init(void) {
    allocated_objects_clear_unsafe(&heap_objects);
}

static void* kmalloc_unsafe(size_t size, uintptr_t caller_addr) {
    // First handle (semi-random) injection of NULL
    if (should_inject_oom_unsafe()) {
        log_debug("Injecting NULL (requested size %u, caller 0x%x)", size, caller_addr);
        return NULL;
    }

    // Actually try to allocate memory if possible
    size_t next_first_free = size_align_up(heap_first_free_index + size, 8);
    if (next_first_free >= STATIC_HEAP_SIZE) {
        panic("Not enough memory for the test.");
        return NULL;
    }
    void* result = &heap[heap_first_free_index];
    heap_first_free_index = next_first_free;

    // Handle leak checking accounting
    on_object_allocation_unsafe(&heap_objects, (uintptr_t)result);

    return result;
}

void* kmalloc(size_t size) {
    uintptr_t caller = GET_CALLER_ADDRESS();
    void* result = kmalloc_unsafe(size, caller);
    return result;
}

static void kfree_unsafe(uintptr_t ptr) {
    if (leak_checking_status != LEAK_CHECK_STATUS_INJECTING) {
        return;
    }
    allocated_objects_release_unsafe(&heap_objects, ptr);
}

void kfree(void* ptr) {
    kfree_unsafe((uintptr_t)ptr);
}

void ktest_leak_check(leak_checking_func_t func, void* data) {
    log_info("Test initializing (will record all allocations).");

    leak_checking_status = LEAK_CHECK_STATUS_COLLECTING;
    func(data);

    leak_checking_status = LEAK_CHECK_STATUS_INJECTING;

    size_t check_loops = allocation_collection_phase_counter + 1;
    for (loop_index = 0; loop_index < check_loops; loop_index++) {
        allocated_objects_clear_unsafe(&heap_objects);
        allocation_counter = 0;
        log_info("Starting test loop %u (of %u)", loop_index + 1, check_loops);
        func(data);
        log_debug("Checking for leaks.");
        allocated_objects_check_all_released_unsafe(&heap_objects);
    }
    log_info("Test done, no leak found.");
}
