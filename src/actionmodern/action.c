#include <stdlib.h>
#include <math.h>

#include <action.h>

void convertFloat(var* v)
{
	if (v->type == ACTION_STACK_VALUE_STRING)
	{
		float temp = (float) atof((char*) v->value);
		v->type = ACTION_STACK_VALUE_F32;
		v->value = VAL(u32, &temp);
	}
}

void actionAdd(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float c = VAL(float, &a->value) + VAL(float, &b->value);
	b->value = VAL(u32, &c);
}

void actionSubtract(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float c = VAL(float, &b->value) - VAL(float, &a->value);
	b->value = VAL(u32, &c);
}

void actionMultiply(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float c = VAL(float, &a->value)*VAL(float, &b->value);
	b->value = VAL(u32, &c);
}

void actionDivide(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float c;
	
	if (a->value == 0.0f)
	{
		// SWF 4:
		b->type = ACTION_STACK_VALUE_STRING;
		b->value = (u64) "#ERROR#";
		
		// SWF 5:
		//~ if (b->value == 0.0f)
		//~ {
			//~ c = NAN;
		//~ }
		
		//~ else if (b->value > 0.0f)
		//~ {
			//~ c = INFINITY;
		//~ }
		
		//~ else
		//~ {
			//~ c = -INFINITY;
		//~ }
	}
	
	else
	{
		c = VAL(float, &b->value)/VAL(float, &a->value);
		b->value = VAL(u32, &c);
	}
}

void actionEquals(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float result = a->value == b->value ? 1.0f : 0.0f;
	b->value = VAL(u64, &result);
}

void actionLess(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float result = a->value < b->value ? 1.0f : 0.0f;
	b->value = VAL(u64, &result);
}

void actionAnd(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float result = a->value != 0.0f && b->value != 0.0f ? 1.0f : 0.0f;
	b->value = VAL(u64, &result);
}

void actionOr(var* a, var* b)
{
	convertFloat(a);
	convertFloat(b);
	
	float result = a->value != 0.0f || b->value != 0.0f ? 1.0f : 0.0f;
	b->value = VAL(u64, &result);
}

void actionNot(var* a)
{
	convertFloat(a);
	
	float result = a->value == 0.0f ? 1.0f : 0.0f;
	a->value = VAL(u64, &result);
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