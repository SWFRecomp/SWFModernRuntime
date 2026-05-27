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
	rwlock_init(&app_context->heap_lock);
}

void* heap_alloc(SWFAppContext* app_context, size_t size)
{
	void* ret;
	
	LOCK_WRITE(app_context->heap_lock,
	{
		ret = o1heapAllocate(app_context->heap_instance, size);
	});
	
	if (UNLIKELY(ret == NULL))
	{
		//~ for (size_t i = 0; i < app_context->active_objects.length; ++i)
		//~ {
			//~ u32* o = (u32*) app_context->active_objects.data[i];
			//~ fprintf(stderr, "unfreed object %d\n", *o);
		//~ }
		
		UNREACHABLE("Error allocating memory");
	}
	
	return ret;
}

void* heap_realloc(SWFAppContext* app_context, void* ptr, size_t size)
{
	void* ret;
	
	LOCK_WRITE(app_context->heap_lock,
	{
		ret = o1heapReallocate(app_context->heap_instance, ptr, size);
	});
	
	if (UNLIKELY(ret == NULL))
	{
		//~ for (size_t i = 0; i < app_context->active_objects.length; ++i)
		//~ {
			//~ u32* o = (u32*) app_context->active_objects.data[i];
			//~ fprintf(stderr, "unfreed object %d\n", *o);
		//~ }
		
		UNREACHABLE("Out of memory, quitting");
	}
	
	return ret;
}

void heap_free(SWFAppContext* app_context, void* ptr)
{
	LOCK_WRITE(app_context->heap_lock,
	{
		o1heapFree(app_context->heap_instance, ptr);
	});
}

void heap_shutdown(SWFAppContext* app_context)
{
	vmem_release(app_context->heap, app_context->heap_size);
}