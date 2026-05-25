#include <toplevel.h>

#include <objects.h>
#include <flashbang_context.h>

#include <initial_strings_decls.h>

void ASSetPropFlags(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	RETURN_VOID();
}

void recompGetLastKey(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	u8 key = FBC->last_key_pressed;
	
	PUSH_INT(key);
}