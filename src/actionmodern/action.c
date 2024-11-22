#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <action.h>

void convertString(var* v, char* v_str)
{
	if (v->type == ACTION_STACK_VALUE_F32)
	{
		snprintf(v_str, 17, "%.15g", VAL(float, &v->value));
		v->type = ACTION_STACK_VALUE_STRING;
		v->value = (u64) v_str;
	}
}

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
		//~ if (a->value == 0.0f)
		//~ {
			//~ c = NAN;
		//~ }
		
		//~ else if (a->value > 0.0f)
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

void actionNot(var* v)
{
	convertFloat(v);
	
	float result = v->value == 0.0f ? 1.0f : 0.0f;
	v->value = VAL(u64, &result);
}

void actionStringEquals(var* a, var* b, char* a_str, char* b_str)
{
	convertString(a, a_str);
	convertString(b, b_str);
	
	float result = strncmp((char*) a->value, (char*) b->value, 1024) == 0 ? 1.0f : 0.0f;
	b->type = ACTION_STACK_VALUE_F32;
	b->value = VAL(u64, &result);
}

void actionStringLength(var* v, char* v_str)
{
	convertString(v, v_str);
	
	float result = (float) strnlen((char*) v->value, 1024);
	v->type = ACTION_STACK_VALUE_F32;
	v->value = VAL(u64, &result);
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