# Milestone 02: Kernel Memory Management

This is the first team milestone in which you will lay down the foundation of
your operating system kernel. The milestone has two parts. The first is just a
rehash of the previous milestones to get your team work started. The second is
something new, and requires you to implement kernel heap memory management.

---

## Part 1: Formatted printing and debug functions

At this point, your team should have access to multiple implementations of the
`printk()` function (one from each team member) and perhaps even multiple
implementations of the functions for dumping function machine code or reading
the value of the stack pointer register. Obviously, there is little need for
multiple implementations of these functions in a single kernel, therefore your
first task as a team is to choose the one implementation that you want to be
using in subsequent development.

All the implementations should be functionally equivalent (if they passed the
tests), but there are other aspects that may make one implementation more
favorable over the others. Therefore you should jointly review and discuss the
available implementations, focusing on code quality, internal design and
extensibility, and make a decision. You may simply pick one that looks best and
integrate it, or you may try to cherry pick and fuse the best parts from
multiple implementations.

In any case, keep in mind that `printk()` is going to be one of your key
debugging aids and you need to trust it. Make sure to support the features
required in milestone M01.

### Formatted printing

Your `printk()` **MUST** support printing the values of the following types:

- characters (via `%c`)
- signed integers in decimal (via `%d`)
- unsigned integers in decimal (via `%u`)
- unsigned integers in hexadecimal (via `%x` and `%X`)
- zero-terminated strings (via `%s`)
- pointers (via `%p`)

Optionally (highly recommended), your `printk()` should also support printing
the values of the following types:

- `list_t *` pointers (via `%pL`), including list size and links
- function pointers (via `%pF`), including machine code dump
- **pointers to memory blocks (via `%pA`)**, for part 2 of this milestone

### Debugging functions

Your kernel should provide the following debugging functions:

- reading the value of the stack pointer (via `debug_get_stack_pointer()`)
- dumping machine code at function address (via `debug_dump_function()`)

