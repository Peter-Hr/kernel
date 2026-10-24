// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug.h>
#include <drivers/printer.h>
#include <lib/print.h>
#include <lib/stdarg.h>
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
                    } else {
                        printer_putchar('[');
                        handleUnsigned(count);
                        printer_putchar(':');
                        printer_putchar(' ');
                    }

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

                void* ptr = va_arg(args, void*);

                handlePointer(ptr);

                if (*itcopy != '\0') {
                    printer_putchar(*itcopy);
                    it = itcopy;
                }

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
