#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define THROW *((u32*) 0) = 0;
#define EXC(str) fprintf(stderr, str); THROW;
#define EXC_ARG(str, arg) fprintf(stderr, str, arg); THROW;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;