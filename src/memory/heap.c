#include <o1heap.h>
#include <string.h>

#include <heap.h>
#include <utils.h>

void heap_init(SWFAppContext* app_context, size_t size)
{
	char* h = vmem_reserve(size);
	app_context->heap = h;
	app_context->heap_size = size;
	app_context->heap_instance = o1heapInit(h, size);
}

void* heap_alloc(SWFAppContext* app_context, size_t size)
{
	return o1heapAllocate(app_context->heap_instance, size);
}

void* heap_calloc(SWFAppContext* app_context, size_t count, size_t size)
{
	size_t total = count * size;
	void* ptr = o1heapAllocate(app_context->heap_instance, total);
	if (ptr) {
		memset(ptr, 0, total);
	}
	return ptr;
}

void heap_free(SWFAppContext* app_context, void* ptr)
{
	o1heapFree(app_context->heap_instance, ptr);
}

void heap_shutdown(SWFAppContext* app_context)
{
	vmem_release(app_context->heap, app_context->heap_size);
}