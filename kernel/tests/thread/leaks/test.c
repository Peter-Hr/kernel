// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Basic thread leak test.
 */

#include <ktest.h>
#include <ktest/leak.h>
#include <ktest/thread.h>
#include <proc/thread.h>

#define LOOPS 5

static void* empty_worker(void* ignored) {
    for (int i = 0; i < LOOPS; i++) {
        thread_yield();
    }
    return NULL;
}

static void leak_check(void* ignored) {
    thread_t* worker;
    int rc = thread_create(&worker, empty_worker, NULL, 0, "leak-worker");
    if (rc != EOK) {
        // This is actually okay, probably the leak test has inserted some
        // NULLs so that the thread was not created.
        return;
    }
    ktest_thread_join_checked(worker, NULL);
}

void kernel_test(void) {
    ktest_start("thread/leaks");

    ktest_leak_check(leak_check, NULL);

    ktest_passed();
}
