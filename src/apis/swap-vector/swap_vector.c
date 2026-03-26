#include <swf.h>
#include <action.h>
#include <heap.h>
#include <utils.h>

#include <swap_vector.h>

#define UP(x) VAL(uintptr_t, x)

void svec_init(SWFAppContext* app_context, SwapVector* v)
{
	v->length = 0;
	v->length_bytes = 0;
	v->arena_capacity = 64*sizeof(uintptr_t);
	v->arena = HALLOC(v->arena_capacity*sizeof(uintptr_t));
}

void svec_push(SWFAppContext* app_context, SwapVector* v, uintptr_t value)
{
	v->length += 1;
	v->length_bytes += sizeof(uintptr_t);
	ENSURE_SIZE(v->arena, v->length_bytes, v->arena_capacity, 1);
	
	char* this = &v->arena[v->length_bytes - sizeof(uintptr_t)];
	UP(this) = value;
}

void svec_remove(SwapVector* v, size_t index)
{
	v->length -= 1;
	v->length_bytes -= sizeof(uintptr_t);
	
	if (UNLIKELY(v->length_bytes == 0))
	{
		return;
	}
	
	char* this = &v->arena[v->length_bytes];
	v->data[index] = UP(this);
}

void svec_pop(SwapVector* v)
{
	v->length -= 1;
	v->length_bytes -= sizeof(uintptr_t);
}

void svec_clear(SwapVector* v)
{
	v->length = 0;
	v->length_bytes = 0;
}

void svec_release(SWFAppContext* app_context, SwapVector* v)
{
	FREE(v->arena);
}