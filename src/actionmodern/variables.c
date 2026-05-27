#include <string.h>

#include <map.h>
#include <common.h>
#include <action.h>
#include <variables.h>
#include <heap.h>

#define VAL(type, x) *((type*) x)

void initMap(SWFAppContext* app_context)
{
	app_context->var_ctx.var_map = hashmap_create();
	app_context->var_ctx.next_str_id = app_context->max_string_id;
}

static int free_variable_callback(const void* key, size_t ksize, uintptr_t value, void* app_context_void)
{
	SWFAppContext* app_context = (SWFAppContext*) app_context_void;
	
	FREE((void*) value);
	
	return 0;
}

void freeMap(SWFAppContext* app_context)
{
	// Free hashmap
	hashmap_iterate(app_context->var_ctx.var_map, free_variable_callback, app_context);
	hashmap_free(app_context->var_ctx.var_map);
}

u32 getStringId(SWFAppContext* app_context, char* str, size_t str_size)
{
	uintptr_t id_ptr;
	
	if (hashmap_get(app_context->var_ctx.var_map, str, str_size, &id_ptr))
	{
		return VAL(u32, id_ptr);
	}
	
	u32 id = (u32) app_context->var_ctx.next_str_id;
	app_context->var_ctx.next_str_id += 1;
	
	id_ptr = (uintptr_t) HALLOC(sizeof(u32));
	VAL(u32, id_ptr) = id;
	
	hashmap_set(app_context->var_ctx.var_map, str, str_size, id_ptr);
	
	return VAL(u32, id_ptr);
}

char* materializeStringList(SWFAppContext* app_context)
{
	// Get the string list
	u64* str_list = (u64*) &STACK_TOP_VALUE;
	u64 num_strings = str_list[0];
	u32 total_size = STACK_TOP_N;
	
	// Allocate heap memory for concatenated result
	char* result = (char*) HALLOC(total_size + 1);
	
	// Concatenate all strings
	char* dest = result;
	for (u64 i = 0; i < 2*num_strings; i += 2)
	{
		char* src = (char*) str_list[i + 1];
		u64 len = str_list[i + 2];
		memcpy(dest, src, len);
		dest += len;
	}
	*dest = '\0';
	
	return result;
}