#pragma once

#include <swf.h>
#include <variables.h>
#include <stackvalue.h>
#include <setjmp.h>

// Forward declarations
typedef struct MovieClip MovieClip;

// MovieClip structure for Flash movie clip properties
struct MovieClip {
	float x, y;
	float xscale, yscale;
	float rotation;
	float alpha;
	float width, height;
	int visible;
	int currentframe;
	int totalframes;
	int framesloaded;
	char name[256];
	char target[256];
	char droptarget[256];
	char url[512];
	// SWF 4+ properties
	float highquality;     // Property 16: _highquality (0, 1, or 2)
	float focusrect;       // Property 17: _focusrect (0 or 1)
	float soundbuftime;    // Property 18: _soundbuftime (in seconds)
	char quality[16];      // Property 19: _quality ("LOW", "MEDIUM", "HIGH", "BEST")
	float xmouse;
	float ymouse;
	MovieClip* parent;     // Parent MovieClip (_root has NULL parent)
};

// Global root MovieClip
extern MovieClip root_movieclip;

// VAL macro must be defined before other macros that use it
#define VAL(type, x) *((type*) x)

// Stack macros - use STACK, SP, OLDSP from swf.h (app_context->stack, etc.)
#define PUSH(t, v) \
	OLDSP = SP; \
	SP -= 4 + 4 + 8 + 8; \
	SP &= ~7; \
	STACK[SP] = t; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u64, &STACK[SP + 16]) = v;

// Push string with ID (for constant strings from compiler)
#define PUSH_STR_ID(v, n, id) \
	OLDSP = SP; \
	SP -= 4 + 4 + 8 + 8; \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STRING; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n; \
	VAL(u32, &STACK[SP + 12]) = id; \
	VAL(char*, &STACK[SP + 16]) = v;

// Push string without ID (for dynamic strings, ID = 0)
#define PUSH_STR(v, n) PUSH_STR_ID(v, n, 0)

#define PUSH_STR_LIST(n, size) \
	OLDSP = VAL(u32, &STACK[SP_SECOND_TOP + 4]); \
	SP -= (u32) (4 + 4 + 8 + size); \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STR_LIST; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n;

#define PUSH_VAR(p) pushVar(app_context, p);

#define POP() \
	SP = VAL(u32, &STACK[SP + 4]);

#define POP_2() \
	POP(); \
	POP();

#define STACK_TOP_TYPE STACK[SP]
#define STACK_TOP_N VAL(u32, &STACK[SP + 8])
#define STACK_TOP_ID VAL(u32, &STACK[SP + 12])
#define STACK_TOP_VALUE VAL(u64, &STACK[SP + 16])

#define SP_SECOND_TOP VAL(u32, &STACK[SP + 4])
#define STACK_SECOND_TOP_TYPE STACK[SP_SECOND_TOP]
#define STACK_SECOND_TOP_N VAL(u32, &STACK[SP_SECOND_TOP + 8])
#define STACK_SECOND_TOP_ID VAL(u32, &STACK[SP_SECOND_TOP + 12])
#define STACK_SECOND_TOP_VALUE VAL(u64, &STACK[SP_SECOND_TOP + 16])

#define INITIAL_STACK_SIZE 8388608  // 8 MB
#define INITIAL_SP INITIAL_STACK_SIZE

extern ActionVar* temp_val;

void initTime(SWFAppContext* app_context);

void pushVar(SWFAppContext* app_context, ActionVar* p);
void popVar(SWFAppContext* app_context, ActionVar* var);
void peekVar(SWFAppContext* app_context, ActionVar* var);
void peekSecondVar(SWFAppContext* app_context, ActionVar* var);
void setVariableByName(const char* var_name, ActionVar* value);

void actionPrevFrame(SWFAppContext* app_context);
void actionToggleQuality(SWFAppContext* app_context);

void actionAdd(SWFAppContext* app_context);
void actionAdd2(SWFAppContext* app_context, char* str_buffer);
void actionSubtract(SWFAppContext* app_context);
void actionMultiply(SWFAppContext* app_context);
void actionDivide(SWFAppContext* app_context);
void actionModulo(SWFAppContext* app_context);
void actionEquals(SWFAppContext* app_context);
void actionLess(SWFAppContext* app_context);
void actionLess2(SWFAppContext* app_context);
void actionEquals2(SWFAppContext* app_context);
void actionAnd(SWFAppContext* app_context);
void actionOr(SWFAppContext* app_context);
void actionNot(SWFAppContext* app_context);
void actionToInteger(SWFAppContext* app_context);
void actionToNumber(SWFAppContext* app_context);
void actionToString(SWFAppContext* app_context, char* str_buffer);
void actionStackSwap(SWFAppContext* app_context);
void actionDuplicate(SWFAppContext* app_context);
void actionGetMember(SWFAppContext* app_context);
void actionTargetPath(SWFAppContext* app_context, char* str_buffer);
void actionEnumerate(SWFAppContext* app_context, char* str_buffer);

// Movie control
void actionGoToLabel(SWFAppContext* app_context, const char* label);
void actionGotoFrame2(SWFAppContext* app_context, u8 play_flag, u16 scene_bias);

// Frame label lookup (returns -1 if not found, otherwise frame index)
int findFrameByLabel(const char* label);

