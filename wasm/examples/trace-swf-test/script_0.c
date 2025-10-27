#include <recomp.h>
#include "script_decls.h"

void script_0(char* stack, u32* sp)
{
	// Push (String)
	PUSH_STR(str_0, 14);
	// Trace
	actionTrace(stack, sp);
}