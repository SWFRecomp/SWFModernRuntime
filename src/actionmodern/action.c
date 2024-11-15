#include <action.h>

void actionTrace(var val)
{
	switch (val.type)
	{
		case ACTION_STACK_VALUE_STRING:
		{
			printf("%s\n", (char*) val.value);
			break;
		}
		
		case ACTION_STACK_VALUE_F32:
		{
			printf("%.15g\n", VAL(float, &val.value));
			break;
		}
	}
}