void actionStringEquals(SWFAppContext* app_context, char* a_str, char* b_str);
void actionStringLength(SWFAppContext* app_context, char* v_str);
void actionStringExtract(SWFAppContext* app_context, char* str_buffer);
void actionMbStringLength(SWFAppContext* app_context, char* v_str);
void actionMbStringExtract(SWFAppContext* app_context, char* str_buffer);
void actionStringAdd(SWFAppContext* app_context, char* a_str, char* b_str);
void actionStringLess(SWFAppContext* app_context);
void actionImplementsOp(SWFAppContext* app_context);
void actionCharToAscii(SWFAppContext* app_context);

void actionGetVariable(SWFAppContext* app_context);
void actionSetVariable(SWFAppContext* app_context);
void actionSetTarget2(SWFAppContext* app_context);
void actionDefineLocal(SWFAppContext* app_context);
void actionDeclareLocal(SWFAppContext* app_context);
void actionGetProperty(SWFAppContext* app_context);
void actionSetProperty(SWFAppContext* app_context);
void actionCloneSprite(SWFAppContext* app_context);
void actionRemoveSprite(SWFAppContext* app_context);
void actionSetTarget(SWFAppContext* app_context, const char* target_name);

void actionNextFrame(SWFAppContext* app_context);
void actionPlay(SWFAppContext* app_context);
void actionGotoFrame(SWFAppContext* app_context, u16 frame);
void actionTrace(SWFAppContext* app_context);
void actionStartDrag(SWFAppContext* app_context);
void actionEndDrag(SWFAppContext* app_context);
void actionStopSounds(SWFAppContext* app_context);
void actionGetURL(SWFAppContext* app_context, const char* url, const char* target);
void actionRandomNumber(SWFAppContext* app_context);
void actionAsciiToChar(SWFAppContext* app_context, char* str_buffer);
void actionMbCharToAscii(SWFAppContext* app_context, char* str_buffer);
void actionGetTime(SWFAppContext* app_context);
void actionMbAsciiToChar(SWFAppContext* app_context, char* str_buffer);
void actionTypeof(SWFAppContext* app_context, char* str_buffer);
void actionCastOp(SWFAppContext* app_context);
void actionCallFunction(SWFAppContext* app_context, char* str_buffer);
void actionReturn(SWFAppContext* app_context);
void actionInitArray(SWFAppContext* app_context);
void actionInitObject(SWFAppContext* app_context);
void actionIncrement(SWFAppContext* app_context);
void actionDecrement(SWFAppContext* app_context);
void actionInstanceOf(SWFAppContext* app_context);
void actionEnumerate2(SWFAppContext* app_context, char* str_buffer);
void actionDelete(SWFAppContext* app_context);
void actionDelete2(SWFAppContext* app_context, char* str_buffer);
void actionBitAnd(SWFAppContext* app_context);
void actionBitOr(SWFAppContext* app_context);
void actionBitXor(SWFAppContext* app_context);
void actionBitLShift(SWFAppContext* app_context);
void actionBitRShift(SWFAppContext* app_context);
void actionBitURShift(SWFAppContext* app_context);
void actionStrictEquals(SWFAppContext* app_context);
void actionGreater(SWFAppContext* app_context);
void actionStringGreater(SWFAppContext* app_context);
void actionExtends(SWFAppContext* app_context);
void actionStoreRegister(SWFAppContext* app_context, u8 register_num);
void actionPushRegister(SWFAppContext* app_context, u8 register_num);
void actionDefineFunction(SWFAppContext* app_context, const char* name, void (*func)(SWFAppContext*), u32 param_count);
void actionCall(SWFAppContext* app_context);
void actionCallMethod(SWFAppContext* app_context, char* str_buffer);
void actionGetURL2(SWFAppContext* app_context, u8 send_vars_method, u8 load_target_flag, u8 load_variables_flag);
void actionSetMember(SWFAppContext* app_context);
void actionNewObject(SWFAppContext* app_context);
void actionNewMethod(SWFAppContext* app_context);

// Function pointer type for DefineFunction2
typedef ActionVar (*Function2Ptr)(SWFAppContext* app_context, ActionVar* args, u32 arg_count, ActionVar* registers, void* this_obj);

void actionDefineFunction2(SWFAppContext* app_context, const char* name, Function2Ptr func, u32 param_count, u8 register_count, u16 flags);
void actionWithStart(SWFAppContext* app_context);
void actionWithEnd(SWFAppContext* app_context);

// Exception handling (try-catch-finally)
void actionThrow(SWFAppContext* app_context);
void actionTryBegin(SWFAppContext* app_context);
bool actionTryExecute(SWFAppContext* app_context);
jmp_buf* actionGetExceptionJmpBuf(SWFAppContext* app_context);
void actionCatchToVariable(SWFAppContext* app_context, const char* var_name);
void actionCatchToRegister(SWFAppContext* app_context, u8 reg_num);
void actionTryEnd(SWFAppContext* app_context);

// Macro for inline setjmp in generated code
#define ACTION_TRY_SETJMP(app_context) setjmp(*actionGetExceptionJmpBuf(app_context))

// Control flow
int evaluateCondition(SWFAppContext* app_context);
bool actionWaitForFrame(SWFAppContext* app_context, u16 frame);
bool actionWaitForFrame2(SWFAppContext* app_context);
