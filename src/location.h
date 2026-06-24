#ifndef LOCATION_H
#define LOCATION_H

/* __LOCATION__ expands to a "file:line" string literal for allocation and
 * leak accounting. It lives in its own header so headers that need it (mem.h)
 * stay independent of common.h include order; common.h pulls it in too. */
#define __S1(x) #x
#define STRINGIFY(x) __S1(x)
#define __LOCATION__ __FILE__ ":" STRINGIFY(__LINE__)

#endif /* LOCATION_H */
