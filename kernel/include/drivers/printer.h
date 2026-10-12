// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DRIVERS_PRINTER_H_
#define _DRIVERS_PRINTER_H_

/** Console hardware address.
 *
 * Note that this is a virtual address used by C code. The address is from
 * kernel region and does not require any special setup of virtual memory
 * translation.
 */
#define PRINTER_ADDRESS (0x90000000)

#ifndef __ASSEMBLER__

/** Display a single character.
 *
 * @param c The character to display.
 */
static inline void printer_putchar(const char c) {
    (*(volatile char*)PRINTER_ADDRESS) = c;
}

#endif
#endif
