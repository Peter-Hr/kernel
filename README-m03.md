# Milestone 03: Kernel Threads and Cooperative Scheduler

The goal of the tasks in this milestone is to extend your kernel with support
for threads and very basic (cooperative) thread scheduling. Once implemented,
you will be able to create threads for different tasks, but because of the
cooperative nature of thread scheduling, the threads will have to voluntarily
give up (yield) the processor to let other threads run.

Preemptive scheduling is the topic of milestone M05, but it requires handling
interrupts from a timer device, which is why it is important to get the simple
things working first. Similarly, before you start working on this milestone,
do make sure that you have a working `printk()` and a kernel heap manager
(at least the bump-pointer variant).

The tasks in this milestone are split into two parts. This reflects the
logical distinction between thread management and thread scheduling, even
though both parts depend on each other and must work together.

---

## Part 1: Kernel threads

To implement support for kernel threads, you need to introduce an object that
represents a thread, and functions (think of them as methods) to
manipulate/manage thread objects. To make your design testable using a common
suite of tests, we prescribe an API that you need to implement.

The required functions are listed in the
[`proc/thread.h`](kernel/include/proc/thread.h) header file, but you will be
probably more interested in the [`proc/thread.c`](kernel/src/proc/thread.c)
source file, which provides empty skeletons of the required functions along
with their descriptions.

