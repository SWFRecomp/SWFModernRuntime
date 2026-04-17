#include <Object.h>

#include <objects.h>

#include <initial_strings_decls.h>

void new_Object(SWFAppContext* app_context, u32 num_args)
{
	for (u32 i = 0; i < num_args; ++i)
	{
		POP();
	}
	
	RETURN_VOID();
}