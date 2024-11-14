#include <unordered_dense.h>

#include <variables.h>

ankerl::unordered_dense::map<char*, var*> var_map;

extern "C" var* getVariable(u64 var_name)
{
	char* var_str = (char*) var_name;
	if (var_map.count(var_str) == 0)
	{
		var_map[var_str] = (var*) malloc(sizeof(var));
	}
	
	return var_map[var_str];
}