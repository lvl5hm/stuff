#ifndef LVL5_TYPES

typedef float f32;
typedef double f64;

typedef char i8;
typedef short i16;
typedef long i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

typedef unsigned char byte;
typedef u32 b32;

typedef u64 Mem_Size;

#define globalvar static

#define assert(cond) if (!(cond)) *(volatile i32 *)nullptr = 10
#define array_count(arr) (sizeof(arr)/sizeof(arr[0]))

#define kilobytes(n) (1024LL*n)
#define megabytes(n) (1024LL*kilobytes(n))
#define gigabytes(n) (1024LL*megabytes(n))


#define PI32 3.14159265358979323846f










#define LVL5_TYPES
#endif