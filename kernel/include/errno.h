// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _ERRNO_H
#define _ERRNO_H

#include <types.h>

typedef enum {
    EOK = 0,
    ENOIMPL = 1,
    ENOMEM = 2,
    EBUSY = 3,
    EEXITED = 4,
    EKILLED = 5,
    ENOENT = 6,
    EINVAL = 7,
} errno_t;

static inline const char* errno_as_str(errno_t err) {
    switch (err) {
    case EOK:
        return "no error";
    case ENOIMPL:
        return "not implemented";
    case ENOMEM:
        return "out of memory";
    case EBUSY:
        return "resource is busy";
    case EEXITED:
        return "thread already exited";
    case EKILLED:
        return "thread was killed";
    case ENOENT:
        return "no such entry";
    case EINVAL:
        return "invalid value";
    default:
        return "unknown error";
    }
}

#endif
