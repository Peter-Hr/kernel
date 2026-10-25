// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Simple test which checks that thread stacks do not overlap.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/thread.h>

/**
 * Expected minimal stack pointer difference between two different threads.
 *
 * We read the stack pointer in identical functions but we take into account
 * that there might be some differences (maybe even randomization in
 * thread_create) and check for smaller difference than full stack size.
 */
#define MIN_STACK_POINTER_DIFFERENCE (THREAD_STACK_SIZE / 2)

/*
 * Helper functions.
 */

static inline uintptr_t get_stack_pointer(void) {
    uintptr_t sp = (uintptr_t)__builtin_stack_address();
    return sp;
}

static size_t get_abs_diff(uintptr_t a, uintptr_t b) {
    if (a > b) {
        return a - b;
    } else {
        return b - a;
    }
}

/**
 * Information about each worker thread.
 */
typedef struct {
    thread_t* thread;
    thread_entry_func_t entry;
    uintptr_t stack_pointer_start;
    uintptr_t stack_pointer_end;
    volatile bool started;
    const char* name;
} info_t;

/** Initialize worker info. */
static void info_init(info_t* info, const char* name, thread_entry_func_t entry) {
    info->thread = NULL;
    info->entry = entry;
    info->stack_pointer_start = 0;
    info->stack_pointer_end = 0;
    info->started = false;
    info->name = name;
}

/** Barrier for ensuring all threads are running at once. */
static volatile bool all_started = false;

/**
 * The actual worker function for all layers. We disable inlining
 * of this function so that both intermediate and leaf workers
 * actually call it.
 */
__attribute__((noinline)) static void* layer_worker(int layer, info_t* my_info, int offset_one, int offset_two) {
    my_info->stack_pointer_start = get_stack_pointer();
    dprintk("%pT: layer %d (%s) running\n", thread_get_current(), layer, my_info->name);

    if (offset_one != 0) {
        info_t* nested_one = my_info + offset_one;
        ktest_thread_create_checked(&nested_one->thread, nested_one->entry, NULL, 0, nested_one->name);
    }

    if (offset_two != 0) {
        info_t* nested_two = my_info + offset_two;
        ktest_thread_create_checked(&nested_two->thread, nested_two->entry, NULL, 0, nested_two->name);
    }

    my_info->started = true;
    while (!all_started) {
        thread_yield();
    }

    my_info->stack_pointer_end = get_stack_pointer();
    thread_yield();

    return NULL;
}

/*
 * kernel_test
 *  [ 0] layer_one
 *  [ 1]   layer_two
 *  [ 2]     layer_three
 *  [ 3]     layer_three
 *  [ 4]   layer_two
 *  [ 5]     layer_three
 *  [ 6]     layer_three
 *  [ 7] layer_one
 *  [ 8]   layer_two
 *  [ 9]     layer_three
 *  [10]     layer_three
 *  [11]   layer_two
 *  [12]     layer_three
 *  [13]     layer_three
 */

#define THREAD_COUNT 14
static info_t infos[THREAD_COUNT];

/*
 * Normally, there would be just a single parametric thread function for
 * all worker threads. However, because we do not require support for the
 * thread function argument, we need a dedicated thread function for each
 * worker to capture the arguments for the actual worker function.
 */

#define layer_1_worker(info) layer_worker(1, (info), 1, 4)
#define layer_2_worker(info) layer_worker(2, (info), 1, 2)
#define layer_3_worker(info) layer_worker(3, (info), 0, 0)

static void* worker_00(void* arg_unused) {
    return layer_1_worker(infos + 0);
}

static void* worker_01(void* arg_unused) {
    return layer_2_worker(infos + 1);
}

static void* worker_02(void* arg_unused) {
    return layer_3_worker(infos + 2);
}

static void* worker_03(void* arg_unused) {
    return layer_3_worker(infos + 3);
}

static void* worker_04(void* arg_unused) {
    return layer_2_worker(infos + 4);
}

static void* worker_05(void* arg_unused) {
    return layer_3_worker(infos + 5);
}

static void* worker_06(void* arg_unused) {
    return layer_3_worker(infos + 6);
}

static void* worker_07(void* arg_unused) {
    return layer_1_worker(infos + 7);
}

static void* worker_08(void* arg_unused) {
    return layer_2_worker(infos + 8);
}

static void* worker_09(void* arg_unused) {
    return layer_3_worker(infos + 9);
}

static void* worker_10(void* arg_unused) {
    return layer_3_worker(infos + 10);
}

static void* worker_11(void* arg_unused) {
    return layer_2_worker(infos + 11);
}

static void* worker_12(void* arg_unused) {
    return layer_3_worker(infos + 12);
}

static void* worker_13(void* arg_unused) {
    return layer_3_worker(infos + 13);
}

void kernel_test(void) {
    ktest_start("thread/stack");

    info_init(&infos[0], "tester-00", &worker_00);
    info_init(&infos[1], "tester-01", &worker_01);
    info_init(&infos[2], "tester-02", &worker_02);
    info_init(&infos[3], "tester-03", &worker_03);
    info_init(&infos[4], "tester-04", &worker_04);
    info_init(&infos[5], "tester-05", &worker_05);
    info_init(&infos[6], "tester-06", &worker_06);
    info_init(&infos[7], "tester-07", &worker_07);
    info_init(&infos[8], "tester-08", &worker_08);
    info_init(&infos[9], "tester-09", &worker_09);
    info_init(&infos[10], "tester-10", &worker_10);
    info_init(&infos[11], "tester-11", &worker_11);
    info_init(&infos[12], "tester-12", &worker_12);
    info_init(&infos[13], "tester-13", &worker_13);

    ktest_thread_create_checked(&infos[0].thread, &worker_00, NULL, 0, infos[0].name);
    ktest_thread_create_checked(&infos[7].thread, &worker_07, NULL, 0, infos[7].name);

    while (true) {
        printk("Waiting for all threads to start ... \n");
        bool all_on_barrier = true;
        for (int i = 0; i < THREAD_COUNT; i++) {
            if (!infos[i].started) {
                all_on_barrier = false;
                break;
            }
        }
        if (all_on_barrier) {
            break;
        }
        thread_yield();
    }
    printk("All threads started.\n");
    all_started = true;
    thread_yield();

    printk("Joining threads ...\n");
    for (int i = 0; i < THREAD_COUNT; i++) {
        ktest_thread_join_checked(infos[i].thread, NULL);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        info_t* this = &infos[i];
        printk("Checking thread %d (%s)\n", i, this->name);

        ktest_assert(this->stack_pointer_start != 0, "thread function to run at all");
        ktest_assert(this->stack_pointer_end != 0, "thread function to run at all");
        size_t self_sp_diff = get_abs_diff(this->stack_pointer_start, this->stack_pointer_end);
        ktest_assert(self_sp_diff < 1024, "$sp moves too much inside one function");

        for (int j = i + 1; j < THREAD_COUNT; j++) {
            info_t* that = &infos[j];
            size_t sp_diff = get_abs_diff(this->stack_pointer_start, that->stack_pointer_start);
            ktest_assert(sp_diff >= MIN_STACK_POINTER_DIFFERENCE,
                    "$sp of different threads are too close together");
        }
    }

    ktest_passed();
}
