// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <abi.h>
#include <proc/context.h>
#include <proc/scheduler.h>
#include <proc/thread.h>

/** Initialize support for threading.
 *
 * Called once at system boot.
 */
void threads_init(void) {
}

/** Create a new thread.
 *
 * The thread is automatically placed into the queue of ready threads.
 *
 * This function allocates space for both stack and the thread_t structure
 * (hence the double <code>**</code> in <code>thread_out</code>).
 *
 * @param thread_out Where to place the initialized thread_t structure.
 * @param entry Thread entry function.
 * @param data Data for the entry function.
 * @param flags Flags (unused).
 * @param name Thread name (for debugging purposes).
 * @return Error code.
 * @retval EOK Thread was created and started (added to ready queue).
 * @retval ENOMEM Not enough memory to complete the operation.
 * @retval EINVAL Invalid flags (unused).
 */
errno_t thread_create(thread_t** thread_out, thread_entry_func_t entry, void* data, unsigned int flags, const char* name) {
    return ENOIMPL;
}

/** Return information about currently executing thread.
 *
 * @retval NULL When no thread was started yet.
 */
thread_t* thread_get_current(void) {
    return NULL;
}

/** Yield the processor. */
void thread_yield(void) {
}

/** Current thread stops execution and is not scheduled until woken up. */
void thread_suspend(void) {
}

/** Terminate currently running thread.
 *
 * Thread can (normally) terminate in two ways: by returning from the entry
 * function or by calling this function. The parameter to this function then
 * has the same meaning as the return value from the entry function.
 *
 * Note that this function never returns.
 *
 * @param retval Data to return in thread_join.
 */
void thread_finish(void* retval) {
    while (1) {
    }
}

/** Tells if a thread finished its execution.
 *
 * Thread finished the execution either when it returned from the entry
 * function, called thread_finish() or was already killed.
 *
 * In other words, when this function returns true, thread_join() should
 * return immediately (it is not required but it is a natural consequence
 * of the above).
 *
 * This function may not be called when thread_join() was already called
 * on the thread in question (because the thread pointer by that time will
 * not be valid).
 *
 * @param thread Thread in question.
 */
bool thread_has_finished(thread_t* thread) {
    return false;
}

/** Wakes-up existing thread.
 *
 * Note that waking-up a running (or ready) thread has no effect (i.e. the
 * function shall not count wake-ups and suspends).
 *
 * Note that waking-up a thread does not mean that it will immediatelly start
 * executing.
 *
 * @param thread Thread to wake-up.
 * @return Error code.
 * @retval EOK Thread was woken-up (or was already ready/running).
 * @retval EEXITED Thread already finished its execution.
 */
errno_t thread_wakeup(thread_t* thread) {
    return ENOIMPL;
}

/** Joins another thread (waits for it to terminate).
 *
 * Calling thread_join blocks until the thread we are joining
 * (i.e. the parameter) terminates (either by being killed, finishing its
 * entry function or calling thread_finish). It is not allowed to join the
 * same thread from two places, where possible (see below) this should be
 * detected and signalled via EBUSY return value.
 *
 * After thread_join returns, all pointers to the joined thread (i.e., to the
 * thread_t structure given as parameter) are considered invalid and should
 * not be used (including, but not limited to, calls to thread_finish,
 * thread_join or thread_kill).
 *
 * Note that thread_create is an allocation function (like kmalloc) while
 * thread_join is somewhat a deallocation function (like kfree). As with kfree,
 * it does not make sense to call thread_join twice or use the pointer in any
 * way after return as it is no longer valid.
 *
 * Implementation note: thread_join must release all memory allocated for the
 * joined thread (assuming a successful join). While it is possible to release
 * the thread stack earlier, the rest of the structure must remain valid until
 * a call to thread_join. For example, thread_kill can release the stack of
 * the killed thread because the thread no longer executes but releasing the
 * stack inside thread_finish is not directly possible as the clean-up function
 * still needs to use the stack (it would be possible to delegate the job to
 * a special clean-up thread so that the stack is released as soon as possible
 * but postponing the stack release to thread_join is a typical choice).
 *
 * Note that <code>retval</code> could be <code>NULL</code> if the caller
 * is not interested in the returned value.
 *
 * @param thread Thread to wait for.
 * @param retval Where to place the value returned from thread_finish.
 * @return Error code.
 * @retval EOK Thread was joined.
 * @retval EBUSY Some other thread is already joining this one.
 * @retval EINVAL The thread to join is invalid (including NULL and self).
 * @retval EKILLED Thread was killed.
 */
errno_t thread_join(thread_t* thread, void** retval) {
    return ENOIMPL;
}

/** Switch CPU context to a different thread.
 *
 * Note that this function must work even if there is no current thread
 * (i.e. for the very first context switch in the system).
 *
 * @param thread Thread to switch to.
 */
void thread_switch_to(thread_t* thread) {
}

/** Kills given thread.
 *
 * Note that this function shall work for any existing thread, including
 * currently running one.
 *
 * Joining a killed thread results in EKILLED return value from thread_join.
 *
 * @param thread Thread to kill.
 * @return Error code.
 * @retval EOK Thread was killed.
 * @retval EEXITED Thread already finished its execution.
 */
errno_t thread_kill(thread_t* thread) {
    return ENOIMPL;
}
