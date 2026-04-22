#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <swap_vector.h>

#include <recomp.h>
#include <initial_strings_defs.h>
#include <heap.h>
#include <object.h>
#include <free_thread.h>
#include <utils.h>

u32 start_time;

// ==================================================================
// Scope Chain for WITH statement
// ==================================================================

#define MAX_SCOPE_DEPTH 16
static ASObject* scope_chain[MAX_SCOPE_DEPTH];
static ActionVar* scope_registers[MAX_SCOPE_DEPTH];
static u32 scope_top_obj = 1;

// ==================================================================
// Global object for ActionScript
// This is initialized from initActions and persists for the lifetime of the runtime
ASObject* _global;

void initActions(SWFAppContext* app_context)
{
	app_context->object_prototype = allocObjectCommon(app_context);
	app_context->object_constructor = allocObjectCommon(app_context);
	
	start_time = get_elapsed_ms();
	
	for (u32 i = 0; i < 2; ++i)
	{
		scope_chain[i] = allocObject(app_context);
		retainObject(scope_chain[i]);
		
		scope_registers[i] = HALLOC(4*sizeof(ActionVar));
		
		for (u32 j = 0; j < 4; ++j)
		{
			scope_registers[i][j].type = ACTION_STACK_VALUE_UNDEFINED;
		}
	}
	
	_global = scope_chain[0];
	
	for (int i = 0; i < sizeof(runtime_funcs)/sizeof(RuntimeFunc); ++i)
	{
		ASObject* obj;
		
		if (runtime_funcs[i].object_string_id == 0)
		{
			obj = _global;
		}
		
		else
		{
			ASProperty* p = getProperty(_global, runtime_funcs[i].object_string_id, NULL, 0);
			
			if (p != NULL)
			{
				obj = (ASObject*) p->value.value;
			}
			
			else
			{
				obj = allocObject(app_context);
				ActionVar obj_v;
				obj_v.type = ACTION_STACK_VALUE_OBJECT;
				obj_v.value = (u64) obj;
				setProperty(app_context, _global, runtime_funcs[i].object_string_id, NULL, 0, &obj_v);
			}
		}
		
		ActionVar v;
		v.type = ACTION_STACK_VALUE_OBJECT;
		
		if (runtime_funcs[i].func_string_id != STR_ID_OBJECT)
		{
			v.object = allocObject(app_context);
		}
		
		else
		{
			v.object = app_context->object_constructor;
		}
		
		Function_init_object(app_context, v.object);
		
		Function_set_members(app_context, v.object,
							 FUNC_TYPE_3,
							 (action_func) runtime_funcs[i].func,
							 NULL,
							 0,
							 0,
							 runtime_funcs[i].func_string_id);
		
		if (runtime_funcs[i].constructor)
		{
			ActionVar proto_var;
			proto_var.type = ACTION_STACK_VALUE_OBJECT;
			
			if (runtime_funcs[i].func_string_id != STR_ID_OBJECT)
			{
				proto_var.object = allocObject(app_context);
			}
			
			else
			{
				proto_var.object = app_context->object_prototype;
			}
			
			setProperty(app_context, v.object, STR_ID_PROTOTYPE, NULL, 0, &proto_var);
		}
		
		setProperty(app_context, obj, runtime_funcs[i].func_string_id, NULL, 0, &v);
	}
	
	for (int i = 0; i < sizeof(runtime_meths)/sizeof(RuntimeFunc); ++i)
	{
		ASObject* obj;
		
		ASProperty* p = getProperty(_global, runtime_meths[i].object_string_id, NULL, 0);
		
		if (p != NULL)
		{
			obj = (ASObject*) p->value.value;
		}
		
		else
		{
			UNIMPLEMENTED("Method on non-existent object");
		}
		
		ActionVar v;
		v.type = ACTION_STACK_VALUE_OBJECT;
		v.object = allocObject(app_context);
		
		Function_init_object(app_context, v.object);
		
		Function_set_members(app_context, v.object,
							 FUNC_TYPE_3,
							 (action_func) runtime_meths[i].func,
							 NULL,
							 0,
							 0,
							 runtime_meths[i].func_string_id);
		
		if (runtime_meths[i].constructor)
		{
			UNIMPLEMENTED("Constructor method");
		}
		
		ASObject* prototype = getProperty(obj, STR_ID_PROTOTYPE, NULL, 0)->value.object;
		setProperty(app_context, prototype, runtime_meths[i].func_string_id, NULL, 0, &v);
	}
	
	mutex_init(&object_queue_lock);
	rbtree_init(&object_free_queue, sizeof(objnode));
	
	for (int i = 0; i < sizeof(static_initializers)/sizeof(action_runtime_func); ++i)
	{
		scope_top_obj += 1;
		scope_chain[scope_top_obj] = allocObject(app_context);
		retainObject(scope_chain[scope_top_obj]);
		
		static_initializers[i](app_context, NULL, 0);
		POP();
		
		OBJ_LOCK_WRITE(scope_chain[scope_top_obj],
		{
			releaseObject(app_context, scope_chain[scope_top_obj]);
		});
		
		scope_top_obj -= 1;
	}
	
	free_thread_handle = thread_start(app_context, freeThread);
}

void freeActions(SWFAppContext* app_context)
{
	thread_join(free_thread_handle);
}

void discardArgs(SWFAppContext* app_context, u32 num_args)
{
	for (u32 i = 0; i < num_args; ++i)
	{
		POP();
	}
}

void searchScopesForPropertyVar(u32 string_id, const char* name, u32 name_len, ActionVar* out_var)
{
	ASProperty* p = NULL;
	
	for (u32 j = 0; j <= scope_top_obj; ++j)
	{
		u32 i = scope_top_obj - j;
		
		OBJ_LOCK_READ(scope_chain[i],
		{
			p = getProperty(scope_chain[i], string_id, name, name_len);
		});
		
		if (p != NULL)
		{
			*out_var = p->value;
			return;
		}
	}
	
	out_var->type = ACTION_STACK_VALUE_UNDEFINED;
}

ASProperty* getPropertyInThisScope(u32 string_id, const char* name, u32 name_len)
{
	ASProperty* p;
	
	OBJ_LOCK_READ(scope_chain[scope_top_obj],
	{
		p = getProperty(scope_chain[scope_top_obj], string_id, name, name_len);
	});
	
	return p;
}

void setPropertyInThisScope(SWFAppContext* app_context, u32 string_id, const char* name, u32 name_len, ActionVar* value)
{
	setProperty(app_context, scope_chain[scope_top_obj], string_id, name, name_len, value);
}

void pushVar(SWFAppContext* app_context, ActionVar* var)
{
	if (IS_OBJ_T(var->type))
	{
		ASObject* po = var->object;
		
		OBJ_LOCK_WRITE(po,
		{
			// the stack now has a reference to this object
			retainObject(po);
		});
	}
	
	switch (var->type)
	{
		case ACTION_STACK_VALUE_STRING:
		{
			// Use heap pointer if variable owns memory, otherwise use numeric_value as pointer
			char* str_ptr = var->owns_memory ?
				var->str :
				(char*) var->value;
				
			PUSH_STR_ID(str_ptr, var->string_id, var->str_size);
			
			break;
		}
		
		default:
		{
			PUSH(var->type, var->value);
			
			break;
		}
	}
}

void pushReg(SWFAppContext* app_context, u8 reg)
{
	pushVar(app_context, &scope_registers[scope_top_obj][reg]);
}

void peekConvert(SWFAppContext* app_context, ActionVar* var)
{
	var->type = STACK_TOP_TYPE;
	
	switch (var->type)
	{
		case ACTION_STACK_VALUE_STR_LIST:
		{
			var->value = (u64) &STACK_TOP_VALUE;
			var->str_size = STACK_TOP_N;
			break;
		}
		
		case ACTION_STACK_VALUE_STRING:
		{
			var->string_id = STACK_TOP_ID;
			var->str_size = STACK_TOP_N;
			
			if (STACK_TOP_OWNS_MEM != 0)
			{
				var->str = HALLOC(STACK_TOP_N + 1);
				var->owns_memory = true;
				memcpy(var->str, &STACK_TOP_VALUE, STACK_TOP_N + 1);
			}
			
			else
			{
				var->value = STACK_TOP_VALUE;
				var->owns_memory = false;
			}
			
			break;
		}
		
		default:
		{
			var->value = STACK_TOP_VALUE;
			break;
		}
	}
}

void copyReg(SWFAppContext* app_context)
{
	if (STACK_TOP_TYPE != ACTION_STACK_VALUE_REGISTER)
	{
		return;
	}
	
	u8 reg = (u8) STACK_TOP_VALUE;
	
	POP();
	pushReg(app_context, reg);
}

void copy2Regs(SWFAppContext* app_context)
{
	ActionVar reg1_v;
	ActionVar reg2_v;
	
	peekConvert(app_context, &reg1_v);
	POP();
	
	peekConvert(app_context, &reg2_v);
	POP();
	
	if (reg2_v.type == ACTION_STACK_VALUE_REGISTER)
	{
		pushReg(app_context, (u8) reg2_v.u32);
	}
	
	else
	{
		pushVar(app_context, &reg2_v);
	}
	
	if (reg1_v.type == ACTION_STACK_VALUE_REGISTER)
	{
		pushReg(app_context, (u8) reg1_v.u32);
	}
	
	else
	{
		pushVar(app_context, &reg1_v);
	}
}

void peekVar(SWFAppContext* app_context, ActionVar* var)
{
	copyReg(app_context);
	peekConvert(app_context, var);
}

void popVar(SWFAppContext* app_context, ActionVar* var)
{
	peekVar(app_context, var);
	
	POP();
}

void peekSecondVar(SWFAppContext* app_context, ActionVar* var)
{
	var->type = STACK_SECOND_TOP_TYPE;
	var->str_size = STACK_SECOND_TOP_N;
	
	if (STACK_SECOND_TOP_TYPE == ACTION_STACK_VALUE_STR_LIST)
	{
		var->value = (u64) &STACK_SECOND_TOP_VALUE;
	}
	
	else if (STACK_SECOND_TOP_TYPE == ACTION_STACK_VALUE_STRING)
	{
		var->value = STACK_SECOND_TOP_VALUE;
		var->owns_memory = false;
		var->string_id = STACK_SECOND_TOP_ID;
	}
	
	else
	{
		var->value = STACK_SECOND_TOP_VALUE;
	}
}

void convertNumericToNumber(SWFAppContext* app_context, ActionVar* v)
{
	f64 d;
	
	switch (v->type)
	{
		case ACTION_STACK_VALUE_F32:
			f32 f = v->f32;
			v->type = ACTION_STACK_VALUE_F64;
			d = (f64) f;
			v->f64 = d;
			break;
		case ACTION_STACK_VALUE_INT:
			s32 i = v->s32;
			v->type = ACTION_STACK_VALUE_F64;
			d = (f64) i;
			v->f64 = d;
			break;
	}
}

