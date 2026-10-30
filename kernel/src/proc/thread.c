// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <abi.h>
#include <debug.h>
#include <errno.h>
#include <mm/heap.h>
#include <proc/context.h>
#include <proc/scheduler.h>
#include <proc/thread.h>

static void thread_set_name(thread_t* thread, const char* name) {
    if (thread == NULL || name == NULL) {
        return;
    }

    int i = 0;
    while (name[i] != '\0' && i < THREAD_NAME_MAX_LENGTH) {
        thread->name[i] = name[i];
        i++;
    }
    thread->name[i] = '\0';
}

struct thread* all_threads_head;

thread_t* current_thread = NULL;

/** Initialize support for threading.
 *
 * Called once at system boot.
 */
void threads_init(void) {
    // 1. Alokujeme pamäť pre "dummy" (boot) thread.
    // heap_init() už prebehlo v kernel_main, takže kmalloc môžeme použiť.
    current_thread = kmalloc(sizeof(thread_t));
    panic_if(current_thread == NULL, "Neuspesna alokacia boot threadu");

    thread_set_name(current_thread, "[BOOT]");

    current_thread->entry_func = NULL;
    current_thread->stack_memory = NULL;
    current_thread->stack_size = 0;

    current_thread->state = THREAD_STATE_RUNNING;
    current_thread->retval = NULL;

    current_thread->next_ready = NULL; // Do plánovača nepôjde
    current_thread->next_global = NULL;

    all_threads_head = current_thread;
}

static void thread_start_wrapper(void) {
    // 1. Získame funkciu a jej argument z aktuálneho threadu
    thread_entry_func_t entry = current_thread->entry_func;
    void* data = current_thread->data;

    // 2. Spustíme samotný kód threadu a uložíme si jeho návratovú hodnotu
    void* retval = entry(data);
    current_thread->retval = retval;

    // 3. Thread skončil svoju prácu. Teraz zavoláme funkciu na jeho korektné ukončenie.
    // Týmto odpovedáme na otázku zo zadania: "Who calls thread_finish()?"
    thread_finish(retval);

    // Sem by sa už procesor nikdy nemal dostať, pretože thread_finish() urobí
    // thread_yield() a prepne na iný thread.
    panic("Thread returned from thread_finish!");
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
    // 0. Validácia parametrov
    if (flags != 0) {
        return EINVAL;
    }
    // 1. Alokácia štruktúry thread_t
    thread_t* new_thread = kmalloc(sizeof(thread_t));
    if (new_thread == NULL) {
        return ENOMEM; // Nedostatok pamäte
    }

    // 2. Alokácia zásobníka pre tento thread
    new_thread->stack_memory = kmalloc(THREAD_STACK_SIZE);
    if (new_thread->stack_memory == NULL) {
        kfree(new_thread); // Ak zlyhá zásobník, musíme vrátiť aj štruktúru, inak máme memory leak
        return ENOMEM;
    }
    new_thread->stack_size = THREAD_STACK_SIZE;

    // 3. Inicializácia premenných
    thread_set_name(new_thread, name);
    new_thread->entry_func = entry;
    new_thread->data = data;
    new_thread->state = THREAD_STATE_READY;
    new_thread->retval = NULL;
    new_thread->is_joined = false;
    new_thread->is_killed = false;

    new_thread->next_global = all_threads_head;
    all_threads_head = new_thread;

    // 4. Nastavenie vrcholu zásobníka (Stack Pointer)
    uintptr_t stack_top = (uintptr_t)new_thread->stack_memory + THREAD_STACK_SIZE;
    stack_top = stack_top & ~((uintptr_t)ABI_STACK_ALIGNMENT - 1);
    new_thread->context.sp = stack_top;

    // 5. Príprava kontextu (Return Address)
    new_thread->context.ra = (unative_t)thread_start_wrapper;

    // 6. Zaradenie do plánovača (Run Queue)
    new_thread->next_ready = NULL;
    scheduler_add_ready_thread(new_thread);

    // 7. Vrátenie výsledku
    if (thread_out != NULL) {
        *thread_out = new_thread;
    }

    return EOK; // Všetko prebehlo úspešne
}

/** Return information about currently executing thread.
 *
 * @retval NULL When no thread was started yet.
 */
thread_t* thread_get_current(void) {
    return current_thread;
}

/** Yield the processor. */
void thread_yield(void) {
    if (current_thread == NULL) {
        return; // Ešte nebeží žiadny thread (ochrana)
    }

    // Ak v planovači nie je nič čo by sa vykonalo, kludne pokračuj v tomto threade.
    if (scheduler_is_empty()) {
        return;
    }
    // Ak thread normálne beží a len sa vzdáva procesora,
    // zmeníme jeho stav na READY a zaradíme ho späť na koniec rady v plánovači.
    if (current_thread->state == THREAD_STATE_RUNNING) {
        current_thread->state = THREAD_STATE_READY;
        scheduler_add_ready_thread(current_thread);
    }

    // Táto funkcia "zastaví" aktuálny kód a prebudí sa až vtedy,
    // keď na tento thread plánovač opäť prepne.
    scheduler_schedule_next();
}

