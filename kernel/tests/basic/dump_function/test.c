// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/code.h>
#include <ktest.h>

/*
 * Test for function dumping routine. We check on loader code
 * as that is the only code that we can be sure how will be compiled
 * and where it would reside in memory.
 */

void kernel_test(void) {
    ktest_start("basic/dump_function");

    puts(KTEST_BLOCK_EXPECTED "f0000000 <loader>:");
    puts(KTEST_BLOCK_EXPECTED "f0000000:        800012b7");
    puts(KTEST_BLOCK_EXPECTED "f0000004:        00028067");
    puts(KTEST_BLOCK_EXPECTED "f0000008:        00000000");
    puts(KTEST_BLOCK_EXPECTED "f000000c:        00000000");

    debug_dump_function("loader", 0xF0000000, 4);

    ktest_passed();
}