void convertNumericToInteger(SWFAppContext* app_context, ActionVar* v)
{
	s32 i;
	
	switch (v->type)
	{
		case ACTION_STACK_VALUE_F32:
			f32 f = v->f32;
			v->type = ACTION_STACK_VALUE_INT;
			i = (s32) f;
			v->s32 = i;
			break;
		case ACTION_STACK_VALUE_F64:
			f64 d = v->f64;
			v->type = ACTION_STACK_VALUE_INT;
			i = (s32) d;
			v->s32 = i;
			break;
	}
}

f64 toNumberCommon(SWFAppContext* app_context, ActionVar* v)
{
	switch (v->type)
	{
		case ACTION_STACK_VALUE_UNDEFINED:
			return NAN;
		
		case ACTION_STACK_VALUE_NULL:
			return +0.0;
		
		case ACTION_STACK_VALUE_BOOLEAN:
			return v->b ? 1.0 : +0.0;
		
		case ACTION_STACK_VALUE_STRING:
		case ACTION_STACK_VALUE_STR_LIST:
			// TODO: implement real ToNumber ECMA-262 3rd Edition algorithm
			char* end;
			return strtod(v->str, &end);
		
		case ACTION_STACK_VALUE_F32:
		case ACTION_STACK_VALUE_INT:
			convertNumericToNumber(app_context, v);
			// fallthrough
		case ACTION_STACK_VALUE_F64:
			return v->f64;
		
		default:
			UNREACHABLE("ToNumber");
	}
	
	return NAN;
}

void toNumber(SWFAppContext* app_context, ActionVar* v)
{
	if (IS_OBJ_P(v))
	{
		UNIMPLEMENTED("ToNumber on an Object");
	}
	
	f64 num = toNumberCommon(app_context, v);
	
	PUSH_F64(num);
}

void toInteger(SWFAppContext* app_context, ActionVar* v)
{
	if (IS_OBJ_P(v))
	{
		UNIMPLEMENTED("ToInteger on an Object");
	}
	
	f64 num = toNumberCommon(app_context, v);
	
	if (num == NAN)
	{
		num = +0.0;
		PUSH_F64(num);
		return;
	}
	
	if (num == +0.0 || num == -0.0 ||
		num == INFINITY || num == -INFINITY)
	{
		PUSH_F64(num);
		return;
	}
	
	num = (num < 0) ? ceil(num) : floor(num);
	PUSH_F64(num);
}

void toPrimitive(SWFAppContext* app_context, ASObject* this)
{
	getAndCallMethod(app_context, this, STR_ID_VALUE_OF, 0);
	
	if (IS_OBJ_T(STACK_TOP_TYPE))
	{
		POP();
		getAndCallMethod(app_context, this, STR_ID_TO_STRING, 0);
	}
}

void toString(SWFAppContext* app_context, ActionVar* v)
{
	char str[64];
	
	// TODO: implement real ToString ECMA-262 3rd Edition algorithm
	snprintf(str, 64, "%.15g", v->f64);
	u32 len = (u32) strnlen(str, 64);
	
	PUSH_STR_STACK(len);
	char* stack_str = (char*) &STACK_TOP_VALUE;
	
	memcpy(stack_str, str, len);
}

ActionStackValueType convertString(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	ActionVar v;
	
	char str[64];
	
	switch (STACK_TOP_TYPE)
	{
		case ACTION_STACK_VALUE_NULL:
		{
			popVar(app_context, &v);
			
			snprintf(str, 64, "null");
			
			u32 len = 4;
			
			PUSH_STR_STACK(len);
			
			STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
			memcpy((char*) &STACK_TOP_VALUE, str, len + 1);
			STACK_TOP_OWNS_MEM = true;
			STACK_TOP_N = len;
			STACK_TOP_ID = 0;
			
			break;
		}
		
		case ACTION_STACK_VALUE_UNDEFINED:
		{
			popVar(app_context, &v);
			
			snprintf(str, 64, "undefined");
			
			u32 len = 9;
			
			PUSH_STR_STACK(len);
			
			STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
			memcpy((char*) &STACK_TOP_VALUE, str, len + 1);
			STACK_TOP_OWNS_MEM = true;
			STACK_TOP_N = len;
			STACK_TOP_ID = 0;
			
			break;
		}
		
		case ACTION_STACK_VALUE_BOOLEAN:
		{
			popVar(app_context, &v);
			
			snprintf(str, 64, (v.b) ? "true" : "false");
			
			u32 len = (v.b) ? 4 : 5;
			
			PUSH_STR_STACK(len);
			
			STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
			memcpy((char*) &STACK_TOP_VALUE, str, len + 1);
			STACK_TOP_OWNS_MEM = true;
			STACK_TOP_N = len;
			STACK_TOP_ID = 0;
			
			break;
		}
		
		case ACTION_STACK_VALUE_F32:
		{
			popVar(app_context, &v);
			f32 temp_val = v.f32;
			
			snprintf(str, 64, "%.15g", temp_val);
			
			u32 len = (u32) strnlen(str, 64);
			
			PUSH_STR_STACK(len);
			
			STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
			memcpy((char*) &STACK_TOP_VALUE, str, len + 1);
			STACK_TOP_OWNS_MEM = true;
			STACK_TOP_N = len;
			STACK_TOP_ID = 0;
			
			break;
		}
		
		case ACTION_STACK_VALUE_F64:
		{
			popVar(app_context, &v);
			f64 temp_val = v.f64;
			
			snprintf(str, 64, "%.15g", temp_val);
			
			u32 len = (u32) strnlen(str, 64);
			
			PUSH_STR_STACK(len);
			
			STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
			memcpy((char*) &STACK_TOP_VALUE, str, len + 1);
			STACK_TOP_OWNS_MEM = true;
			STACK_TOP_N = len;
			STACK_TOP_ID = 0;
			
			break;
		}
		
		case ACTION_STACK_VALUE_INT:
		{
			popVar(app_context, &v);
			s32 temp_val = v.s32;
			
			snprintf(str, 64, "%d", temp_val);
			
			u32 len = (u32) strnlen(str, 64);
			
			PUSH_STR_STACK(len);
			
			STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
			memcpy((char*) &STACK_TOP_VALUE, str, len + 1);
			STACK_TOP_OWNS_MEM = true;
			STACK_TOP_N = len;
			STACK_TOP_ID = 0;
			
			break;
		}
	}
	
	return ACTION_STACK_VALUE_STRING;
}

ActionStackValueType convertFloat(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING)
	{
		f64 temp = atof((char*) VAL(u64, &STACK_TOP_VALUE));
		STACK_TOP_TYPE = ACTION_STACK_VALUE_F64;
		VAL(u64, &STACK_TOP_VALUE) = VAL(u64, &temp);
		
		return ACTION_STACK_VALUE_F64;
	}
	
	return ACTION_STACK_VALUE_F32;
}

ActionStackValueType convertDouble(SWFAppContext* app_context)
{
	// TODO: refactor to just use ActionVars on the stack
	
	copyReg(app_context);
	
	ActionVar v;
	popVar(app_context, &v);
	
	toNumber(app_context, &v);
	
	return ACTION_STACK_VALUE_F64;
}

ActionStackValueType convertInt(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	s32 i;
	
	switch (STACK_TOP_TYPE)
	{
		case ACTION_STACK_VALUE_STRING:
			i = atoi((char*) VAL(u64, &STACK_TOP_VALUE));
			VAL(u32, &STACK_TOP_VALUE) = i;
			break;
		case ACTION_STACK_VALUE_F32:
			f32 f = VAL(f32, &STACK_TOP_VALUE);
			i = (s32) f;
			VAL(u32, &STACK_TOP_VALUE) = i;
			break;
		case ACTION_STACK_VALUE_F64:
			f64 d = VAL(f64, &STACK_TOP_VALUE);
			i = (s32) d;
			VAL(u32, &STACK_TOP_VALUE) = i;
			break;
	}
	
	STACK_TOP_TYPE = ACTION_STACK_VALUE_INT;
	
	return ACTION_STACK_VALUE_INT;
}

ActionStackValueType convertIntECMA(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	ActionVar v;
	popVar(app_context, &v);
	
	toInteger(app_context, &v);
	
	return ACTION_STACK_VALUE_F64;
}

// fully ECMA 262-3 compliant
ActionStackValueType convertBool(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	// TODO: make the macros for checking objects better
	if ((STACK_TOP_TYPE & 0x10) != 0x00)
	{
		VAL(bool, &STACK_TOP_VALUE) = true;
		STACK_TOP_TYPE = ACTION_STACK_VALUE_BOOLEAN;
		return ACTION_STACK_VALUE_BOOLEAN;
	}
	
	bool b;
	
	switch (STACK_TOP_TYPE)
	{
		case ACTION_STACK_VALUE_STRING:
			b = STACK_TOP_N != 0;
			VAL(bool, &STACK_TOP_VALUE) = b;
			break;
		case ACTION_STACK_VALUE_F32:
			f32 f = VAL(f32, &STACK_TOP_VALUE);
			b = f != +0.0f && f != -0.0f && f != NAN;
			VAL(bool, &STACK_TOP_VALUE) = b;
			break;
		case ACTION_STACK_VALUE_F64:
			f64 d = VAL(f64, &STACK_TOP_VALUE);
			b = d != +0.0 && d != -0.0 && d != NAN;
			VAL(bool, &STACK_TOP_VALUE) = b;
			break;
		case ACTION_STACK_VALUE_INT:
			int i = VAL(s32, &STACK_TOP_VALUE);
			b = i != 0;
			VAL(bool, &STACK_TOP_VALUE) = b;
			break;
	}
	
	STACK_TOP_TYPE = ACTION_STACK_VALUE_BOOLEAN;
	
	return ACTION_STACK_VALUE_BOOLEAN;
}

void actionAdd(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	double c = b.f64 + a.f64;
	PUSH_F64(c);
}

