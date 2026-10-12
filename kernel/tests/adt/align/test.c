// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Charles University

#include <adt/align.h>
#include <ktest.h>

#define ASSERT_ALIGN_UP(expected, value, alignment) \
    ktest_assert(uintptr_align_up(value, alignment) == expected, \
            "aligned-up value is %u, expecting %u", \
            uintptr_align_up(value, alignment), \
            expected); \
    ktest_assert(size_align_up(value, alignment) == expected, \
            "aligned-up value is %u, expecting %u", \
            size_align_up(value, alignment), \
            expected);

#define ASSERT_ALIGN_DOWN(expected, value, alignment) \
    ktest_assert(uintptr_align_down(value, alignment) == expected, \
            "aligned-down value is %u, expecting %u", \
            uintptr_align_down(value, alignment), \
            expected);

void kernel_test(void) {
    ktest_start("adt/align");

    ASSERT_ALIGN_UP(0, 0, 4);
    ASSERT_ALIGN_UP(0, 0, 32);

    ASSERT_ALIGN_UP(4, 1, 4);
    ASSERT_ALIGN_UP(4, 2, 4);
    ASSERT_ALIGN_UP(4, 3, 4);
    ASSERT_ALIGN_UP(4, 4, 4);
    ASSERT_ALIGN_UP(8, 5, 4);

    ASSERT_ALIGN_UP(512, 512, 512);
    ASSERT_ALIGN_UP(1024, 513, 512);

    //

    ASSERT_ALIGN_DOWN(0, 0, 4);
    ASSERT_ALIGN_DOWN(0, 3, 4);
    ASSERT_ALIGN_DOWN(4, 4, 4);
    ASSERT_ALIGN_DOWN(4, 7, 4);
    ASSERT_ALIGN_DOWN(8, 8, 4);
    ASSERT_ALIGN_DOWN(8, 11, 4);
    ASSERT_ALIGN_DOWN(12, 12, 4);

    ASSERT_ALIGN_DOWN(0, 0, 16);
    ASSERT_ALIGN_DOWN(0, 15, 16);
    ASSERT_ALIGN_DOWN(16, 16, 16);
    ASSERT_ALIGN_DOWN(16, 31, 16);
    ASSERT_ALIGN_DOWN(32, 32, 16);
    ASSERT_ALIGN_DOWN(32, 47, 16);
    ASSERT_ALIGN_DOWN(48, 48, 16);

    ASSERT_ALIGN_DOWN(32, 32, 32);
    ASSERT_ALIGN_DOWN(32, 63, 32);
    ASSERT_ALIGN_DOWN(64, 64, 32);

    ktest_passed();
}
