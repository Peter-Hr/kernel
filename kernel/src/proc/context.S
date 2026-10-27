// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <proc/context.h>

.text

/*
 * void cpu_switch_context(
 *     context_t * a0_this_context,
 *     context_t * a1_next_context
 * )
 *
 * Switches processor to another context. The first argument points to
 * "this" context, i.e., the context structure where the current state
 * of the CPU will be stored.
 *
 * The second argument points to the "next" context, from which the
 * state of the CPU will be restored to resume execution.
 *
 * Note that from the perspective of the RISC-V ABI, the caller can only
 * assume that this function will preserve the callee-saved registers
 * s0-s11 along with sp and ra.
 *
 * We do not need to save the other registers here, because whoever needed
 * them preserved had to save them on the stack earlier. This is why the
 * context_t structure only contains the callee-saved registers.
 */
.globl cpu_switch_context

cpu_switch_context:
    /*
     * Save the general-purpose registers into the context
     * structure pointed to by $a0.
     */
    CONTEXT_SAVE_GENERAL_REGISTERS a0

    /*
     * Load the general-purpose registers from the target context.
     * The register $a1 is not being restored so we can use it as
     * a base register. Note that this switches to another stack!
     */
    CONTEXT_LOAD_GENERAL_REGISTERS a1

    ret

.end cpu_switch_context
