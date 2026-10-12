// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/csr.h>
#include <drivers/machine.h>


.section .handlers, "ax"

/*
 * Machine-mode trap handler.
 *
 * Services any exceptions/interrupts not delegated to supervisor mode.
 *
 * This handler runs in M-mode and simply dumps general purpose registers
 * together with 'mepc', 'mcause' and 'mtval' CSRs. Then it enters MSIM
 * interactive mode to allow the user to inspect registers, memory or
 * anything else of interest. Finally, it halts the simulator, because
 * there is nothing else to be done.
 */
.balign 4
machine_trap_handler:
    // Dump general registers.
    machine_dump_rd_asm

    // Dump relevant Control and Status Registers (CSRs).
    li t0, CSR_MEPC
    machine_dump_csr_t0_asm
    li t0, CSR_MCAUSE
    machine_dump_csr_t0_asm
    li t0, CSR_MTVAL
    machine_dump_csr_t0_asm

    // Switch MSIM to interactive mode first, then halt it.
    machine_enter_debugger_asm
    machine_halt_asm


.section .head, "ax"

/*
 * Kernel entry point (the boot loader code at 0xF0000000 jumps here).
 *
 * The Machine Trap-Vector Base-Address (mtvec) register are set to address of
 * the machine mode trap handler. Because the lowest two bits of the handler
 * addresses will be zero (due to 4-byte alignment), the vector MODE field is
 * set to 'direct', which means that all exceptions cause the processor to jump
 * to the trap handler BASE address.
 *
 * The MODE field in the Supervisor Address Translation and Protection (satp)
 * register is set to zero (`bare` mode), which disables address translation
 * in supervisor mode, thus providing identity mapping without page tables.
 *
 * Prior to jumping to kernel C code, the stack pointer is initialized so that
 * the area immediately preceding the 'start' entry point becomes an initial
 * 4-KiB stack (which should be enough for everybody).
 *
 * The final jump to the kernel C entry point ('kernel_main' function) does not
 * change the processor privilege mode in any way (the code continues to run
 * in 'machine' mode).
 */
.org 0x1000
start:
    // Set M-mode trap handler base, use 'direct' vector mode.
    la t0, machine_trap_handler
    csrw mtvec, t0

    // Enable 'bare' mode to disable address translation.
    // Requires all other bits in 'satp' to be zero.
    csrw satp, zero

    // Setup initial stack and jump to kernel C code in 'kernel_main'.
    la sp, start
    jal ra, kernel_main


    // Paranoia: this is unreachable, but just in case...
    machine_halt_asm
    loop:
        j loop
