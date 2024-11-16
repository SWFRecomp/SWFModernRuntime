#include <stdlib.h>

#include <action.h>

void convertFloats(var* a, var* b)
{
	if (a->type == ACTION_STACK_VALUE_STRING)
	{
		float temp = (float) atof((char*) a->value);
		a->type = ACTION_STACK_VALUE_F32;
		a->value = VAL(u32, &temp);
	}
	
	if (b->type == ACTION_STACK_VALUE_STRING)
	{
		float temp = (float) atof((char*) b->value);
		b->type = ACTION_STACK_VALUE_F32;
		b->value = VAL(u32, &temp);
	}
}

void actionAdd(var* a, var* b)
{
	convertFloats(a, b);
	
	float c = VAL(float, &a->value) + VAL(float, &b->value);
	
	a->value = VAL(u32, &c);
}

void actionTrace(var* val)
{
	switch (val->type)
	{
		case ACTION_STACK_VALUE_STRING:
		{
			printf("%s\n", (char*) val->value);
			break;
		}
		
		case ACTION_STACK_VALUE_F32:
		{
			printf("%.15g\n", VAL(float, &val->value));
			break;
		}
	}
}