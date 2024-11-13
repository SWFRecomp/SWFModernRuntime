#include <action.h>

void actionTrace(const char* str)
{
	printf("%s\n", str);
}

void actionTrace(var val)
{
	switch (val.type)
	{
		case ACTION_STACK_VALUE_STRING:
		{
			printf("%s\n", val.value);
			break;
		}
		
		case ACTION_STACK_VALUE_F32:
		{
			printf("%.15g\n", val.value);
			break;
		}
	}
}