// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DRIVERS_MACHINE_H
#define _DRIVERS_MACHINE_H

/** MSIM-specific instruction for dumping general registers. */
#define machine_dump_rd_asm .word 0x8C100073

/** MSIM-specific instruction for halting the simulation (power-off). */
#define machine_halt_asm .word 0x8C000073

/** MSIM-specific instruction for entering interactive mode. */
#define machine_enter_debugger_asm ebreak

/** MSIM-specific instruction for dumping CSR indexed by t0. */
#define machine_dump_csr_t0_asm .word 0x8C4002F3

#ifndef __ASSEMBLER__

/** Halts the (virtual) machine. */
static __attribute__((noreturn)) inline void machine_halt(void) {
    __asm__ volatile(".word 0x8C000073\n");
    while (1) {
        // Unreachable but required by the compiler
    }
}

/** Enter MSIM interactive mode. */
static inline void machine_enter_debugger(void) {
    __asm__ volatile("ebreak\n");
}

/** Enter MSIM interactive mode and then halt MSIM. */
static __attribute__((noreturn)) inline void machine_halt_via_debugger(void) {
    machine_enter_debugger();
    machine_halt();
}

/** Start dumping executed instructions to MSIM output */
static inline void machine_set_trace(void) {
    __asm__ volatile(".word 0x8C200073\n");
}

/** Stop dumping executed instructions to MSIM output */
static inline void machine_reset_trace(void) {
    __asm__ volatile(".word 0x8C300073\n");
}

/** Dump the specified CSR by number
 *
 * @param csr_num The number of the CSR to dump
 */
static inline void machine_dump_csr_by_num(int csr_num) {
    /* Use custom instruction which dumps a CSR that
     * is specified by its number in a register (in this case t0)
     */
    __asm__ volatile(
            "mv t0, %0\n"
            ".word 0x8C4002F3\n"
            :
            : "r"(csr_num)
            : "t0");
}

#endif

#endif
