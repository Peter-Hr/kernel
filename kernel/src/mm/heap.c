// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/align.h>
#include <drivers/printer.h>
#include <mm/heap.h>
#include <mm/self.h>

typedef struct {
    uintptr_t start;
    uintptr_t current;
    uintptr_t end;
} heap_state_t;

static heap_state_t heap;

static uintptr_t detect_end_of_available_memory(uintptr_t start) {
    return 0;
}

/** Heap initialization.
 *
 * Called once during system boot.
 */
void heap_init(void) {
    heap.start = kernel_get_image_end();
    heap.current = heap.start;
    heap.end = detect_end_of_available_memory(heap.start);
}


/** Allocate size bytes on heap.
 *
 * The behavior when size is zero is not defined and none of the provided
 * tests will ever call kmalloc with 0 as the parameter. The implementation may
 * choose to return NULL or a unique pointer for such call (i.e., follow the
 * C standard). Note that causing a kernel panic or not checking for such
 * case at all is also a valid option.
 *
 * @param size Amount of memory to allocate.
 * @return Pointer to allocated memory.
 * @retval NULL Out of memory.
 */
void* kmalloc(size_t size) {
    size_t aligned_size = uintptr_align_up(size, 4);

    if (heap.current + aligned_size > heap.end) {
        return NULL;
    }

    void* result = (void*)heap.current;
    heap.current += aligned_size;
    return result;
}

/** Free memory previously allocated by kmalloc.
 *
 * The behavior when ptr is NULL is not defined and none of the provided
 * tests will ever call kfree with NULL as the parameter. Note that accepting
 * NULL as a parameter follows the C standard behavior, i.e. everything that
 * malloc can return can be passed to free. In the context of a kernel,
 * aborting the execution on receiving NULL is also an option as that might
 * signal an abnormal situation in a different module.
 *
 * @param ptr Pointer to memory to be freed.
 */
void kfree(void* ptr) {
    // unused yet
}
