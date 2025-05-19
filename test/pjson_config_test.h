#ifndef __PJSON_CONFIG_H__
#define __PJSON_CONFIG_H__
#include "unity_memory.h"

// Define any signature decoration for vector APIs
#define PJSON_API(name) name

// Define PJSON_NO_LOCALE if localeconv (locale.h) is not available.
// #define PJSON_NO_LOCALE

#ifndef PJSON_INTERNAL_BUFFER_FIXED_SIZE
#define PJSON_INTERNAL_BUFFER_FIXED_SIZE (64)
#endif

#if !defined(pjson_malloc) && !defined(pjson_realloc) && !defined(pjson_free)
#define pjson_malloc unity_malloc
#define pjson_realloc unity_realloc
#define pjson_free unity_free
#else
#if !defined(pjson_malloc) || !defined(pjson_realloc) || !defined(pjson_free)
#error "Incomplete memory management override. Define either all of pjson_malloc, pjson_realloc and pjson_free or none of them."
#endif
#endif

#endif // __PJSON_CONFIG_H__