void actionAdd2(SWFAppContext* app_context)
{
	copy2Regs(app_context);
	
	if (IS_OBJ_T(STACK_TOP_TYPE) || IS_OBJ_T(STACK_SECOND_TOP_TYPE))
	{
		ActionVar a;
		popVar(app_context, &a);
		
		ActionVar b;
		popVar(app_context, &b);
		
		if (IS_OBJ_T(b.type))
		{
			ASObject* this = b.object;
			
			toPrimitive(app_context, this);
		}
		
		else
		{
			pushVar(app_context, &b);
		}
		
		if (IS_OBJ_T(a.type))
		{
			ASObject* this = a.object;
			
			toPrimitive(app_context, this);
		}
		
		else
		{
			pushVar(app_context, &a);
		}
	}
	
	if (IS_STR_T(STACK_TOP_TYPE) || IS_STR_T(STACK_SECOND_TOP_TYPE))
	{
		convertString(app_context);
		ActionVar a_str;
		popVar(app_context, &a_str);
		
		convertString(app_context);
		ActionVar b_str;
		popVar(app_context, &b_str);
		
		u32 a_n = a_str.str_size;
		u32 b_n = b_str.str_size;
		
		PUSH_STR_STACK(b_n + a_n);
		
		memcpy((char*) &STACK_TOP_VALUE, b_str.str, b_n);
		memcpy(((char*) &STACK_TOP_VALUE) + b_n, a_str.str, a_n);
		*((u8*) (((char*) &STACK_TOP_VALUE) + b_n + a_n)) = '\0';
		
		return;
	}
	
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	// wait it's just 754 do i need to do any of this LOL
	
	if ((b.f64 == NAN || a.f64 == NAN) ||
		b.f64 == INFINITY && a.f64 == -INFINITY ||
		b.f64 == -INFINITY && a.f64 == INFINITY)
	{
		f64 nan = NAN;
		PUSH_F64(&nan);
		return;
	}
	
	if (b.f64 == INFINITY || a.f64 == INFINITY)
	{
		f64 inf = INFINITY;
		PUSH_F64(inf);
		return;
	}
	
	if (b.f64 == -INFINITY || a.f64 == -INFINITY)
	{
		f64 ninf = -INFINITY;
		PUSH_F64(ninf);
		return;
	}
	
	if (b.f64 == -0.0 && a.f64 == -0.0)
	{
		f64 n0 = -0.0;
		PUSH_F64(n0);
		return;
	}
	
	if ((b.f64 == +0.0 || b.f64 == -0.0) &&
		(a.f64 == +0.0 || a.f64 == -0.0))
	{
		f64 p0 = +0.0;
		PUSH_F64(p0);
		return;
	}
	
	// eh too late now
	
	double c = b.f64 + a.f64;
	PUSH_F64(c);
}

void actionSubtract(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	double c = b.f64 - a.f64;
	PUSH_F64(c);
}

void actionMultiply(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	double c = b.f64*a.f64;
	PUSH_F64(c);
}

void actionDivide(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	f64 c;
	
	if (a.f64 == 0.0)
	{
		// TODO: handle versioning from the recompiler
		switch (app_context->version)
		{
			case 4:
			{
				PUSH_STR("#ERROR#", 8);
				break;
			}
			
			case 5:
			{
				if (a.f64 == 0.0)
				{
					c = NAN;
					PUSH_F64(c);
				}
				
				else if (a.f64 > 0.0)
				{
					c = INFINITY;
					PUSH_F64(c);
				}
				
				else
				{
					c = -INFINITY;
					PUSH_F64(c);
				}
			}
		}
		
		return;
	}
	
	c = b.f64/a.f64;
	PUSH_F64(c);
}

void actionModulo(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	if (UNLIKELY(a.f64 == 0.0))
	{
		f64 nan = NAN;
		PUSH_F64(nan);
		return;
	}
	
	f64 mod = fmod(b.f64, a.f64);
	PUSH_F64(mod);
}

void actionIncrement(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar v;
	popVar(app_context, &v);
	
	f64 inc = v.f64 + 1.0;
	PUSH_F64(inc);
}

void actionDecrement(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar v;
	popVar(app_context, &v);
	
	f64 dec = v.f64 - 1.0;
	PUSH_F64(dec);
}

// ==================================================================
// Bitwise Operations
// ==================================================================

