#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include <recomp.h>

ActionStackValueType convertString(char* stack, u32* sp, char* var_str)
{
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_F32)
	{
		STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
		VAL(u64, &STACK_TOP_VALUE) = (u64) var_str;
		snprintf(var_str, 17, "%.15g", VAL(float, &STACK_TOP_VALUE));
	}
	
	return ACTION_STACK_VALUE_STRING;
}

ActionStackValueType convertFloat(char* stack, u32* sp)
{
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING)
	{
		double temp = atof((char*) VAL(u64, &STACK_TOP_VALUE));
		STACK_TOP_TYPE = ACTION_STACK_VALUE_F64;
		VAL(u64, &STACK_TOP_VALUE) = VAL(u64, &temp);
		
		return ACTION_STACK_VALUE_F64;
	}
	
	return ACTION_STACK_VALUE_F32;
}

ActionStackValueType convertDouble(char* stack, u32* sp)
{
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_F32)
	{
		double temp = VAL(double, &STACK_TOP_VALUE);
		STACK_TOP_TYPE = ACTION_STACK_VALUE_F64;
		VAL(u64, &STACK_TOP_VALUE) = VAL(u64, &temp);
	}
	
	return ACTION_STACK_VALUE_F64;
}

void pushVar(char* stack, u32* sp, ActionVar* var)
{
	switch (var->type)
	{
		case ACTION_STACK_VALUE_F32:
		case ACTION_STACK_VALUE_F64:
		{
			PUSH(var->type, var->value);
			
			break;
		}
		
		case ACTION_STACK_VALUE_STRING:
		{
			PUSH_STR((char*) var->value, var->str_size);
			
			break;
		}
	}
}

void peekVar(char* stack, u32* sp, ActionVar* var)
{
	var->type = STACK_TOP_TYPE;
	var->str_size = STACK_TOP_N;
	
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STR_LIST)
	{
		var->value = (u64) &STACK_TOP_VALUE;
	}
	
	else
	{
		var->value = VAL(u64, &STACK_TOP_VALUE);
	}
}

void popVar(char* stack, u32* sp, ActionVar* var)
{
	peekVar(stack, sp, var);
	
	POP();
}

void actionAdd(char* stack, u32* sp)
{
	convertFloat(stack, sp);
	ActionVar a;
	popVar(stack, sp, &a);
	
	convertFloat(stack, sp);
	ActionVar b;
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		double c = b_val + a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		double c = b_val + a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) + VAL(float, &a.value);
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

