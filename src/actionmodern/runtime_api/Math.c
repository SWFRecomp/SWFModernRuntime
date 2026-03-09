#include <Math.h>

#include <initial_strings_decls.h>

void Math_abs(SWFAppContext* app_context)
{
	ASProperty* arg1 = getPropertyInThisScope(STR_ID_X, NULL, 0);
	
	s32 val = (s32) arg1->value.value;
	
	PUSH(ACTION_STACK_VALUE_INT, val < 0 ? -val : val);
}