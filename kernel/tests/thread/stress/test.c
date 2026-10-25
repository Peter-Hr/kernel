// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Stress test for scheduler and thread management.
 *
 * Create up to THREAD_COUNT threads that repeatedly suspend or wake-up
 * other threads. All threads should eventually finish.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/thread.h>

#define THREAD_COUNT 100
#define ACTIONS_MAX 200
#define ACTIONS_MIN 100

typedef struct {
    thread_t* thread;
    unative_t seed;
} worker_info_t;

static volatile bool all_started = false;
static size_t thread_count;
static worker_info_t workers[THREAD_COUNT];

static inline unative_t get_simple_rand(unative_t* seed) {
    *seed = (*seed * 1439) % 211 + 7;
    return (*seed) >> 4;
}

static inline unative_t get_simple_rand_range(unative_t* seed, unative_t lower, unative_t upper) {
    return lower + get_simple_rand(seed) % (upper - lower);
}

static inline void thread_wakeup_robust(thread_t* thread) {
    errno_t err = thread_wakeup(thread);
    if (err == EEXITED) {
        return;
    }
    ktest_assert_errno(err, "thread_wakeup");
}

static void* worker_with_random_action(void* arg) {
    // Spin until all threads are created.
    while (!all_started) {
        thread_yield();
    }

    worker_info_t* myself = (worker_info_t*)arg;

    unsigned int loops = get_simple_rand_range(&myself->seed, ACTIONS_MIN, ACTIONS_MAX);
    for (unsigned int i = 0; i < loops; i++) {
        int action = get_simple_rand(&myself->seed) % 7;
        switch (action) {
        case 0:
            thread_suspend();
            break;
        case 1:
            thread_wakeup_robust(workers[get_simple_rand(&myself->seed) % thread_count].thread);
            break;
        default:
            thread_yield();
        }
    }

    return NULL;
}

static size_t get_running_count() {
    //
    // Without disabling interrupts, the number of running threads is only an
    // approximation (upper bound), but monotonically decreasing towards zero.
    //
    size_t result = 0;
    for (size_t i = 0; i < thread_count; i++) {
        if (!thread_has_finished(workers[i].thread)) {
            result++;
        }
    }

    return result;
}

static void make_thread_name(const char* prefix, char* buffer, size_t size, size_t thread_index) {
    uint8_t digits[sizeof(size_t) * 8];

    // Convert worker index.
    size_t digit_count = 0;
    if (thread_index == 0) {
        digits[0] = 0;
        digit_count = 1;
    } else {
        while (thread_index > 0) {
            digits[digit_count] = thread_index % 10;
            thread_index /= 10;
            digit_count++;
        }
    }

    // Start with worker name.
    while ((size > 0) && (*prefix != 0)) {
        *buffer = *prefix;

        buffer++;
        prefix++;
        size--;
    }

    // Append worker index.
    while ((size > 0) && (digit_count > 0)) {
        *buffer = digits[digit_count - 1] + '0';

        buffer++;
        digit_count--;
        size--;
    }

    *buffer = 0;
}

void kernel_test(void) {
    ktest_start("thread/stress");

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        workers[i].seed = i + 1;
        workers[i].thread = NULL;
    }

    for (thread_count = 0; thread_count < THREAD_COUNT; thread_count++) {
        char name[THREAD_NAME_MAX_LENGTH + 1];
        make_thread_name("worker", name, THREAD_NAME_MAX_LENGTH + 1, thread_count);

        errno_t err = thread_create(&workers[thread_count].thread, worker_with_random_action, &workers[thread_count], 0, name);
        if (err == ENOMEM) {
            break;
        }

        ktest_assert_errno(err, "thread_create");
        dprintk("Created thread #%u: %pT.\n", thread_count, worker[thread_count].thread);
    }

    printk("Created %u threads...\n", thread_count);

    //
    // Signal the worker threads to stop spinning and start running actions.
    // Because some actions suspend their worker thread, we need to wake them
    // from the main test thread (the workers are independent).
    //
    all_started = true;

    size_t count = thread_count;
    while (count > 0) {
        // Try to wake up all "unfinished" threads.
        for (size_t i = 0; i < thread_count; i++) {
            if (!thread_has_finished(workers[i].thread)) {
                thread_wakeup_robust(workers[i].thread);
            }
        }

        count = get_running_count();
        printk(" ... %u threads running ...\n", count);

        // Give proportionally more CPU time to workers.
        for (size_t i = 0; i < 3; i++) {
            thread_yield();
        }
    }

    for (size_t i = 0; i < thread_count; i++) {
        ktest_thread_join_checked(workers[i].thread, NULL);
    }

    ktest_passed();
}
