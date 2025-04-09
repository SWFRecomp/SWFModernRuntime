#include <unordered_dense.h>

#include <variables.h>

ankerl::unordered_dense::map<char*, ActionVar*> var_map;

ActionVar* getVariable(char* var_name)
{
	if (var_map.count(var_name) == 0)
	{
		var_map[var_name] = (ActionVar*) malloc(sizeof(ActionVar));
	}
	
	return var_map[var_name];
}