void actionBitAnd(SWFAppContext* app_context)
{
	convertInt(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertInt(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	s32 and = b.u32 & a.u32;
	PUSH_INT(and);
}

void actionBitOr(SWFAppContext* app_context)
{
	convertInt(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertInt(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	s32 or = b.u32 | a.u32;
	PUSH_INT(or);
}

void actionBitLShift(SWFAppContext* app_context)
{
	convertInt(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertInt(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	s32 lsh = b.u32 << (a.u32 & 0b11111);
	PUSH_INT(lsh);
}

void actionBitRShift(SWFAppContext* app_context)
{
	convertInt(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertInt(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	s32 rsh = b.s32 >> (a.u32 & 0b11111);
	PUSH_INT(rsh);
}

void actionBitURShift(SWFAppContext* app_context)
{
	convertInt(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertInt(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	u32 rsh = b.u32 >> (a.u32 & 0b11111);
	PUSH_INT(rsh);
}

void actionBitXor(SWFAppContext* app_context)
{
	convertInt(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertInt(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	u32 xor = b.u32 ^ a.u32;
	PUSH_INT(xor);
}

// ==================================================================
// Comparison Operations
// ==================================================================

void actionEquals(SWFAppContext* app_context)
{
	convertDouble(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertDouble(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	bool equals = b.f64 == a.f64;
	PUSH_BOOL(equals);
}

void actionEquals2(SWFAppContext* app_context)
{
	ActionVar a;
	popVar(app_context, &a);
	
	ActionVar b;
	popVar(app_context, &b);
	
	if (b.type != a.type)
	{
		UNIMPLEMENTED("Equals2 of differing types");
	}
	
	if (b.type == ACTION_STACK_VALUE_UNDEFINED ||
		b.type == ACTION_STACK_VALUE_NULL)
	{
		PUSH_BOOL(true);
		return;
	}
	
	if (!IS_NUM_T(b.type))
	{
		if (IS_STR_T(b.type))
		{
			if (b.str_size != a.str_size)
			{
				PUSH_BOOL(false);
				return;
			}
			
			PUSH_BOOL(strncmp(b.str, a.str, b.str_size) == 0);
			return;
		}
		
		if (b.type == ACTION_STACK_VALUE_BOOLEAN)
		{
			PUSH_BOOL(b.b && a.b || !b.b && !a.b);
			return;
		}
		
		// why does this need double parens, sadge
		PUSH_BOOL((b.object == a.object));
		return;
	}
	
	if (IS_OBJ_T(b.type))
	{
		toPrimitive(app_context, b.object);
	}
	
	if (IS_OBJ_T(a.type))
	{
		toPrimitive(app_context, a.object);
	}
	
	convertNumericToNumber(app_context, &b);
	convertNumericToNumber(app_context, &a);
	
	if (b.f64 == NAN || a.f64 == NAN)
	{
		PUSH_BOOL(false);
		return;
	}
	
	if (b.f64 == a.f64 ||
		b.f64 == +0.0 && a.f64 == -0.0 ||
		b.f64 == -0.0 && a.f64 == +0.0)
	{
		PUSH_BOOL(true);
		return;
	}
	
	PUSH_BOOL(false);
}

void actionLess(SWFAppContext* app_context)
{
	ActionVar a;
	convertDouble(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertDouble(app_context);
	popVar(app_context, &b);
	
	bool less = b.f64 < a.f64;
	PUSH_BOOL(less);
}

void actionLess2(SWFAppContext* app_context)
{
	copy2Regs(app_context);
	
	if (IS_OBJ_T(STACK_TOP_TYPE) || IS_OBJ_T(STACK_SECOND_TOP_TYPE))
	{
		ActionVar a;
		popVar(app_context, &a);
		
		ActionVar b;
		popVar(app_context, &b);
		
		if (IS_OBJ_T(b.type))
		{
			ASObject* this = b.object;
			
			toPrimitive(app_context, this);
		}
		
		if (IS_OBJ_T(a.type))
		{
			ASObject* this = a.object;
			
			toPrimitive(app_context, this);
		}
		
		pushVar(app_context, &b);
		
		pushVar(app_context, &a);
	}
	
	if (IS_STR_T(STACK_TOP_TYPE) && IS_STR_T(STACK_SECOND_TOP_TYPE))
	{
		UNIMPLEMENTED("Less2 String comparison");
	}
	
	ActionVar a;
	convertDouble(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertDouble(app_context);
	popVar(app_context, &b);
	
	if (b.f64 == NAN || a.f64 == NAN)
	{
		PUSH_UNDEFINED();
		return;
	}
	
	if (b.f64 == a.f64 ||
		b.f64 == +0.0 && a.f64 == -0.0 ||
		b.f64 == -0.0 && a.f64 == +0.0)
	{
		PUSH_BOOL(false);
		return;
	}
	
	if (b.f64 == INFINITY)
	{
		PUSH_BOOL(false);
		return;
	}
	
	if (a.f64 == INFINITY)
	{
		PUSH_BOOL(true);
		return;
	}
	
	if (a.f64 == -INFINITY)
	{
		PUSH_BOOL(false);
		return;
	}
	
	if (b.f64 == -INFINITY)
	{
		PUSH_BOOL(true);
		return;
	}
	
	PUSH_BOOL(b.f64 < a.f64);
}

void actionAnd(SWFAppContext* app_context)
{
	ActionVar a;
	convertBool(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertBool(app_context);
	popVar(app_context, &b);
	
	bool and = b.b && a.b;
	PUSH_BOOL(and);
}

void actionOr(SWFAppContext* app_context)
{
	ActionVar a;
	convertBool(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertBool(app_context);
	popVar(app_context, &b);
	
	bool or = b.b || a.b;
	PUSH_BOOL(or);
}

void actionNot(SWFAppContext* app_context)
{
	ActionVar v;
	convertBool(app_context);
	popVar(app_context, &v);
	
	bool b = !v.b;
	PUSH_BOOL(b);
}

// ==================================================================
// String Operations
// ==================================================================

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

void actionStringEquals(SWFAppContext* app_context, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertString(app_context);
	popVar(app_context, &b);
	
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
	PUSH_F32(result);
}

void actionStringLength(SWFAppContext* app_context, char* v_str)
{
	ActionVar v;
	convertString(app_context);
	popVar(app_context, &v);
	
	PUSH_INT(v.str_size);
}

void actionStringAdd(SWFAppContext* app_context, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(app_context);
	peekVar(app_context, &a);
	
	ActionVar b;
	convertString(app_context);
	peekSecondVar(app_context, &b);
	
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
	
	PUSH_STR_LIST(b.str_size + a.str_size, (u32) sizeof(u64)*(2*num_strings + 1));
	
	u64* str_list = (u64*) &STACK_TOP_VALUE;
	str_list[0] = num_strings;
	
	if (b.type == ACTION_STACK_VALUE_STR_LIST)
	{
		u64* b_list = (u64*) b.value;
		
		for (u64 i = 0; i < num_b_strings; ++i)
		{
			u64 str_i = 2*i;
			str_list[str_i + 1] = b_list[str_i + 1];
			str_list[str_i + 2] = b_list[str_i + 2];
		}
	}
	
	else
	{
		str_list[1] = b.value;
		str_list[2] = b.str_size;
	}
	
	if (a.type == ACTION_STACK_VALUE_STR_LIST)
	{
		u64* a_list = (u64*) a.value;
		
		for (u64 i = 0; i < num_a_strings; ++i)
		{
			u64 str_i = 2*i;
			str_list[str_i + 1 + 2*num_b_strings] = a_list[str_i + 1];
			str_list[str_i + 2 + 2*num_b_strings] = a_list[str_i + 2];
		}
	}
	
	else
	{
		str_list[1 + 2*num_b_strings] = a.value;
		str_list[1 + 2*num_b_strings + 1] = a.str_size;
	}
}

// ==================================================================
// Variable Operations
// ==================================================================

void actionGetVariable(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	// Read variable name info from stack
	u32 string_id = STACK_TOP_ID;
	char* var_name = (char*) STACK_TOP_VALUE;
	u32 var_name_len = STACK_TOP_N;
	
	// Pop variable name
	POP();
	
	// Get variable (fast path for constant strings)
	ASProperty* p = NULL;
	
	switch (string_id)
	{
		case 0:
		{
			// TODO: Dynamic string - use hashmap (O(n))
			//~ var = getVariable(app_context, var_name, var_name_len);
			EXC_ARG("Unimplemented: string %s has no id\n", var_name);
			break;
		}
		
		case STR_ID_GLOBAL:
		{
			PUSH_OBJ(_global);
			
			return;
		}
		
		default:
		{
			// Constant string - use scope object (O(lg(n)))
			for (u32 i = scope_top_obj; i < MAX_SCOPE_DEPTH; --i)
			{
				OBJ_LOCK_READ(scope_chain[i],
				{
					p = getProperty(scope_chain[i], string_id, var_name, var_name_len);
				});
				
				if (p != NULL)
				{
					break;
				}
			}
			
			break;
		}
	}
	
	if (p != NULL)
	{
		// Push variable value to stack
		PUSH_VAR(&p->value);
		
		if (IS_OBJ(p->value))
		{
			ASObject* po = (ASObject*) p->value.value;
			
			OBJ_LOCK_WRITE(po,
			{
				// the stack now has a reference to this object
				retainObject(po);
			});
		}
	}
	
	else
	{
		PUSH_UNDEFINED();
	}
}

void actionSetVariable(SWFAppContext* app_context)
{
	// Stack layout: [value] [name] <- sp
	// We need value at top, name at second
	
	ActionVar value;
	peekVar(app_context, &value);
	
	if (IS_OBJ(value))
	{
		ASObject* o = (ASObject*) value.value;
		
		OBJ_LOCK_WRITE(o,
		{
			// we now have a reference to this object
			retainObject((ASObject*) value.value);
		});
	}
	
	POP();
	
	copyReg(app_context);
	
	// Read variable name info
	u32 string_id = STACK_TOP_ID;
	char* var_name = (char*) STACK_TOP_VALUE;
	u32 var_name_len = STACK_TOP_N;
	
	POP();
	
	ASProperty* p = NULL;
	
	ASObject* scope_obj;
	
	switch (string_id)
	{
		case 0:
		{
			// TODO: Dynamic string - use hashmap (O(n))
			//~ var = getVariable(app_context, var_name, var_name_len);
			EXC_ARG("Unimplemented: string %s has no id\n", var_name);
			break;
		}
		
		case STR_ID_GLOBAL:
		{
			setProperty(app_context, _global, string_id, var_name, var_name_len, &value);
			
			// sue me
			goto release_value;
		}
		
		default:
		{
			// Constant string - use scope object (O(lg(n)))
			for (u32 i = scope_top_obj; i < MAX_SCOPE_DEPTH; --i)
			{
				scope_obj = scope_chain[i];
				
				OBJ_LOCK_READ(scope_obj,
				{
					p = getProperty(scope_obj, string_id, var_name, var_name_len);
				});
				
				if (p != NULL)
				{
					break;
				}
			}
			
			break;
		}
	}
	
	if (p != NULL)
	{
		bool should_release = false;
		ASObject* old_obj;
		
		if (IS_OBJ(p->value))
		{
			should_release = true;
			old_obj = (ASObject*) p->value.value;
		}
		
		OBJ_LOCK_WRITE(scope_obj,
		{
			p->value = value;
		});
		
		if (should_release)
		{
			OBJ_LOCK_WRITE(old_obj,
			{
				releaseObject(app_context, old_obj);
			});
		}
	}
	
	else
	{
		setProperty(app_context, scope_chain[1], string_id, var_name, var_name_len, &value);
	}
	
	release_value:
	
	if (IS_OBJ(value))
	{
		ASObject* o = (ASObject*) value.value;
		
		OBJ_LOCK_WRITE(o,
		{
			// we no longer have a reference to this object
			releaseObject(app_context, o);
		});
	}
}

void actionToNumber(SWFAppContext* app_context)
{
	convertDouble(app_context);
}

void actionToString(SWFAppContext* app_context)
{
	convertString(app_context);
}

// ==================================================================
// Utility Operations
// ==================================================================

void actionTrace(SWFAppContext* app_context)
{
	ActionVar v;
	popVar(app_context, &v);
	
	if (IS_OBJ_T(v.type))
	{
		getAndCallMethod(app_context, v.object, STR_ID_TO_STRING, 0);
		popVar(app_context, &v);
		
		printf("%s\n", v.str);
		
		return;
	}
	
	switch (v.type)
	{
		case ACTION_STACK_VALUE_STRING:
		{
			printf("%s\n", v.str);
			
			if (v.owns_memory)
			{
				FREE(v.str);
			}
			
			break;
		}
		
		case ACTION_STACK_VALUE_NULL:
		{
			printf("null\n");
			break;
		}
		
		case ACTION_STACK_VALUE_UNDEFINED:
		{
			printf("undefined\n");
			break;
		}
		
		case ACTION_STACK_VALUE_STR_LIST:
		{
			u64* str_list = (u64*) &v.value;
			
			for (u64 i = 0; i < 2*str_list[0]; i += 2)
			{
				printf("%s", (char*) str_list[i + 1]);
			}
			
			printf("\n");
			
			break;
		}
		
		case ACTION_STACK_VALUE_F32:
		{
			printf("%.15g\n", v.f32);
			break;
		}
		
		case ACTION_STACK_VALUE_F64:
		{
			printf("%.15g\n", v.f64);
			break;
		}
		
		case ACTION_STACK_VALUE_INT:
		{
			printf("%d\n", v.s32);
			break;
		}
		
		default:
		{
			fprintf(stderr, "Bad print type: %d\n", v.type);
			break;
		}
	}
	
	fflush(stdout);
}

void actionGetTime(SWFAppContext* app_context)
{
	u32 delta_ms = get_elapsed_ms() - start_time;
	float delta_ms_f32 = (float) delta_ms;
	
	PUSH_F32(delta_ms_f32);
}

// ==================================================================
// EnumeratedName helper structures for property enumeration
// ==================================================================

typedef struct EnumeratedName EnumeratedName;

struct EnumeratedName {
	const char* name;
	u32 name_length;
	EnumeratedName* next;
};

/**
 * Check if a property name has already been enumerated
 */
static int isPropertyEnumerated(EnumeratedName* head, const char* name, u32 name_length)
{
	EnumeratedName* current = head;
	while (current != NULL)
	{
		if (current->name_length == name_length &&
		    strncmp(current->name, name, name_length) == 0)
		{
			return 1; // Found - property was already enumerated
		}
		current = current->next;
	}
	return 0; // Not found
}

/**
 * Add a property name to the enumerated list
 */
static void addEnumeratedName(EnumeratedName** head, const char* name, u32 name_length)
{
	EnumeratedName* node = (EnumeratedName*) malloc(sizeof(EnumeratedName));
	if (node == NULL)
	{
		return; // Out of memory, skip this property
	}
	node->name = name;
	node->name_length = name_length;
	node->next = *head;
	*head = node;
}

/**
 * Free the enumerated names list
 */
static void freeEnumeratedNames(EnumeratedName* head)
{
	while (head != NULL)
	{
		EnumeratedName* next = head->next;
		free(head);
		head = next;
	}
}

void actionEnumerate(SWFAppContext* app_context, char* str_buffer)
{
	//~ // Step 1: Pop variable name from stack
	//~ // Stack layout for strings: +0=type, +4=oldSP, +8=length, +12=string_id, +16=pointer
	//~ u32 string_id = VAL(u32, &STACK[SP + 12]);
	//~ char* var_name = (char*) VAL(u64, &STACK[SP + 16]);
	//~ u32 var_name_len = VAL(u32, &STACK[SP + 8]);
	//~ POP();
	
//~ #ifdef DEBUG
	//~ printf("[DEBUG] actionEnumerate: looking up variable '%.*s' (len=%u, id=%u)\n",
	       //~ var_name_len, var_name, var_name_len, string_id);
//~ #endif

	//~ // Step 2: Look up the variable
	//~ ActionVar* var = NULL;
	//~ if (string_id > 0)
	//~ {
		//~ // Constant string - use array lookup (O(1))
		//~ var = getVariableById(app_context, string_id);
	//~ }
	//~ else
	//~ {
		//~ // Dynamic string - use hashmap (O(n))
		//~ var = getVariable(app_context, var_name, var_name_len);
	//~ }
	
	//~ // Step 3: Check if variable exists and is an object
	//~ if (!var || var->type != ACTION_STACK_VALUE_OBJECT)
	//~ {
//~ #ifdef DEBUG
		//~ if (!var)
			//~ printf("[DEBUG] actionEnumerate: variable not found\n");
		//~ else
			//~ printf("[DEBUG] actionEnumerate: variable is not an object (type=%d)\n", var->type);
//~ #endif
		//~ // Variable not found or not an object - push null terminator only
		//~ PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
		//~ return;
	//~ }
	
	//~ // Step 4: Get the object from the variable
	//~ ASObject* obj = (ASObject*) VAL(u64, &var->value);
	//~ if (obj == NULL)
	//~ {
//~ #ifdef DEBUG
		//~ printf("[DEBUG] actionEnumerate: object pointer is NULL\n");
//~ #endif
		//~ // Null object - push null terminator only
		//~ PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
		//~ return;
	//~ }
	
	//~ // Step 5: Collect all enumerable properties from the entire prototype chain
	//~ // We need to collect them first to push in reverse order
	
	//~ // Temporary storage for property names (we'll push them to stack after collecting)
	//~ typedef struct PropList {
		//~ const char* name;
		//~ u32 name_length;
		//~ struct PropList* next;
	//~ } PropList;
	
	//~ PropList* prop_head = NULL;
	//~ u32 total_props = 0;
	
	//~ // Track which properties we've already seen (to handle shadowing)
	//~ EnumeratedName* enumerated_head = NULL;
	
	//~ // Walk the prototype chain
	//~ ASObject* current_obj = obj;
	//~ int chain_depth = 0;
	//~ const int MAX_CHAIN_DEPTH = 100; // Prevent infinite loops
	
	//~ while (current_obj != NULL && chain_depth < MAX_CHAIN_DEPTH)
	//~ {
		//~ chain_depth++;
		
//~ #ifdef DEBUG
		//~ printf("[DEBUG] actionEnumerate: walking prototype chain depth=%d, num_used=%u\n",
		       //~ chain_depth, current_obj->num_used);
//~ #endif

		//~ // Enumerate properties from this level
		//~ for (u32 i = 0; i < current_obj->num_used; i++)
		//~ {
			//~ const char* prop_name = current_obj->properties[i].name;
			//~ u32 prop_name_len = current_obj->properties[i].name_length;
			//~ u8 prop_flags = current_obj->properties[i].flags;
			
			//~ // Skip if property is not enumerable (DontEnum)
			//~ if (!(prop_flags & PROPERTY_FLAG_ENUMERABLE))
			//~ {
//~ #ifdef DEBUG
				//~ printf("[DEBUG] actionEnumerate: skipping non-enumerable property '%.*s'\n",
				       //~ prop_name_len, prop_name);
//~ #endif
				//~ continue;
			//~ }
			
			//~ // Skip if we've already enumerated this property name (shadowing)
			//~ if (isPropertyEnumerated(enumerated_head, prop_name, prop_name_len))
			//~ {
//~ #ifdef DEBUG
				//~ printf("[DEBUG] actionEnumerate: skipping shadowed property '%.*s'\n",
				       //~ prop_name_len, prop_name);
//~ #endif
				//~ continue;
			//~ }
			
			//~ // Add to enumerated list
			//~ addEnumeratedName(&enumerated_head, prop_name, prop_name_len);
			
			//~ // Add to property list (for later pushing to stack)
			//~ PropList* node = (PropList*) malloc(sizeof(PropList));
			//~ if (node != NULL)
			//~ {
				//~ node->name = prop_name;
				//~ node->name_length = prop_name_len;
				//~ node->next = prop_head;
				//~ prop_head = node;
				//~ total_props++;
				
//~ #ifdef DEBUG
				//~ printf("[DEBUG] actionEnumerate: added enumerable property '%.*s'\n",
				       //~ prop_name_len, prop_name);
//~ #endif
			//~ }
		//~ }
		
		//~ // Move to prototype via __proto__ property
		//~ ActionVar* proto_var = getProperty(current_obj, 0, "__proto__", 9);
		//~ if (proto_var != NULL && proto_var->type == ACTION_STACK_VALUE_OBJECT)
		//~ {
			//~ current_obj = (ASObject*) proto_var->value;
//~ #ifdef DEBUG
			//~ printf("[DEBUG] actionEnumerate: following __proto__ to next level\n");
//~ #endif
		//~ }
		//~ else
		//~ {
			//~ // End of prototype chain
			//~ current_obj = NULL;
		//~ }
	//~ }
	
	//~ // Free the enumerated names list
	//~ freeEnumeratedNames(enumerated_head);
	
//~ #ifdef DEBUG
	//~ printf("[DEBUG] actionEnumerate: collected %u enumerable properties total\n", total_props);
//~ #endif

	//~ // Step 6: Push null terminator first
	//~ // This marks the end of the enumeration for for..in loops
	//~ PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
	
	//~ // Step 7: Push property names from the list (they're already in reverse order)
	//~ while (prop_head != NULL)
	//~ {
		//~ PUSH_STR((char*)prop_head->name, prop_head->name_length);
		
		//~ PropList* next = prop_head->next;
		//~ free(prop_head);
		//~ prop_head = next;
	//~ }
}

bool evaluateCondition(SWFAppContext* app_context)
{
	ActionVar v;
	convertBool(app_context);
	popVar(app_context, &v);
	
	return v.b;
}

void actionDefineLocal(SWFAppContext* app_context)
{
	// Stack layout: [name, value] <- sp
	// According to AS2 spec for DefineLocal:
	// Pop value first, then name
	// So VALUE is at top (*sp), NAME is at second (SP_SECOND_TOP)
	
	// DefineLocal ALWAYS creates/updates in the local scope
	// We have a local scope object - define variable as a property
	ASObject* local_scope = scope_chain[scope_top_obj];
	ActionVar value_var;
	peekVar(app_context, &value_var);
	
	if (IS_OBJ_T(value_var.type))
	{
		OBJ_LOCK_WRITE(value_var.object,
		{
			retainObject(value_var.object);
		});
	}
	
	POP();
	
	copyReg(app_context);
	
	// Read variable name info
	// Stack layout for strings: +0=type, +4=oldSP, +8=length, +12=string_id, +16=pointer
	u32 string_id = STACK_TOP_ID;
	char* var_name = (char*) STACK_TOP_VALUE;
	u32 var_name_len = STACK_TOP_N;
	
	POP();
	
	// Set property on the local scope object
	// This will create the property if it doesn't exist, or update if it does
	setProperty(app_context, local_scope, string_id, var_name, var_name_len, &value_var);
	
	if (IS_OBJ_T(value_var.type))
	{
		OBJ_LOCK_WRITE(value_var.object,
		{
			releaseObject(app_context, value_var.object);
		});
	}
}

void actionDefineLocal2(SWFAppContext* app_context)
{
	// DefineLocal2 pops only the variable name (no value)
	// It declares a local variable initialized to undefined
	
	// Stack layout: [name] <- sp
	
	copyReg(app_context);
	
	// Read variable name info
	u32 string_id = STACK_TOP_ID;
	char* var_name = (char*) STACK_TOP_VALUE;
	u32 var_name_len = STACK_TOP_N;
	
	// Pop the name
	POP();
	
	// Declare variable as undefined property at top of scope
	ASObject* local_scope = scope_chain[scope_top_obj];
	
	// Create an undefined value
	ActionVar undefined_var;
	undefined_var.type = ACTION_STACK_VALUE_UNDEFINED;
	
	// Set property on the local scope object
	// This will create the property if it doesn't exist
	setProperty(app_context, local_scope, string_id, var_name, var_name_len, &undefined_var);
}

void actionTypeOf(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	// Peek at the type without modifying value
	u8 type = STACK_TOP_TYPE;
	
	// Pop the value
	POP();
	
	// Determine type string based on stack type
	char* type_str;
	switch (type)
	{
		case ACTION_STACK_VALUE_F32:
		case ACTION_STACK_VALUE_F64:
			type_str = "number";
			break;
			
		case ACTION_STACK_VALUE_STRING:
		case ACTION_STACK_VALUE_STR_LIST:
			type_str = "string";
			break;
			
		//~ case ACTION_STACK_VALUE_FUNCTION:
			//~ type_str = "function";
			//~ break;
			
		case ACTION_STACK_VALUE_OBJECT:
			// Arrays are objects in ActionScript (typeof [] returns "object")
			type_str = "object";
			break;
			
		case ACTION_STACK_VALUE_UNDEFINED:
			type_str = "undefined";
			break;
			
		default:
			type_str = "undefined";
			break;
	}
	
	// Copy to str_buffer and push
	u32 len = (u32) strnlen(type_str, 16);
	PUSH_STR(type_str, len);
}

void actionDelete2(SWFAppContext* app_context, char* str_buffer)
{
	//~ // Delete2 deletes a named property/variable
	//~ // Pops the name from the stack, deletes it, pushes success boolean
	
	//~ // Read variable name from stack
	//~ u8 name_type = STACK_TOP_TYPE;
	//~ u32 string_id = 0;
	//~ char* var_name = NULL;
	//~ u32 var_name_len = 0;
	
	//~ // Get the variable name string
	//~ if (name_type == ACTION_STACK_VALUE_STRING)
	//~ {
		//~ string_id = STACK_TOP_ID;
		//~ var_name = (char*) STACK_TOP_VALUE;
		//~ var_name_len = STACK_TOP_N;
	//~ }
	
	//~ else if (name_type == ACTION_STACK_VALUE_STR_LIST)
	//~ {
		//~ // Materialize string list
		//~ var_name = materializeStringList(app_context);
		//~ var_name_len = strlen(var_name);
	//~ }
	
	//~ // Pop the variable name
	//~ POP();
	
	//~ // Default: assume deletion succeeds (Flash behavior)
	//~ bool success = true;
	
	//~ // Try to delete from scope chain (innermost to outermost)
	//~ for (int i = scope_depth - 1; i >= 0; i--)
	//~ {
		//~ if (scope_chain[i] != NULL)
		//~ {
			//~ // Check if property exists in this scope object
			//~ ActionVar* prop = getProperty(scope_chain[i], string_id, var_name, var_name_len);
			//~ if (prop != NULL)
			//~ {
				//~ // Found in scope chain - delete it
				//~ success = deleteProperty(app_context, scope_chain[i], var_name, var_name_len);
				
				//~ // Push result and return
				//~ float result = success ? 1.0f : 0.0f;
				//~ PUSH_F32(result);
				//~ return;
			//~ }
		//~ }
	//~ }
	
	//~ // Not found in scope chain - check global variables
	//~ // Note: In Flash, you cannot delete variables declared with 'var', so we return false
	//~ // However, if the variable doesn't exist at all, we return true (Flash behavior)
	//~ if (getVariable(app_context, var_name, var_name_len) != NULL)
	//~ {
		//~ // Variable exists but is a 'var' declaration - cannot delete
		//~ success = false;
	//~ }
	//~ else
	//~ {
		//~ // Variable doesn't exist - Flash returns true
		//~ success = true;
	//~ }
	
	//~ // Push result
	//~ float result = success ? 1.0f : 0.0f;
	//~ PUSH_F32(result);
}

/**
 * Helper function to check if an object is an instance of a constructor
 *
 * Implements the same logic as ActionScript's instanceof operator:
 * 1. Checks prototype chain - walks __proto__ looking for constructor's prototype
 * 2. Checks interface implementation - for AS2 interfaces
 *
 * @param obj_var Pointer to the object to check
 * @param ctor_var Pointer to the constructor function
 * @return 1 if object is instance of constructor, 0 otherwise
 */
static int checkInstanceOf(ActionVar* obj_var, ActionVar* ctor_var)
{
	//~ // Primitives (number, string, undefined) are never instances
	//~ if (obj_var->type == ACTION_STACK_VALUE_F32 ||
		//~ obj_var->type == ACTION_STACK_VALUE_F64 ||
		//~ obj_var->type == ACTION_STACK_VALUE_STRING ||
		//~ obj_var->type == ACTION_STACK_VALUE_UNDEFINED)
	//~ {
		//~ return 0;
	//~ }
	
	//~ // Object and constructor must be object types
	//~ if (obj_var->type != ACTION_STACK_VALUE_OBJECT &&
		//~ obj_var->type != ACTION_STACK_VALUE_ARRAY &&
		//~ obj_var->type != ACTION_STACK_VALUE_FUNCTION)
	//~ {
		//~ return 0;
	//~ }
	
	//~ if (ctor_var->type != ACTION_STACK_VALUE_OBJECT &&
		//~ ctor_var->type != ACTION_STACK_VALUE_FUNCTION)
	//~ {
		//~ return 0;
	//~ }
	
	//~ ASObject* obj = (ASObject*) obj_var->value;
	//~ ASObject* ctor = (ASObject*) ctor_var->value;
	
	//~ if (obj == NULL || ctor == NULL)
	//~ {
		//~ return 0;
	//~ }
	
	//~ // Get the constructor's "prototype" property
	//~ ActionVar* ctor_proto_var = getProperty(ctor, 0, "prototype", 9);
	//~ if (ctor_proto_var == NULL)
	//~ {
		//~ return 0;
	//~ }
	
	//~ // Get the prototype object
	//~ if (ctor_proto_var->type != ACTION_STACK_VALUE_OBJECT)
	//~ {
		//~ return 0;
	//~ }
	
	//~ ASObject* ctor_proto = (ASObject*) ctor_proto_var->value;
	//~ if (ctor_proto == NULL)
	//~ {
		//~ return 0;
	//~ }
	
	//~ // Walk up the object's prototype chain via __proto__ property
	//~ // Start with the object's __proto__
	//~ ActionVar* current_proto_var = getProperty(obj, 0, "__proto__", 9);
	
	//~ // Maximum chain depth to prevent infinite loops
	//~ int max_depth = 100;
	//~ int depth = 0;
	
	//~ while (current_proto_var != NULL && depth < max_depth)
	//~ {
		//~ depth++;
		
		//~ // Check if this prototype matches the constructor's prototype
		//~ if (current_proto_var->type == ACTION_STACK_VALUE_OBJECT)
		//~ {
			//~ ASObject* current_proto = (ASObject*) current_proto_var->value;
			
			//~ if (current_proto == ctor_proto)
			//~ {
				//~ // Found a match!
				//~ return 1;
			//~ }
			
			//~ // Continue up the chain
			//~ current_proto_var = getProperty(current_proto, 0, "__proto__", 9);
		//~ }
		//~ else
		//~ {
			//~ // Non-object in prototype chain, stop
			//~ break;
		//~ }
	//~ }
	
	//~ // Check interface implementation (ActionScript 2.0 implements keyword)
	//~ if (implementsInterface(obj, ctor))
	//~ {
		//~ return 1;
	//~ }
	
	// Not found in prototype chain or interfaces
	return 0;
}

void actionStoreRegister(SWFAppContext* app_context, u8 reg_i)
{
	// Peek the top of stack (don't pop!)
	ActionVar value;
	peekVar(app_context, &value);
	
	ActionVar* reg = &scope_registers[scope_top_obj][reg_i];
	
	if (IS_OBJ_T(value.type))
	{
		OBJ_LOCK_WRITE(value.object,
		{
			retainObject(value.object);
		});
	}
	
	if (IS_OBJ_T(reg->type))
	{
		OBJ_LOCK_WRITE(reg->object,
		{
			releaseObject(app_context, reg->object);
		});
	}
	
	// Store value in register
	*reg = value;
}

//~ void actionInitArray(SWFAppContext* app_context)
//~ {
	//~ // 1. Pop array element count
	//~ convertFloat(app_context);
	//~ ActionVar count_var;
	//~ popVar(app_context, &count_var);
	//~ u32 num_elements = (u32) VAL(float, &count_var.value);
	
	//~ // 2. Allocate array
	//~ ASArray* arr = allocArray(app_context, num_elements);
	//~ if (!arr) {
		//~ // Handle allocation failure - push empty array or null
		//~ PUSH_F32((float){0.0f});
		//~ return;
	//~ }
	//~ arr->length = num_elements;
	
	//~ // 3. Pop elements and populate array
	//~ // Per SWF spec: elements were pushed in reverse order (rightmost first, leftmost last)
	//~ // Stack has: [..., elem_N, elem_N-1, ..., elem_1] with elem_1 on top
	//~ // We pop and store sequentially: pop elem_1 -> arr[0], pop elem_2 -> arr[1], etc.
	//~ for (u32 i = 0; i < num_elements; i++) {
		//~ ActionVar elem;
		//~ popVar(app_context, &elem);
		//~ arr->elements[i] = elem;
		
		//~ // If element is array, increment refcount
		//~ if (elem.type == ACTION_STACK_VALUE_ARRAY) {
			//~ retainArray((ASArray*) elem.value);
		//~ }
		//~ // Could also handle ACTION_STACK_VALUE_OBJECT here if needed
	//~ }
	
	//~ // 4. Push array reference to stack
	//~ PUSH(ACTION_STACK_VALUE_ARRAY, (u64) arr);
//~ }

void actionSetMember(SWFAppContext* app_context)
{
	// Stack layout (from top to bottom):
	// 1. value (the value to assign)
	// 2. property_name (the name of the property)
	// 3. object (the object to set the property on)
	
	// Fetch the value to assign
	ActionVar value_var;
	peekVar(app_context, &value_var);
	
	if (IS_OBJ(value_var))
	{
		ASObject* o = (ASObject*) value_var.value;
		
		OBJ_LOCK_WRITE(o,
		{
			// we now have a reference to this object
			retainObject(o);
		});
	}
	
	POP();
	
	// Pop the property name
	ActionVar prop_name_var;
	peekVar(app_context, &prop_name_var);
	
	ASObject* prop_obj;
	
	if (IS_OBJ(prop_name_var))
	{
		prop_obj = prop_name_var.object;
		
		OBJ_LOCK_WRITE(prop_obj,
		{
			// we now have a reference to this object
			retainObject(prop_obj);
		});
		
		toPrimitive(app_context, prop_obj);
		
		popVar(app_context, &prop_name_var);
	}
	
	POP();
	
	if (UNLIKELY(prop_name_var.type != ACTION_STACK_VALUE_STRING))
	{
		if (IS_OBJ_T(prop_name_var.type))
		{
			EXC("Bad property key");
		}
		
		if (IS_NUM_T(prop_name_var.type))
		{
			convertNumericToInteger(app_context, &prop_name_var);
		}
	}
	
	// Fetch the object
	ActionVar obj_var;
	peekVar(app_context, &obj_var);
	
	// Check if the object is actually an object type
	if (IS_OBJ(obj_var))
	{
		ASObject* obj = (ASObject*) obj_var.value;
		
		OBJ_LOCK_WRITE(obj,
		{
			// we now have a reference to this object
			retainObject(obj);
		});
		
		ASObject* constructor = getConstructor(obj);
		
		switch (Function_get_func_name_string_id(app_context, constructor))
		{
			case STR_ID_ARRAY:
			{
				s32 i = prop_name_var.s32;
				Array_setElement(app_context, obj, i, &value_var);
				
				break;
			}
			
			default:
			{
				// Set the property on the object
				setProperty(app_context, obj, prop_name_var.string_id, NULL, 0, &value_var);
				
				break;
			}
		}
		
		OBJ_LOCK_WRITE(obj,
		{
			// we no longer have a reference to this object
			releaseObject(app_context, obj);
		});
	}
	
	POP();
	
	if (IS_OBJ(value_var))
	{
		ASObject* o = (ASObject*) value_var.value;
		
		OBJ_LOCK_WRITE(o,
		{
			// we no longer have a reference to this object
			releaseObject(app_context, o);
		});
	}
	
	if (IS_OBJ(prop_name_var))
	{
		OBJ_LOCK_WRITE(prop_name_var.object,
		{
			// we now have a reference to this object
			releaseObject(app_context, prop_name_var.object);
		});
	}
	
	// If it's not an object type, we silently ignore the operation
	// (Flash behavior for setting properties on non-objects)
}

void actionInitObject(SWFAppContext* app_context)
{
	// Step 1: Pop property count from stack
	convertFloat(app_context);
	ActionVar count_var;
	popVar(app_context, &count_var);
	u32 num_props = (u32) VAL(float, &count_var.value);
	
	// Step 2: Allocate object
	ASObject* obj = allocObject(app_context);
	
	// Step 3: Pop property name/value pairs from stack
	// Properties are in reverse order: rightmost property is on top of stack
	// Stack order is: [..., value1, name1, ..., valueN, nameN, count]
	// So after popping count, top of stack is nameN
	for (u32 i = 0; i < num_props; i++)
	{
		// Pop property name first (it's on top)
		ActionVar name_var;
		convertString(app_context);
		popVar(app_context, &name_var);
		
		// Pop property value (it's below the name)
		ActionVar value;
		popVar(app_context, &value);
		u32 string_id = 0;
		const char* name = NULL;
		u32 name_length = 0;
		
		// Handle string name
		string_id = name_var.string_id;
		name = name_var.owns_memory ?
			name_var.str :
			(const char*) name_var.value;
		name_length = name_var.str_size;
		
		// Store property using the object API
		// This handles refcount management if value is an object
		setProperty(app_context, obj, string_id, name, name_length, &value);
	}
	
	// Step 4: Push object reference to stack
	// The object has refcount = 1 from allocation
	PUSH(ACTION_STACK_VALUE_OBJECT, (u64) obj);
}

void actionDelete(SWFAppContext* app_context)
{
	// Stack layout (from top to bottom):
	// 1. property_name (string) - name of property to delete
	// 2. object_name (string) - name of variable containing the object
	
	// Pop property name
	ActionVar prop_name_var;
	popVar(app_context, &prop_name_var);
	
	const char* prop_name = NULL;
	u32 prop_name_len = 0;
	
	if (prop_name_var.type == ACTION_STACK_VALUE_STRING)
	{
		prop_name = prop_name_var.owns_memory ?
			prop_name_var.str :
			(const char*) prop_name_var.value;
		prop_name_len = prop_name_var.str_size;
	}
	else
	{
		// Property name must be a string
		// Return true (AS2 spec: returns true for invalid operations)
		float result = 1.0f;
		PUSH_F32(result);
		return;
	}
	
	// Pop object name (variable name)
	ActionVar obj_name_var;
	popVar(app_context, &obj_name_var);
	
	const char* obj_name = NULL;
	u32 obj_name_len = 0;
	
	if (obj_name_var.type == ACTION_STACK_VALUE_STRING)
	{
		obj_name = obj_name_var.owns_memory ?
			obj_name_var.str :
			(const char*) obj_name_var.value;
		obj_name_len = obj_name_var.str_size;
	}
	else
	{
		// Object name must be a string
		// Return true (AS2 spec: returns true for invalid operations)
		float result = 1.0f;
		PUSH_F32(result);
		return;
	}
	
	// Look up the variable to get the object
	ActionVar* obj_var = getVariable(app_context, (char*)obj_name, obj_name_len);
	
	// If variable doesn't exist, return true (AS2 spec)
	if (obj_var == NULL)
	{
		float result = 1.0f;
		PUSH_F32(result);
		return;
	}
	
	// If variable is not an object, return true (AS2 spec)
	if (obj_var->type != ACTION_STACK_VALUE_OBJECT)
	{
		float result = 1.0f;
		PUSH_F32(result);
		return;
	}
	
	// Get the object
	ASObject* obj = (ASObject*) obj_var->value;
	
	// If object is NULL, return true
	if (obj == NULL)
	{
		float result = 1.0f;
		PUSH_F32(result);
		return;
	}
	
	// Delete the property
	bool success = deleteProperty(app_context, obj, prop_name, prop_name_len);
	
	// Push result (1.0 for success, 0.0 for failure)
	float result = success ? 1.0f : 0.0f;
	PUSH_F32(result);
}

void actionGetMember(SWFAppContext* app_context)
{
	ActionVar prop_name_var;
	peekVar(app_context, &prop_name_var);
	
	ASObject* prop_obj;
	
	if (IS_OBJ(prop_name_var))
	{
		prop_obj = prop_name_var.object;
		
		OBJ_LOCK_WRITE(prop_obj,
		{
			// we now have a reference to this object
			retainObject(prop_obj);
		});
		
		toPrimitive(app_context, prop_obj);
		
		popVar(app_context, &prop_name_var);
	}
	
	POP();
	
	if (UNLIKELY(prop_name_var.type != ACTION_STACK_VALUE_STRING))
	{
		if (IS_OBJ_T(prop_name_var.type))
		{
			EXC("Bad property key");
		}
		
		if (IS_NUM_T(prop_name_var.type))
		{
			convertNumericToInteger(app_context, &prop_name_var);
		}
	}
	
	// 2. Pop object (second on stack)
	ActionVar obj_var;
	peekVar(app_context, &obj_var);
	
	ASObject* obj = (ASObject*) obj_var.value;
	
	// Check if the object is actually an object type
	if (IS_OBJ(obj_var))
	{
		OBJ_LOCK_WRITE(obj,
		{
			// we now have a reference to this object
			retainObject(obj);
		});
	}
	
	POP();
	
	bool special_object = false;
	
	// 3. Handle different object types
	switch (obj_var.type)
	{
		case ACTION_STACK_VALUE_OBJECT:
		{
			ASObject* constructor = getConstructor(obj);
			
			switch (Function_get_func_name_string_id(app_context, constructor))
			{
				case STR_ID_ARRAY:
				{
					s32 i = prop_name_var.s32;
					PUSH_VAR(Array_getElement(app_context, obj, i));
					
					special_object = true;
					
					break;
				}
			}
			
			if (special_object)
			{
				break;
			}
			
			ASProperty* prop;
			
			OBJ_LOCK_READ(obj,
			{
				// Look up property
				prop = getPropertyWithPrototype(obj, prop_name_var.string_id, NULL, 0);
			});
			
			if (prop != NULL)
			{
				// Property found - push its value
				pushVar(app_context, &prop->value);
			}
			
			else
			{
				// Property not found - push undefined
				PUSH_UNDEFINED();
			}
			
			break;
		}
		
		case ACTION_STACK_VALUE_STRING:
		{
			// Handle string properties
			if (prop_name_var.string_id == STR_ID_LENGTH)
			{
				PUSH_F32((f32) obj_var.str_size);
			}
			
			else
			{
				EXC_ARG("Tried to get property %s of String type\n", prop_name_var.str);
			}
			
			break;
		}
		
		default:
		{
			// Other primitive types (number, undefined, etc.) - push undefined
			PUSH_UNDEFINED();
			
			break;
		}
	}
	
	if (IS_OBJ(obj_var))
	{
		OBJ_LOCK_WRITE(obj,
		{
			// we no longer have a reference to this object
			releaseObject(app_context, obj);
		});
	}
	
	if (IS_OBJ(prop_name_var))
	{
		OBJ_LOCK_WRITE(prop_name_var.object,
		{
			// we now have a reference to this object
			releaseObject(app_context, prop_name_var.object);
		});
	}
}

void callFunction(SWFAppContext* app_context, ASObject* this, ActionVar* func_v, u32 num_args)
{
	ASObject* func_obj = func_v->object;
	
	scope_top_obj += 1;
	
	scope_chain[scope_top_obj] = allocObject(app_context);
	retainObject(scope_chain[scope_top_obj]);
	
	switch (Function_get_func_type(app_context, func_obj))
	{
		case FUNC_TYPE_1:
		{
			scope_registers[scope_top_obj] = HALLOC(4*sizeof(ActionVar));
			
			ActionVar* regs = scope_registers[scope_top_obj];
			
			for (u8 i = 0; i < 4; ++i)
			{
				regs[i].type = ACTION_STACK_VALUE_UNDEFINED;
			}
			
			if (this != NULL)
			{
				ActionVar this_v;
				this_v.type = ACTION_STACK_VALUE_OBJECT;
				this_v.object = this;
				
				setPropertyInThisScope(app_context, STR_ID_THIS, NULL, 0, &this_v);
			}
			
			u32* args = Function_get_args(app_context, func_obj);
			
			// Pop arguments from stack (in reverse order)
			if (num_args > 0)
			{
				for (u32 i = 0; i < num_args; ++i)
				{
					ActionVar v;
					peekVar(app_context, &v);
					setPropertyInThisScope(app_context, args[i], NULL, 0, &v);
					POP();
				}
			}
			
			Function_get_func(app_context, func_obj)(app_context);
			
			FREE(regs);
			break;
		}
		
		case FUNC_TYPE_2:
		{
			u8 reg_count = Function_get_reg_count(app_context, func_obj);
			u16 flags = Function_get_flags(app_context, func_obj);
			
			scope_registers[scope_top_obj] = HALLOC((reg_count + 1)*sizeof(ActionVar));
			
			ActionVar* regs = scope_registers[scope_top_obj];
			
			for (u8 i = 0; i < reg_count + 1; ++i)
			{
				regs[i].type = ACTION_STACK_VALUE_UNDEFINED;
			}
			
			Function2Param* args2 = Function_get_args(app_context, func_obj);
			
			// Pop arguments from stack (in reverse order)
			if (num_args > 0)
			{
				for (u32 i = 0; i < num_args; ++i)
				{
					
					Function2Param* arg = &args2[i];
					
					if (arg->reg == 0)
					{
						ActionVar v;
						peekVar(app_context, &v);
						setPropertyInThisScope(app_context, arg->string_id, NULL, 0, &v);
						POP();
					}
					
					else
					{
						popVar(app_context, &regs[arg->reg]);
					}
				}
			}
			
			u8 next_preload = 1;
			
			if (flags & FUNC_FLAG_PRELOAD_PARENT)
			{
				EXC("_parent not implemented\n");
			}
			
			if (flags & FUNC_FLAG_PRELOAD_ROOT)
			{
				EXC("_root not implemented\n");
			}
			
			if ((flags & FUNC_FLAG_SUPPRESS_SUPER) == 0)
			{
				EXC("super not implemented\n");
			}
			
			if ((flags & FUNC_FLAG_SUPPRESS_ARGUMENTS) == 0)
			{
				EXC("arguments not implemented\n");
			}
			
			if ((flags & FUNC_FLAG_SUPPRESS_THIS) == 0)
			{
				assert(this != NULL);
				
				ActionVar this_v;
				this_v.type = ACTION_STACK_VALUE_OBJECT;
				this_v.object = this;
				
				setPropertyInThisScope(app_context, STR_ID_THIS, NULL, 0, &this_v);
				
				if (flags & FUNC_FLAG_PRELOAD_THIS)
				{
					regs[next_preload] = this_v;
					next_preload += 1;
				}
			}
			
			if (flags & FUNC_FLAG_PRELOAD_GLOBAL)
			{
				regs[next_preload].type = ACTION_STACK_VALUE_OBJECT;
				regs[next_preload].object = _global;
				next_preload += 1;
			}
			
			Function_get_func(app_context, func_obj)(app_context);
			
			FREE(regs);
			break;
		}
		
		case FUNC_TYPE_3:
		{
			action_runtime_func f = (action_runtime_func) Function_get_func(app_context, func_obj);
			f(app_context, this, num_args);
			
			break;
		}
	}
	
	releaseObject(app_context, scope_chain[scope_top_obj]);
	
	scope_top_obj -= 1;
}

void getAndCallMethod(SWFAppContext* app_context, ASObject* this, u32 method_name, u32 num_args)
{
	ActionVar meth_v = getPropertyWithPrototype(this, method_name, NULL, 0)->value;
	callFunction(app_context, this, &meth_v, num_args);
}

void actionNewObject(SWFAppContext* app_context)
{
	// 1. Pop constructor name (string)
	ActionVar ctor_name_var;
	peekVar(app_context, &ctor_name_var);
	
	ASObject* ctor_name_obj = ctor_name_var.object;
	
	if (IS_OBJ_T(ctor_name_var.type))
	{
		UNIMPLEMENTED("NewObject constructor name object\n");
		
		//~ OBJ_LOCK_WRITE(ctor_name_obj,
		//~ {
			//~ retainObject(ctor_name_obj);
		//~ });
	}
	
	POP();
	
	// 2. Pop number of arguments
	ActionVar num_args_var;
	peekVar(app_context, &num_args_var);
	u32 num_args = (u32) num_args_var.value;
	
	ASObject* num_args_obj = num_args_var.object;
	
	if (IS_OBJ_T(num_args_var.type))
	{
		UNIMPLEMENTED("NewObject num args object\n");
		
		//~ OBJ_LOCK_WRITE(num_args_obj,
		//~ {
			//~ retainObject(num_args_obj);
		//~ });
	}
	
	POP();
	
	// Try to find existing constructor function
	ActionVar func_v;
	searchScopesForPropertyVar(ctor_name_var.string_id, NULL, 0, &func_v);
	
	if (func_v.type != ACTION_STACK_VALUE_UNDEFINED)
	{
		// Create new object to serve as 'this'
		ASObject* this = allocObject(app_context);
		
		OBJ_LOCK_WRITE(this,
		{
			retainObject(this);
		});
		
		ASProperty* prototype = getProperty(func_v.object, STR_ID_PROTOTYPE, NULL, 0);
		
		ActionVar proto_ref_var;
		proto_ref_var.type = ACTION_STACK_VALUE_OBJECT;
		proto_ref_var.object = prototype->value.object;
		
		setProperty(app_context, this, STR_ID_PROTO, NULL, 0, &proto_ref_var);
		
		setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &func_v);
		
		callFunction(app_context, this, &func_v, num_args);
		POP();
		
		PUSH_OBJ(this);
		
		OBJ_LOCK_WRITE(this,
		{
			releaseObject(app_context, this);
		});
	}
	
	else
	{
		EXC_ARG("Constructor function %s not found.\n", (char*) ctor_name_var.value);
	}
	
	//~ if (IS_OBJ_T(ctor_name_var.type))
	//~ {
		//~ OBJ_LOCK_WRITE(ctor_name_obj,
		//~ {
			//~ releaseObject(app_context, ctor_name_obj);
		//~ });
	//~ }
	
	//~ if (IS_OBJ_T(num_args_var.type))
	//~ {
		//~ OBJ_LOCK_WRITE(num_args_obj,
		//~ {
			//~ releaseObject(app_context, num_args_obj);
		//~ });
	//~ }
}

/**
 * ActionNewMethod (0x53) - Create new object by calling method on object as constructor
 *
 * Stack layout: [method_name] [object] [num_args] [arg1] [arg2] ... <- sp
 *
 * SWF Specification behavior:
 * 1. Pops the name of the method from the stack
 * 2. Pops the ScriptObject from the stack
 *    - If method name is blank: object is treated as function object (constructor)
 *    - If method name not blank: named method of object is invoked as constructor
 * 3. Pops the number of arguments from the stack
 * 4. Executes the method call as constructor
 * 5. Pushes the newly constructed object to the stack
 */
void actionNewMethod(SWFAppContext* app_context)
{
	// Pop constructor method name (string)
	ActionVar ctor_name_var;
	peekVar(app_context, &ctor_name_var);
	
	ASObject* ctor_name_obj = ctor_name_var.object;
	
	if (IS_OBJ_T(ctor_name_var.type))
	{
		UNIMPLEMENTED("NewMethod constructor name object");
		
		//~ OBJ_LOCK_WRITE(ctor_name_obj,
		//~ {
			//~ retainObject(ctor_name_obj);
		//~ });
	}
	
	POP();
	
	// Pop object which holds the method
	ActionVar object_var;
	peekVar(app_context, &object_var);
	
	ASObject* obj = object_var.object;
	
	if (IS_OBJ_T(ctor_name_var.type))
	{
		OBJ_LOCK_WRITE(obj,
		{
			retainObject(obj);
		});
	}
	
	POP();
	
	// Pop number of arguments
	ActionVar num_args_var;
	peekVar(app_context, &num_args_var);
	u32 num_args = (u32) num_args_var.value;
	
	ASObject* num_args_obj = num_args_var.object;
	
	if (IS_OBJ_T(ctor_name_var.type))
	{
		UNIMPLEMENTED("NewMethod num args object");
		
		//~ OBJ_LOCK_WRITE(num_args_obj,
		//~ {
			//~ retainObject(num_args_obj);
		//~ });
	}
	
	POP();
	
	// Try to find constructor method
	ActionVar func_v;
	getPropertyVar(obj, ctor_name_var.string_id, NULL, 0, &func_v);
	
	if (func_v.type != ACTION_STACK_VALUE_UNDEFINED)
	{
		// Constructor found
		// Create new object to serve as 'this'
		ASObject* this = allocObject(app_context);
		
		OBJ_LOCK_WRITE(this,
		{
			retainObject(this);
		});
		
		ASProperty* prototype = getProperty(func_v.object, STR_ID_PROTOTYPE, NULL, 0);
		
		ActionVar proto_ref_var;
		proto_ref_var.type = ACTION_STACK_VALUE_OBJECT;
		proto_ref_var.object = prototype->value.object;
		
		setProperty(app_context, this, STR_ID_PROTO, NULL, 0, &proto_ref_var);
		
		setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &func_v);
		
		callFunction(app_context, this, &func_v, num_args);
		POP();
		
		PUSH_OBJ(this);
		
		OBJ_LOCK_WRITE(this,
		{
			releaseObject(app_context, this);
		});
	}
	
	else
	{
		EXC_ARG("Constructor method %s not found.\n", (char*) ctor_name_var.value);
	}
	
	if (IS_OBJ_T(object_var.type))
	{
		OBJ_LOCK_WRITE(obj,
		{
			releaseObject(app_context, obj);
		});
	}
}

void actionDefineFunction(SWFAppContext* app_context, u32 string_id, action_func func, u32* args, bool anonymous)
{
	assert(string_id != 0);
	
	// Create function object
	ASObject* func_obj = allocObject(app_context);
	
	Function_init_object(app_context, func_obj);
	
	Function_set_members(app_context, func_obj,
						 FUNC_TYPE_1,
						 func,
						 args,
						 0,
						 0,
						 string_id);
	
	ActionVar proto_var;
	proto_var.type = ACTION_STACK_VALUE_OBJECT;
	proto_var.object = allocObject(app_context);
	
	setProperty(app_context, func_obj, STR_ID_PROTOTYPE, NULL, 0, &proto_var);
	
	if (!anonymous)
	{
		// If named, store in variable
		ActionVar func_var;
		func_var.type = ACTION_STACK_VALUE_OBJECT;
		func_var.object = func_obj;
		
		setPropertyInThisScope(app_context, string_id, NULL, 0, &func_var);
	}
	
	else
	{
		// Anonymous function: push to stack
		PUSH_OBJ(func_obj);
	}
}

void actionDefineFunction2(SWFAppContext* app_context, u32 string_id, action_func func, Function2Param* args, u8 reg_count, u16 flags, bool anonymous)
{
	assert(string_id != 0);
	
	// Create function object
	ASObject* func_obj = allocObject(app_context);
	
	Function_init_object(app_context, func_obj);
	
	Function_set_members(app_context, func_obj,
						 FUNC_TYPE_2,
						 func,
						 args,
						 reg_count,
						 flags,
						 string_id);
	
	ActionVar proto_var;
	proto_var.type = ACTION_STACK_VALUE_OBJECT;
	proto_var.object = allocObject(app_context);
	
	setProperty(app_context, func_obj, STR_ID_PROTOTYPE, NULL, 0, &proto_var);
	
	if (!anonymous)
	{
		// If named, store in variable
		ActionVar func_var;
		func_var.type = ACTION_STACK_VALUE_OBJECT;
		func_var.object = func_obj;
		
		setPropertyInThisScope(app_context, string_id, NULL, 0, &func_var);
	}
	
	else
	{
		// Anonymous function: push to stack
		PUSH_OBJ(func_obj);
	}
}

void actionCallFunction(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	// 1. Pop function name (string) from stack
	char* func_name = (char*) STACK_TOP_VALUE;
	u32 string_id = STACK_TOP_ID;
	POP();
	
	// 2. Pop number of arguments
	ActionVar num_args_var;
	popVar(app_context, &num_args_var);
	u32 num_args = (u32) num_args_var.value;
	
	ActionVar func_v;
	searchScopesForPropertyVar(string_id, NULL, 0, &func_v);
	
	if (func_v.type == ACTION_STACK_VALUE_UNDEFINED)
	{
		// Function not found - throw
		EXC_ARG("Function not found: %s\n", func_name);
		return;
	}
	
	callFunction(app_context, NULL, &func_v, num_args);
}

void actionCallMethod(SWFAppContext* app_context)
{
	copyReg(app_context);
	
	if (IS_OBJ_T(STACK_TOP_TYPE))
	{
		UNIMPLEMENTED("CallMethod method name object");
		
		//~ OBJ_LOCK_WRITE(obj,
		//~ {
			//~ retainObject(obj);
		//~ });
	}
	
	// Pop method name (string) from stack
	char* func_name = (char*) STACK_TOP_VALUE;
	u32 string_id = STACK_TOP_ID;
	POP();
	
	// Pop object from stack
	ActionVar this_v;
	peekVar(app_context, &this_v);
	
	ASObject* this = this_v.object;
	
	if (IS_OBJ_T(this_v.type))
	{
		OBJ_LOCK_WRITE(this,
		{
			retainObject(this);
		});
	}
	
	POP();
	
	// Pop number of arguments
	ActionVar num_args_var;
	peekVar(app_context, &num_args_var);
	u32 num_args = (u32) num_args_var.value;
	
	if (IS_OBJ_T(num_args_var.type))
	{
		UNIMPLEMENTED("CallMethod num args object");
		
		//~ OBJ_LOCK_WRITE(obj,
		//~ {
			//~ retainObject(obj);
		//~ });
	}
	
	POP();
	
	switch (this_v.type)
	{
		case ACTION_STACK_VALUE_F32:
		case ACTION_STACK_VALUE_F64:
		case ACTION_STACK_VALUE_INT:
		{
			switch (string_id)
			{
				case STR_ID_TO_STRING:
				{
					convertNumericToNumber(app_context, &this_v);
					toString(app_context, &this_v);
					
					break;
				}
				
				case STR_ID_VALUE_OF:
				{
					PUSH_VAR(&this_v);
					
					break;
				}
				
				default:
				{
					// Function not found - throw
					EXC_ARG("Function not found: %s\n", func_name);
				}
			}
			
			return;
		}
		
		case ACTION_STACK_VALUE_STRING:
		{
			switch (string_id)
			{
				case STR_ID_TO_STRING:
				{
					PUSH_VAR(&this_v);
					
					break;
				}
				
				case STR_ID_VALUE_OF:
				{
					toNumber(app_context, &this_v);
					
					break;
				}
				
				default:
				{
					// Function not found - throw
					EXC_ARG("Function not found: %s\n", func_name);
				}
			}
			
			return;
		}
	}
	
	ASProperty* meth_p = NULL;
	
	if (string_id != STR_ID_EMPTY)
	{
		meth_p = getPropertyWithPrototype(this, string_id, NULL, 0);
	}
	
	else
	{
		EXC("Callable objects not implemented (ActionCallMethod).");
	}
	
	if (meth_p != NULL)
	{
		callFunction(app_context, this, &meth_p->value, num_args);
	}
	
	else
	{
		// Function not found - throw
		EXC_ARG("Function not found: %s\n", func_name);
	}
	
	if (IS_OBJ_T(this_v.type))
	{
		OBJ_LOCK_WRITE(this,
		{
			releaseObject(app_context, this);
		});
	}
}