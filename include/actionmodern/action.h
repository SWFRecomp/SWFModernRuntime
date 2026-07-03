#pragma once

#include <swf.h>
#include <objects.h>
#include <variables.h>
#include <stackvalue.h>

#define STACK_VAR_SIZE (4 + 4 + 8 + 8)

#define PUSH(t, v) \
	OLDSP = SP; \
	SP -= STACK_VAR_SIZE; \
	SP &= ~7; \
	STACK[SP] = t; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u64, &STACK[SP + 16]) = v;

// Push string with ID (for constant strings from compiler)
#define PUSH_STR_ID(v, id, n) \
	OLDSP = SP; \
	SP -= STACK_VAR_SIZE; \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STRING; \
	STACK[SP + 1] = false; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n; \
	VAL(u32, &STACK[SP + 12]) = id; \
	VAL(char*, &STACK[SP + 16]) = v;

// Push dynamic string onto the stack
#define PUSH_STR_STACK(n) \
	OLDSP = SP; \
	SP -= (u32) (4 + 4 + 8 + (n + 1)); \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STRING; \
	STACK[SP + 1] = true; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n; \
	VAL(u32, &STACK[SP + 12]) = 0;

// Push string without ID (for dynamic strings, ID = 0)
#define PUSH_STR(v, n) PUSH_STR_ID(v, 0, n)

#define PUSH_STR_LIST(n, size) \
	OLDSP = VAL(u32, &STACK[SP_SECOND_TOP + 4]); \
	SP -= (u32) (4 + 4 + 8 + size); \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STR_LIST; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n; \
	VAL(u32, &STACK[SP + 12]) = 0;

#define PUSH_NULL() PUSH(ACTION_STACK_VALUE_NULL, 0)
#define PUSH_UNDEFINED() PUSH(ACTION_STACK_VALUE_UNDEFINED, 0)

#define PUSH_F32(f) PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &(f)));
#define PUSH_F64(f) PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &(f)));
#define PUSH_INT(i) PUSH(ACTION_STACK_VALUE_INT, (i));
#define PUSH_BOOL(b) PUSH(ACTION_STACK_VALUE_BOOLEAN, (b));

#define PUSH_OBJ(o) \
	PUSH(ACTION_STACK_VALUE_OBJECT, (u64) o) \
	OBJ_LOCK_WRITE(o, \
	{ \
		retainObject(o); \
	});

#define PUSH_VAR(p) pushVar(app_context, p)

#define POP() \
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_OBJECT) \
	{ \
		ASObject* o = (ASObject*) STACK_TOP_VALUE; \
		OBJ_LOCK_WRITE(o, \
		{ \
			releaseObject(app_context, o); \
		}); \
	} \
	SP = VAL(u32, &STACK[SP + 4]);

#define POP_2() \
	POP(); \
	POP();

#define DISCARD_ARGS(n) discardArgs(app_context, n)

#define STACK_TOP_TYPE STACK[SP]
#define STACK_TOP_OWNS_MEM STACK[SP + 1]
#define STACK_TOP_N VAL(u32, &STACK[SP + 8])
#define STACK_TOP_ID VAL(u32, &STACK[SP + 12])
#define STACK_TOP_VALUE VAL(u64, &STACK[SP + 16])

#define SP_SECOND_TOP VAL(u32, &STACK[SP + 4])
#define STACK_SECOND_TOP_TYPE STACK[SP_SECOND_TOP]
#define STACK_SECOND_TOP_OWNS_MEM STACK[SP_SECOND_TOP + 1]
#define STACK_SECOND_TOP_N VAL(u32, &STACK[SP_SECOND_TOP + 8])
#define STACK_SECOND_TOP_ID VAL(u32, &STACK[SP_SECOND_TOP + 12])
#define STACK_SECOND_TOP_VALUE VAL(u64, &STACK[SP_SECOND_TOP + 16])

#define FUNC_FLAG_PRELOAD_PARENT        0b0000000010000000
#define FUNC_FLAG_PRELOAD_ROOT          0b0000000001000000
#define FUNC_FLAG_SUPPRESS_SUPER        0b0000000000100000
#define FUNC_FLAG_PRELOAD_SUPER         0b0000000000010000
#define FUNC_FLAG_SUPPRESS_ARGUMENTS    0b0000000000001000
#define FUNC_FLAG_PRELOAD_ARGUMENTS     0b0000000000000100
#define FUNC_FLAG_SUPPRESS_THIS         0b0000000000000010
#define FUNC_FLAG_PRELOAD_THIS          0b0000000000000001
#define FUNC_FLAG_PRELOAD_GLOBAL        0b0000000100000000

#define IS_NULL(v) (v.type == ACTION_STACK_VALUE_NULL)
#define IS_UNDEFINED(v) (v.type == ACTION_STACK_VALUE_UNDEFINED)
#define IS_NULL_UNDEFINED(v) (IS_NULL(v) || IS_UNDEFINED(v))

#define IS_STR_T(t) (t == ACTION_STACK_VALUE_STRING || t == ACTION_STACK_VALUE_STR_LIST)
#define IS_NUM_T(t) (t == ACTION_STACK_VALUE_F32 || t == ACTION_STACK_VALUE_F64 || t == ACTION_STACK_VALUE_INT)

