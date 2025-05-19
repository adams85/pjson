#ifndef INCLUDED_VEC_CONFIG_DEFAULT_H
#define INCLUDED_VEC_CONFIG_DEFAULT_H
#include <stddef.h>
#include "unity_memory.h"

#define VEC_INIT_CAPACITY 4
#define VEC_GROW_CAPACITY(n) ((n) + (n >> 1))

typedef size_t vec_size_t;

#define VEC_API(name) name

//
// Structure alignment helpers
//
#if defined(__GNUC__) || defined(__clang__)
#define VEC_PRE_ALIGN
#define VEC_POST_ALIGN __attribute__ ((aligned (8)))
#elif defined(WIN32)
#define VEC_PRE_ALIGN __declspec(align(8))
#define VEC_POST_ALIGN
#endif

//
// Memory allocator overrides
//
#define VEC_MALLOC unity_malloc
#define VEC_FREE unity_free
#define VEC_REALLOC unity_realloc


#endif // INCLUDED_VEC_CONFIG_DEFAULT_H
