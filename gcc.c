
#include "gcc.h"

#if INTERFACE
#if defined(__GNUC__) \
    && (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define UNUSED __attribute__ ((__unused__))

#else
#define UNUSED
#ifndef __attribute__
#define __attribute__(__ignored__)
#endif
#endif

#define IGNORE_VOLATILE_CHARP(_x_) ((char*)(_x_))

#define STFU(_x_) (_x_)
#endif
