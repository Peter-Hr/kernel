// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_CONTEXT_H
#define _PROC_CONTEXT_H

/*
 * This file is shared between C and assembler sources, because we
 * want to keep information on low-level structures in the same file.
 * We use ifdefs to include the portions relevant to each context.
 */

/** Size of the context_t structure below, needed in assembler code. */
#define CONTEXT_SIZE (14 * 4)

#ifndef __ASSEMBLER__

#include <types.h>

/**
 * CPU context
 *
 * This context structure is used for switching between threads in the
 * kernel. We could save all the registers, but most of them have been
 * already saved on the stack by the compiler.
 *
 * This is because the RISC-V ABI mandates that a function only needs
 * to preserve the callee-saved temporaries ($s1-$11), the stack pointer
 * ($sp), and the frame pointer ($fp, which is also considered $s0 in case
 * a function does not use a frame pointer). We also need to preserve
 * the return address ($ra). Preserving other registers across a
 * function call is the responsibility of the caller.
 *
 * From the perspective of the C compiler, context switch is just a
 * function call, so if the compiler wants to preserve registers other
 * than those listed, it must save them on the stack before the call.
 * This also holds for all callers higher up in the call stack.
 */
typedef struct {
    // Callee-saved temporaries (s0-s11 + sp).
    unative_t s0;
    unative_t s1;
    unative_t s2;
    unative_t s3;
    unative_t s4;
    unative_t s5;
    unative_t s6;
    unative_t s7;
    unative_t s8;
    unative_t s9;
    unative_t s10;
    unative_t s11;

    unative_t sp;
    unative_t ra;
} context_t;

void cpu_switch_context(context_t* this_context, context_t* next_context);

#else /* __ASSEMBLER__ */
// clang-format off

/*
 * The register offsets MUST match the context_t structure.
 */
.set CONTEXT_S0_OFFSET,  0*4
.set CONTEXT_S1_OFFSET,  1*4
.set CONTEXT_S2_OFFSET,  2*4
.set CONTEXT_S3_OFFSET,  3*4
.set CONTEXT_S4_OFFSET,  4*4
.set CONTEXT_S5_OFFSET,  5*4
.set CONTEXT_S6_OFFSET,  6*4
.set CONTEXT_S7_OFFSET,  7*4
.set CONTEXT_S8_OFFSET,  8*4
.set CONTEXT_S9_OFFSET,  9*4
.set CONTEXT_S10_OFFSET, 10*4
.set CONTEXT_S11_OFFSET, 11*4

.set CONTEXT_SP_OFFSET, 12*4
.set CONTEXT_RA_OFFSET, 13*4


/*
 * The CONTEXT_SAVE_GENERAL_REGISTERS macro stores general registers
 * into the context_t structure. The macro can be used with any general
 * purpose register (no registers are destroyed in this code).
 */
.macro CONTEXT_SAVE_GENERAL_REGISTERS base

    sw s0,  CONTEXT_S0_OFFSET(\base)
    sw s1,  CONTEXT_S1_OFFSET(\base)
    sw s2,  CONTEXT_S2_OFFSET(\base)
    sw s3,  CONTEXT_S3_OFFSET(\base)
    sw s4,  CONTEXT_S4_OFFSET(\base)
    sw s5,  CONTEXT_S5_OFFSET(\base)
    sw s6,  CONTEXT_S6_OFFSET(\base)
    sw s7,  CONTEXT_S7_OFFSET(\base)
    sw s8,  CONTEXT_S8_OFFSET(\base)
    sw s9,  CONTEXT_S9_OFFSET(\base)
    sw s10, CONTEXT_S10_OFFSET(\base)
    sw s11, CONTEXT_S11_OFFSET(\base)

    sw ra, CONTEXT_RA_OFFSET(\base)
    sw sp, CONTEXT_SP_OFFSET(\base)


.endm CONTEXT_SAVE_GENERAL_REGISTERS


/*
 * The CONTEXT_LOAD_GENERAL_REGISTERS macro loads general registers from
 * the context_t structure. The macro \base parameter should be a register
 * that is different from the registers being restored. The $sp register is
 * an exception, because it is restored last (on purpose).
 *
 * Also keep in mind that the macro restores the stack pointer, which means
 * that it will switch to another stack.
 */
.macro CONTEXT_LOAD_GENERAL_REGISTERS base

    lw s0,  CONTEXT_S0_OFFSET(\base)
    lw s1,  CONTEXT_S1_OFFSET(\base)
    lw s2,  CONTEXT_S2_OFFSET(\base)
    lw s3,  CONTEXT_S3_OFFSET(\base)
    lw s4,  CONTEXT_S4_OFFSET(\base)
    lw s5,  CONTEXT_S5_OFFSET(\base)
    lw s6,  CONTEXT_S6_OFFSET(\base)
    lw s7,  CONTEXT_S7_OFFSET(\base)
    lw s8,  CONTEXT_S8_OFFSET(\base)
    lw s9,  CONTEXT_S9_OFFSET(\base)
    lw s10, CONTEXT_S10_OFFSET(\base)
    lw s11, CONTEXT_S11_OFFSET(\base)

    lw ra, CONTEXT_RA_OFFSET(\base)
    lw sp, CONTEXT_SP_OFFSET(\base)

.endm CONTEXT_LOAD_GENERAL_REGISTERS

// clang-format on
#endif /* __ASSEMBLER__ */

#endif
