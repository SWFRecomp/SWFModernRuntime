#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <recomp.h>
#include <utils.h>

u32 start_time;

void initTime()
{
	start_time = get_elapsed_ms();
}

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
	u32 oldSP;

	switch (var->type)
	{
		case ACTION_STACK_VALUE_F32:
		case ACTION_STACK_VALUE_F64:
		{
			PUSH(var->type, var->data.numeric_value);

			break;
		}

		case ACTION_STACK_VALUE_STRING:
		{
			// Use heap pointer if variable owns memory, otherwise use numeric_value as pointer
			char* str_ptr = var->data.string_data.owns_memory ?
				var->data.string_data.heap_ptr :
				(char*) var->data.numeric_value;

			PUSH_STR(str_ptr, var->str_size);

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
		var->data.numeric_value = (u64) &STACK_TOP_VALUE;
	}

	else
	{
		var->data.numeric_value = VAL(u64, &STACK_TOP_VALUE);
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
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		double c = b_val + a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		double c = b_val + a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value) + VAL(float, &a.data.numeric_value);
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionSubtract(char* stack, u32* sp)
{
	convertFloat(stack, sp);
	ActionVar a;
	popVar(stack, sp, &a);
	
	convertFloat(stack, sp);
	ActionVar b;
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		double c = b_val - a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		double c = b_val - a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value) - VAL(float, &a.data.numeric_value);
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionMultiply(char* stack, u32* sp)
{
	convertFloat(stack, sp);
	ActionVar a;
	popVar(stack, sp, &a);
	
	convertFloat(stack, sp);
	ActionVar b;
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		double c = b_val*a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		double c = b_val*a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value)*VAL(float, &a.data.numeric_value);
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionDivide(char* stack, u32* sp)
{
	convertFloat(stack, sp);
	ActionVar a;
	popVar(stack, sp, &a);
	
	convertFloat(stack, sp);
	ActionVar b;
	popVar(stack, sp, &b);
	
	if (VAL(float, &a.data.numeric_value) == 0.0f)
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
			double a_val = VAL(double, &a.data.numeric_value);
			double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
			
			double c = b_val/a_val;
			PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
		}
		
		else if (b.type == ACTION_STACK_VALUE_F64)
		{
			double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
			double b_val = VAL(double, &b.data.numeric_value);
			
			double c = b_val/a_val;
			PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
		}
		
		else
		{
			float c = VAL(float, &b.data.numeric_value)/VAL(float, &a.data.numeric_value);
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
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		float c = b_val == a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		float c = b_val == a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value) == VAL(float, &a.data.numeric_value) ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionLess(char* stack, u32* sp)
{
	ActionVar a;
	convertFloat(stack, sp);
	popVar(stack, sp, &a);
	
	ActionVar b;
	convertFloat(stack, sp);
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		float c = b_val < a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		float c = b_val < a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value) < VAL(float, &a.data.numeric_value) ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionAnd(char* stack, u32* sp)
{
	ActionVar a;
	convertFloat(stack, sp);
	popVar(stack, sp, &a);
	
	ActionVar b;
	convertFloat(stack, sp);
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		float c = b_val != 0.0 && a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		float c = b_val != 0.0 && a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value) != 0.0f && VAL(float, &a.data.numeric_value) != 0.0f ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionOr(char* stack, u32* sp)
{
	ActionVar a;
	convertFloat(stack, sp);
	popVar(stack, sp, &a);
	
	ActionVar b;
	convertFloat(stack, sp);
	popVar(stack, sp, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.data.numeric_value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.data.numeric_value) : VAL(double, &b.data.numeric_value);
		
		float c = b_val != 0.0 || a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.data.numeric_value) : VAL(double, &a.data.numeric_value);
		double b_val = VAL(double, &b.data.numeric_value);
		
		float c = b_val != 0.0 || a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.data.numeric_value) != 0.0f || VAL(float, &a.data.numeric_value) != 0.0f ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionNot(char* stack, u32* sp)
{
	ActionVar v;
	convertFloat(stack, sp);
	popVar(stack, sp, &v);
	
	float result = v.data.numeric_value == 0.0f ? 1.0f : 0.0f;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u64, &result));
}

