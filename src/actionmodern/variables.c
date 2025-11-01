#include <errno.h>
#include <string.h>

#include <map.h>
#include <common.h>
#include <variables.h>

#define VAL(type, x) *((type*) x)

hashmap* var_map = NULL;
ActionVar** var_array = NULL;
size_t var_array_size = 0;

void initMap()
{
	var_map = hashmap_create();
}

void initVarArray(size_t max_string_id)
{
	var_array_size = max_string_id;
	var_array = (ActionVar**) calloc(var_array_size, sizeof(ActionVar*));

	if (!var_array)
	{
		EXC("Failed to allocate variable array\n");
		exit(1);
	}
}

static int free_variable_callback(const void *key, size_t ksize, uintptr_t value, void *usr)
{
	ActionVar* var = (ActionVar*) value;

	// Free heap-allocated strings
	if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_data.owns_memory)
	{
		free(var->data.string_data.heap_ptr);
	}

	free(var);
	return 0;
}

void freeMap()
{
	if (var_map)
	{
		hashmap_iterate(var_map, free_variable_callback, NULL);
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
				    var_array[i]->data.string_data.owns_memory)
				{
					free(var_array[i]->data.string_data.heap_ptr);
				}
				free(var_array[i]);
			}
		}
		free(var_array);
		var_array = NULL;
		var_array_size = 0;
	}
}

ActionVar* getVariableById(u32 string_id)
{
	if (string_id == 0 || string_id >= var_array_size)
	{
		// Invalid ID or dynamic string (ID = 0)
		return NULL;
	}

	// Lazy allocation
	if (!var_array[string_id])
	{
		ActionVar* var = (ActionVar*) malloc(sizeof(ActionVar));
		if (!var)
		{
			EXC("Failed to allocate variable\n");
			return NULL;
		}

		// Initialize with unset type
		var->type = ACTION_STACK_VALUE_STRING;
		var->str_size = 0;
		var->data.string_data.heap_ptr = NULL;
		var->data.string_data.owns_memory = false;

		var_array[string_id] = var;
	}

	return var_array[string_id];
}

ActionVar* getVariable(char* var_name, size_t key_size)
{
	ActionVar* var;

	if (hashmap_get(var_map, var_name, key_size, (uintptr_t*) &var))
	{
		return var;
	}

	do
	{
		var = (ActionVar*) malloc(sizeof(ActionVar));
	} while (errno != 0);

	// Initialize with unset type
	var->type = ACTION_STACK_VALUE_STRING;
	var->str_size = 0;
	var->data.string_data.heap_ptr = NULL;
	var->data.string_data.owns_memory = false;

	hashmap_set(var_map, var_name, key_size, (uintptr_t) var);

	return var;
}

char* materializeStringList(char* stack, u32 sp)
{
	ActionStackValueType type = stack[sp];

	if (type == ACTION_STACK_VALUE_STR_LIST)
	{
		// Get the string list
		u64* str_list = (u64*) &stack[sp + 16];
		u64 num_strings = str_list[0];
		u32 total_size = VAL(u32, &stack[sp + 8]);

		// Allocate heap memory for concatenated result
		char* result = (char*) malloc(total_size + 1);
		if (!result)
		{
			EXC("Failed to allocate memory for string variable\n");
			return NULL;
		}

		// Concatenate all strings
		char* dest = result;
		for (u64 i = 0; i < num_strings; i++)
		{
			char* src = (char*) str_list[i + 1];
			size_t len = strlen(src);
			memcpy(dest, src, len);
			dest += len;
		}
		*dest = '\0';

		return result;
	}
	else if (type == ACTION_STACK_VALUE_STRING)
	{
		// Single string - duplicate it
		char* src = (char*) VAL(u64, &stack[sp + 16]);
		return strdup(src);
	}

	// Not a string type
	return NULL;
}

void setVariableWithValue(ActionVar* var, char* stack, u32 sp)
{
	// Free old string if variable owns memory
	if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_data.owns_memory)
	{
		free(var->data.string_data.heap_ptr);
		var->data.string_data.owns_memory = false;
	}

	ActionStackValueType type = stack[sp];

	if (type == ACTION_STACK_VALUE_STRING || type == ACTION_STACK_VALUE_STR_LIST)
	{
		// Materialize string to heap
		char* heap_str = materializeStringList(stack, sp);
		if (!heap_str)
		{
			// Allocation failed, variable becomes unset
			var->type = ACTION_STACK_VALUE_STRING;
			var->str_size = 0;
			var->data.numeric_value = 0;
			return;
		}

		var->type = ACTION_STACK_VALUE_STRING;
		var->str_size = strlen(heap_str);
		var->data.string_data.heap_ptr = heap_str;
		var->data.string_data.owns_memory = true;
	}
	else
	{
		// Numeric types - store directly
		var->type = type;
		var->str_size = VAL(u32, &stack[sp + 8]);
		var->data.numeric_value = VAL(u64, &stack[sp + 16]);
	}
}