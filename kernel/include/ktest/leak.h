// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Charles University

#ifndef _KTEST_LEAK_H
#define _KTEST_LEAK_H

#include <types.h>

typedef void (*leak_checking_func_t)(void*);

/**
 * Execute a simple leak checking run.
 *
 * The test is rather trivial in its implementation but should catch at
 * least the most common bugs and omissions in the code.
 *
 * The important idea is that we supply our own implementation of allocation
 * functions (such as kmalloc) and hence we are able to check that all objects
 * are actually returned (kfreed). That means that for the check to work
 * the standard functions should NOT be linked with the kernel.
 *
 * Note that our implementation of kmalloc etc. is trivial and uses a statically
 * defined array as a backing storage. This will not work for large tests but
 * we are okay with that so far.
 *
 * The actual check is performed in the following way. The user provides a
 * function and we will check that this function (when executed) releases all
 * allocated resources. The function must be as idempotent as possible as
 * it will be called multiple times.
 *
 * Firstly, the provided function is called and inside our allocation functions
 * (kmalloc) we record the amount of allocations done.
 *
 * Then we call this function in a loop and in each loop we record address of
 * every allocated object but we also fail some allocations (see below). At the
 * end of the loop, we check that all allocated objects were returned.
 * Currently, the implementation deterministically fails n-th allocation in n-th
 * loop and fails no allocation in the last loop. That should catch the most
 * common bugs: not freeing a properly destroyed object and not freeing all
 * resources when further allocations fail (e.g. if we allocate a thread
 * structure but fail to allocate a stack, we should release the structure
 * as well).
 */
void ktest_leak_check(leak_checking_func_t func, void* data);

#endif