int evaluateCondition(char* stack, u32* sp)
{
	ActionVar v;
	convertFloat(stack, sp);
	popVar(stack, sp, &v);
	
	return v.data.numeric_value != 0.0f;
}

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
		cmp_result = strcmp_list_a_list_b(a.data.numeric_value, b.data.numeric_value);
	}
	
	else if (a_is_list && !b_is_list)
	{
		cmp_result = strcmp_list_a_not_b(a.data.numeric_value, b.data.numeric_value);
	}
	
	else if (!a_is_list && b_is_list)
	{
		cmp_result = strcmp_not_a_list_b(a.data.numeric_value, b.data.numeric_value);
	}
	
	else
	{
		cmp_result = strcmp((char*) a.data.numeric_value, (char*) b.data.numeric_value);
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
		num_b_strings = *((u64*) b.data.numeric_value);
	}
	
	else
	{
		num_b_strings = 1;
	}
	
	num_strings += num_b_strings;
	
	if (a.type == ACTION_STACK_VALUE_STR_LIST)
	{
		num_a_strings = *((u64*) a.data.numeric_value);
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
		u64* b_list = (u64*) b.data.numeric_value;
		
		for (u64 i = 0; i < num_b_strings; ++i)
		{
			str_list[i + 1] = b_list[i + 1];
		}
	}
	
	else
	{
		str_list[1] = b.data.numeric_value;
	}
	
	if (a.type == ACTION_STACK_VALUE_STR_LIST)
	{
		u64* a_list = (u64*) a.data.numeric_value;
		
		for (u64 i = 0; i < num_a_strings; ++i)
		{
			str_list[i + 1 + num_b_strings] = a_list[i + 1];
		}
	}
	
	else
	{
		str_list[1 + num_b_strings] = a.data.numeric_value;
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
	
	fflush(stdout);

	POP();
}

void actionGetVariable(char* stack, u32* sp)
{
	u32 oldSP;

	// Read variable name info from stack
	u32 string_id = VAL(u32, &stack[*sp + 4]);
	char* var_name = (char*) VAL(u64, &stack[*sp + 16]);
	u32 var_name_len = VAL(u32, &stack[*sp + 8]);

	// Pop variable name
	POP();

	// Get variable (fast path for constant strings)
	ActionVar* var;
	if (string_id != 0)
	{
		// Constant string - use array (O(1))
		var = getVariableById(string_id);
	}
	else
	{
		// Dynamic string - use hashmap (O(n))
		var = getVariable(var_name, var_name_len);
	}

	if (!var)
	{
		// Variable not found - push empty string
		PUSH_STR("", 0);
		return;
	}

	// Push variable value to stack
	PUSH_VAR(var);
}

void actionSetVariable(char* stack, u32* sp)
{
	// Stack layout: [value] [name] <- sp
	// We need value at top, name at second

	u32 value_sp = *sp;
	u32 var_name_sp = SP_SECOND_TOP;

	// Read variable name info
	u32 string_id = VAL(u32, &stack[var_name_sp + 4]);
	char* var_name = (char*) VAL(u64, &stack[var_name_sp + 16]);
	u32 var_name_len = VAL(u32, &stack[var_name_sp + 8]);

	// Get variable (fast path for constant strings)
	ActionVar* var;
	if (string_id != 0)
	{
		// Constant string - use array (O(1))
		var = getVariableById(string_id);
	}
	else
	{
		// Dynamic string - use hashmap (O(n))
		var = getVariable(var_name, var_name_len);
	}

	if (!var)
	{
		// Failed to get/create variable
		POP_2();
		return;
	}

	// Set variable value (uses existing string materialization!)
	setVariableWithValue(var, stack, value_sp);

	// Pop both value and name
	POP_2();
}

void actionGetTime(char* stack, u32* sp)
{
	u32 delta_ms = get_elapsed_ms() - start_time;
	float delta_ms_f32 = (float) delta_ms;
	
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &delta_ms_f32));
}