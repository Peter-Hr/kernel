// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <proc/scheduler.h>
#include <proc/thread.h>

static thread_t* ready_queue_head = NULL;
static thread_t* ready_queue_tail = NULL;

/** Vráti true, ak v plánovači nie sú žiadne pripravené vlákna. */
bool scheduler_is_empty(void) {
    return (ready_queue_head == NULL);
}

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
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

    thread->next_ready = NULL;

    // Pridanie na koniec
    if (ready_queue_tail == NULL) {
        ready_queue_head = thread;
        ready_queue_tail = thread;
    } else {
        ready_queue_tail->next_ready = thread;
        ready_queue_tail = thread;
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
    if (thread == NULL || ready_queue_head == NULL) {
        return;
    }

    thread_t* curr = ready_queue_head;
    thread_t* prev = NULL;

    while (curr != NULL) {
        if (curr == thread) {
            if (prev == NULL) {
                // prvy prvok
                ready_queue_head = curr->next_ready;
            } else {
                // stred
                prev->next_ready = curr->next_ready;
            }

            // posledny
            if (curr == ready_queue_tail) {
                ready_queue_tail = prev;
            }

            curr->next_ready = NULL;
            break;
        }
        prev = curr;
        curr = curr->next_ready;
    }
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    if (ready_queue_head == NULL) {
        panic("Scheduler queue is empty, system has nothing to run!");
    }

    thread_t* next_thread = ready_queue_head;

    // posunut frontu
    ready_queue_head = next_thread->next_ready;
    if (ready_queue_head == NULL) {
        ready_queue_tail = NULL;
    }
    next_thread->next_ready = NULL;

    // zmena kontextu
    thread_switch_to(next_thread);
}