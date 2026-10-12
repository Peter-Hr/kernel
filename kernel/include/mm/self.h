// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Charles University

#ifndef _MM_SELF_H
#define _MM_SELF_H

/** Where the entrypoint to the kernel lies */
#define KERNEL_ENTRYPOINT 0x80001000

#ifndef __ASSEMBLER__ /* Don't include the following in assembler */

#include <adt/align.h>
#include <types.h>

/** Address at the kernel end.
 *
 * Prefer to use kernel_get_image_end() to read this value.
 *
 * The symbol is defined in the linker script, the actual value is provided during linking.
 * The type of the symbol (a zero-length array of bytes) is chosen to suggest that there
 * may be some bytes there but we do not know how many.
 */
extern uint8_t _kernel_end[0];

/** Get address at the kernel end.
 *
 * This address returns first memory address after kernel code (including space for global
 * variables etc.) where it is safe to allocate memory.
 */
static inline uintptr_t kernel_get_image_end(void) {
    return uintptr_align_up((uintptr_t)_kernel_end, 32);
}

#endif
#endif
