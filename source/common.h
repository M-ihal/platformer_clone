#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
typedef double float64_t;
typedef float  float32_t;

#include <malloc.h>
#define malloc_struct(T)              (T *)malloc(sizeof(T))
#define malloc_and_zero(bytes)           __malloc_and_zero(bytes)
#define malloc_and_zero_struct(T)   (T *)__malloc_and_zero(sizeof(T))
#define malloc_and_zero_array(T, n) (T *)__malloc_and_zero(sizeof(T) * (n))

#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define array_count(arr)    ((sizeof(arr) / sizeof(arr[0])))
#define zero_struct(ptr)    (memset((ptr), 0, sizeof(*(ptr))))
#define zero_array(arr)     (memset((arr), 0, sizeof(arr)))
#define zero_memory(ptr, b) (memset((ptr), 0, (b)))

inline void *__malloc_and_zero(size_t bytes) {
    void *mem = malloc(bytes);
    if(mem) memset(mem, 0, bytes);
    return mem;
}

#define safe_divide(n, d) (((d) == 0) ? 0 : (n) / (d))
#define _CONCAT0(a, b) a##b
#define  CONCAT(a, b) _CONCAT0(a, b)
#define  swap_2(a, b) { const auto CONCAT(_temp, __LINE__) = (a); (a) = (b); (b) = CONCAT(_temp, __LINE__); }
#define BOOL_STRING(boolean) ((boolean) == true ? "true" : "false")

#define B(n)  (n)
#define KB(n) (B(n)  * 1024)
#define MB(n) (KB(n) * 1024)
#define GB(n) (MB(n) * 1024)

#endif /* _COMMON_H */