#include <variables.h>

#include <map.h>

hashmap* var_map = NULL;

void initMap()
{
	var_map = hashmap_create();
}

ActionVar* getVariable(char* var_name, size_t key_size)
{
	ActionVar* var;
	
	if (hashmap_get(var_map, var_name, key_size, (uintptr_t*) &var))
	{
		return var;
	}
	
	var = (ActionVar*) malloc(sizeof(ActionVar));
	hashmap_set(var_map, var_name, key_size, (uintptr_t) var);
	
	return var;
}