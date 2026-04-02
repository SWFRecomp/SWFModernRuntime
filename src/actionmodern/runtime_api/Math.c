#include <math.h>

#include <Math.h>

#include <initial_strings_decls.h>

void Math_abs(SWFAppContext* app_context)
{
	ASProperty* arg1 = getPropertyInThisScope(STR_ID_X, NULL, 0);
	
	convertVarDouble(&arg1->value);
	
	f64 x = arg1->value.f64;
	x = x < 0.0 ? -x : x;
	
	PUSH_F64(x);
}