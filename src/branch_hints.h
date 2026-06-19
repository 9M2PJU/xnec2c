/*
 *  Branch-prediction hints for hot paths.
 *
 *  Single source for the likely()/unlikely() macros shared by the managed
 *  allocator and the common header; both include this so either may be
 *  included first without redefining the hints.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#ifndef BRANCH_HINTS_H
#define BRANCH_HINTS_H 1

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#endif /* BRANCH_HINTS_H */
