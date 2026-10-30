// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug.h>
#include <proc/scheduler.h>
#include <proc/thread.h>

static list_t ready_queue;

/** Vráti true, ak v plánovači nie sú žiadne pripravené vlákna. */
bool scheduler_is_empty(void) {
    return list_is_empty(&ready_queue);
}

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    list_init(&ready_queue);
}

/** Marks given thread as ready to be executed.
 *
 * It is expected that this thread would be added at the end
 * of the queue to run in round-robin fashion.
 *
 * @param thread Thread to make runnable.
 */
void scheduler_add_ready_thread(thread_t* thread) {
    if (thread == NULL) {
        return;
    }

    if (!link_is_connected(&thread->ready_link)) {
        list_append(&ready_queue, &thread->ready_link);
    }
}

/** Removes given thread from scheduling.
 *
 * Expected to be called when thread is suspended, terminates execution
 * etc.
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_remove_thread(thread_t* thread) {
    if (thread == NULL) {
        return;
    }

    list_remove(&thread->ready_link);
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    link_t* link = list_pop(&ready_queue);

    if (link == NULL) {
        panic("Scheduler queue is empty, system has nothing to run!");
    }

    thread_t* next_thread = list_item(link, thread_t, ready_link);

    thread_switch_to(next_thread);
}