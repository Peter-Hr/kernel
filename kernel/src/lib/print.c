// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug.h>
#include <drivers/printer.h>
#include <lib/print.h>
#include <lib/stdarg.h>
#include <mm/heap.h>
#include <proc/thread.h>
#include <types.h>

static void handleStr(const char* str) {
    if (str != NULL) {
        for (size_t i = 0; str[i] != '\0'; i++)
            printer_putchar(str[i]);
    } else {
        handleStr("(null)");
    }
}

static void handleHex(unsigned long number, bool uppercase) {
    char digits[sizeof(unsigned long) * 2];
    char c;
    size_t i = 0;

    if (uppercase)
        c = 'A';
    else
        c = 'a';

    if (number == 0) {
        printer_putchar('0');
    }

    while (number > 0) {
        unsigned long n = (number % 16);
        if (n > 9) {
            digits[i] = c + (n - 10);
        } else {
            digits[i] = '0' + n;
        }

        number /= 16;
        i++;
    }

    while (i > 0) {
        i--;
        printer_putchar(digits[i]);
    }
}

static void handlePointer(const void* pointer) {
    if (pointer == NULL) {
        handleStr("(nil)");
    } else {
        handleStr("0x");
        handleHex((unsigned long)pointer, false);
    }
}

static void handleUnsigned(unsigned int n) {
    char digits[sizeof(unsigned int) * 3];
    size_t i = 0;

    if (n == 0) {
        printer_putchar('0');
    }

    while (n > 0) {
        digits[i] = '0' + (n % 10);
        n /= 10;
        i++;
    }

    while (i > 0) {
        i--;
        printer_putchar(digits[i]);
    }
}

static void handleDecimal(int n) {
    unsigned int number;

    if (n < 0) {
        printer_putchar('-');
        number = 0u - (unsigned int)n;
    } else {
        number = (unsigned int)n;
    }

    handleUnsigned(number);
}

static void handleHexByte(uint32_t number) {
    for (int shift = 28; shift >= 0; shift -= 4) {
        unsigned int digit = (number >> shift) & 0xf;

        if (digit < 10) {
            printer_putchar('0' + digit);
        } else {
            printer_putchar('a' + digit - 10);
        }
    }
}

#if HEAP_HAS_BLOCK_HEADERS
static void print_heap_neighbour(const char* label, link_t* link) {
    handleStr(label);
    if (!heap_contains((uintptr_t)link)) {
        handleStr("none");
        return;
    }
    heap_block_t* block = list_item(link, heap_block_t, link);
    handleUnsigned(block->size);
    handleStr(block->used ? " used" : " free");
}
#endif

static void print_heap_block(void* ptr) {
    handlePointer(ptr);
#if HEAP_HAS_BLOCK_HEADERS
    if (!heap_contains((uintptr_t)ptr - sizeof(heap_block_t))) {
        handleStr("[not heap]");
        return;
    }
    heap_block_t* hdr = (heap_block_t*)ptr - 1;
    printer_putchar('[');
    handleUnsigned(hdr->size);
    handleStr(hdr->used ? " used" : " free");
    print_heap_neighbour(" | prev ", hdr->link.prev);
    print_heap_neighbour(" | next ", hdr->link.next);
    printer_putchar(']');
#endif
}

static void print_thread(thread_t* t) {
    printer_putchar('[');
    handleStr(t->name);
    printer_putchar(' ');
    if (t->state == THREAD_STATE_SUSPENDED) {
        handleStr("(suspended)");
    }
    if (t->state == THREAD_STATE_RUNNING) {
        handleStr("(running)");
    }
    if (t->state == THREAD_STATE_DEAD) {
        handleStr("(dead)");
    }
    if (t->state == THREAD_STATE_READY) {
        handleStr("(ready)");
    }
    printer_putchar(']');
}

/** Prints given formatted string to console.
 *
 * @param format printf-style formatting string.
 */
void printk(const char* format, ...) {
    const char* it = format;

    va_list args;
    va_start(args, format);
    bool next = false;

    while (*it != '\0') {
        if (next) {
            next = false;
            switch (*it) {
            case 'c': {
                char c = va_arg(args, int);

                printer_putchar(c);

                break;
            }
            case 'u': {
                unsigned int number = va_arg(args, unsigned int);

                handleUnsigned(number);

                break;
            }
            case 'd': {
                int number = va_arg(args, int);

                handleDecimal(number);

                break;
            }
            case 'x': {
                unsigned int number = va_arg(args, unsigned int);

                handleHex(number, false);

                break;
            }
            case 'X': {
                unsigned int number = va_arg(args, unsigned int);

                handleHex(number, true);

                break;
            }
            case 's': {
                const char* str = va_arg(args, const char*);

                handleStr(str);

                break;
            }
            case 'p': {
                const char* itcopy = it;
                itcopy++;

                if (*itcopy == 'L') {
                    it = itcopy;

                    list_t* list = va_arg(args, list_t*);
                    if (list == NULL) {
                        handleStr("(nil)");
                        break;
                    }
                    link_t* link = list->head.next;

                    unsigned int count = 0;

                    while (link != &list->head) {
                        count++;
                        link = link->next;
                    }

                    handlePointer(list);

                    if (count == 0) {
                        handleStr("[empty]");
                        break;
                    }
                    printer_putchar('[');
                    handleUnsigned(count);
                    printer_putchar(':');
                    printer_putchar(' ');

                    link = list->head.next;
                    while (link != &list->head) {
                        handlePointer(link);

                        link = link->next;
                        if (link != &list->head) {
                            printer_putchar('-');
                        }
                    }

                    printer_putchar(']');

                    break;
                }

                if (*itcopy == 'F') {
                    it = itcopy;

                    uintptr_t address = va_arg(args, uintptr_t);
                    if (address == 0) {
                        handleStr("(nil)");
                        break;
                    }
                    uint32_t* instructions = (uint32_t*)address;
                    size_t numberOfInstuctions = 4;

                    handlePointer((void*)address);
                    printer_putchar('[');

                    for (size_t i = 0; i < numberOfInstuctions; i++) {
                        if (i > 0) {
                            printer_putchar(' ');
                        }

                        handleHexByte(instructions[i]);
                    }

                    printer_putchar(']');

                    break;
                }

                if (*itcopy == 'A') {
                    it = itcopy;
                    void* ptr = va_arg(args, void*);
                    if (ptr == NULL) {
                        handleStr("(nil)");
                    } else {
                        print_heap_block(ptr);
                    }
                    break;
                }

                if (*itcopy == 'T') {
                    it = itcopy;
                    thread_t* thread = va_arg(args, void*);
                    if (thread == NULL) {
                        handleStr("(nil)");
                    } else {
                        print_thread(thread);
                    }
                    break;
                }

                void* ptr = va_arg(args, void*);
                handlePointer(ptr);
                break;
            }
            case '%':
                printer_putchar('%');
                break;
            default:
                break;
            }
        } else {
            if (*it == '%') {
                next = true;
            } else {
                printer_putchar(*it);
            }
        }
        it++;
    }
    va_end(args);
}

/** Prints given string to console, terminating it with newline.
 *
 * @param s String to print.
 */
void puts(const char* s) {
    while (*s != '\0') {
        printer_putchar(*s);
        s++;
    }
    printer_putchar('\n');
}
