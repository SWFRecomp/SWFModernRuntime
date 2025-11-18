#include <o1heap.h>

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

void* heap_aligned_alloc(SWFAppContext* app_context, size_t alignment, size_t size)
{
	char* m = (char*) o1heapAllocate(app_context->heap_instance, size + alignment);
	m += alignment;
	return (void*) ((uintptr_t) m & ~(alignment - 1));
}

void heap_free(SWFAppContext* app_context, void* ptr)
{
	o1heapFree(app_context->heap_instance, ptr);
}

void heap_shutdown(SWFAppContext* app_context)
{
	vmem_release(app_context->heap, app_context->heap_size);
}