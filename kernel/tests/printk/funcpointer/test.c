// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Charles University

#include <ktest.h>
#include <lib/print.h>

void kernel_test(void) {
    puts(KTEST_EXPECTED "Loader: 0xf0000000[800012b7 00028067 00000000 00000000].");
    printk(KTEST_ACTUAL "Loader: %pF.\n", 0xF0000000);

    ktest_passed();
}