#define RETURN_VOID() PUSH_UNDEFINED()

#define VAL(type, x) *((type*) x)

#define INITIAL_STACK_SIZE 8388608  // 8 MB
#define INITIAL_SP INITIAL_STACK_SIZE

#define _GLOBAL app_context->scope_chain[0]

typedef struct
{
	u8 reg;
	u32 string_id;
} Function2Param;

void initActions(SWFAppContext* app_context);
void freeActions(SWFAppContext* app_context);

void discardArgs(SWFAppContext* app_context, u32 num_args);

void pushVar(SWFAppContext* app_context, ActionVar* p);
void pushReg(SWFAppContext* app_context, u8 reg);

void releaseObjectVar(SWFAppContext* app_context, ActionVar* var);

void peekVar(SWFAppContext* app_context, ActionVar* var);
void popVar(SWFAppContext* app_context, ActionVar* var);

void convertNumericToNumber(SWFAppContext* app_context, ActionVar* v);
void convertNumericToInteger(SWFAppContext* app_context, ActionVar* v);

void toNumber(SWFAppContext* app_context, ActionVar* v);
void toPrimitive(SWFAppContext* app_context, ASObject* this);
void toString(SWFAppContext* app_context, ActionVar* v);

ActionStackValueType convertString(SWFAppContext* app_context);
ActionStackValueType convertDouble(SWFAppContext* app_context);
ActionStackValueType convertIntECMA(SWFAppContext* app_context);

ASProperty* getPropertyInThisScope(SWFAppContext* app_context, u32 string_id, const char* name, u32 name_len);
void setPropertyInThisScope(SWFAppContext* app_context, u32 string_id, const char* name, u32 name_len, ActionVar* value);

void callFunction(SWFAppContext* app_context, ASObject* this, ActionVar* func_v, u32 num_args);
void getAndCallMethod(SWFAppContext* app_context, ASObject* this, u32 method_name, u32 num_args);
bool getAndCallMethodIfExists(SWFAppContext* app_context, ASObject* this, u32 method_name, u32 num_args);

bool evaluateCondition(SWFAppContext* app_context);

// Arithmetic Operations
void actionAdd(SWFAppContext* app_context);
void actionAdd2(SWFAppContext* app_context);
void actionSubtract(SWFAppContext* app_context);
void actionMultiply(SWFAppContext* app_context);
void actionDivide(SWFAppContext* app_context);
void actionModulo(SWFAppContext* app_context);
void actionIncrement(SWFAppContext* app_context);
void actionDecrement(SWFAppContext* app_context);

// Bitwise Operations
void actionBitAnd(SWFAppContext* app_context);
void actionBitOr(SWFAppContext* app_context);
void actionBitLShift(SWFAppContext* app_context);
void actionBitRShift(SWFAppContext* app_context);
void actionBitURShift(SWFAppContext* app_context);
void actionBitXor(SWFAppContext* app_context);

// Comparison Operations
void actionEquals(SWFAppContext* app_context);
void actionEquals2(SWFAppContext* app_context);
void actionLess(SWFAppContext* app_context);
void actionLess2(SWFAppContext* app_context);
void actionGreater(SWFAppContext* app_context);
void actionAnd(SWFAppContext* app_context);
void actionOr(SWFAppContext* app_context);
void actionNot(SWFAppContext* app_context);

// String Operations
void actionStringEquals(SWFAppContext* app_context, char* a_str, char* b_str);
void actionStringLength(SWFAppContext* app_context, char* v_str);
void actionStringAdd(SWFAppContext* app_context, char* a_str, char* b_str);

// Variable Operations
void actionGetVariable(SWFAppContext* app_context);
void actionSetVariable(SWFAppContext* app_context);
void actionToInteger(SWFAppContext* app_context);
void actionToNumber(SWFAppContext* app_context);
void actionToString(SWFAppContext* app_context);
void actionTypeOf(SWFAppContext* app_context);

// Utility Operations
void actionTrace(SWFAppContext* app_context);
void actionGetTime(SWFAppContext* app_context);

// Object Operations
void actionGetMember(SWFAppContext* app_context);
void actionSetMember(SWFAppContext* app_context);
void actionEnumerate(SWFAppContext* app_context, char* str_buffer);
void actionDelete(SWFAppContext* app_context);
void actionDelete2(SWFAppContext* app_context, char* str_buffer);
void actionNewObject(SWFAppContext* app_context);
void actionNewMethod(SWFAppContext* app_context);
void actionExtends(SWFAppContext* app_context);
void actionInitObject(SWFAppContext* app_context);
void actionCastOp(SWFAppContext* app_context);

// Array Operations
void actionInitArray(SWFAppContext* app_context);

// Function Operations
void actionDefineLocal(SWFAppContext* app_context);
void actionDefineLocal2(SWFAppContext* app_context);
void actionCallFunction(SWFAppContext* app_context);
void actionCallMethod(SWFAppContext* app_context);

// Stack/Register Operations
void actionStoreRegister(SWFAppContext* app_context, u8 register_num);

// Function Definitions
void actionDefineFunction(SWFAppContext* app_context, u32 string_id, action_func func, u32* args, bool anonymous);
void actionDefineFunction2(SWFAppContext* app_context, u32 string_id, action_func func, Function2Param* args, u8 reg_count, u16 flags, bool anonymous);