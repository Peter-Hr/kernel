// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Charles University

/*
 * Check that kernel is robust against a weird thread name.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/thread.h>

#define TRICKY_THREAD_NAME_LENGTH 1024
#define TRICKY_THREAD_NAME_CONSTANT 'U' // 0x55

static char tricky_thread_name[TRICKY_THREAD_NAME_LENGTH];

static void* empty_worker(void* ignored) {
    thread_yield();
    return NULL;
}

void kernel_test(void) {
    ktest_start("thread/names");

    for (size_t index = 0; index < TRICKY_THREAD_NAME_LENGTH; index++) {
        tricky_thread_name[index] = TRICKY_THREAD_NAME_CONSTANT;
    }

    thread_t* worker;
    ktest_thread_create_checked(&worker, empty_worker, NULL, 0, tricky_thread_name);
    ktest_thread_join_checked(worker, NULL);

    ktest_passed();
}
