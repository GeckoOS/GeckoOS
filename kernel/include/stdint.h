#pragma once
// the basic types outside of boot since not all parts of kernel should access
// boot
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef short char16_t;
typedef int char32_t;

// NULL is by the way
#define NULL  ((void*)0)