/** Current thread stops execution and is not scheduled until woken up. */
void thread_suspend(void) {
    if (current_thread == NULL) {
        return;
    }

    if (current_thread->state == THREAD_STATE_RUNNING) {
        current_thread->state = THREAD_STATE_SUSPENDED;
    }

    scheduler_schedule_next();
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
    if (current_thread == NULL) {
        panic("thread_finish called without an active thread!");
    }

    // Uložíme návratovú hodnotu pre prípad, že na ňu niekto čaká (cez thread_join)
    current_thread->retval = retval;

    current_thread->state = THREAD_STATE_DEAD;

    // dalsi thread sa moze spustit
    scheduler_schedule_next();

    // ak funkcia pokracuje tak je zle!
    panic("Thread continued execution after thread_finish!");
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
    if (thread == NULL) {
        return true;
    }

    return (thread->state == THREAD_STATE_DEAD);
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
    if (thread == NULL) {
        return EINVAL;
    }

    // Ak skončil tak EEXITED
    if (thread->state == THREAD_STATE_DEAD) {
        return EEXITED;
    }

    // Zobudim ho, pridam do schedulera.
    if (thread->state == THREAD_STATE_SUSPENDED) {
        thread->state = THREAD_STATE_READY;
        scheduler_add_ready_thread(thread);
    }

    return EOK;
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
    // Neplatný pointer alebo pokus čakať sám na seba
    if (thread == NULL || thread == current_thread) {
        return EINVAL;
    }

    //  Ochrana pred viacnásobným čakaním (EBUSY)
    if (thread->is_joined) {
        return EBUSY;
    }
    thread->is_joined = true;

    // Kým thread neprejde do stavu DEAD, neustále sa vzdávame procesora.
    while (thread->state != THREAD_STATE_DEAD) {
        thread_yield();
    }

    // Thread skončil, prečítame jeho návratovú hodnotu
    if (retval != NULL) {
        *retval = thread->retval;
    }

    // Poznačíme si výsledok, kým ešte štruktúra existuje
    errno_t return_status = thread->is_killed ? EKILLED : EOK;

    // Najprv musíme thread bezpečne vymazať z nášho globálneho zoznamu
    thread_t* prev = NULL;
    thread_t* curr = all_threads_head;
    while (curr != NULL) {
        if (curr == thread) {
            if (prev == NULL) { // Mažeme prvý prvok
                all_threads_head = curr->next_global;
            } else { // Mažeme prvok zo stredu alebo konca
                prev->next_global = curr->next_global;
            }
            break;
        }
        prev = curr;
        curr = curr->next_global;
    }

    // Uvoľnenie zásobníka a samotnej štruktúry
    if (thread->stack_memory != NULL) {
        kfree(thread->stack_memory);
    }
    kfree(thread);

    return return_status;
}

/** Switch CPU context to a different thread.
 *
 * Note that this function must work even if there is no current thread
 * (i.e. for the very first context switch in the system).
 *
 * @param thread Thread to switch to.
 */
void thread_switch_to(thread_t* thread) {
    if (thread == NULL) {
        panic("Attempted to switch to a NULL thread!");
    }

    // Rovnaky thread, nepripinaj.
    if (thread == current_thread) {
        return;
    }

    // Pointer na starý.
    thread_t* old_thread = current_thread;

    // Nový je running.
    thread->state = THREAD_STATE_RUNNING;

    // Gloablny pointer na aktuálny thread.
    current_thread = thread;

    // Zmena, ak nahodou by neexistoval context v threade predtym, urobime si dummy.
    if (old_thread != NULL) {
        cpu_switch_context(&old_thread->context, &thread->context);
    } else {
        context_t dummy_context;
        cpu_switch_context(&dummy_context, &thread->context);
    }
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
    if (thread == NULL) {
        return EINVAL; // Ochrana pre neplatný argument
    }

    // Ak už je thread mŕtvy, nemáme čo zabíjať
    if (thread->state == THREAD_STATE_DEAD) {
        return EEXITED;
    }

    // pre thread_join, aby vedel že bol thread zabitý a nečakal a dal return EKILLED
    thread->is_killed = true;

    if (thread == current_thread) {
        // ZABÍJAME SEBA SAMÉHO
        // V tomto prípade nesmieme uvoľniť svoj vlastný zásobník (lebo na ňom práve sme).
        // Proste sa len finishne bez návratovej hodnoty.
        thread_finish(NULL);
        // kedže sa finishlo, sem sa nepokračuje

    } else {
        // pamať si uvolní join
        thread->state = THREAD_STATE_DEAD;
        scheduler_remove_thread(thread); // Odstránime ho z plánovača, aby sa už nikdy nespustil
    }

    return EOK;
}
