#include <map.h>
#include <common.h>
#include <action.h>
#include <variables.h>
#include <heap.h>

#define VAL(type, x) *((type*) x)

hashmap* var_map = NULL;
ActionVar** var_array = NULL;
size_t var_array_size = 0;

void initMap()
{
	var_map = hashmap_create();
}

void initVarArray(SWFAppContext* app_context, size_t max_string_id)
{
	var_array_size = max_string_id + 1;
	var_array = (ActionVar**) HALLOC(var_array_size*sizeof(ActionVar*));
	
	for (size_t i = 1; i < var_array_size; ++i)
	{
		var_array[i] = (ActionVar*) HALLOC(sizeof(ActionVar));
	}
}

static int free_variable_callback(const void* key, size_t ksize, uintptr_t value, SWFAppContext* app_context)
{
	ActionVar* var = (ActionVar*) value;
	
	// Free heap-allocated strings
	if (var->type == ACTION_STACK_VALUE_STRING && var->owns_memory)
	{
		FREE(var->heap_ptr);
	}
	
	FREE(var);
	return 0;
}

ActionVar* getVariableById(SWFAppContext* app_context, u32 string_id)
{
	return var_array[string_id];
}

ActionVar* getVariable(SWFAppContext* app_context, char* var_name, size_t key_size)
{
	ActionVar* var;
	
	if (hashmap_get(var_map, var_name, key_size, (uintptr_t*) &var))
	{
		return var;
	}
	
	var = (ActionVar*) HALLOC(sizeof(ActionVar));
	
	hashmap_set(var_map, var_name, key_size, (uintptr_t) var);
	
	return var;
}

char* materializeStringList(SWFAppContext* app_context, char* stack, u32* sp)
{
	// Get the string list
	u64* str_list = (u64*) STACK_TOP_VALUE;
	u64 num_strings = str_list[0];
	u32 total_size = STACK_TOP_N;
	
	// Allocate heap memory for concatenated result
	char* result = (char*) HALLOC(total_size + 1);
	
	// Concatenate all strings
	char* dest = result;
	for (u64 i = 0; i < num_strings; i++)
	{
		char* src = (char*) str_list[i + 1];
		size_t len = strlen(src);  // BAD
		memcpy(dest, src, len);
		dest += len;
	}
	*dest = '\0';
	
	return result;
}

void setVariableWithValue(SWFAppContext* app_context, ActionVar* var, char* stack, u32* sp)
{
	// Free old string if variable owns memory
	if (var->type == ACTION_STACK_VALUE_STRING && var->owns_memory)
	{
		free(var->heap_ptr);
		var->owns_memory = false;
	}
	
	ActionStackValueType type = STACK_TOP_TYPE;
	
	if (type == ACTION_STACK_VALUE_STR_LIST)
	{
		// Materialize string to heap
		char* heap_str = materializeStringList(app_context, stack, sp);
		u32 total_size = STACK_TOP_N;
		
		var->type = ACTION_STACK_VALUE_STRING;
		var->str_size = total_size;
		var->heap_ptr = heap_str;
		var->owns_memory = true;
	}
	
	else
	{
		// Numeric types and regular strings - store directly
		var->type = type;
		var->str_size = STACK_TOP_N;
		var->value = STACK_TOP_VALUE;
	}
}

void freeMap(SWFAppContext* app_context)
{
	if (var_map)
	{
		hashmap_iterate(var_map, free_variable_callback, app_context);
		hashmap_free(var_map);
		var_map = NULL;
	}
	
	// Free array-based variables
	if (var_array)
	{
		for (size_t i = 0; i < var_array_size; i++)
		{
			if (var_array[i])
			{
				// Free heap-allocated strings
				if (var_array[i]->type == ACTION_STACK_VALUE_STRING &&
				    var_array[i]->owns_memory)
				{
					FREE(var_array[i]->heap_ptr);
				}
				
				FREE(var_array[i]);
			}
		}
		
		FREE(var_array);
		var_array = NULL;
		var_array_size = 0;
	}
}