The provided code already contains the `assert` macro in `debug.h` and also
`machine_enter_debugger()` to programmatically switch MSIM into interactive
mode from your C code
(recall the [kernel exercise](https://msim.readthedocs.io/en/latest/tutorial/mini-kernel.html#entering-the-debugger)).

---

## Part 2: Kernel heap allocator

So far, your kernel code could not use dynamic memory allocation. This was not
really a problem for milestone M01, because you could tell how much memory
you will need at compilation time and this amount did not change at runtime.

However, without the ability to allocate memory dynamically, your kernel would
be limited to fixed amount of all kernel objects, forcing you to come up with
specific numbers for a specific machine configuration (in terms of available
memory). While such a restriction would be acceptable in certain domains, e.g.,
real-time kernels in safety- and mission-critical systems, it is not acceptable
in a kernel (albeit simple) for a general-purpose system.

Consequently, the goal of the second part of this milestone is to implement
support for **dynamic memory allocation** in your kernel.

## Basic functions

Your kernel heap implementation needs to provide three basic functions (see
the `mm/heap.h` header file for function signatures):

- `heap_init()`, which initializes the heap (and needs to be called prior
   to making any calls to the following two functions).

- `kmalloc()`, which allocates a block of memory of given size.

- `kfree()`, which frees a (previously allocated) block of memory.


The entry point to your kernel is the `kernel_main()` function. The key
responsibility of that function is to initialize various kernel subsystems
and start the thread scheduling loop.

In user-space, the runtime library executes various initializers before calling
your `main()` function. In the kernel, not only is the runtime library
unavailable, but you also need to keep the order of subsystem initialization
under control, because some subsystems depend on other subsystems. This is best
done by being explicit about the order of subsystem initialization, and the
`kernel_main()` function is a perfect place for such kind of work.

Currently, there is not much going on in the `kernel_main()` function, but this
will change in subsequent milestones. At the moment, `heap_init()` is called
very early during boot, hence subsequent parts of the kernel can be sure
they can allocate memory through `kmalloc()`.

Please keep the key responsibility of `kernel_main()` in mind and avoid using
the function for other than coordination purposes (especially avoid putting
low-level code dealing with implementation details into that function).


## Bump-pointer heap

Managing heap can be relatively tricky, because you need to keep track of a
variable number of objects (allocated and free blocks) without using standard
dynamic memory allocation functions. To get a minimal working solution (so that
you can better understand the problem), it helps to think of a simple solution
first.

Imagine a user-space program that only calls `malloc()` (and never `free()`).
Such a program will either complete its task with whatever memory was
available, or it will run out of memory. Implementing a heap allocator for such
a program would be very simple -- the allocator could simply keep a pointer to
the last free memory location and advance the pointer forward on each call to
`malloc()` and do nothing on a call to `free()`. This is sometimes called
*Bump Pointer Allocator*, because all it does is bumping a pointer forward.

To pass this milestone, you can implement the `kmalloc()` function using this
type of allocator, but keep in mind that a working `kfree()` simplifies further
development for you. Feel free to return to the allocator later if you choose
to go this way.

In fact, given the simplicity of the bump-pointer allocator, we would generally
recommend to implement it first so that you become more accustomed to working
in the kernel environment. Obviously, certain tests will fail without a working
`kfree()`, but you will get *something working* to boost your confidence and
relieve pressure. With a working bump-pointer allocator safely committed to
your repository, you can move on and implement a proper heap allocator.

You can keep both at the same time, using different function name prefixes
for different implementations (e.g., `bump_kmalloc` and `bump_kfree`) and use
C preprocessor macros and conditional compilation to select the actual
implementation of the `kmalloc` and `kfree` functions.


## Memory detection

The amount of memory the simulated computer has is determined by the directives
in the `msim.conf` configuration file. However, this file **IS NOT** visible to
your kernel and there is no firmware (BIOS) that would provide the information
about available memory to the kernel through some predefined structures in
memory.

Therefore the kernel needs to find this information on its own, and apart from
compile-time constants, the simplest way to determine the amount of available
memory is to probe the memory by writing to it and checking if the previously
written value can be read back.

Your implementation **MUST NOT** rely on the fact that MSIM returns a special
value when reading non-existent memory, nor can it rely on any compile-time
constants (such as those used by the tests) to find out how much memory your
system has.

Your implementation can assume that all memory is available in a single
contiguous block, starting with the kernel image. The `kernel_get_image_end()`
function returns the address of the first byte after the kernel image.
Note that it is safe to probe the memory in blocks of 4 KiB to speed up the
detection.

For practical purposes, you can also safely assume that there will never be
more than 256MB of memory available (the simulated RAM starts at 0x80000000
and at 0x90000000 is placed the printer device).


## Debugging extension for `printk()`

As a debugging aid, especially when working on a full-featured heap allocator,
we recommend extending `printk()` to support printing information about an
allocated memory block via the `%pA` specifier.

We do not prescribe any specific format, but depending on the nature of your
implementation, you should be able to print the size of the allocated block
and information about the state of the sibling blocks.

---

## Support code

At this point, you should be familiar with the general layout of the skeleton
code we provide together with the milestone. Compared to milestone M01,
this milestone also provides `src/mm/heap.c` which contains empty definitions
of the "public" functions that need to be implemented.

### Tests

Just like in the previous milestones, the basic correctness of your implementation
will be primarily evaluated using automated tests. This milestone retains all
tests from the previous milestones and adds several new tests for the kernel
heap allocator in `kernel/tests/heap` directory.

The heap allocator tests are intentionally simple to allow even the bump-pointer
implementation to pass, which is why you should consider adding your own tests.
As mentioned earlier - implementing a heap allocator can be a bit
tricky and error-prone, but it is a fundamental subsystem of the kernel and
you need to be able to trust it. If you cannot manage kernel heap properly,
you will run into problems implementing (and debugging) other parts of your
kernel.

The `suites` subdirectory contains individual test suites for the base
assignment as well as for various extensions. The fuzzy tests stress test your
implementation but are not executed regularly on GitLab because of the extra
load. Make sure your solution passes these too.

---

## Development hints

Experience shows that the implementation of memory detection can be quite
tricky and that you should start with a basic implementation that is
obviously correct and safe.

Some teams cannot resist the urge to implement a precise detection of
available memory (often based on bisection). This is fine, but it is better
to postpone it until you have a working solution and time to spare.

Keep in mind that memory is detected only once in the lifetime of an operating
system, so there is no room for the added code complexity to ever pay off.

It is possible (and recommended) to develop the heap allocator in parallel
with the implementation of memory detection. Your heap initialization code
will probably contain something like the following fragment:

```c
uintptr_t heap_start = kernel_get_image_end();
uintptr_t heap_end = detect_end_of_available_memory(heap_start);
```

However, instead of waiting for memory detection to be finished and working,
you can start implementing the heap allocator using a statically allocated
memory block (and thus mock the memory detection results):

```c
/* 16K heap in uint32_t type. */
#define MOCK_HEAP_SIZE (16*1024)
static uint32_t mock_heap[MOCK_HEAP_SIZE / sizeof(uint32_t)];
...
uintptr_t heap_start = (uintptr_t) &mock_heap[0];
uintptr_t heap_end = heap_start + sizeof(mock_heap);
```

The above code declares a 16KiB array of `uint32_t` that we can treat as
the available memory the heap allocator is supposed to manage. The array
is just a 16KiB block of data that is part of the kernel image, but more
importantly, it gives us the precise start address and the size of the
block, which is all the heap allocator needs to know about a block of
memory it manages.

Note that we are using the `uint32_t` type to ensure that the array will
start on an address aligned to 4 bytes (`sizeof(uint32_t)`).

Including the array in the kernel image also naturally shifts the starting
address (the value returned by the `kernel_get_image_end()` function) for
detecting the available memory. However, this does not really affect the
detection mechanism in any way, which allows it to be developed in parallel
with the heap allocator.

Finally, the mock heap can be also easily used to check your heap allocator
implementation with different/strange heap sizes. More experienced C developers
may use conditional compilation (e.g., the `#ifdef` directives) to enable
switching between real and mock heap for debugging purposes.

---

## Submission

This milestone must pass the baseline tests for both M01 and M02. Extended
functionality is covered by extra test suites (see comments on the first line
of the respective suite for details).

Submitting a working **baseline solution** tagged with `m02` within the deadline
of this milestone allows you to gain bonus points towards a better grade.

Please, see also the notes in the main [README.md](README.md).
