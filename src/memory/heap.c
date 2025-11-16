#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "o1heap.h"

#include "memory/heap.h"
#include "utils.h"

#define DEFAULT_INITIAL_HEAP_SIZE (64 * 1024 * 1024)  // 64 MB

void heap_init(SWFAppContext* app_context, size_t initial_size)
{
	if (initial_size == 0)
	{
		initial_size = DEFAULT_INITIAL_HEAP_SIZE;
	}
	
	app_context->heap = vmem_reserve(initial_size);
	vmem_commit(app_context->heap, initial_size);
	app_context->heap_current_size = initial_size;
	
	u32 page_size = getpagesize();
	
	app_context->heap_instance = o1heapInit(app_context->heap, initial_size);
	
	for (int i = 0; i < initial_size/page_size; ++i)
	{
		app_context->heap[i*page_size] = 0;
	}
	
	app_context->heap_inited = true;
}

void* heap_alloc(SWFAppContext* app_context, size_t size)
{
	return o1heapAllocate(app_context->heap_instance, size);
}

void heap_free(SWFAppContext* app_context, void* ptr)
{
	o1heapFree(app_context->heap_instance, ptr);
}

void heap_shutdown(SWFAppContext* app_context)
{
	free(app_context->heap);
}
