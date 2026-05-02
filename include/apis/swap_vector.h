#pragma once

#include <stdint.h>
#include <stddef.h>

#include <common.h>

#define SVEC_INIT(v) svec_init(app_context, v)
#define SVEC_PUSH(v, x) svec_push(app_context, v, (uintptr_t) x)
#define SVEC_REMOVE(v, i) svec_remove(v, i)
#define SVEC_POP(v) svec_pop(v)
#define SVEC_CLEAR(v) svec_clear(v)
#define SVEC_RELEASE(v) svec_release(app_context, v)
#define SVEC_TOP(v) (v)->data[(v)->length - 1]

typedef struct
{
	union
	{
		char* restrict arena;
		uintptr_t* restrict data;
	};
	size_t length;
	size_t length_bytes;
	size_t arena_capacity;
} SwapVector;

typedef struct SWFAppContext SWFAppContext;

void svec_init(SWFAppContext* app_context, SwapVector* v);
void svec_push(SWFAppContext* app_context, SwapVector* v, uintptr_t value);
void svec_remove(SwapVector* v, size_t index);
void svec_pop(SwapVector* v);
void svec_clear(SwapVector* v);
void svec_release(SWFAppContext* app_context, SwapVector* v);