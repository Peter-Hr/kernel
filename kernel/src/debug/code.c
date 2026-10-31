// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/code.h>
#include <lib/print.h>

static void print_hex_padded(uint32_t val) {
    const char hex_chars[] = "0123456789abcdef";
    
    for (int shift = 28; shift >= 0; shift -= 4) {
        // Vypočítame index (0 až 15) a priamo vypíšeme znak z poľa
        printk("%c", hex_chars[(val >> shift) & 0xF]);
    }
}

/** Dump function code at given address.
 *
 * Generally, the output should look like disassembly without
 * mnemonics part.
 *
 * @param name Function name to print in header.
 * @param address Function address.
 * @instruction_count How many instructions to print.
 */
void debug_dump_function(const char* name, uintptr_t address, size_t instruction_count) {
    if (name == NULL) {
        name = "(null)";
    }

    // hlavička
    print_hex_padded((uint32_t)address);
    printk(" <%s>:\n", name);

    uint32_t* instructions = (uint32_t*)address;

    //každá inštrukcia
    for (size_t i = 0; i < instruction_count; i++) {
        uintptr_t current_address = address + (i * 4);
        
        print_hex_padded((uint32_t)current_address);
        
        printk(":        ");
        
        print_hex_padded(instructions[i]);
        printk("\n");
    }
}