The [`proc/thread.h`](kernel/include/proc/thread.h) header file also contains a
skeleton of the `thread_t` type which represents a thread object. At this
point, the thread object contains a name and a pointer to a function of type
`thread_entry_func_t`. The function represents the code to be executed by a
thread (more on this [later](#thread-entry-function)). You will need to add
other fields (as necessary) to the `thread_t` type.

In general, the thread subsystem should keep track of all threads and manage
the state of individual threads. Of the functions that you need to implement,
some deal with subsystem initialization and thread creation, some provide
information about thread state, and some need to cooperate with the scheduler
(by calling the scheduler API).

One particular function, `thread_switch_to()`, is of special interest to the
scheduler, because it provides the ability to switch between threads. To
implement such a function, you will need to be able to switch CPU context.

In general, parts of the context switching code need to be implemented in
CPU-specific assembly language. This is to maintain control over the code
being executed, but primarily because the code requires working with CPU
features that are unavailable at the level of the C programming language. To
this end, the solution skeleton provides the `cpu_switch_context()` function
which does the actual switching of CPU context -- you just need to use it
properly in your implementation of `thread_switch_to()`.

The `cpu_switch_context()` function is declared in the
[`proc/context.h`](kernel/include/proc/context.h) header file, along with the
`context_t` type which represents the CPU context that needs saving on context
switches (mostly callee-saved registers at this point). The function has two
parameters, `this_context` and `next_context`. Both parameters are pointers to
the `context_t` type. When called, the function saves the current CPU context
into the structure pointed to by `this_context`, and loads another context
(the one to switch to) from `next_context`.

While not strictly necessary, we suggest that you review the implementation of
the `cpu_switch_context()` function in
[`proc/context.S`](kernel/src/proc/context.S). In particular, it is important to
understand the implications of changing the `sp` and `ra` registers before the
function returns: **the function does not return to the caller in the
current context --- it returns to the caller in the target context**.


### Thread entry function

We have mentioned earlier that a thread entry function (of type
`thread_entry_func_t`, see also the `entry_func` field in the `thread_t`
structure) represents the code to be executed by a thread. The function
expects to **receive a single argument and returns a single value**. Both the
argument and the return value are of type `void *` and need to be cast to a
proper type by the code using them.

This means that to start a thread, you need to prepare a CPU context which,
when switched to, causes the CPU to jump to your code as if an actual function
call was made. This in turn requires some understanding of the function
calling conventions, i.e., what the compiler-generated code expects to find
and where.

This is defined in the specification of the Application Binary Interface (ABI)
for a given CPU and is normally handled by the compiler. However, when we need
to jump-start a thread, it becomes our responsibility. In general, the ABI
specification is a rather long read, but we only need a few details.



For the RISC-V CPU, the ABI specification states (among other things):

> The base integer calling convention provides eight argument registers,
> `a0`-`a7`, the first two of which are also used to return values. Scalars
> that are at most `XLEN` bits wide are passed in a single argument register,
> or on the stack by value if none is available.

and also:

> The stack grows downwards (towards lower addresses and that **the stack
> pointer shall be aligned to 128-bit boundary upon procedure entry**. The
> first argument passed on the stack is located at offset zero of the stack
> pointer on function entry; following arguments are stored at
> correspondingly higher addresses.

Because the machine code of any function produced by the compiler will have
the above expectations, your implementation of threads needs to ensure that
they hold when you make the CPU jump into the compiler-generated code of the
thread entry function.

To this end, the implementation skeleton defines the `ABI_STACK_ALIGNMENT`
symbol which you should use for stack pointer alignment. The symbol is defined
in the [`abi.h`](kernel/include/abi.h) header file.

For more details, we suggest to review the **Procedure Calling Convention**
section of the [RISC-V ABI Specification](https://d3s.mff.cuni.cz/files/teaching/nswi200/202324/doc/riscv-abi.pdf)


### Additional hints/suggestions

To better understand the problem, consider the following questions when
implementing the prescribed API functions:

* Who calls `thread_finish()` when the thread entry function finishes (returns)?
* Who is responsible for thread destruction?
    * When do we release resources (memory) associated with a thread?
* What is special about the first context switch?
    * What does `this_context` parameter represent in that case?
* How do we cause `cpu_switch_context()` to jump into the thread entry
  function of a newly created thread?

Also, to simplify debugging, consider extending your `printk()`
implementation to support printing information about a given thread by
providing a `T` modifier to the `%p` format specifier (similar to `L`
for lists).

---

## Part 2: Cooperative scheduler

As mentioned above, scheduling is logically distinct from thread
management, even though both subsystems need to cooperate. A thread
subsystem provides the scheduler with the ability to switch between
threads, while the scheduler provides the thread subsystem with the
ability to trigger scheduling actions, e.g., when a thread wants to
yield the CPU to someone else, or when a thread needs to be suspended.
To give you an idea what services are needed from the scheduler, we
again prescribe an API that your solution needs to implement.

The required functions are listed in the
[`proc/scheduler.h`](kernel/include/proc/scheduler.h) header file. The
description of each function, along with a skeleton that needs to be filled
in, can be found in the [`proc/scheduler.c`](kernel/src/proc/scheduler.c)
source file.

In general, the scheduler needs to keep track of all schedulable threads
so that when `scheduler_schedule_next()` is called, the scheduler can
switch to a thread that should be running.

As mentioned earlier, a cooperative scheduler requires individual
threads to yield the CPU to other threads. This can be done by calling
the `thread_yield()` function from the currently executing thread, which
should trigger a scheduling action.

Note that this milestone requires only a very basic scheduler and thread
management. If your solution requires sharing a lot of information between
the `scheduler.c` and `thread.c` modules, it is very likely that you have
not assigned the responsibilities well (the scheduler only needs to keep
track of *runnable threads*, while the thread subsystem may want to keep
track of *all threads*).

We also advise against making your code unnecessarily complex by adding
support for thread priorities or by using advanced data structures (such
as red-black trees which seem to be very popular): plain linked list will be
fine. That does not mean you should not experiment, just make sure you have
a working (and obviously correct) implementation first.

---

## Support code

In addition to the files containing the thread and scheduler API, the
corresponding empty function definitions, and the `cpu_switch_context()`
function mentioned above, there are few other changes in the initial
code provided for this milestone.

First of all, the `kernel_main()` function was changed to create a thread
for running the system (or the tests). As a result, expect _all_ tests to
fail after merging the skeleton code for this milestone, because the tests
will not even start. Do not let that discourage you.

The [`debug.h`](kernel/include/debug.h) header file provides the `panic()`
and `panic_if()` macros that can be used to halt the simulator if your code
encounters an unrecoverable error. A typical example can be found in the
`kernel_main()` function, where the kernel panics if creating the main
kernel thread fails. Unlike assertions, kernel panics are not removed
from production builds.

The [`errno.h`](kernel/include/errno.h) header file provides the `errno_t` type
for passing error information around. Feel free to add your own error codes,
but do not forget to extend the `errno_as_str()` function which provides a
human-readable description of the error.

The [`ktest.h`](kernel/include/ktest.h) header file contains a new macro,
`ktest_assert_errno()` to simplify testing error values for `EOK`.

### Tests

Just like in previous milestones, we provide a suite of tests to help
you stay on the right track. This milestone retains all tests from the
previous milestone and adds several new tests for kernel threads in the
[`tests/thread`](kernel/tests/thread) directory.

The tests primarily target the thread management API and only test the
working of the scheduler indirectly. Because the tests assume
cooperative scheduling, they are rather simple and mostly test basic
things --- they should help you check that you have not forgotten any
major part of the implementation, but it may be possible to pass some of
the tests even with a solution that does not work correctly.

Keep in mind that you (not the tests) are responsible for the correctness of
your solution. Your implementation will be reviewed manually after the final
deadline and must include correctly working scheduler and thread management
functions to actually pass.

Note that the special test `thread/leaks` tries to check that you are properly
returning memory when thread terminates or when resource allocation partially
fails. To this end it provides a simplified version of `kmalloc` and `kfree`
that it links with instead of with your own implementation. We believe that
it will detect the most common issues and when run with `--debug` it should
pin-point the leaked memory for you. Please, report any false alarms or issues
with this test as it is introduced for the first time this year.

---

## Submission

The **baseline solution** for this milestone must pass the baseline tests for
`M01`, `M02` and `M03`. The commit representing the solution must be tagged
with `m03`. Submitting a **working baseline solution** within this milestone's
deadline allows you to gain bonus points towards a better grade.

Extended features are usually covered by separate test suites (see comments on
the first line of the respective suite for details). Some features cannot be
reliably tested and will be evaluated during final review.

Please, see also the notes in the main [README.md](README.md).
