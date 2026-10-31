// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/align.h>
#include <drivers/printer.h>
#include <lib/print.h>
#include <mm/heap.h>
#include <mm/self.h>

typedef struct {
    uintptr_t start;
    uintptr_t current;
    uintptr_t end;
} heap_state_t;

static heap_state_t heap;
static list_t blocks;

static uintptr_t detect_end_of_available_memory(uintptr_t start) {
    uintptr_t current = uintptr_align_up(start, 0x1000);

    while (current < 0x90000000) {
        volatile uint32_t* ptr = (volatile uint32_t*)current;

        *ptr = 0xDEADBEEF; // TEST VALUE

        if (*ptr != 0xDEADBEEF) {
            break; // IF NOT SAVED, OUT OF MEMORY
        }

        current += 0x1000;
    }

    return current;
}

/** Heap initialization.
 *
 * Called once during system boot.
 */
void heap_init(void) {
    heap.start = uintptr_align_up(kernel_get_image_end(), 8);
    heap.end = detect_end_of_available_memory(heap.start);

    if (HEAP_USE_BUMP) {
        heap.current = heap.start;
    } else {
        heap_block_t* first = (heap_block_t*)heap.start;
        first->used = false;
        first->size = heap.end - heap.start - sizeof(heap_block_t);

        list_init(&blocks);
        list_append(&blocks, &first->link);
        printk("heap: 0x%x-0x%x, first block size %u\n", heap.start, heap.end, first->size);
    }
}

static void* bump_kmalloc(size_t size) {
    uintptr_t current_aligned = uintptr_align_up(heap.current, 8);

    size_t aligned_size = uintptr_align_up(size, 8);

    if (aligned_size < size) {
        return NULL;
    }

    if (current_aligned + aligned_size < current_aligned || current_aligned + aligned_size > heap.end) {
        return NULL;
    }

    void* result = (void*)heap.current;
    heap.current += aligned_size;
    return result;
}

static void link_insert_after(link_t* x, link_t* new_link) {
    link_t* y = x->next;

    new_link->prev = x;
    new_link->next = y;
    y->prev = new_link;
    x->next = new_link;
}

static void* real_kmalloc(size_t size) {
    size_t aligned_size = uintptr_align_up(size, 8);

    if (aligned_size < size) {
        return NULL;
    }

    list_foreach(blocks, heap_block_t, link, hdr) {
        if (hdr->used || aligned_size > hdr->size) {
            continue;
        }
        size_t leftover = hdr->size - aligned_size;

        if (leftover < 24) {
            hdr->used = true;
            return hdr + 1;
        } else {
            hdr->size = leftover - 16;
            heap_block_t* new_hdr = (heap_block_t*)((uint8_t*)hdr + 16 + hdr->size);
            new_hdr->used = true;
            new_hdr->size = aligned_size;
            link_insert_after(&hdr->link, &new_hdr->link);
            return new_hdr + 1;
        }
    }
    return NULL;
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
    if (HEAP_USE_BUMP) {
        return bump_kmalloc(size);
    }
    return real_kmalloc(size);
}

static void bump_kfree(void* ptr) {
}

static void real_kfree(void* ptr) {
    heap_block_t* hdr = (heap_block_t*)ptr - 1; // header
    hdr->used = false;

    link_t* right_link = hdr->link.next;
    if (right_link != &blocks.head) {
        heap_block_t* right = list_item(right_link, heap_block_t, link);
        if (!right->used) {
            hdr->size += 16 + right->size; // hdr eats right
            list_remove(&right->link); // right gone
        }
    }

    link_t* left_link = hdr->link.prev;
    if (left_link != &blocks.head) {
        heap_block_t* left = list_item(left_link, heap_block_t, link);
        if (!left->used) {
            left->size += 16 + hdr->size; // left eats hdr
            list_remove(&hdr->link); // hdr gone
        }
    }
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
    if (HEAP_USE_BUMP) {
        bump_kfree(ptr);
    }
    return real_kfree(ptr);
}

/** is address inside the heap? */
bool heap_contains(uintptr_t addr) {
    return addr >= heap.start && addr < heap.end;
}