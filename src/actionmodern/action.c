#include <stdlib.h>
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

ActionStackValueType concatenateStringList(ActionVar* v, char** out_str)
{
	if (v->type != ACTION_STACK_VALUE_STR_LIST)
	{
		*out_str = NULL;
		return v->type;
	}
	
	char** str_list = (char**) v->value;
	u64 out_str_i = 0;
	
	do
	{
		*out_str = malloc(v->str_size + 1);
	} while (errno != 0);
	
	for (u64 i = 0; i < ((u64) str_list[0]); ++i)
	{
		char c = 1;
		u64 j = 0;
		
		while (c != 0)
		{
			c = str_list[i + 1][j];
			(*out_str)[out_str_i] = c;
			j += 1;
			out_str_i += 1;
		}
		
		out_str_i -= 1;
	}
	
	return ACTION_STACK_VALUE_STR_LIST;
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

void actionStringEquals(char* stack, u32* sp, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(stack, sp, a_str);
	popVar(stack, sp, &a);
	
	ActionVar b;
	convertString(stack, sp, b_str);
	popVar(stack, sp, &b);
	
	char* final_a_str;
	char* final_b_str;
	
	int free_a = 1;
	int free_b = 1;
	
	if (concatenateStringList(&a, &final_a_str) != ACTION_STACK_VALUE_STR_LIST)
	{
		final_a_str = (char*) a.value;
		free_a = 0;
	}
	
	if (concatenateStringList(&b, &final_b_str) != ACTION_STACK_VALUE_STR_LIST)
	{
		final_b_str = (char*) b.value;
		free_b = 0;
	}
	
	int cmp_result = strcmp(final_a_str, final_b_str);
	
	if (free_a)
	{
		free(final_a_str);
	}
	
	if (free_b)
	{
		free(final_b_str);
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