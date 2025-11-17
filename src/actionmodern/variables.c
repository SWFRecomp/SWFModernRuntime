#include <map.h>

#include <variables.h>
#include <heap.h>

hashmap* var_map = NULL;

void initMap()
{
	var_map = hashmap_create();
}

void freeMap()
{
	hashmap_free(var_map);
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