//~ void actionSubtract(u64 a, u64 b)
//~ {
	//~ convertFloat(a);
	//~ convertFloat(b);
	
	//~ if (a->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(b);
		
		//~ double c = VAL(double, &a->value) - VAL(double, &b->value);
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else if (b->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(a);
		
		//~ double c = VAL(double, &a->value) - VAL(double, &b->value);
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else
	//~ {
		//~ float c = VAL(float, &a->value) - VAL(float, &b->value);
		//~ b->value = VAL(u32, &c);
	//~ }
//~ }

//~ void actionMultiply(u64 a, u64 b)
//~ {
	//~ convertFloat(a);
	//~ convertFloat(b);
	
	//~ if (a->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(b);
		
		//~ double c = VAL(double, &a->value)*VAL(double, &b->value);
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else if (b->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(a);
		
		//~ double c = VAL(double, &a->value)*VAL(double, &b->value);
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else
	//~ {
		//~ float c = VAL(float, &a->value)*VAL(float, &b->value);
		//~ b->value = VAL(u32, &c);
	//~ }
//~ }

void actionDivide(char* stack, u32* sp)
{
	convertFloat(stack, sp);
	ActionVar a;
	popVar(stack, sp, &a);
	
	convertFloat(stack, sp);
	ActionVar b;
	popVar(stack, sp, &b);
	
	if (VAL(float, &a.value) == 0.0f)
	{
		// SWF 4:
		PUSH_STR("#ERROR#", 8);
		
		// SWF 5:
		//~ if (a->value == 0.0f)
		//~ {
			//~ float c = NAN;
		//~ }
		
		//~ else if (a->value > 0.0f)
		//~ {
			//~ float c = INFINITY;
		//~ }
		
		//~ else
		//~ {
			//~ float c = -INFINITY;
		//~ }
	}
	
	else
	{
		if (a.type == ACTION_STACK_VALUE_F64)
		{
			double a_val = VAL(double, &a.value);
			double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
			
			double c = b_val/a_val;
			PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
		}
		
		else if (b.type == ACTION_STACK_VALUE_F64)
		{
			double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
			double b_val = VAL(double, &b.value);
			
			double c = b_val/a_val;
			PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
		}
		
		else
		{
			float c = VAL(float, &b.value)/VAL(float, &a.value);
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
		}
	}
}

void actionEquals(char* stack, u32* sp)
{
	convertFloat(stack, sp);
	ActionVar a;
	popVar(stack, sp, &a);
	
	convertFloat(stack, sp);
	ActionVar b;
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		float c = b_val == a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		float c = b_val == a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) == VAL(float, &a.value) ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

//~ void actionLess(u64 a, u64 b)
//~ {
	//~ convertFloat(a);
	//~ convertFloat(b);
	
	//~ if (a->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(b);
		
		//~ double c = VAL(double, &a->value) < VAL(double, &b->value) ? 1.0f : 0.0f;
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else if (b->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(a);
		
		//~ double c = VAL(double, &a->value) < VAL(double, &b->value) ? 1.0f : 0.0f;
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else
	//~ {
		//~ float c = VAL(float, &a->value) < VAL(float, &b->value) ? 1.0f : 0.0f;
		//~ b->value = VAL(u32, &c);
	//~ }
//~ }

//~ void actionAnd(u64 a, u64 b)
//~ {
	//~ convertFloat(a);
	//~ convertFloat(b);
	
	//~ if (a->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(b);
		
		//~ double c = VAL(double, &a->value) != 0.0f && VAL(double, &b->value) != 0.0f ? 1.0f : 0.0f;
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else if (b->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(a);
		
		//~ double c = VAL(double, &a->value) != 0.0f && VAL(double, &b->value) != 0.0f ? 1.0f : 0.0f;
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else
	//~ {
		//~ float c = VAL(float, &a->value) != 0.0f && VAL(float, &b->value) != 0.0f ? 1.0f : 0.0f;
		//~ b->value = VAL(u32, &c);
	//~ }
	
	//~ float result = a->value != 0.0f && b->value != 0.0f ? 1.0f : 0.0f;
	//~ b->value = VAL(u64, &result);
//~ }

//~ void actionOr(u64 a, u64 b)
//~ {
	//~ convertFloat(a);
	//~ convertFloat(b);
	
	//~ if (a->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(b);
		
		//~ double c = VAL(double, &a->value) != 0.0f || VAL(double, &b->value) != 0.0f ? 1.0f : 0.0f;
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else if (b->type == ACTION_STACK_VALUE_F64)
	//~ {
		//~ convertDouble(a);
		
		//~ double c = VAL(double, &a->value) != 0.0f || VAL(double, &b->value) != 0.0f ? 1.0f : 0.0f;
		//~ b->value = VAL(u64, &c);
	//~ }
	
	//~ else
	//~ {
		//~ float c = VAL(float, &a->value) != 0.0f || VAL(float, &b->value) != 0.0f ? 1.0f : 0.0f;
		//~ b->value = VAL(u32, &c);
	//~ }
//~ }

//~ void actionNot(u64 v)
//~ {
	//~ convertFloat(v);
	
	//~ float result = v->value == 0.0f ? 1.0f : 0.0f;
	//~ v->value = VAL(u64, &result);
//~ }

int strcmp_list_a_list_b(u64 a_value, u64 b_value)
{
	char** a_list = (char**) a_value;
	char** b_list = (char**) b_value;
	
	u64 num_a_strings = (u64) a_list[0];
	u64 num_b_strings = (u64) b_list[0];
	
	u64 a_str_i = 0;
	u64 b_str_i = 0;
	
	u64 a_i = 0;
	u64 b_i = 0;
	
	u64 min_count = (num_a_strings < num_b_strings) ? num_a_strings : num_b_strings;
	
	while (1)
	{
		char c_a = a_list[a_str_i + 1][a_i];
		char c_b = b_list[b_str_i + 1][b_i];
		
		if (c_a == 0)
		{
			if (a_str_i + 1 != min_count)
			{
				a_str_i += 1;
				a_i = 0;
				continue;
			}
			
			else
			{
				return c_a - c_b;
			}
		}
		
		if (c_b == 0)
		{
			if (b_str_i + 1 != min_count)
			{
				b_str_i += 1;
				b_i = 0;
				continue;
			}
			
			else
			{
				return c_a - c_b;
			}
		}
		
		if (c_a != c_b)
		{
			return c_a - c_b;
		}
		
		a_i += 1;
		b_i += 1;
	}
	
	EXC("um how lol\n");
	return 0;
}

int strcmp_list_a_not_b(u64 a_value, u64 b_value)
{
	char** a_list = (char**) a_value;
	char* b_str = (char*) b_value;
	
	u64 num_a_strings = (u64) a_list[0];
	
	u64 a_str_i = 0;
	
	u64 a_i = 0;
	u64 b_i = 0;
	
	while (1)
	{
		char c_a = a_list[a_str_i + 1][a_i];
		char c_b = b_str[b_i];
		
		if (c_a == 0)
		{
			if (a_str_i + 1 != num_a_strings)
			{
				a_str_i += 1;
				a_i = 0;
				continue;
			}
			
			else
			{
				return c_a - c_b;
			}
		}
		
		if (c_a != c_b)
		{
			return c_a - c_b;
		}
		
		a_i += 1;
		b_i += 1;
	}
	
	EXC("um how lol\n");
	return 0;
}

int strcmp_not_a_list_b(u64 a_value, u64 b_value)
{
	char* a_str = (char*) a_value;
	char** b_list = (char**) b_value;
	
	u64 num_b_strings = (u64) b_list[0];
	
	u64 b_str_i = 0;
	
	u64 a_i = 0;
	u64 b_i = 0;
	
	while (1)
	{
		char c_a = a_str[a_i];
		char c_b = b_list[b_str_i + 1][b_i];
		
		if (c_b == 0)
		{
			if (b_str_i + 1 != num_b_strings)
			{
				b_str_i += 1;
				b_i = 0;
				continue;
			}
			
			else
			{
				return c_a - c_b;
			}
		}
		
		if (c_a != c_b)
		{
			return c_a - c_b;
		}
		
		a_i += 1;
		b_i += 1;
	}
	
	EXC("um how lol\n");
	return 0;
}

void actionStringEquals(char* stack, u32* sp, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(stack, sp, a_str);
	popVar(stack, sp, &a);
	
	ActionVar b;
	convertString(stack, sp, b_str);
	popVar(stack, sp, &b);
	
	int cmp_result;
	
	int a_is_list = a.type == ACTION_STACK_VALUE_STR_LIST;
	int b_is_list = b.type == ACTION_STACK_VALUE_STR_LIST;
	
	if (a_is_list && b_is_list)
	{
		cmp_result = strcmp_list_a_list_b(a.value, b.value);
	}
	
	else if (a_is_list && !b_is_list)
	{
		cmp_result = strcmp_list_a_not_b(a.value, b.value);
	}
	
	else if (!a_is_list && b_is_list)
	{
		cmp_result = strcmp_not_a_list_b(a.value, b.value);
	}
	
	else
	{
		cmp_result = strcmp((char*) a.value, (char*) b.value);
	}
	
	float result = cmp_result == 0 ? 1.0f : 0.0f;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionStringLength(char* stack, u32* sp, char* v_str)
{
	ActionVar v;
	convertString(stack, sp, v_str);
	popVar(stack, sp, &v);
	
	float str_size = (float) v.str_size;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &str_size));
}

void actionStringAdd(char* stack, u32* sp, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(stack, sp, a_str);
	peekVar(stack, sp, &a);
	
	ActionVar b;
	convertString(stack, sp, b_str);
	peekVar(stack, &SP_SECOND_TOP, &b);
	
	u64 num_a_strings;
	u64 num_b_strings;
	u64 num_strings = 0;
	
	if (b.type == ACTION_STACK_VALUE_STR_LIST)
	{
		num_b_strings = *((u64*) b.value);
	}
	
	else
	{
		num_b_strings = 1;
	}
	
	num_strings += num_b_strings;
	
	if (a.type == ACTION_STACK_VALUE_STR_LIST)
	{
		num_a_strings = *((u64*) a.value);
	}
	
	else
	{
		num_a_strings = 1;
	}
	
	num_strings += num_a_strings;
	
	PUSH_STR_LIST(b.str_size + a.str_size, (u32) sizeof(u64)*(num_strings + 1));
	
	u64* str_list = (u64*) &STACK_TOP_VALUE;
	str_list[0] = num_strings;
	
	if (b.type == ACTION_STACK_VALUE_STR_LIST)
	{
		u64* b_list = (u64*) b.value;
		
		for (u64 i = 0; i < num_b_strings; ++i)
		{
			str_list[i + 1] = b_list[i + 1];
		}
	}
	
	else
	{
		str_list[1] = b.value;
	}
	
	if (a.type == ACTION_STACK_VALUE_STR_LIST)
	{
		u64* a_list = (u64*) a.value;
		
		for (u64 i = 0; i < num_a_strings; ++i)
		{
			str_list[i + 1 + num_b_strings] = a_list[i + 1];
		}
	}
	
	else
	{
		str_list[1 + num_b_strings] = a.value;
	}
}

void actionTrace(char* stack, u32* sp)
{
	ActionStackValueType type = STACK_TOP_TYPE;
	
	switch (type)
	{
		case ACTION_STACK_VALUE_STRING:
		{
			printf("%s\n", (char*) STACK_TOP_VALUE);
			break;
		}
		
		case ACTION_STACK_VALUE_STR_LIST:
		{
			u64* str_list = (u64*) &STACK_TOP_VALUE;
			
			for (u64 i = 0; i < str_list[0]; ++i)
			{
				printf("%s", (char*) str_list[i + 1]);
			}
			
			printf("\n");
			
			break;
		}
		
		case ACTION_STACK_VALUE_F32:
		{
			printf("%.15g\n", VAL(float, &STACK_TOP_VALUE));
			break;
		}
		
		case ACTION_STACK_VALUE_F64:
		{
			printf("%.15g\n", VAL(double, &STACK_TOP_VALUE));
			break;
		}
	}
	
	POP();
}