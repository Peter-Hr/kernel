// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_THREAD_H
#define _PROC_THREAD_H

#include <adt/list.h>
#include <errno.h>
#include <proc/context.h>
#include <types.h>

/** Thread stack size.
 *
 * Set quite liberally because stack overflows are notoriously
 * difficult to debug (and difficult to detect too).
 */
#define THREAD_STACK_SIZE 4096

/** Max length of a thread name (excluding the terminating zero). */
#define THREAD_NAME_MAX_LENGTH 31

/** Thread entry function (similar to pthreads). */
typedef void* (*thread_entry_func_t)(void*);

/** Information about any existing thread. */
typedef struct thread thread_t;

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_DEAD,
    THREAD_STATE_SUSPENDED
} thread_state_t;

struct thread {
    char name[THREAD_NAME_MAX_LENGTH + 1];
    thread_entry_func_t entry_func;
    void* data;

    context_t context;

    // --- heap ---
    void* stack_memory; // najmenšia adresa alkovana
    size_t stack_size; // velkost alokovanej pamata

    thread_state_t state; // aktaulny stav threadu
    void* retval; // návratová hodnota funkcie entry_funck(data)

    link_t ready_link;

    bool is_joined; // True, ak už nejaký thread zavolal thread_join na tento thread
    bool is_killed; // True, ak bol thread zabitý
};

void threads_init(void);

errno_t thread_create(thread_t** thread, thread_entry_func_t entry, void* data, unsigned int flags, const char* name);

thread_t* thread_get_current(void);
void thread_yield(void);
void thread_suspend(void);
void thread_finish(void* retval) __attribute__((noreturn));
bool thread_has_finished(thread_t* thread);

errno_t thread_wakeup(thread_t* thread);
errno_t thread_join(thread_t* thread, void** retval);
void thread_switch_to(thread_t* thread);

errno_t thread_kill(thread_t* thread);

#endif
