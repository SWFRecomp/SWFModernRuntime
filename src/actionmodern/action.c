#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>

// constants.h is generated per-test and contains SWF_FRAME_COUNT
// It's optional - if not present, SWF_FRAME_COUNT defaults are used
#ifdef __has_include
#  if __has_include("constants.h")
#    include "constants.h"
#  endif
#endif

#include <recomp.h>
#include <utils.h>
#include <swf.h>
#include <heap.h>
#include <actionmodern/object.h>

u32 start_time;

// ==================================================================
// Scope Chain for WITH statement
// ==================================================================

#define MAX_SCOPE_DEPTH 32
static ASObject* scope_chain[MAX_SCOPE_DEPTH];
static u32 scope_depth = 0;

// ==================================================================
// Function Storage and Management
// ==================================================================

// Function pointer types
typedef void (*SimpleFunctionPtr)(SWFAppContext* app_context);
typedef ActionVar (*Function2Ptr)(SWFAppContext* app_context, ActionVar* args, u32 arg_count, ActionVar* registers, void* this_obj);

// Function object structure
typedef struct ASFunction {
	char name[256];           // Function name (can be empty for anonymous)
	u8 function_type;         // 1 = simple (DefineFunction), 2 = advanced (DefineFunction2)
	u32 param_count;          // Number of parameters
	
	// For DefineFunction (type 1)
	SimpleFunctionPtr simple_func;
	
	// For DefineFunction2 (type 2)
	Function2Ptr advanced_func;
	u8 register_count;
	u16 flags;
} ASFunction;

// Function registry
#define MAX_FUNCTIONS 256
static ASFunction* function_registry[MAX_FUNCTIONS];
static u32 function_count = 0;

// Helper to look up function by name
static ASFunction* lookupFunctionByName(const char* name, u32 name_len) {
	for (u32 i = 0; i < function_count; i++) {
		if (strlen(function_registry[i]->name) == name_len &&
		    strncmp(function_registry[i]->name, name, name_len) == 0) {
			return function_registry[i];
		}
	}
	return NULL;
}

// Helper to look up function from ActionVar
static ASFunction* lookupFunctionFromVar(ActionVar* var) {
	if (var->type != ACTION_STACK_VALUE_FUNCTION) {
		return NULL;
	}
	return (ASFunction*) var->value;
}

void initTime(SWFAppContext* app_context)
{
	start_time = get_elapsed_ms();
	
	// Initialize global object if not already initialized
	if (global_object == NULL) {
		global_object = allocObject(app_context, 16);  // Start with capacity for 16 global properties
	}
}

// ==================================================================
// Display Control Operations
// ==================================================================

void actionToggleQuality(SWFAppContext* app_context)
{
	// In NO_GRAPHICS mode, this is a no-op
	// In full graphics mode, this would toggle between high and low quality rendering
	// affecting anti-aliasing, smoothing, etc.
	
	#ifdef DEBUG
	printf("[ActionToggleQuality] Toggled render quality\n");
	#endif
}

// ==================================================================
// avmplus-compatible Random Number Generator
// Based on Adobe's ActionScript VM (avmplus) implementation
// Source: https://github.com/adobe/avmplus/blob/master/core/MathUtils.cpp
// ==================================================================

typedef struct {
	uint32_t uValue;           // Random result and seed for next random result
	uint32_t uXorMask;         // XOR mask for generating the next random value
	uint32_t uSequenceLength;  // Number of values in the sequence
} TRandomFast;

#define kRandomPureMax 0x7FFFFFFFL

// XOR masks for random number generation (generates 2^n - 1 numbers)
static const uint32_t Random_Xor_Masks[31] = {
	0x00000003L, 0x00000006L, 0x0000000CL, 0x00000014L, 0x00000030L, 0x00000060L, 0x000000B8L, 0x00000110L,
	0x00000240L, 0x00000500L, 0x00000CA0L, 0x00001B00L, 0x00003500L, 0x00006000L, 0x0000B400L, 0x00012000L,
	0x00020400L, 0x00072000L, 0x00090000L, 0x00140000L, 0x00300000L, 0x00400000L, 0x00D80000L, 0x01200000L,
	0x03880000L, 0x07200000L, 0x09000000L, 0x14000000L, 0x32800000L, 0x48000000L, 0xA3000000L
};

// Global RNG state (initialized on first use or at startup)
static TRandomFast global_random_state = {0, 0, 0};

// Initialize the random number generator with a seed
static void RandomFastInit(TRandomFast *pRandomFast, uint32_t seed) {
	int32_t n = 31;
	pRandomFast->uValue = seed;
	pRandomFast->uSequenceLength = (1L << n) - 1L;
	pRandomFast->uXorMask = Random_Xor_Masks[n - 2];
}

// Generate next random value using XOR shift
static int32_t RandomFastNext(TRandomFast *pRandomFast) {
	if (pRandomFast->uValue & 1L) {
		pRandomFast->uValue = (pRandomFast->uValue >> 1L) ^ pRandomFast->uXorMask;
	} else {
		pRandomFast->uValue >>= 1L;
	}
	return (int32_t)pRandomFast->uValue;
}

// Hash function for additional randomness
static int32_t RandomPureHasher(int32_t iSeed) {
	const int32_t c1 = 1376312589L;
	const int32_t c2 = 789221L;
	const int32_t c3 = 15731L;
	
	iSeed = ((iSeed << 13) ^ iSeed) - (iSeed >> 21);
	int32_t iResult = (iSeed * (iSeed * iSeed * c3 + c2) + c1) & kRandomPureMax;
	iResult += iSeed;
	iResult = ((iResult << 13) ^ iResult) - (iResult >> 21);
	
	return iResult;
}

// Generate a random number (avmplus implementation)
static int32_t GenerateRandomNumber(TRandomFast *pRandomFast) {
	// Initialize if needed (first call or uninitialized)
	if (pRandomFast->uValue == 0) {
		// Use time-based seed for first initialization
		RandomFastInit(pRandomFast, (uint32_t)time(NULL));
	}
	
	int32_t aNum = RandomFastNext(pRandomFast);
	aNum = RandomPureHasher(aNum * 71L);
	return aNum & kRandomPureMax;
}

// AS2 random(max) function - returns integer in range [0, max)
static int32_t Random(int32_t range, TRandomFast *pRandomFast) {
	if (range <= 0) {
		return 0;
	}
	
	int32_t randomNumber = GenerateRandomNumber(pRandomFast);
	return randomNumber % range;
}

// ==================================================================
// Global object for ActionScript _global
// This is initialized on first use and persists for the lifetime of the runtime
ASObject* global_object = NULL;

ActionStackValueType convertString(SWFAppContext* app_context, char* var_str)
{
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_F32)
	{
		float temp_val = VAL(float, &STACK_TOP_VALUE);  // Save the float value first!
		STACK_TOP_TYPE = ACTION_STACK_VALUE_STRING;
		VAL(u64, &STACK_TOP_VALUE) = (u64) var_str;
		snprintf(var_str, 17, "%.15g", temp_val);  // Use the saved value
	}
	
	return ACTION_STACK_VALUE_STRING;
}

ActionStackValueType convertFloat(SWFAppContext* app_context)
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

ActionStackValueType convertDouble(SWFAppContext* app_context)
{
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_F32)
	{
		double temp = VAL(double, &STACK_TOP_VALUE);
		STACK_TOP_TYPE = ACTION_STACK_VALUE_F64;
		VAL(u64, &STACK_TOP_VALUE) = VAL(u64, &temp);
	}
	
	return ACTION_STACK_VALUE_F64;
}

void pushVar(SWFAppContext* app_context, ActionVar* var)
{
	switch (var->type)
	{
		case ACTION_STACK_VALUE_F32:
		case ACTION_STACK_VALUE_F64:
		case ACTION_STACK_VALUE_UNDEFINED:
		case ACTION_STACK_VALUE_OBJECT:
		case ACTION_STACK_VALUE_FUNCTION:
		{
			PUSH(var->type, var->value);
			
			break;
		}
		
		case ACTION_STACK_VALUE_STRING:
		{
			// Use heap pointer if variable owns memory, otherwise use numeric_value as pointer
			char* str_ptr = var->owns_memory ?
				var->heap_ptr :
				(char*) var->value;
				
			PUSH_STR_ID(str_ptr, var->str_size, var->string_id);
			
			break;
		}
	}
}

void peekVar(SWFAppContext* app_context, ActionVar* var)
{
	var->type = STACK_TOP_TYPE;
	var->str_size = STACK_TOP_N;
	
	if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STR_LIST)
	{
		var->value = (u64) &STACK_TOP_VALUE;
		var->string_id = 0;  // String lists don't have IDs
	}
	else if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING)
	{
		// For strings, store pointer and mark as not owning memory (it's on the stack)
		var->value = VAL(u64, &STACK_TOP_VALUE);
		var->heap_ptr = (char*) var->value;
		var->owns_memory = false;
		var->string_id = VAL(u32, &STACK[SP + 12]);  // Read string_id from stack
	}
	else
	{
		var->value = VAL(u64, &STACK_TOP_VALUE);
		var->string_id = 0;  // Non-string types don't have IDs
	}
	
	// Initialize owns_memory to false for non-heap strings
	// (When the value is in numeric_value, not string_data.heap_ptr)
	if (var->type == ACTION_STACK_VALUE_STRING)
	{
		var->owns_memory = false;
	}
}

void popVar(SWFAppContext* app_context, ActionVar* var)
{
	peekVar(app_context, var);
	
	POP();
}

void peekSecondVar(SWFAppContext* app_context, ActionVar* var)
{
	u32 second_sp = SP_SECOND_TOP;
	var->type = STACK[second_sp];
	var->str_size = VAL(u32, &STACK[second_sp + 8]);
	
	if (STACK[second_sp] == ACTION_STACK_VALUE_STR_LIST)
	{
		var->value = (u64) &VAL(u64, &STACK[second_sp + 16]);
		var->string_id = 0;
	}
	else if (STACK[second_sp] == ACTION_STACK_VALUE_STRING)
	{
		var->value = VAL(u64, &STACK[second_sp + 16]);
		var->heap_ptr = (char*) var->value;
		var->owns_memory = false;
		var->string_id = VAL(u32, &STACK[second_sp + 12]);
	}
	else
	{
		var->value = VAL(u64, &STACK[second_sp + 16]);
		var->string_id = 0;
	}
	
	if (var->type == ACTION_STACK_VALUE_STRING)
	{
		var->owns_memory = false;
	}
}

void actionPrevFrame(SWFAppContext* app_context)
{
	// Suppress unused parameter warning
	(void)app_context;
	
	// Access global frame control variables
	extern size_t current_frame;
	extern size_t next_frame;
	extern int manual_next_frame;
	
	// Move to previous frame if not already at first frame
	if (current_frame > 0)
	{
		next_frame = current_frame - 1;
		manual_next_frame = 1;
	}
	// If already at frame 0, do nothing (stay on current frame)
}

void actionAdd(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
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

void actionAdd2(SWFAppContext* app_context, char* str_buffer)
{
	// Peek at types without popping
	u8 type_a = STACK_TOP_TYPE;
	
	// Move to second value
	u32 sp_second = VAL(u32, &(STACK[SP + 4]));  // Get previous_sp
	u8 type_b = STACK[sp_second];  // Type of second value
	
	// Check if either operand is a string
	if (type_a == ACTION_STACK_VALUE_STRING || type_b == ACTION_STACK_VALUE_STRING) {
		// String concatenation path
		
		// Convert first operand to string (top of stack - right operand)
		char str_a[17];
		convertString(app_context, str_a);
		// Get the string pointer (either str_a if converted, or original if already string)
		const char* str_a_ptr = (const char*) VAL(u64, &STACK_TOP_VALUE);
		POP();
		
		// Convert second operand to string (second on stack - left operand)
		char str_b[17];
		convertString(app_context, str_b);
		// Get the string pointer
		const char* str_b_ptr = (const char*) VAL(u64, &STACK_TOP_VALUE);
		POP();
		
		// Concatenate (left + right = b + a)
		snprintf(str_buffer, 17, "%s%s", str_b_ptr, str_a_ptr);
		
		// Push result
		PUSH_STR(str_buffer, strlen(str_buffer));
	} else {
		// Numeric addition path
		
		// Convert and pop first operand
		convertFloat(app_context);
		ActionVar a;
		popVar(app_context, &a);
		
		// Convert and pop second operand
		convertFloat(app_context);
		ActionVar b;
		popVar(app_context, &b);
		
		// Perform addition (same logic as actionAdd)
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
}

void actionSubtract(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		double c = b_val - a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		double c = b_val - a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) - VAL(float, &a.value);
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionMultiply(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		double c = b_val*a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		double c = b_val*a_val;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value)*VAL(float, &a.value);
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionDivide(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
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

void actionModulo(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	if (VAL(float, &a.value) == 0.0f)
	{
		// SWF 4: Division by zero returns error string
		PUSH_STR("#ERROR#", 8);
	}
	
	else
	{
		if (a.type == ACTION_STACK_VALUE_F64)
		{
			double a_val = VAL(double, &a.value);
			double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
			
			double c = fmod(b_val, a_val);
			PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
		}
		
		else if (b.type == ACTION_STACK_VALUE_F64)
		{
			double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
			double b_val = VAL(double, &b.value);
			
			double c = fmod(b_val, a_val);
			PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
		}
		
		else
		{
			float c = fmodf(VAL(float, &b.value), VAL(float, &a.value));
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
		}
	}
}

void actionEquals(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
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

void actionLess(SWFAppContext* app_context)
{
	ActionVar a;
	convertFloat(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertFloat(app_context);
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		float c = b_val < a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		float c = b_val < a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) < VAL(float, &a.value) ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionLess2(SWFAppContext* app_context)
{
	ActionVar a;
	convertFloat(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertFloat(app_context);
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		float c = b_val < a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		float c = b_val < a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) < VAL(float, &a.value) ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionGreater(SWFAppContext* app_context)
{
	ActionVar a;
	convertFloat(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertFloat(app_context);
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		float c = b_val > a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		float c = b_val > a_val ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) > VAL(float, &a.value) ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionAnd(SWFAppContext* app_context)
{
	ActionVar a;
	convertFloat(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertFloat(app_context);
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		float c = b_val != 0.0 && a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		float c = b_val != 0.0 && a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) != 0.0f && VAL(float, &a.value) != 0.0f ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionOr(SWFAppContext* app_context)
{
	ActionVar a;
	convertFloat(app_context);
	popVar(app_context, &a);
	
	ActionVar b;
	convertFloat(app_context);
	popVar(app_context, &b);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = VAL(double, &a.value);
		double b_val = b.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &b.value) : VAL(double, &b.value);
		
		float c = b_val != 0.0 || a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else if (b.type == ACTION_STACK_VALUE_F64)
	{
		double a_val = a.type == ACTION_STACK_VALUE_F32 ? (double) VAL(float, &a.value) : VAL(double, &a.value);
		double b_val = VAL(double, &b.value);
		
		float c = b_val != 0.0 || a_val != 0.0 ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &c));
	}
	
	else
	{
		float c = VAL(float, &b.value) != 0.0f || VAL(float, &a.value) != 0.0f ? 1.0f : 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &c));
	}
}

void actionNot(SWFAppContext* app_context)
{
	ActionVar v;
	convertFloat(app_context);
	popVar(app_context, &v);
	
	float result = v.value == 0.0f ? 1.0f : 0.0f;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u64, &result));
}

void actionToInteger(SWFAppContext* app_context)
{
	ActionVar v;
	convertFloat(app_context);
	popVar(app_context, &v);
	
	float f = VAL(float, &v.value);
	
	// Handle special values: NaN and Infinity -> 0
	if (isnan(f) || isinf(f)) {
		f = 0.0f;
	} else {
		// Convert to 32-bit signed integer (truncate toward zero)
		int32_t int_value = (int32_t)f;
		// Convert back to float for pushing
		f = (float)int_value;
	}
	
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &f));
}

void actionToNumber(SWFAppContext* app_context)
{
	// Convert top of stack to number
	// convertFloat() handles all type conversions:
	// - Number: return as-is
	// - String: parse as number (empty→0, invalid→NaN)
	// - Boolean: true→1, false→0
	// - Null/undefined: NaN
	convertFloat(app_context);
	// Value is already converted on stack in-place
}

void actionToString(SWFAppContext* app_context, char* str_buffer)
{
	// Convert top of stack to string
	// If already string, this does nothing
	// If float, converts using snprintf with %.15g format
	convertString(app_context, str_buffer);
}

void actionStackSwap(SWFAppContext* app_context)
{
	// Pop top value (value1)
	ActionVar val1;
	popVar(app_context, &val1);
	
	// Pop second value (value2)
	ActionVar val2;
	popVar(app_context, &val2);
	
	// Push value1 (was on top, now goes to second position)
	pushVar(app_context, &val1);
	
	// Push value2 (was second, now goes to top)
	pushVar(app_context, &val2);
}

/**
 * Helper structure to track enumerated property names
 * Used to prevent duplicates when walking the prototype chain
 */
typedef struct EnumeratedName {
	const char* name;
	u32 name_length;
	struct EnumeratedName* next;
} EnumeratedName;

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
	// Step 1: Pop variable name from stack
	// Stack layout for strings: +0=type, +4=oldSP, +8=length, +12=string_id, +16=pointer
	u32 string_id = VAL(u32, &STACK[SP + 12]);
	char* var_name = (char*) VAL(u64, &STACK[SP + 16]);
	u32 var_name_len = VAL(u32, &STACK[SP + 8]);
	POP();
	
#ifdef DEBUG
	printf("[DEBUG] actionEnumerate: looking up variable '%.*s' (len=%u, id=%u)\n",
	       var_name_len, var_name, var_name_len, string_id);
#endif

	// Step 2: Look up the variable
	ActionVar* var = NULL;
	if (string_id > 0)
	{
		// Constant string - use array lookup (O(1))
		var = getVariableById(app_context, string_id);
	}
	else
	{
		// Dynamic string - use hashmap (O(n))
		var = getVariable(app_context, var_name, var_name_len);
	}
	
	// Step 3: Check if variable exists and is an object
	if (!var || var->type != ACTION_STACK_VALUE_OBJECT)
	{
#ifdef DEBUG
		if (!var)
			printf("[DEBUG] actionEnumerate: variable not found\n");
		else
			printf("[DEBUG] actionEnumerate: variable is not an object (type=%d)\n", var->type);
#endif
		// Variable not found or not an object - push null terminator only
		PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
		return;
	}
	
	// Step 4: Get the object from the variable
	ASObject* obj = (ASObject*) VAL(u64, &var->value);
	if (obj == NULL)
	{
#ifdef DEBUG
		printf("[DEBUG] actionEnumerate: object pointer is NULL\n");
#endif
		// Null object - push null terminator only
		PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
		return;
	}
	
	// Step 5: Collect all enumerable properties from the entire prototype chain
	// We need to collect them first to push in reverse order
	
	// Temporary storage for property names (we'll push them to stack after collecting)
	typedef struct PropList {
		const char* name;
		u32 name_length;
		struct PropList* next;
	} PropList;
	
	PropList* prop_head = NULL;
	u32 total_props = 0;
	
	// Track which properties we've already seen (to handle shadowing)
	EnumeratedName* enumerated_head = NULL;
	
	// Walk the prototype chain
	ASObject* current_obj = obj;
	int chain_depth = 0;
	const int MAX_CHAIN_DEPTH = 100; // Prevent infinite loops
	
	while (current_obj != NULL && chain_depth < MAX_CHAIN_DEPTH)
	{
		chain_depth++;
		
#ifdef DEBUG
		printf("[DEBUG] actionEnumerate: walking prototype chain depth=%d, num_used=%u\n",
		       chain_depth, current_obj->num_used);
#endif

		// Enumerate properties from this level
		for (u32 i = 0; i < current_obj->num_used; i++)
		{
			const char* prop_name = current_obj->properties[i].name;
			u32 prop_name_len = current_obj->properties[i].name_length;
			u8 prop_flags = current_obj->properties[i].flags;
			
			// Skip if property is not enumerable (DontEnum)
			if (!(prop_flags & PROPERTY_FLAG_ENUMERABLE))
			{
#ifdef DEBUG
				printf("[DEBUG] actionEnumerate: skipping non-enumerable property '%.*s'\n",
				       prop_name_len, prop_name);
#endif
				continue;
			}
			
			// Skip if we've already enumerated this property name (shadowing)
			if (isPropertyEnumerated(enumerated_head, prop_name, prop_name_len))
			{
#ifdef DEBUG
				printf("[DEBUG] actionEnumerate: skipping shadowed property '%.*s'\n",
				       prop_name_len, prop_name);
#endif
				continue;
			}
			
			// Add to enumerated list
			addEnumeratedName(&enumerated_head, prop_name, prop_name_len);
			
			// Add to property list (for later pushing to stack)
			PropList* node = (PropList*) malloc(sizeof(PropList));
			if (node != NULL)
			{
				node->name = prop_name;
				node->name_length = prop_name_len;
				node->next = prop_head;
				prop_head = node;
				total_props++;
				
#ifdef DEBUG
				printf("[DEBUG] actionEnumerate: added enumerable property '%.*s'\n",
				       prop_name_len, prop_name);
#endif
			}
		}
		
		// Move to prototype via __proto__ property
		ActionVar* proto_var = getProperty(current_obj, "__proto__", 9);
		if (proto_var != NULL && proto_var->type == ACTION_STACK_VALUE_OBJECT)
		{
			current_obj = (ASObject*) proto_var->value;
#ifdef DEBUG
			printf("[DEBUG] actionEnumerate: following __proto__ to next level\n");
#endif
		}
		else
		{
			// End of prototype chain
			current_obj = NULL;
		}
	}
	
	// Free the enumerated names list
	freeEnumeratedNames(enumerated_head);
	
#ifdef DEBUG
	printf("[DEBUG] actionEnumerate: collected %u enumerable properties total\n", total_props);
#endif

	// Step 6: Push null terminator first
	// This marks the end of the enumeration for for..in loops
	PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
	
	// Step 7: Push property names from the list (they're already in reverse order)
	while (prop_head != NULL)
	{
		PUSH_STR((char*)prop_head->name, prop_head->name_length);
		
		PropList* next = prop_head->next;
		free(prop_head);
		prop_head = next;
	}
}


int evaluateCondition(SWFAppContext* app_context)
{
	ActionVar v;
	convertFloat(app_context);
	popVar(app_context, &v);
	
	return v.value != 0.0f;
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

void actionStringEquals(SWFAppContext* app_context, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(app_context, a_str);
	popVar(app_context, &a);
	
	ActionVar b;
	convertString(app_context, b_str);
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
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionStringLength(SWFAppContext* app_context, char* v_str)
{
	ActionVar v;
	convertString(app_context, v_str);
	popVar(app_context, &v);
	
	float str_size = (float) v.str_size;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &str_size));
}

void actionStringExtract(SWFAppContext* app_context, char* str_buffer)
{
	// Pop length
	convertFloat(app_context);
	ActionVar length_var;
	popVar(app_context, &length_var);
	int length = (int)VAL(float, &length_var.value);
	
	// Pop index
	convertFloat(app_context);
	ActionVar index_var;
	popVar(app_context, &index_var);
	int index = (int)VAL(float, &index_var.value);
	
	// Pop string
	char src_buffer[17];
	convertString(app_context, src_buffer);
	ActionVar src_var;
	popVar(app_context, &src_var);
	const char* src = src_var.owns_memory ?
		src_var.heap_ptr :
		(char*) src_var.value;
		
	// Get source string length
	int src_len = src_var.str_size;
	
	// Handle out-of-bounds index
	if (index < 0) index = 0;
	if (index >= src_len) {
		str_buffer[0] = '\0';
		PUSH_STR(str_buffer, 0);
		return;
	}
	
	// Handle out-of-bounds length
	if (length < 0) length = 0;
	if (index + length > src_len) {
		length = src_len - index;
	}
	
	// Extract substring
	int i;
	for (i = 0; i < length && i < 16; i++) {  // Limit to buffer size
		str_buffer[i] = src[index + i];
	}
	str_buffer[i] = '\0';
	
	// Push result
	PUSH_STR(str_buffer, i);
}

void actionMbStringLength(SWFAppContext* app_context, char* v_str)
{
	// Convert top of stack to string (if it's a number, converts it to string in v_str)
	convertString(app_context, v_str);
	
	// Get the string pointer from stack
	const unsigned char* str = (const unsigned char*) VAL(u64, &STACK_TOP_VALUE);
	
	// Pop the string value
	POP();
	
	// Count UTF-8 characters
	int count = 0;
	while (*str != '\0') {
		// Check UTF-8 sequence length
		if ((*str & 0x80) == 0) {
			// 1-byte sequence (0xxxxxxx)
			str += 1;
		} else if ((*str & 0xE0) == 0xC0) {
			// 2-byte sequence (110xxxxx)
			str += 2;
		} else if ((*str & 0xF0) == 0xE0) {
			// 3-byte sequence (1110xxxx)
			str += 3;
		} else if ((*str & 0xF8) == 0xF0) {
			// 4-byte sequence (11110xxx)
			str += 4;
		} else {
			// Invalid UTF-8, skip one byte
			str += 1;
		}
		count++;
	}
	
	// Push result
	float result = (float)count;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionMbStringExtract(SWFAppContext* app_context, char* str_buffer)
{
	// Pop count (number of characters to extract)
	convertFloat(app_context);
	ActionVar count_var;
	popVar(app_context, &count_var);
	int count = (int)VAL(float, &count_var.value);
	
	// Pop index (starting character position)
	convertFloat(app_context);
	ActionVar index_var;
	popVar(app_context, &index_var);
	int index = (int)VAL(float, &index_var.value);
	
	// Pop string
	char input_buffer[17];
	convertString(app_context, input_buffer);
	ActionVar src_var;
	popVar(app_context, &src_var);
	const char* src = src_var.owns_memory ?
		src_var.heap_ptr :
		(char*) src_var.value;
		
	// If index or count are invalid, return empty string
	if (index < 0 || count < 0) {
		str_buffer[0] = '\0';
		PUSH_STR(str_buffer, 0);
		return;
	}
	
	// Navigate to starting character position (UTF-8 aware)
	const unsigned char* str = (const unsigned char*)src;
	int current_char = 0;
	
	// Skip to index'th character
	while (*str != '\0' && current_char < index) {
		// Advance by one UTF-8 character
		if ((*str & 0x80) == 0) {
			str += 1;  // 1-byte character
		} else if ((*str & 0xE0) == 0xC0) {
			str += 2;  // 2-byte character
		} else if ((*str & 0xF0) == 0xE0) {
			str += 3;  // 3-byte character
		} else if ((*str & 0xF8) == 0xF0) {
			str += 4;  // 4-byte character
		} else {
			str += 1;  // Invalid, skip one byte
		}
		current_char++;
	}
	
	// If we reached end of string before index, return empty
	if (*str == '\0') {
		str_buffer[0] = '\0';
		PUSH_STR(str_buffer, 0);
		return;
	}
	
	// Extract count characters
	const unsigned char* start = str;
	current_char = 0;
	
	while (*str != '\0' && current_char < count) {
		// Advance by one UTF-8 character
		if ((*str & 0x80) == 0) {
			str += 1;
		} else if ((*str & 0xE0) == 0xC0) {
			str += 2;
		} else if ((*str & 0xF0) == 0xE0) {
			str += 3;
		} else if ((*str & 0xF8) == 0xF0) {
			str += 4;
		} else {
			str += 1;
		}
		current_char++;
	}
	
	// Copy substring to buffer
	int length = str - start;
	if (length > 16) length = 16;  // Buffer size limit
	memcpy(str_buffer, start, length);
	str_buffer[length] = '\0';
	
	// Push result
	PUSH_STR(str_buffer, length);
}

void actionCharToAscii(SWFAppContext* app_context)
{
	// Convert top of stack to string
	char str_buffer[17];
	convertString(app_context, str_buffer);
	
	// Pop the string value
	ActionVar v;
	popVar(app_context, &v);
	
	// Get pointer to the string
	const char* str = (const char*) v.value;
	
	// Handle empty string edge case
	if (str == NULL || str[0] == '\0' || v.str_size == 0) {
		// Push NaN for empty string (Flash behavior)
		float result = 0.0f / 0.0f;  // NaN
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return;
	}
	
	// Get ASCII/Unicode code of first character
	// Use unsigned char to ensure values 128-255 are handled correctly
	float code = (float)(unsigned char)str[0];
	
	// Push result
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &code));
}

void actionStringAdd(SWFAppContext* app_context, char* a_str, char* b_str)
{
	ActionVar a;
	convertString(app_context, a_str);
	peekVar(app_context, &a);
	
	ActionVar b;
	convertString(app_context, b_str);
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

// ==================================================================
// MovieClip Control Actions
// ==================================================================

void actionNextFrame(SWFAppContext* app_context)
{
	(void)app_context;  // Not used but required for consistent API
	// Advance to the next frame
	extern size_t current_frame;
	extern size_t next_frame;
	extern int manual_next_frame;
	
	next_frame = current_frame + 1;
	manual_next_frame = 1;
}

void actionTrace(SWFAppContext* app_context)
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
		
		case ACTION_STACK_VALUE_UNDEFINED:
		{
			printf("undefined\n");
			break;
		}
	}
	
	fflush(stdout);
	
	POP();
}

/**
 * ActionGotoFrame - Go to specified frame and stop
 *
 * Opcode: 0x81
 * SWF Version: 3+
 *
 * Jumps to the specified frame in the timeline and stops playback.
 * This implements the "gotoAndStop" semantics - the timeline will
 * jump to the target frame and halt there.
 *
 * Frame indexing: The frame parameter is 0-based (frame 0 is the first frame).
 *
 * Behavior:
 * - Sets next_frame to the specified frame index
 * - Sets manual_next_frame flag to trigger the jump
 * - Sets is_playing = 0 to stop playback at the target frame
 * - Validates frame boundaries (frame must be < g_frame_count)
 * - If frame is out of bounds, ignores the jump (continues current frame)
 *
 * @param stack - Pointer to the runtime stack (unused but required for API consistency)
 * @param sp - Pointer to stack pointer (unused but required for API consistency)
 * @param frame - Target frame index (0-based)
 */
void actionGotoFrame(SWFAppContext* app_context, u16 frame)
{
	// Suppress unused parameter warnings
	(void)app_context;
	
	// Access global frame control variables
	extern size_t current_frame;
	extern size_t next_frame;
	extern int manual_next_frame;
	extern int is_playing;
	extern size_t g_frame_count;
	
	// Frame boundary validation
	// If the target frame is out of bounds, ignore the jump
	if (frame >= g_frame_count)
	{
		// Target frame doesn't exist - no-op
		// Continue execution in current frame
		return;
	}
	
	// Set the next frame to the specified frame index
	next_frame = frame;
	
	// Signal manual frame navigation (overrides automatic playback advancement)
	manual_next_frame = 1;
	
	// Stop playback at the target frame (gotoAndStop semantics)
	// This is the key difference from just advancing the frame counter
	is_playing = 0;
}

/**
 * findFrameByLabel - Lookup frame number by label
 *
 * Searches the frame_label_data array (generated by SWFRecomp) for a matching label.
 * Returns the frame index if found, -1 otherwise.
 *
 * @param label - The frame label to search for
 * @return Frame index (0-based) or -1 if not found
 */
int findFrameByLabel(const char* label)
{
	if (!label)
	{
		return -1;
	}
	
	// Extern declarations for generated frame label data
	typedef struct {
		const char* label;
		size_t frame;
	} FrameLabelEntry;
	
	extern FrameLabelEntry frame_label_data[];
	extern size_t frame_label_count;
	
	// Search through frame labels
	for (size_t i = 0; i < frame_label_count; i++)
	{
		if (frame_label_data[i].label && strcmp(frame_label_data[i].label, label) == 0)
		{
			return (int)frame_label_data[i].frame;
		}
	}
	
	return -1;  // Not found
}

/**
 * ActionGoToLabel - Navigate to a frame by its label
 *
 * Looks up the frame number associated with the specified label and jumps to that frame.
 * If the label is not found, the action is ignored (per Flash spec).
 *
 * Frame labels are defined in the SWF file using FrameLabel tags and extracted
 * by SWFRecomp during compilation.
 *
 * @param stack - The execution stack (unused)
 * @param sp - Stack pointer (unused)
 * @param label - The frame label to navigate to
 */
void actionGoToLabel(SWFAppContext* app_context, const char* label)
{
	extern size_t next_frame;
	extern int manual_next_frame;
	extern int is_playing;
	
	// Debug output
	printf("// GoToLabel: %s\n", label ? label : "(null)");
	fflush(stdout);
	
	if (!label)
	{
		return;
	}
	
	// Look up frame by label
	int frame_index = findFrameByLabel(label);
	
	if (frame_index >= 0)
	{
		// Navigate to the frame
		next_frame = (size_t)frame_index;
		manual_next_frame = 1;
		
		// Stop playback (like gotoAndStop)
		is_playing = 0;
		
		// Note: Actual navigation will occur in the frame loop
	}
	// If label not found, ignore (per Flash spec - no action taken)
}

/**
 * ActionGotoFrame2 - Stack-based frame navigation
 *
 * Stack: [ frame_identifier ] -> [ ]
 *
 * Pops a frame identifier (number or string) from the stack and navigates
 * to that frame. The Play flag controls whether to stop or continue playing.
 *
 * Frame identifier can be:
 * - A number: Frame index (0-based)
 * - A string: Frame label, optionally prefixed with target path (e.g., "/MovieClip:label")
 *
 * Edge cases:
 * - Negative frame numbers: Treated as frame 0
 * - Invalid frame types: Ignored with warning
 * - Nonexistent labels: Ignored (spec says action is ignored)
 * - Target paths: Parsed but not fully supported in NO_GRAPHICS mode
 *
 * SWF version: 4+
 * Opcode: 0x9F
 *
 * @param stack Pointer to the runtime stack
 * @param sp Pointer to stack pointer
 * @param play_flag 0 = go to frame and stop, 1 = go to frame and play
 * @param scene_bias Number to add to numeric frame (for multi-scene movies)
 */
void actionGotoFrame2(SWFAppContext* app_context, u8 play_flag, u16 scene_bias)
{
	// Pop frame identifier from stack
	ActionVar frame_var;
	popVar(app_context, &frame_var);
	
	if (frame_var.type == ACTION_STACK_VALUE_F32) {
		// Numeric frame
		float frame_float;
		memcpy(&frame_float, &frame_var.value, sizeof(float));
		
		// Handle negative frames (treat as 0)
		s32 frame_num = (s32)frame_float;
		if (frame_num < 0) {
			frame_num = 0;
		}
		
		// Apply scene bias
		frame_num += scene_bias;
		
		printf("GotoFrame2: frame %d (play=%d)\n", frame_num, play_flag);
		fflush(stdout);
		
		// Note: Actual frame navigation requires MovieClip structure and frame management
		// In NO_GRAPHICS mode, we just log the navigation
	}
	else if (frame_var.type == ACTION_STACK_VALUE_STRING) {
		// Frame label - may include target path
		const char* frame_str = (const char*)frame_var.value;
		
		if (frame_str == NULL) {
			printf("GotoFrame2: null label (ignored)\n");
			fflush(stdout);
			return;
		}
		
		// Parse target path if present (format: "target:frame" or "/target:frame")
		const char* target = NULL;
		const char* frame_part = frame_str;
		const char* colon = strchr(frame_str, ':');
		
		if (colon != NULL) {
			// Target path present
			size_t target_len = colon - frame_str;
			static char target_buffer[256];
			
			if (target_len < sizeof(target_buffer)) {
				memcpy(target_buffer, frame_str, target_len);
				target_buffer[target_len] = '\0';
				target = target_buffer;
				frame_part = colon + 1;  // Frame label/number after the colon
			}
		}
		
		// Check if frame_part is numeric or a label
		char* endptr;
		long frame_num = strtol(frame_part, &endptr, 10);
		
		if (endptr != frame_part && *endptr == '\0') {
			// It's a numeric frame
			if (frame_num < 0) {
				frame_num = 0;
			}
			
			if (target) {
				printf("GotoFrame2: target '%s', frame %ld (play=%d)\n", target, frame_num, play_flag);
			} else {
				printf("GotoFrame2: frame %ld (play=%d)\n", frame_num, play_flag);
			}
		} else {
			// It's a frame label
			if (target) {
				printf("GotoFrame2: target '%s', label '%s' (play=%d)\n", target, frame_part, play_flag);
			} else {
				printf("GotoFrame2: label '%s' (play=%d)\n", frame_part, play_flag);
			}
		}
		
		fflush(stdout);
		
		// Note: Frame label lookup and navigation requires:
		// - Frame label registry (mapping labels to frame numbers)
		// - MovieClip context switching for target paths
		// In NO_GRAPHICS mode, we just log the navigation
	}
	else if (frame_var.type == ACTION_STACK_VALUE_UNDEFINED) {
		// Undefined - ignore
		printf("GotoFrame2: undefined frame (ignored)\n");
		fflush(stdout);
	}
	else {
		// Invalid type - ignore with warning
		printf("GotoFrame2: invalid frame type %d (ignored)\n", frame_var.type);
		fflush(stdout);
	}
}

/**
 * ActionStopSounds - Stops all currently playing sounds
 *
 * Stack: [ ... ] -> [ ... ] (no stack changes)
 *
 * Instructs Flash Player to stop playing all sounds. This operation:
 * - Stops all currently playing audio across all timelines
 * - Has global effect (not affected by SetTarget)
 * - Does not prevent new sounds from playing
 * - Has no effect on the stack
 * - Has no parameters
 *
 * Implementation notes:
 * - NO_GRAPHICS mode: This is a no-op (no audio system available)
 * - Full graphics mode: Would interface with audio subsystem to stop all channels
 *
 * SWF version: 4+
 * Opcode: 0x09
 *
 * @param stack Pointer to the runtime stack (unused - no stack operations)
 * @param sp Pointer to stack pointer (unused - no stack operations)
 */
void actionStopSounds(SWFAppContext* app_context)
{
	// Suppress unused parameter warnings
	(void)app_context;
	
	// In NO_GRAPHICS mode, this is a no-op since there is no audio subsystem
	#ifndef NO_GRAPHICS
	// In full graphics mode, would stop all audio channels
	// This would require interfacing with the audio subsystem:
	// if (audio_context) {
	//     stopAllAudioChannels(audio_context);
	// }
	#endif
	
	// No stack operations required - opcode has no parameters and no return value
	// This opcode has global effect and does not modify the stack
}

/**
 * ActionGetURL - Load a URL into browser frame or Flash level
 *
 * Opcode: 0x83
 * SWF Version: 3+
 *
 * Instructs Flash Player to get the URL specified by the url parameter.
 * The URL can be any type: HTML file, image, or another SWF file.
 * If playing in a browser, the URL is displayed in the frame specified by target.
 *
 * Special targets:
 * - "_blank": Open in new window
 * - "_self": Open in current window/frame
 * - "_parent": Open in parent frame
 * - "_top": Open in top-level frame
 * - "_level0", "_level1", etc.: Load SWF into Flash Player level
 * - Named string: Open in named frame/window
 *
 * Current Implementation:
 * This is a simplified implementation for NO_GRAPHICS mode that logs the URL
 * request to stdout. Full implementation would require:
 * - Browser integration or HTTP client for web URLs
 * - SWF loader for _level targets
 * - Frame/window management for browser targets
 *
 * Edge cases handled:
 * - Null URL or target (logged as "(null)")
 * - Empty strings (logged as-is)
 *
 * @param stack Pointer to the runtime stack (unused in current implementation)
 * @param sp Pointer to stack pointer (unused in current implementation)
 * @param url The URL to load (can be relative or absolute)
 * @param target The target window/frame/level
 */
void actionGetURL(SWFAppContext* app_context, const char* url, const char* target)
{
	// Handle null pointers
	const char* safe_url = url ? url : "(null)";
	const char* safe_target = target ? target : "(null)";
	
	// Log the URL request for verification in NO_GRAPHICS mode
	// Format: "// GetURL: <url> -> <target>"
	printf("// GetURL: %s -> %s\n", safe_url, safe_target);
	
	// Note: Full implementation would check target type and dispatch accordingly:
	// - _level targets: Load SWF file into specified level
	// - Browser targets (_blank, _self, etc.): Open in browser window/frame
	// - Named targets: Open in named frame/window
	// - JavaScript URLs: Execute JavaScript (if enabled)
	// - Security: Check cross-domain policy, validate URL scheme
}

void actionGetVariable(SWFAppContext* app_context)
{
	// Read variable name info from stack
	// Stack layout for strings: +0=type, +4=oldSP, +8=length, +12=string_id, +16=pointer
	u32 string_id = VAL(u32, &STACK[SP + 12]);
	char* var_name = (char*) VAL(u64, &STACK[SP + 16]);
	u32 var_name_len = VAL(u32, &STACK[SP + 8]);
	
	// Pop variable name
	POP();
	
	// First check scope chain (innermost to outermost)
	for (int i = scope_depth - 1; i >= 0; i--)
	{
		if (scope_chain[i] != NULL)
		{
			// Try to find property in this scope object
			ActionVar* prop = getProperty(scope_chain[i], var_name, var_name_len);
			if (prop != NULL)
			{
				// Found in scope chain - push its value
				PUSH_VAR(prop);
				return;
			}
		}
	}
	
	// Not found in scope chain - check global variables
	ActionVar* var = NULL;
	if (string_id != 0)
	{
		// Constant string - use array (O(1))
		var = getVariableById(app_context, string_id);
		
		// Fall back to hashmap if array lookup doesn't find the variable
		// (This can happen for catch variables that are set by name but have a string ID)
		if (var == NULL || (var->type == ACTION_STACK_VALUE_STRING && var->str_size == 0))
		{
			var = getVariable(app_context, var_name, var_name_len);
		}
	}
	else
	{
		// Dynamic string - use hashmap (O(n))
		var = getVariable(app_context, var_name, var_name_len);
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

void actionSetVariable(SWFAppContext* app_context)
{
	// Stack layout: [name, value] <- sp
	// According to spec: Pop value first, then name
	// So VALUE is at top (*sp), NAME is at second (SP_SECOND_TOP)
	
	u32 value_sp = SP;
	u32 var_name_sp = SP_SECOND_TOP;
	
	// Read variable name info
	// Stack layout for strings: +0=type, +4=oldSP, +8=length, +12=string_id, +16=pointer
	u32 string_id = VAL(u32, &STACK[var_name_sp + 12]);
	
	char* var_name = (char*) VAL(u64, &STACK[var_name_sp + 16]);
	
	u32 var_name_len = VAL(u32, &STACK[var_name_sp + 8]);
	
	// First check scope chain (innermost to outermost)
	for (int i = scope_depth - 1; i >= 0; i--)
	{
		if (scope_chain[i] != NULL)
		{
			// Try to find property in this scope object
			ActionVar* prop = getProperty(scope_chain[i], var_name, var_name_len);
			if (prop != NULL)
			{
				// Found in scope chain - set it there
				ActionVar value_var;
				peekVar(app_context, &value_var);
				setProperty(app_context, scope_chain[i], var_name, var_name_len, &value_var);
				
				// Pop both value and name
				POP_2();
				return;
			}
		}
	}
	
	// Not found in scope chain - set as global variable
	
	ActionVar* var;
	if (string_id != 0)
	{
		// Constant string - use array (O(1))
		var = getVariableById(app_context, string_id);
	}
	else
	{
		// Dynamic string - use hashmap (O(n))
		var = getVariable(app_context, var_name, var_name_len);
	}
	
	if (!var)
	{
		// Failed to get/create variable
		POP_2();
		return;
	}
	
	// Set variable value (uses existing string materialization!)
	setVariableWithValue(app_context, var);
	
	// Pop both value and name
	POP_2();
}

void actionDefineLocal(SWFAppContext* app_context)
{
	// Stack layout: [name, value] <- sp
	// According to AS2 spec for DefineLocal:
	// Pop value first, then name
	// So VALUE is at top (*sp), NAME is at second (SP_SECOND_TOP)
	
	u32 value_sp = SP;
	u32 var_name_sp = SP_SECOND_TOP;
	
	// Read variable name info
	// Stack layout for strings: +0=type, +4=oldSP, +8=length, +12=string_id, +16=pointer
	u32 string_id = VAL(u32, &STACK[var_name_sp + 12]);
	char* var_name = (char*) VAL(u64, &STACK[var_name_sp + 16]);
	u32 var_name_len = VAL(u32, &STACK[var_name_sp + 8]);
	
	// DefineLocal ALWAYS creates/updates in the local scope
	// If there's a scope object (function context), define it there
	// Otherwise, fall back to global scope (for testing without full function support)
	
	if (scope_depth > 0 && scope_chain[scope_depth - 1] != NULL)
	{
		// We have a local scope object - define variable as a property
		ASObject* local_scope = scope_chain[scope_depth - 1];
		
		ActionVar value_var;
		peekVar(app_context, &value_var);
		
		// Set property on the local scope object
		// This will create the property if it doesn't exist, or update if it does
		setProperty(app_context, local_scope, var_name, var_name_len, &value_var);
		
		// Pop both value and name
		POP_2();
		return;
	}
	
	// No local scope - fall back to global variable
	// This allows testing DefineLocal without full function infrastructure
	ActionVar* var;
	if (string_id != 0)
	{
		// Constant string - use array (O(1))
		var = getVariableById(app_context, string_id);
	}
	else
	{
		// Dynamic string - use hashmap (O(n))
		var = getVariable(app_context, var_name, var_name_len);
	}
	
	if (!var)
	{
		// Failed to get/create variable
		POP_2();
		return;
	}
	
	// Set variable value
	setVariableWithValue(app_context, var);
	
	// Pop both value and name
	POP_2();
}

void actionDeclareLocal(SWFAppContext* app_context)
{
	// DECLARE_LOCAL pops only the variable name (no value)
	// It declares a local variable initialized to undefined
	
	// Stack layout: [name] <- sp
	
	// Read variable name info
	u32 string_id = VAL(u32, &STACK[SP + 12]);
	char* var_name = (char*) VAL(u64, &STACK[SP + 16]);
	u32 var_name_len = VAL(u32, &STACK[SP + 8]);
	
	// Check if we're in a local scope (function context)
	if (scope_depth > 0 && scope_chain[scope_depth - 1] != NULL)
	{
		// We have a local scope object - declare variable as undefined property
		ASObject* local_scope = scope_chain[scope_depth - 1];
		
		// Create an undefined value
		ActionVar undefined_var;
		undefined_var.type = ACTION_STACK_VALUE_UNDEFINED;
		undefined_var.str_size = 0;
		undefined_var.value = 0;
		
		// Set property on the local scope object
		// This will create the property if it doesn't exist
		setProperty(app_context, local_scope, var_name, var_name_len, &undefined_var);
		
		// Pop the name
		POP();
		return;
	}
	
	// Not in a function - show warning and treat as no-op
	// (In AS2, DECLARE_LOCAL outside a function is technically invalid)
	printf("Warning: DECLARE_LOCAL outside function for variable '%s'\n", var_name);
	
	// Pop the name
	POP();
}

void actionRandomNumber(SWFAppContext* app_context)
{
	// Pop maximum value
	convertFloat(app_context);
	ActionVar max_var;
	popVar(app_context, &max_var);
	int max = (int) VAL(float, &max_var.value);
	
	// Generate random number using avmplus-compatible RNG
	// This matches Flash Player's exact behavior for speedrunners
	int random_val = Random(max, &global_random_state);
	
	// Push result as float
	float result = (float) random_val;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionAsciiToChar(SWFAppContext* app_context, char* str_buffer)
{
	// Convert top of stack to number
	convertFloat(app_context);
	
	// Pop the numeric value
	ActionVar a;
	popVar(app_context, &a);
	
	// Get integer code (truncate decimal)
	float val = VAL(float, &a.value);
	int code = (int)val;
	
	// Handle out-of-range values (wrap to 0-255)
	code = code & 0xFF;
	
	// Create single-character string
	str_buffer[0] = (char)code;
	str_buffer[1] = '\0';
	
	// Push result string
	PUSH_STR(str_buffer, 1);
}

void actionMbCharToAscii(SWFAppContext* app_context, char* str_buffer)
{
	// Convert top of stack to string
	convertString(app_context, str_buffer);
	
	// Get string pointer from stack
	const char* str = (const char*) VAL(u64, &STACK_TOP_VALUE);
	
	// Pop the string value
	POP();
	
	// Handle empty string edge case
	if (str == NULL || str[0] == '\0') {
		float result = 0.0f;  // Return 0 for empty string
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return;
	}
	
	// Decode UTF-8 first character
	unsigned int codepoint = 0;
	unsigned char c = (unsigned char)str[0];
	
	if ((c & 0x80) == 0) {
		// 1-byte sequence (0xxxxxxx)
		codepoint = c;
	} else if ((c & 0xE0) == 0xC0) {
		// 2-byte sequence (110xxxxx 10xxxxxx)
		codepoint = ((c & 0x1F) << 6) | ((unsigned char)str[1] & 0x3F);
	} else if ((c & 0xF0) == 0xE0) {
		// 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
		codepoint = ((c & 0x0F) << 12) |
		            (((unsigned char)str[1] & 0x3F) << 6) |
		            ((unsigned char)str[2] & 0x3F);
	} else if ((c & 0xF8) == 0xF0) {
		// 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
		codepoint = ((c & 0x07) << 18) |
		            (((unsigned char)str[1] & 0x3F) << 12) |
		            (((unsigned char)str[2] & 0x3F) << 6) |
		            ((unsigned char)str[3] & 0x3F);
	}
	
	// Push result as float
	float result = (float)codepoint;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionGetTime(SWFAppContext* app_context)
{
	u32 delta_ms = get_elapsed_ms() - start_time;
	float delta_ms_f32 = (float) delta_ms;
	
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &delta_ms_f32));
}

void actionMbAsciiToChar(SWFAppContext* app_context, char* str_buffer)
{
	// Convert top of stack to number
	convertFloat(app_context);
	
	// Pop the numeric value
	ActionVar a;
	popVar(app_context, &a);
	
	// Get integer code point
	float value = a.type == ACTION_STACK_VALUE_F32 ? VAL(float, &a.value) : (float)VAL(double, &a.value);
	unsigned int codepoint = (unsigned int)value;
	
	// Validate code point range (0 to 0x10FFFF for valid Unicode)
	if (codepoint > 0x10FFFF) {
		// Push empty string for invalid code points
		str_buffer[0] = '\0';
		PUSH_STR(str_buffer, 0);
		return;
	}
	
	// Encode as UTF-8
	int len = 0;
	if (codepoint <= 0x7F) {
		// 1-byte sequence
		str_buffer[len++] = (char)codepoint;
	} else if (codepoint <= 0x7FF) {
		// 2-byte sequence
		str_buffer[len++] = (char)(0xC0 | (codepoint >> 6));
		str_buffer[len++] = (char)(0x80 | (codepoint & 0x3F));
	} else if (codepoint <= 0xFFFF) {
		// 3-byte sequence
		str_buffer[len++] = (char)(0xE0 | (codepoint >> 12));
		str_buffer[len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		str_buffer[len++] = (char)(0x80 | (codepoint & 0x3F));
	} else {
		// 4-byte sequence
		str_buffer[len++] = (char)(0xF0 | (codepoint >> 18));
		str_buffer[len++] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
		str_buffer[len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		str_buffer[len++] = (char)(0x80 | (codepoint & 0x3F));
	}
	str_buffer[len] = '\0';
	
	// Push result string
	PUSH_STR(str_buffer, len);
}

void actionTypeof(SWFAppContext* app_context, char* str_buffer)
{
	// Peek at the type without modifying value
	u8 type = STACK_TOP_TYPE;
	
	// Pop the value
	POP();
	
	// Determine type string based on stack type
	const char* type_str;
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
			
		case ACTION_STACK_VALUE_FUNCTION:
			type_str = "function";
			break;
			
		case ACTION_STACK_VALUE_OBJECT:
		case ACTION_STACK_VALUE_ARRAY:
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
	int len = strlen(type_str);
	strncpy(str_buffer, type_str, 16);
	str_buffer[len] = '\0';
	PUSH_STR(str_buffer, len);
}

void actionDelete2(SWFAppContext* app_context, char* str_buffer)
{
	// Delete2 deletes a named property/variable
	// Pops the name from the stack, deletes it, pushes success boolean
	
	// Read variable name from stack
	u32 var_name_sp = SP;
	u8 name_type = STACK[var_name_sp];
	char* var_name = NULL;
	u32 var_name_len = 0;
	
	// Get the variable name string
	if (name_type == ACTION_STACK_VALUE_STRING)
	{
		var_name = (char*) VAL(u64, &STACK[var_name_sp + 16]);
		var_name_len = VAL(u32, &STACK[var_name_sp + 8]);
	}
	else if (name_type == ACTION_STACK_VALUE_STR_LIST)
	{
		// Materialize string list
		var_name = materializeStringList(app_context);
		var_name_len = strlen(var_name);
	}
	
	// Pop the variable name
	POP();
	
	// Default: assume deletion succeeds (Flash behavior)
	bool success = true;
	
	// Try to delete from scope chain (innermost to outermost)
	for (int i = scope_depth - 1; i >= 0; i--)
	{
		if (scope_chain[i] != NULL)
		{
			// Check if property exists in this scope object
			ActionVar* prop = getProperty(scope_chain[i], var_name, var_name_len);
			if (prop != NULL)
			{
				// Found in scope chain - delete it
				success = deleteProperty(app_context, scope_chain[i], var_name, var_name_len);
				
				// Push result and return
				float result = success ? 1.0f : 0.0f;
				PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
				return;
			}
		}
	}
	
	// Not found in scope chain - check global variables
	// Note: In Flash, you cannot delete variables declared with 'var', so we return false
	// However, if the variable doesn't exist at all, we return true (Flash behavior)
	if (getVariable(app_context, var_name, var_name_len) != NULL)
	{
		// Variable exists but is a 'var' declaration - cannot delete
		success = false;
	}
	else
	{
		// Variable doesn't exist - Flash returns true
		success = true;
	}
	
	// Push result
	float result = success ? 1.0f : 0.0f;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
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
	// Primitives (number, string, undefined) are never instances
	if (obj_var->type == ACTION_STACK_VALUE_F32 ||
		obj_var->type == ACTION_STACK_VALUE_F64 ||
		obj_var->type == ACTION_STACK_VALUE_STRING ||
		obj_var->type == ACTION_STACK_VALUE_UNDEFINED)
	{
		return 0;
	}
	
	// Object and constructor must be object types
	if (obj_var->type != ACTION_STACK_VALUE_OBJECT &&
		obj_var->type != ACTION_STACK_VALUE_ARRAY &&
		obj_var->type != ACTION_STACK_VALUE_FUNCTION)
	{
		return 0;
	}
	
	if (ctor_var->type != ACTION_STACK_VALUE_OBJECT &&
		ctor_var->type != ACTION_STACK_VALUE_FUNCTION)
	{
		return 0;
	}
	
	ASObject* obj = (ASObject*) obj_var->value;
	ASObject* ctor = (ASObject*) ctor_var->value;
	
	if (obj == NULL || ctor == NULL)
	{
		return 0;
	}
	
	// Get the constructor's "prototype" property
	ActionVar* ctor_proto_var = getProperty(ctor, "prototype", 9);
	if (ctor_proto_var == NULL)
	{
		return 0;
	}
	
	// Get the prototype object
	if (ctor_proto_var->type != ACTION_STACK_VALUE_OBJECT)
	{
		return 0;
	}
	
	ASObject* ctor_proto = (ASObject*) ctor_proto_var->value;
	if (ctor_proto == NULL)
	{
		return 0;
	}
	
	// Walk up the object's prototype chain via __proto__ property
	// Start with the object's __proto__
	ActionVar* current_proto_var = getProperty(obj, "__proto__", 9);
	
	// Maximum chain depth to prevent infinite loops
	int max_depth = 100;
	int depth = 0;
	
	while (current_proto_var != NULL && depth < max_depth)
	{
		depth++;
		
		// Check if this prototype matches the constructor's prototype
		if (current_proto_var->type == ACTION_STACK_VALUE_OBJECT)
		{
			ASObject* current_proto = (ASObject*) current_proto_var->value;
			
			if (current_proto == ctor_proto)
			{
				// Found a match!
				return 1;
			}
			
			// Continue up the chain
			current_proto_var = getProperty(current_proto, "__proto__", 9);
		}
		else
		{
			// Non-object in prototype chain, stop
			break;
		}
	}
	
	// Check interface implementation (ActionScript 2.0 implements keyword)
	if (implementsInterface(obj, ctor))
	{
		return 1;
	}
	
	// Not found in prototype chain or interfaces
	return 0;
}

void actionCastOp(SWFAppContext* app_context)
{
	// CastOp implementation (ActionScript 2.0 cast operator)
	// Pops object to cast, pops constructor, checks if object is instance of constructor
	// Returns object if cast succeeds, null if it fails
	
	// Pop object to cast
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// Pop constructor function
	ActionVar ctor_var;
	popVar(app_context, &ctor_var);
	
	// Check if object is an instance of constructor using prototype chain + interfaces
	if (checkInstanceOf(&obj_var, &ctor_var))
	{
		// Cast succeeds - push the object back
		pushVar(app_context, &obj_var);
	}
	else
	{
		// Cast fails - push null
		ActionVar null_var;
		null_var.type = ACTION_STACK_VALUE_UNDEFINED;
		null_var.value = 0;
		null_var.str_size = 0;
		pushVar(app_context, &null_var);
	}
}

void actionDuplicate(SWFAppContext* app_context)
{
	// Get the type of the top stack entry
	u8 type = STACK_TOP_TYPE;
	
	// Handle different types appropriately
	if (type == ACTION_STACK_VALUE_STRING)
	{
		// For strings, we need to copy both the pointer and the length
		const char* str = (const char*) VAL(u64, &STACK_TOP_VALUE);
		u32 len = STACK_TOP_N;  // Length is stored at offset +8
		u32 id = VAL(u32, &STACK[SP + 12]);  // String ID is at offset +12
		
		// Push a copy of the string (shallow copy - same pointer)
		PUSH_STR_ID(str, len, id);
	}
	else
	{
		// For other types (numeric, etc.), just copy the value
		u64 value = STACK_TOP_VALUE;
		PUSH(type, value);
	}
}

void actionReturn(SWFAppContext* app_context)
{
	// The return value is already at the top of the stack.
	// The generated C code includes a "return;" statement that exits
	// the function, leaving the value on the stack for the caller.
	// No operation needed here - the translation layer handles
	// the actual return via C return statement.
}

void actionIncrement(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double val = VAL(double, &a.value);
		double result = val + 1.0;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &result));
	}
	else
	{
		float val = VAL(float, &a.value);
		float result = val + 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
	}
}

void actionDecrement(SWFAppContext* app_context)
{
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	if (a.type == ACTION_STACK_VALUE_F64)
	{
		double val = VAL(double, &a.value);
		double result = val - 1.0;
		PUSH(ACTION_STACK_VALUE_F64, VAL(u64, &result));
	}
	else
	{
		float val = VAL(float, &a.value);
		float result = val - 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
	}
}

void actionInstanceOf(SWFAppContext* app_context)
{
	// Pop constructor function
	ActionVar constr_var;
	popVar(app_context, &constr_var);
	
	// Pop object
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// Check if object is an instance of constructor using prototype chain + interfaces
	int result = checkInstanceOf(&obj_var, &constr_var);
	
	// Push result as float (1.0 for true, 0.0 for false)
	float result_val = result ? 1.0f : 0.0f;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_val));
}

void actionEnumerate2(SWFAppContext* app_context, char* str_buffer)
{
	// Pop object reference from stack
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// Push undefined as terminator
	PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
	
	// Handle different types
	if (obj_var.type == ACTION_STACK_VALUE_OBJECT)
	{
		// Object enumeration - push property names in reverse order
		ASObject* obj = (ASObject*) obj_var.value;
		
		if (obj != NULL && obj->num_used > 0)
		{
			// Enumerate properties in reverse order (last to first)
			// This way when they're popped, they'll come out in the correct order
			for (int i = obj->num_used - 1; i >= 0; i--)
			{
				const char* prop_name = obj->properties[i].name;
				u32 prop_name_len = obj->properties[i].name_length;
				
				// Push property name as string
				PUSH_STR(prop_name, prop_name_len);
			}
		}
		
		#ifdef DEBUG
		printf("// Enumerate2: enumerated %u properties from object\n",
			obj ? obj->num_used : 0);
		#endif
	}
	else if (obj_var.type == ACTION_STACK_VALUE_ARRAY)
	{
		// Array enumeration - push indices as strings
		ASArray* arr = (ASArray*) obj_var.value;
		
		if (arr != NULL && arr->length > 0)
		{
			// Enumerate indices in reverse order
			for (int i = arr->length - 1; i >= 0; i--)
			{
				// Convert index to string
				snprintf(str_buffer, 17, "%d", i);
				u32 len = strlen(str_buffer);
				
				// Push index as string
				PUSH_STR(str_buffer, len);
			}
		}
		
		#ifdef DEBUG
		printf("// Enumerate2: enumerated %u indices from array\n",
			arr ? arr->length : 0);
		#endif
	}
	else
	{
		// Non-object/non-array: just the undefined terminator
		#ifdef DEBUG
		printf("// Enumerate2: non-enumerable type, only undefined pushed\n");
		#endif
	}
}

void actionBitAnd(SWFAppContext* app_context)
{

	// Convert and pop second operand (a)
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	// Convert and pop first operand (b)
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	// Convert to 32-bit signed integers (truncate, don't round)
	int32_t a_int = (int32_t)VAL(float, &a.value);
	int32_t b_int = (int32_t)VAL(float, &b.value);
	
	// Perform bitwise AND
	int32_t result = b_int & a_int;
	
	// Push result as float (ActionScript stores all numbers as float)
	float result_f = (float)result;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_f));
}

void actionBitOr(SWFAppContext* app_context)
{

	// Convert and pop second operand (a)
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	// Convert and pop first operand (b)
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	// Convert to 32-bit signed integers (truncate, don't round)
	int32_t a_int = (int32_t)VAL(float, &a.value);
	int32_t b_int = (int32_t)VAL(float, &b.value);
	
	// Perform bitwise OR
	int32_t result = b_int | a_int;
	
	// Push result as float (ActionScript stores all numbers as float)
	float result_f = (float)result;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_f));
}

void actionBitXor(SWFAppContext* app_context)
{

	// Convert and pop second operand (a)
	convertFloat(app_context);
	ActionVar a;
	popVar(app_context, &a);
	
	// Convert and pop first operand (b)
	convertFloat(app_context);
	ActionVar b;
	popVar(app_context, &b);
	
	// Convert to 32-bit signed integers (truncate, don't round)
	int32_t a_int = (int32_t)VAL(float, &a.value);
	int32_t b_int = (int32_t)VAL(float, &b.value);
	
	// Perform bitwise XOR
	int32_t result = b_int ^ a_int;
	
	// Push result as float (ActionScript stores all numbers as float)
	float result_f = (float)result;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_f));
}

void actionBitLShift(SWFAppContext* app_context)
{

	// Pop shift count (first argument)
	convertFloat(app_context);
	ActionVar shift_count_var;
	popVar(app_context, &shift_count_var);
	
	// Pop value to shift (second argument)
	convertFloat(app_context);
	ActionVar value_var;
	popVar(app_context, &value_var);
	
	// Convert to 32-bit signed integers (truncate, don't round)
	int32_t shift_count = (int32_t)VAL(float, &shift_count_var.value);
	int32_t value = (int32_t)VAL(float, &value_var.value);
	
	// Mask shift count to 5 bits (0-31 range)
	shift_count = shift_count & 0x1F;
	
	// Perform left shift
	int32_t result = value << shift_count;
	
	// Push result as float (ActionScript stores all numbers as float)
	float result_f = (float)result;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_f));
}

void actionBitRShift(SWFAppContext* app_context)
{

	// Pop shift count (first argument)
	convertFloat(app_context);
	ActionVar shift_count_var;
	popVar(app_context, &shift_count_var);
	
	// Pop value to shift (second argument)
	convertFloat(app_context);
	ActionVar value_var;
	popVar(app_context, &value_var);
	
	// Convert to 32-bit signed integers
	int32_t shift_count = (int32_t)VAL(float, &shift_count_var.value);
	int32_t value = (int32_t)VAL(float, &value_var.value);
	
	// Mask shift count to 5 bits (0-31 range)
	shift_count = shift_count & 0x1F;
	
	// Perform arithmetic right shift (sign-extending)
	// In C, >> on signed int is arithmetic shift
	int32_t result = value >> shift_count;
	
	// Convert result back to float for stack
	float result_f = (float)result;
	
	// Push result
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_f));
}

void actionBitURShift(SWFAppContext* app_context)
{

	// Pop shift count (first argument)
	convertFloat(app_context);
	ActionVar shift_count_var;
	popVar(app_context, &shift_count_var);
	
	// Pop value to shift (second argument)
	convertFloat(app_context);
	ActionVar value_var;
	popVar(app_context, &value_var);
	
	// Convert to integers
	int32_t shift_count = (int32_t)VAL(float, &shift_count_var.value);
	
	// IMPORTANT: Use UNSIGNED for logical shift
	uint32_t value = (uint32_t)((int32_t)VAL(float, &value_var.value));
	
	// Mask shift count to 5 bits (0-31 range)
	shift_count = shift_count & 0x1F;
	
	// Perform logical (unsigned) right shift
	// In C, >> on unsigned int is logical shift (zero-fill)
	uint32_t result = value >> shift_count;
	
	// Convert result back to float for stack
	// Cast through double to preserve full 32-bit unsigned value
	float result_float = (float)((double)result);
	
	// Push result
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result_float));
}

void actionStrictEquals(SWFAppContext* app_context)
{
	// Pop first argument (no type conversion - strict equality!)
	ActionVar a;
	popVar(app_context, &a);
	
	// Pop second argument (no type conversion - strict equality!)
	ActionVar b;
	popVar(app_context, &b);
	
	float result = 0.0f;
	
	// First check: types must match
	if (a.type == b.type)
	{
		// Second check: values must match
		switch (a.type)
		{
			case ACTION_STACK_VALUE_F32:
			{
				float a_val = VAL(float, &a.value);
				float b_val = VAL(float, &b.value);
				result = (a_val == b_val) ? 1.0f : 0.0f;
				break;
			}
			
			case ACTION_STACK_VALUE_F64:
			{
				double a_val = VAL(double, &a.value);
				double b_val = VAL(double, &b.value);
				result = (a_val == b_val) ? 1.0f : 0.0f;
				break;
			}
			
			case ACTION_STACK_VALUE_STRING:
			{
				const char* str_a = (const char*) a.value;
				const char* str_b = (const char*) b.value;
				// Check for NULL pointers first
				if (str_a != NULL && str_b != NULL) {
					result = (strcmp(str_a, str_b) == 0) ? 1.0f : 0.0f;
				} else {
					// If either is NULL, they're only equal if both are NULL
					result = (str_a == str_b) ? 1.0f : 0.0f;
				}
				break;
			}
			
			case ACTION_STACK_VALUE_STR_LIST:
			{
				// For string lists, use strcmp_list_a_list_b
				int cmp_result = strcmp_list_a_list_b(a.value, b.value);
				result = (cmp_result == 0) ? 1.0f : 0.0f;
				break;
			}
			
			// For other types (OBJECT, etc.), compare raw values
			default:
				#ifdef DEBUG
				printf("[DEBUG] STRICT_EQUALS: type=%d, a.ptr=%p, b.ptr=%p, equal=%d\n",
					a.type, (void*)a.value, (void*)b.value,
					a.value == b.value);
				#endif
				result = (a.value == b.value) ? 1.0f : 0.0f;
				break;
		}
	}
	else
	{
		// different types, result remains 0.0f (false)
		#ifdef DEBUG
		printf("[DEBUG] STRICT_EQUALS: type mismatch - a.type=%d, b.type=%d\n", a.type, b.type);
		#endif
	}
	
	// Push boolean result
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionEquals2(SWFAppContext* app_context)
{
	// Pop first argument (arg1)
	ActionVar a;
	popVar(app_context, &a);
	
	// Pop second argument (arg2)
	ActionVar b;
	popVar(app_context, &b);
	
	float result = 0.0f;
	
	// ECMA-262 equality algorithm (Section 11.9.3)
	
	// 1. If types are the same, use strict equality
	if (a.type == b.type)
	{
		switch (a.type)
		{
			case ACTION_STACK_VALUE_F32:
			{
				float a_val = VAL(float, &a.value);
				float b_val = VAL(float, &b.value);
				// NaN is never equal to anything, including itself (ECMA-262)
				if (isnan(a_val) || isnan(b_val)) {
					result = 0.0f;
				} else {
					result = (a_val == b_val) ? 1.0f : 0.0f;
				}
				break;
			}
			
			case ACTION_STACK_VALUE_F64:
			{
				double a_val = VAL(double, &a.value);
				double b_val = VAL(double, &b.value);
				// NaN is never equal to anything, including itself (ECMA-262)
				if (isnan(a_val) || isnan(b_val)) {
					result = 0.0f;
				} else {
					result = (a_val == b_val) ? 1.0f : 0.0f;
				}
				break;
			}
			
			case ACTION_STACK_VALUE_STRING:
			{
				const char* str_a = (const char*) a.value;
				const char* str_b = (const char*) b.value;
				if (str_a != NULL && str_b != NULL) {
					result = (strcmp(str_a, str_b) == 0) ? 1.0f : 0.0f;
				} else {
					result = (str_a == str_b) ? 1.0f : 0.0f;
				}
				break;
			}
			
			case ACTION_STACK_VALUE_BOOLEAN:
			{
				// Boolean values are stored in numeric_value as 0 (false) or 1 (true)
				u32 a_val = (u32) a.value;
				u32 b_val = (u32) b.value;
				result = (a_val == b_val) ? 1.0f : 0.0f;
				break;
			}
			
			case ACTION_STACK_VALUE_NULL:
			{
				// null == null is true
				result = 1.0f;
				break;
			}
			
			case ACTION_STACK_VALUE_UNDEFINED:
			{
				// undefined == undefined is true
				result = 1.0f;
				break;
			}
			
			default:
				// For other types (OBJECT, etc.), compare raw values (reference equality)
				result = (a.value == b.value) ? 1.0f : 0.0f;
				break;
		}
	}
	// 2. Special case: null == undefined (ECMA-262)
	else if ((a.type == ACTION_STACK_VALUE_NULL && b.type == ACTION_STACK_VALUE_UNDEFINED) ||
	         (a.type == ACTION_STACK_VALUE_UNDEFINED && b.type == ACTION_STACK_VALUE_NULL))
	{
		result = 1.0f;
	}
	// 3. Number vs String: convert string to number
	else if ((a.type == ACTION_STACK_VALUE_F32 || a.type == ACTION_STACK_VALUE_F64) &&
	         b.type == ACTION_STACK_VALUE_STRING)
	{
		const char* str_b = (const char*) b.value;
		float b_num = (str_b != NULL) ? (float)atof(str_b) : 0.0f;
		float a_val = (a.type == ACTION_STACK_VALUE_F32) ?
		              VAL(float, &a.value) :
		              (float)VAL(double, &a.value);
		// NaN is never equal to anything (ECMA-262)
		if (isnan(a_val) || isnan(b_num)) {
			result = 0.0f;
		} else {
			result = (a_val == b_num) ? 1.0f : 0.0f;
		}
	}
	else if (a.type == ACTION_STACK_VALUE_STRING &&
	         (b.type == ACTION_STACK_VALUE_F32 || b.type == ACTION_STACK_VALUE_F64))
	{
		const char* str_a = (const char*) a.value;
		float a_num = (str_a != NULL) ? (float)atof(str_a) : 0.0f;
		float b_val = (b.type == ACTION_STACK_VALUE_F32) ?
		              VAL(float, &b.value) :
		              (float)VAL(double, &b.value);
		// NaN is never equal to anything (ECMA-262)
		if (isnan(a_num) || isnan(b_val)) {
			result = 0.0f;
		} else {
			result = (a_num == b_val) ? 1.0f : 0.0f;
		}
	}
	// 4. Boolean: convert to number and compare recursively
	else if (a.type == ACTION_STACK_VALUE_BOOLEAN)
	{
		// Convert boolean to number (true = 1.0, false = 0.0)
		u32 a_bool = (u32) a.value;
		float a_num = a_bool ? 1.0f : 0.0f;
		ActionVar a_as_num;
		a_as_num.type = ACTION_STACK_VALUE_F32;
		a_as_num.value = VAL(u64, &a_num);
		
		// Push back and recurse (simulated)
		// For efficiency, we inline the comparison instead
		if (b.type == ACTION_STACK_VALUE_F32 || b.type == ACTION_STACK_VALUE_F64)
		{
			float b_val = (b.type == ACTION_STACK_VALUE_F32) ?
			              VAL(float, &b.value) :
			              (float)VAL(double, &b.value);
			result = (a_num == b_val) ? 1.0f : 0.0f;
		}
		else if (b.type == ACTION_STACK_VALUE_STRING)
		{
			const char* str_b = (const char*) b.value;
			float b_num = (str_b != NULL) ? (float)atof(str_b) : 0.0f;
			result = (a_num == b_num) ? 1.0f : 0.0f;
		}
		// Boolean vs null/undefined is false
		else if (b.type == ACTION_STACK_VALUE_NULL || b.type == ACTION_STACK_VALUE_UNDEFINED)
		{
			result = 0.0f;
		}
	}
	else if (b.type == ACTION_STACK_VALUE_BOOLEAN)
	{
		// Convert boolean to number (true = 1.0, false = 0.0)
		u32 b_bool = (u32) b.value;
		float b_num = b_bool ? 1.0f : 0.0f;
		
		if (a.type == ACTION_STACK_VALUE_F32 || a.type == ACTION_STACK_VALUE_F64)
		{
			float a_val = (a.type == ACTION_STACK_VALUE_F32) ?
			              VAL(float, &a.value) :
			              (float)VAL(double, &a.value);
			result = (a_val == b_num) ? 1.0f : 0.0f;
		}
		else if (a.type == ACTION_STACK_VALUE_STRING)
		{
			const char* str_a = (const char*) a.value;
			float a_num = (str_a != NULL) ? (float)atof(str_a) : 0.0f;
			result = (a_num == b_num) ? 1.0f : 0.0f;
		}
		// Boolean vs null/undefined is false
		else if (a.type == ACTION_STACK_VALUE_NULL || a.type == ACTION_STACK_VALUE_UNDEFINED)
		{
			result = 0.0f;
		}
	}
	// 5. null or undefined compared with anything else (except each other) is false
	else if (a.type == ACTION_STACK_VALUE_NULL || a.type == ACTION_STACK_VALUE_UNDEFINED ||
	         b.type == ACTION_STACK_VALUE_NULL || b.type == ACTION_STACK_VALUE_UNDEFINED)
	{
		result = 0.0f;
	}
	// 6. Different types not covered above: false
	// (This handles cases like object vs number, etc.)
	
	// Push boolean result (1.0 = true, 0.0 = false)
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionStringGreater(SWFAppContext* app_context)
{

	// Get first string (arg1)
	ActionVar a;
	popVar(app_context, &a);
	const char* str_a = (const char*) a.value;
	
	// Get second string (arg2)
	ActionVar b;
	popVar(app_context, &b);
	const char* str_b = (const char*) b.value;
	
	// Compare: b > a (using strcmp)
	// strcmp returns positive if str_b > str_a
	float result = (strcmp(str_b, str_a) > 0) ? 1.0f : 0.0f;
	
	// Push boolean result
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

// ==================================================================
// Inheritance (EXTENDS opcode)
// ==================================================================

void actionExtends(SWFAppContext* app_context)
{
	// Pop superclass constructor from stack
	ActionVar superclass;
	popVar(app_context, &superclass);
	
	// Pop subclass constructor from stack
	ActionVar subclass;
	popVar(app_context, &subclass);
	
	// Verify both are objects/functions
	if (superclass.type != ACTION_STACK_VALUE_OBJECT &&
	    superclass.type != ACTION_STACK_VALUE_FUNCTION)
	{
#ifdef DEBUG
		printf("[DEBUG] actionExtends: superclass is not an object/function (type=%d)\n",
		       superclass.type);
#endif
		return;
	}
	
	if (subclass.type != ACTION_STACK_VALUE_OBJECT &&
	    subclass.type != ACTION_STACK_VALUE_FUNCTION)
	{
#ifdef DEBUG
		printf("[DEBUG] actionExtends: subclass is not an object/function (type=%d)\n",
		       subclass.type);
#endif
		return;
	}
	
	// Get constructor objects
	ASObject* super_func = (ASObject*) superclass.value;
	ASObject* sub_func = (ASObject*) subclass.value;
	
	if (super_func == NULL || sub_func == NULL)
	{
#ifdef DEBUG
		printf("[DEBUG] actionExtends: NULL constructor object\n");
#endif
		return;
	}
	
	// Create new prototype object
	ASObject* new_proto = allocObject(app_context, 0);
	if (new_proto == NULL)
	{
#ifdef DEBUG
		printf("[DEBUG] actionExtends: Failed to allocate new prototype\n");
#endif
		return;
	}
	
	// Get superclass prototype property
	ActionVar* super_proto_var = getProperty(super_func, "prototype", 9);
	
	// Set __proto__ of new prototype to superclass prototype
	if (super_proto_var != NULL)
	{
		setProperty(app_context, new_proto, "__proto__", 9, super_proto_var);
	}
	
	// Set constructor property to superclass
	setProperty(app_context, new_proto, "constructor", 11, &superclass);
	
#ifdef DEBUG
	printf("[DEBUG] actionExtends: Set constructor property - type=%d, ptr=%p\n",
		superclass.type, (void*)superclass.value);
		
	// Verify it was set correctly
	ActionVar* check = getProperty(new_proto, "constructor", 11);
	if (check != NULL) {
		printf("[DEBUG] actionExtends: Retrieved constructor - type=%d, ptr=%p\n",
			check->type, (void*)check->value);
	}
#endif

	// Set subclass prototype to new object
	ActionVar new_proto_var;
	new_proto_var.type = ACTION_STACK_VALUE_OBJECT;
	new_proto_var.value = (u64) new_proto;
	new_proto_var.str_size = 0;
	
	setProperty(app_context, sub_func, "prototype", 9, &new_proto_var);
	
	// Release our reference to new_proto
	// (setProperty retained it when setting as prototype)
	releaseObject(app_context, new_proto);
	
#ifdef DEBUG
	printf("[DEBUG] actionExtends: Prototype chain established\n");
#endif

	// Note: No values pushed back on stack
}

// ==================================================================
// Register Storage (up to 256 registers for SWF 5+)
// ==================================================================

#define MAX_REGISTERS 256
static ActionVar g_registers[MAX_REGISTERS];

void actionStoreRegister(SWFAppContext* app_context, u8 register_num)
{
	// Validate register number
	if (register_num >= MAX_REGISTERS) {
		return;
	}
	
	// Peek the top of stack (don't pop!)
	ActionVar value;
	peekVar(app_context, &value);
	
	// Store value in register
	g_registers[register_num] = value;
}

void actionPushRegister(SWFAppContext* app_context, u8 register_num)
{
	// Validate register number
	if (register_num >= MAX_REGISTERS) {
		// Push undefined for invalid register
		float undef = 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &undef));
		return;
	}
	
	ActionVar* reg = &g_registers[register_num];
	
	// Push register value to stack
	if (reg->type == ACTION_STACK_VALUE_F32 || reg->type == ACTION_STACK_VALUE_F64) {
		PUSH(reg->type, reg->value);
	} else if (reg->type == ACTION_STACK_VALUE_STRING) {
		const char* str = (const char*) reg->value;
		PUSH_STR(str, reg->str_size);
	} else if (reg->type == ACTION_STACK_VALUE_STR_LIST) {
		// String list - push reference
		PUSH_STR_LIST(reg->str_size, 0);
	} else {
		// Undefined or unknown type - push 0
		float undef = 0.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &undef));
	}
}

void actionStringLess(SWFAppContext* app_context)
{
	// Get first string (arg1)
	ActionVar a;
	popVar(app_context, &a);
	const char* str_a = (const char*) a.value;
	
	// Get second string (arg2)
	ActionVar b;
	popVar(app_context, &b);
	const char* str_b = (const char*) b.value;
	
	// Compare: b < a (using strcmp)
	// strcmp returns negative if str_b < str_a
	float result = (strcmp(str_b, str_a) < 0) ? 1.0f : 0.0f;
	
	// Push boolean result
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionImplementsOp(SWFAppContext* app_context)
{
	// ActionImplementsOp implements the ActionScript "implements" keyword
	// It specifies the interfaces that a class implements, for use by instanceof and CastOp
	
	// Step 1: Pop constructor function (the class) from stack
	ActionVar constructor_var;
	popVar(app_context, &constructor_var);
	
	// Validate that it's an object
	if (constructor_var.type != ACTION_STACK_VALUE_OBJECT)
	{
		fprintf(stderr, "ERROR: actionImplementsOp - constructor is not an object\n");
		return;
	}
	
	ASObject* constructor = (ASObject*) constructor_var.value;
	
	// Step 2: Pop count of interfaces from stack
	ActionVar count_var;
	popVar(app_context, &count_var);
	
	// Convert to number if needed
	u32 interface_count = 0;
	if (count_var.type == ACTION_STACK_VALUE_F32)
	{
		interface_count = (u32) *((float*)&count_var.value);
	}
	else if (count_var.type == ACTION_STACK_VALUE_F64)
	{
		interface_count = (u32) *((double*)&count_var.value);
	}
	else
	{
		fprintf(stderr, "ERROR: actionImplementsOp - interface count is not a number\n");
		return;
	}
	
	// Step 3: Allocate array for interface constructors
	ASObject** interfaces = NULL;
	if (interface_count > 0)
	{
		interfaces = (ASObject**) malloc(sizeof(ASObject*) * interface_count);
		if (interfaces == NULL)
		{
			fprintf(stderr, "ERROR: actionImplementsOp - failed to allocate interfaces array\n");
			return;
		}
		
		// Pop each interface constructor from stack
		// Note: Interfaces are pushed in order, so we pop them in reverse
		for (u32 i = 0; i < interface_count; i++)
		{
			ActionVar iface_var;
			popVar(app_context, &iface_var);
			
			if (iface_var.type != ACTION_STACK_VALUE_OBJECT)
			{
				fprintf(stderr, "ERROR: actionImplementsOp - interface %u is not an object\n", i);
				// Clean up allocated interfaces
				for (u32 j = 0; j < i; j++)
				{
					releaseObject(app_context, interfaces[j]);
				}
				free(interfaces);
				return;
			}
			
			// Store in reverse order (last popped goes first)
			interfaces[interface_count - 1 - i] = (ASObject*) iface_var.value;
		}
	}
	
	// Step 4: Set the interface list on the constructor
	// This transfers ownership of the interfaces array
	setInterfaceList(app_context, constructor, interfaces, interface_count);
	
#ifdef DEBUG
	printf("[DEBUG] actionImplementsOp: constructor=%p, interface_count=%u\n",
		(void*)constructor, interface_count);
#endif

	// Note: No values pushed back on stack (ImplementsOp has no return value)
}

/**
 * ActionCall - Calls a subroutine (frame actions)
 *
 * Stack: [ frame_identifier ] -> [ ]
 *
 * Pops a frame identifier from the stack and executes the actions in that frame.
 * After the frame actions complete, execution resumes at the instruction after
 * the ActionCall instruction.
 *
 * Frame identifier can be:
 * - A number: Frame index (0-based in g_frame_funcs array)
 * - A string (numeric): Parsed as frame number
 * - A string (label): Frame label (requires label registry - not implemented)
 * - With target path: "/target:frame" or "/target:label" (requires MovieClip tree - not implemented)
 *
 * Edge cases:
 * - Negative frame numbers: Ignored (no action)
 * - Out of range frames: Ignored (no action)
 * - Invalid frame types: Ignored with warning
 * - Null/undefined: Ignored (no action)
 * - Frame labels: Parsed and logged but not executed (requires label registry)
 * - Target paths: Parsed and logged but not executed (requires MovieClip infrastructure)
 *
 * SWF version: 4+
 * Opcode: 0x9E
 *
 * @param stack Pointer to the runtime stack
 * @param sp Pointer to stack pointer
 */
void actionCall(SWFAppContext* app_context)
{
	// Access global frame info (set by swfStart)
	extern frame_func* g_frame_funcs;
	extern size_t g_frame_count;
	extern int quit_swf;
	
	// Pop frame identifier from stack
	ActionVar frame_var;
	popVar(app_context, &frame_var);
	
	if (frame_var.type == ACTION_STACK_VALUE_F32) {
		// Numeric frame
		float frame_float;
		memcpy(&frame_float, &frame_var.value, sizeof(float));
		
		// Handle negative frames (ignore)
		s32 frame_num = (s32)frame_float;
		if (frame_num < 0) {
			printf("// Call: negative frame %d (ignored)\n", frame_num);
			fflush(stdout);
			return;
		}
		
		// Validate frame is in range
		if (g_frame_funcs && (size_t)frame_num < g_frame_count) {
			printf("// Call: frame %d\n", frame_num);
			fflush(stdout);
			
			// Save quit_swf state to prevent frame from terminating execution
			int saved_quit_swf = quit_swf;
			quit_swf = 0;
			
			// Call the frame function (executes frame actions)
			// Note: This calls the full frame function including ShowFrame
			g_frame_funcs[frame_num](app_context);
			
			// Restore quit_swf state (only quit if we were already quitting)
			quit_swf = saved_quit_swf;
		} else {
			printf("// Call: frame %d out of range (ignored, total frames: %zu)\n", frame_num, g_frame_count);
			fflush(stdout);
		}
	}
	else if (frame_var.type == ACTION_STACK_VALUE_STRING) {
		// Frame label or number as string - may include target path
		const char* frame_str = (const char*)frame_var.value;
		
		if (frame_str == NULL) {
			printf("// Call: null frame identifier (ignored)\n");
			fflush(stdout);
			return;
		}
		
		// Parse target path if present (format: "target:frame" or "/target:frame")
		const char* target = NULL;
		const char* frame_part = frame_str;
		const char* colon = strchr(frame_str, ':');
		
		if (colon != NULL) {
			// Target path present
			size_t target_len = colon - frame_str;
			static char target_buffer[256];
			
			if (target_len < sizeof(target_buffer)) {
				memcpy(target_buffer, frame_str, target_len);
				target_buffer[target_len] = '\0';
				target = target_buffer;
				frame_part = colon + 1;  // Frame label/number after the colon
			}
		}
		
		// Check if frame_part is numeric or a label
		char* endptr;
		long frame_num = strtol(frame_part, &endptr, 10);
		
		if (endptr != frame_part && *endptr == '\0') {
			// It's a numeric frame
			if (frame_num < 0) {
				if (target) {
					printf("// Call: target '%s', negative frame %ld (ignored)\n", target, frame_num);
				} else {
					printf("// Call: negative frame %ld (ignored)\n", frame_num);
				}
				fflush(stdout);
				return;
			}
			
			if (target) {
				// Target path specified - requires MovieClip infrastructure
				printf("// Call: target '%s', frame %ld (target paths not implemented)\n", target, frame_num);
				fflush(stdout);
				// Note: Full implementation would require MovieClip tree traversal
			} else {
				// Main timeline - can execute
				if (g_frame_funcs && (size_t)frame_num < g_frame_count) {
					printf("// Call: frame %ld\n", frame_num);
					fflush(stdout);
					
					// Save quit_swf state to prevent frame from terminating execution
					int saved_quit_swf = quit_swf;
					quit_swf = 0;
					
					// Call the frame function (executes frame actions)
					g_frame_funcs[frame_num](app_context);
					
					// Restore quit_swf state
					quit_swf = saved_quit_swf;
				} else {
					printf("// Call: frame %ld out of range (ignored, total frames: %zu)\n", frame_num, g_frame_count);
					fflush(stdout);
				}
			}
		} else {
			// It's a frame label
			if (target) {
				printf("// Call: target '%s', label '%s' (frame labels not implemented)\n", target, frame_part);
			} else {
				printf("// Call: label '%s' (frame labels not implemented)\n", frame_part);
			}
			fflush(stdout);
			
			// Note: Frame label lookup requires:
			// - Frame label registry (mapping labels to frame numbers)
			// - SWFRecomp to parse FrameLabel tags (tag type 43) and generate the registry
			// - MovieClip context switching for target paths
		}
	}
	else if (frame_var.type == ACTION_STACK_VALUE_UNDEFINED) {
		// Undefined - ignore
		printf("// Call: undefined frame (ignored)\n");
		fflush(stdout);
	}
	else {
		// Invalid type - ignore with warning
		printf("// Call: invalid frame type %d (ignored)\n", frame_var.type);
		fflush(stdout);
	}
	// If frame not found or invalid, do nothing (per SWF spec)
}

// Helper function to print a string value that may be a regular string or STR_LIST
static void printStringValue(ActionVar* var)
{
	if (var->type == ACTION_STACK_VALUE_STRING) {
		printf("%s", (const char*)var->value);
	} else if (var->type == ACTION_STACK_VALUE_STR_LIST) {
		// STR_LIST: first element is count, rest are string pointers
		u64* str_list = (u64*)var->value;
		u64 count = str_list[0];
		for (u64 i = 0; i < count; i++) {
			printf("%s", (const char*)str_list[i + 1]);
		}
	}
	// For other types, print nothing (empty string)
}

/**
 * ActionGetURL2 - Stack-based URL loading with HTTP method support
 *
 * Stack: [ url, target ] -> [ ]
 *
 * Pops target and URL from stack, then performs URL loading based on flags:
 * - SendVarsMethod (bits 7-6): 0=None, 1=GET, 2=POST
 * - LoadTargetFlag (bit 1): 0=browser window, 1=sprite path
 * - LoadVariablesFlag (bit 0): 0=load content, 1=load variables
 *
 * In NO_GRAPHICS mode: Logs the operation but does not perform actual
 * HTTP requests, browser integration, SWF loading, or variable setting.
 * Full implementation would require:
 * - HTTP client (libcurl or similar)
 * - Platform-specific browser integration
 * - SWF parser and loader
 * - Full sprite/timeline variable management
 * - Security sandbox enforcement
 *
 * SWF version: 4+
 * Opcode: 0x9A
 *
 * @param stack Pointer to the runtime stack
 * @param sp Pointer to stack pointer
 * @param send_vars_method HTTP method: 0=None, 1=GET, 2=POST
 * @param load_target_flag Target type: 0=window, 1=sprite
 * @param load_variables_flag Load type: 0=content, 1=variables
 */
void actionGetURL2(SWFAppContext* app_context, u8 send_vars_method, u8 load_target_flag, u8 load_variables_flag)
{
	// Pop target from stack
	// convertString() is called to handle the case where the value might be a number
	// that needs to be converted to a string, though in practice URLs/targets are always strings
	char target_str[17];
	ActionVar target_var;
	convertString(app_context, target_str);
	popVar(app_context, &target_var);
	
	// Pop URL from stack
	char url_str[17];
	ActionVar url_var;
	convertString(app_context, url_str);
	popVar(app_context, &url_var);
	
	// Determine HTTP method
	const char* method = "NONE";
	if (send_vars_method == 1) method = "GET";
	else if (send_vars_method == 2) method = "POST";
	
	// Determine operation type
	bool is_sprite = (load_target_flag == 1);
	bool load_vars = (load_variables_flag == 1);
	
	// Log the operation (NO_GRAPHICS mode implementation)
	// In a full implementation, this would perform the actual operation
	if (is_sprite) {
		// Load into sprite/movieclip
		if (load_vars) {
			// Load variables into sprite
			// Full implementation: Make HTTP request, parse x-www-form-urlencoded response,
			// set variables in target sprite scope
			printf("// LoadVariables: ");
			printStringValue(&url_var);
			printf(" -> ");
			printStringValue(&target_var);
			printf(" (method: %s)\n", method);
		} else {
			// Load SWF into sprite
			// Full implementation: Download SWF file, parse it, load into target sprite path
			printf("// LoadMovie: ");
			printStringValue(&url_var);
			printf(" -> ");
			printStringValue(&target_var);
			printf("\n");
		}
	} else {
		// Load into browser window
		if (load_vars) {
			// Load variables into timeline
			// Full implementation: Make HTTP request, parse response, set variables in timeline
			printf("// LoadVariables: ");
			printStringValue(&url_var);
			printf(" (method: %s)\n", method);
		} else {
			// Open URL in browser
			// Full implementation: Open URL in specified browser window/frame using
			// platform-specific APIs (e.g., system(), ShellExecute on Windows, open on macOS)
			printf("// OpenURL: ");
			printStringValue(&url_var);
			printf(" (target: ");
			printStringValue(&target_var);
			if (send_vars_method != 0) {
				printf(", method: %s", method);
			}
			printf(")\n");
		}
	}
}

void actionInitArray(SWFAppContext* app_context)
{
	// 1. Pop array element count
	convertFloat(app_context);
	ActionVar count_var;
	popVar(app_context, &count_var);
	u32 num_elements = (u32) VAL(float, &count_var.value);
	
	// 2. Allocate array
	ASArray* arr = allocArray(app_context, num_elements);
	if (!arr) {
		// Handle allocation failure - push empty array or null
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &(float){0.0f}));
		return;
	}
	arr->length = num_elements;
	
	// 3. Pop elements and populate array
	// Per SWF spec: elements were pushed in reverse order (rightmost first, leftmost last)
	// Stack has: [..., elem_N, elem_N-1, ..., elem_1] with elem_1 on top
	// We pop and store sequentially: pop elem_1 -> arr[0], pop elem_2 -> arr[1], etc.
	for (u32 i = 0; i < num_elements; i++) {
		ActionVar elem;
		popVar(app_context, &elem);
		arr->elements[i] = elem;
		
		// If element is array, increment refcount
		if (elem.type == ACTION_STACK_VALUE_ARRAY) {
			retainArray((ASArray*) elem.value);
		}
		// Could also handle ACTION_STACK_VALUE_OBJECT here if needed
	}
	
	// 4. Push array reference to stack
	PUSH(ACTION_STACK_VALUE_ARRAY, (u64) arr);
}

void actionSetMember(SWFAppContext* app_context)
{
	// Stack layout (from top to bottom):
	// 1. value (the value to assign)
	// 2. property_name (the name of the property)
	// 3. object (the object to set the property on)
	
	// Pop the value to assign
	ActionVar value_var;
	popVar(app_context, &value_var);
	
	// Pop the property name
	// The property name should be a string on the stack
	ActionVar prop_name_var;
	popVar(app_context, &prop_name_var);
	
	// Get the property name as string
	const char* prop_name = NULL;
	u32 prop_name_len = 0;
	
	if (prop_name_var.type == ACTION_STACK_VALUE_STRING)
	{
		// If it's a string, use it directly
		prop_name = (const char*) prop_name_var.value;
		prop_name_len = prop_name_var.str_size;
	}
	else if (prop_name_var.type == ACTION_STACK_VALUE_F32 || prop_name_var.type == ACTION_STACK_VALUE_F64)
	{
		// If it's a number, convert it to string (for array indices)
		// Use a static buffer for conversion
		static char index_buffer[32];
		if (prop_name_var.type == ACTION_STACK_VALUE_F32)
		{
			float f = VAL(float, &prop_name_var.value);
			snprintf(index_buffer, sizeof(index_buffer), "%.15g", f);
		}
		else
		{
			double d = VAL(double, &prop_name_var.value);
			snprintf(index_buffer, sizeof(index_buffer), "%.15g", d);
		}
		prop_name = index_buffer;
		prop_name_len = strlen(index_buffer);
	}
	else
	{
		// Unknown type for property name - error case
		// Just pop the object and return
		POP();
		return;
	}
	
	// Pop the object
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// Check if the object is actually an object type
	if (obj_var.type == ACTION_STACK_VALUE_OBJECT)
	{
		ASObject* obj = (ASObject*) obj_var.value;
		if (obj != NULL)
		{
			// Set the property on the object
			setProperty(app_context, obj, prop_name, prop_name_len, &value_var);
		}
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
	
#ifdef DEBUG
	printf("[DEBUG] actionInitObject: creating object with %u properties\n", num_props);
#endif

	// Step 2: Allocate object with the specified number of properties
	ASObject* obj = allocObject(app_context, num_props);
	if (obj == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate object in actionInitObject\n");
		// Push null/undefined object on error
		PUSH(ACTION_STACK_VALUE_OBJECT, 0);
		return;
	}
	
	// Step 3: Pop property name/value pairs from stack
	// Properties are in reverse order: rightmost property is on top of stack
	// Stack order is: [..., value1, name1, ..., valueN, nameN, count]
	// So after popping count, top of stack is nameN
	for (u32 i = 0; i < num_props; i++)
	{
		// Pop property name first (it's on top)
		ActionVar name_var;
		popVar(app_context, &name_var);
		
		// Pop property value (it's below the name)
		ActionVar value;
		popVar(app_context, &value);
		const char* name = NULL;
		u32 name_length = 0;
		
		// Handle string name
		if (name_var.type == ACTION_STACK_VALUE_STRING)
		{
			name = name_var.owns_memory ?
				name_var.heap_ptr :
				(const char*) name_var.value;
			name_length = name_var.str_size;
		}
		else
		{
			// If name is not a string, skip this property
			fprintf(stderr, "WARNING: Property name is not a string (type=%d), skipping\n", name_var.type);
			continue;
		}
		
#ifdef DEBUG
		printf("[DEBUG] actionInitObject: setting property '%.*s'\n", name_length, name);
#endif

		// Store property using the object API
		// This handles refcount management if value is an object
		setProperty(app_context, obj, name, name_length, &value);
	}
	
	// Step 4: Push object reference to stack
	// The object has refcount = 1 from allocation
	PUSH(ACTION_STACK_VALUE_OBJECT, (u64) obj);
	
#ifdef DEBUG
	printf("[DEBUG] actionInitObject: pushed object %p to stack\n", (void*)obj);
#endif
}

// Helper function to push undefined value
static void pushUndefined(SWFAppContext* app_context)
{
	PUSH(ACTION_STACK_VALUE_UNDEFINED, 0);
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
			prop_name_var.heap_ptr :
			(const char*) prop_name_var.value;
		prop_name_len = prop_name_var.str_size;
	}
	else
	{
		// Property name must be a string
		// Return true (AS2 spec: returns true for invalid operations)
		float result = 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
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
			obj_name_var.heap_ptr :
			(const char*) obj_name_var.value;
		obj_name_len = obj_name_var.str_size;
	}
	else
	{
		// Object name must be a string
		// Return true (AS2 spec: returns true for invalid operations)
		float result = 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return;
	}
	
	// Look up the variable to get the object
	ActionVar* obj_var = getVariable(app_context, (char*)obj_name, obj_name_len);
	
	// If variable doesn't exist, return true (AS2 spec)
	if (obj_var == NULL)
	{
		float result = 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return;
	}
	
	// If variable is not an object, return true (AS2 spec)
	if (obj_var->type != ACTION_STACK_VALUE_OBJECT)
	{
		float result = 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return;
	}
	
	// Get the object
	ASObject* obj = (ASObject*) obj_var->value;
	
	// If object is NULL, return true
	if (obj == NULL)
	{
		float result = 1.0f;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return;
	}
	
	// Delete the property
	bool success = deleteProperty(app_context, obj, prop_name, prop_name_len);
	
	// Push result (1.0 for success, 0.0 for failure)
	float result = success ? 1.0f : 0.0f;
	PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
}

void actionGetMember(SWFAppContext* app_context)
{
	// 1. Convert and pop property name (top of stack)
	char str_buffer[17];
	convertString(app_context, str_buffer);
	const char* prop_name = (const char*) VAL(u64, &STACK_TOP_VALUE);
	u32 prop_name_len = STACK_TOP_N;
	POP();
	
	// 2. Pop object (second on stack)
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// 3. Handle different object types
	if (obj_var.type == ACTION_STACK_VALUE_OBJECT)
	{
		// Handle AS object
		ASObject* obj = (ASObject*) obj_var.value;
		
		if (obj == NULL)
		{
			pushUndefined(app_context);
			return;
		}
		
		// Look up property with prototype chain support
		ActionVar* prop = getPropertyWithPrototype(obj, prop_name, prop_name_len);
		
		if (prop != NULL)
		{
			// Property found - push its value
			pushVar(app_context, prop);
		}
		else
		{
			// Property not found - push undefined
			pushUndefined(app_context);
		}
	}
	else if (obj_var.type == ACTION_STACK_VALUE_STRING)
	{
		// Handle string properties
		if (strcmp(prop_name, "length") == 0)
		{
			// Get string pointer
			const char* str = obj_var.owns_memory ?
				obj_var.heap_ptr :
				(const char*) obj_var.value;
				
			// Push length as float
			float len = (float) strlen(str);
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &len));
		}
		else
		{
			// Other properties don't exist on strings
			pushUndefined(app_context);
		}
	}
	else if (obj_var.type == ACTION_STACK_VALUE_ARRAY)
	{
		// Handle array properties
		ASArray* arr = (ASArray*) obj_var.value;
		
		if (arr == NULL)
		{
			pushUndefined(app_context);
			return;
		}
		
		// Check if accessing the "length" property
		if (strcmp(prop_name, "length") == 0)
		{
			// Push array length as float
			float len = (float) arr->length;
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &len));
		}
		else
		{
			// Try to parse property name as an array index
			char* endptr;
			long index = strtol(prop_name, &endptr, 10);
			
			// Check if conversion was successful and entire string was consumed
			if (*endptr == '\0' && index >= 0)
			{
				// Valid numeric index
				ActionVar* elem = getArrayElement(arr, (u32)index);
				if (elem != NULL)
				{
					// Element exists - push its value
					pushVar(app_context, elem);
				}
				else
				{
					// Index out of bounds - push undefined
					pushUndefined(app_context);
				}
			}
			else
			{
				// Non-numeric property name - arrays don't have other properties
				pushUndefined(app_context);
			}
		}
	}
	else
	{
		// Other primitive types (number, undefined, etc.) - push undefined
		pushUndefined(app_context);
	}
}

void actionNewObject(SWFAppContext* app_context)
{
	// 1. Pop constructor name (string)
	ActionVar ctor_name_var;
	popVar(app_context, &ctor_name_var);
	const char* ctor_name;
	u32 ctor_name_len;
	if (ctor_name_var.type == ACTION_STACK_VALUE_STRING)
	{
		ctor_name = ctor_name_var.owns_memory ?
			ctor_name_var.heap_ptr :
			(const char*) ctor_name_var.value;
		ctor_name_len = ctor_name_var.str_size;
	}
	else
	{
		// Fallback if not a string (shouldn't happen in normal cases)
		ctor_name = "Object";
		ctor_name_len = 6;
	}
	
	// 2. Pop number of arguments
	convertFloat(app_context);
	ActionVar num_args_var;
	popVar(app_context, &num_args_var);
	u32 num_args = (u32) VAL(float, &num_args_var.value);
	
	// 3. Pop arguments from stack (store them temporarily)
	// Limit to 16 arguments for simplicity
	ActionVar args[16];
	if (num_args > 16)
	{
		num_args = 16;
	}
	
	// Pop arguments in reverse order (first arg is deepest on stack)
	for (int i = (int)num_args - 1; i >= 0; i--)
	{
		popVar(app_context, &args[i]);
	}
	
	// 4. Create new object based on constructor name
	void* new_obj = NULL;
	ActionStackValueType obj_type = ACTION_STACK_VALUE_OBJECT;
	
	if (strcmp(ctor_name, "Array") == 0)
	{
		// Handle Array constructor
		if (num_args == 0)
		{
			// new Array() - empty array
			ASArray* arr = allocArray(app_context, 4);
			arr->length = 0;
			new_obj = arr;
		}
		else if (num_args == 1 &&
		         (args[0].type == ACTION_STACK_VALUE_F32 ||
		          args[0].type == ACTION_STACK_VALUE_F64))
		{
			// new Array(length) - array with specified length
			float length_f = (args[0].type == ACTION_STACK_VALUE_F32) ?
				VAL(float, &args[0].value) :
				(float) VAL(double, &args[0].value);
			u32 length = (u32) length_f;
			ASArray* arr = allocArray(app_context, length > 0 ? length : 4);
			arr->length = length;
			new_obj = arr;
		}
		else
		{
			// new Array(elem1, elem2, ...) - array with elements
			ASArray* arr = allocArray(app_context, num_args);
			arr->length = num_args;
			for (u32 i = 0; i < num_args; i++)
			{
				arr->elements[i] = args[i];
				// Retain if object/array
				if (args[i].type == ACTION_STACK_VALUE_OBJECT)
				{
					retainObject((ASObject*) args[i].value);
				}
				else if (args[i].type == ACTION_STACK_VALUE_ARRAY)
				{
					retainArray((ASArray*) args[i].value);
				}
			}
			new_obj = arr;
		}
		obj_type = ACTION_STACK_VALUE_ARRAY;
		PUSH(ACTION_STACK_VALUE_ARRAY, (u64) new_obj);
		return;
	}
	else if (strcmp(ctor_name, "Object") == 0)
	{
		// Handle Object constructor
		// Create empty object with initial capacity
		ASObject* obj = allocObject(app_context, 8);
		new_obj = obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
		return;
	}
	else if (strcmp(ctor_name, "Date") == 0)
	{
		// Handle Date constructor
		// In a full implementation, this would parse date arguments
		// For now, create object with basic time property set to current time
		ASObject* date = allocObject(app_context, 4);
		
		// Set time property to current milliseconds since epoch
		ActionVar time_var;
		time_var.type = ACTION_STACK_VALUE_F64;
		double current_time = (double)time(NULL) * 1000.0;  // Convert to milliseconds
		VAL(double, &time_var.value) = current_time;
		setProperty(app_context, date, "time", 4, &time_var);
		
		new_obj = date;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
		return;
	}
	else if (strcmp(ctor_name, "String") == 0)
	{
		// Handle String constructor
		// new String() or new String(value)
		ASObject* str_obj = allocObject(app_context, 4);
		
		// If argument provided, convert to string and store as value property
		if (num_args > 0)
		{
			// Convert first argument to string
			char str_buffer[256];
			const char* str_value = "";
			
			if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				str_value = args[0].owns_memory ?
					args[0].heap_ptr :
					(const char*) args[0].value;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				snprintf(str_buffer, sizeof(str_buffer), "%.15g", VAL(float, &args[0].value));
				str_value = str_buffer;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				snprintf(str_buffer, sizeof(str_buffer), "%.15g", VAL(double, &args[0].value));
				str_value = str_buffer;
			}
			
			// Store as property
			ActionVar value_var;
			value_var.type = ACTION_STACK_VALUE_STRING;
			value_var.str_size = strlen(str_value);
			value_var.heap_ptr = strdup(str_value);
			value_var.owns_memory = true;
			setProperty(app_context, str_obj, "value", 5, &value_var);
		}
		
		new_obj = str_obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
		return;
	}
	else if (strcmp(ctor_name, "Number") == 0)
	{
		// Handle Number constructor
		// new Number() or new Number(value)
		ASObject* num_obj = allocObject(app_context, 4);
		
		// Store numeric value as property
		ActionVar value_var;
		if (num_args > 0)
		{
			// Convert first argument to number
			if (args[0].type == ACTION_STACK_VALUE_F32 || args[0].type == ACTION_STACK_VALUE_F64)
			{
				value_var = args[0];
			}
			else if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				const char* str = args[0].owns_memory ?
					args[0].heap_ptr :
					(const char*) args[0].value;
				double num = atof(str);
				value_var.type = ACTION_STACK_VALUE_F64;
				VAL(double, &value_var.value) = num;
			}
			else
			{
				// Default to 0
				value_var.type = ACTION_STACK_VALUE_F32;
				VAL(float, &value_var.value) = 0.0f;
			}
		}
		else
		{
			// No arguments - default to 0
			value_var.type = ACTION_STACK_VALUE_F32;
			VAL(float, &value_var.value) = 0.0f;
		}
		
		setProperty(app_context, num_obj, "value", 5, &value_var);
		new_obj = num_obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
		return;
	}
	else if (strcmp(ctor_name, "Boolean") == 0)
	{
		// Handle Boolean constructor
		// new Boolean() or new Boolean(value)
		ASObject* bool_obj = allocObject(app_context, 4);
		
		// Store boolean value as property
		ActionVar value_var;
		value_var.type = ACTION_STACK_VALUE_F32;
		
		if (num_args > 0)
		{
			// Convert first argument to boolean (0 or 1)
			float bool_val = 0.0f;
			
			if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				bool_val = (VAL(float, &args[0].value) != 0.0f) ? 1.0f : 0.0f;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				bool_val = (VAL(double, &args[0].value) != 0.0) ? 1.0f : 0.0f;
			}
			else if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				const char* str = args[0].owns_memory ?
					args[0].heap_ptr :
					(const char*) args[0].value;
				bool_val = (str != NULL && strlen(str) > 0) ? 1.0f : 0.0f;
			}
			
			VAL(float, &value_var.value) = bool_val;
		}
		else
		{
			// No arguments - default to false
			VAL(float, &value_var.value) = 0.0f;
		}
		
		setProperty(app_context, bool_obj, "value", 5, &value_var);
		new_obj = bool_obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
		return;
	}
	else
	{
		// Try to find user-defined constructor function
		ASFunction* ctor_func = lookupFunctionByName(ctor_name, ctor_name_len);
		
		if (ctor_func != NULL)
		{
			// User-defined constructor found
			// Create new object to serve as 'this'
			ASObject* obj = allocObject(app_context, 8);
			new_obj = obj;
			
			// Call the constructor with 'this' binding
			if (ctor_func->function_type == 1)
			{
				// DefineFunction (type 1) - simple function
				// Push 'this' and arguments to stack, call function
				// Note: Constructor return value is discarded per spec
				
				// For now, just create the object without calling constructor
				// Full implementation would require stack manipulation to call constructor
			}
			else if (ctor_func->function_type == 2)
			{
				// DefineFunction2 (type 2) - advanced function with registers
				// This supports 'this' binding and proper constructor semantics
				
				// Prepare arguments for the constructor
				ActionVar registers[256] = {0};  // Max registers
				
				// Call constructor with 'this' binding
				// Note: Return value is discarded per ActionScript spec for constructors
				if (ctor_func->advanced_func != NULL)
				{
					ActionVar return_value = ctor_func->advanced_func(app_context, args, num_args, registers, obj);
					
					// Check if constructor returned an object (override default behavior)
					// Per ECMAScript spec: if constructor returns object, use it; otherwise use 'this'
					if (return_value.type == ACTION_STACK_VALUE_OBJECT && return_value.value != 0)
					{
						// Constructor returned an object - use it instead of default 'this'
						releaseObject(app_context, obj);  // Release the originally created object
						new_obj = (ASObject*) return_value.value;
						retainObject((ASObject*) new_obj);  // Retain the returned object
					}
					// Note: If constructor returns non-object, we use the original 'this' object
				}
			}
			
			PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
			return;
		}
		else
		{
			// Unknown constructor - create generic object
			ASObject* obj = allocObject(app_context, 8);
			new_obj = obj;
			PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
			return;
		}
	}
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
 *
 * Current implementation:
 * - Built-in constructors supported: Array, Object, Date, String, Number, Boolean
 * - String/Number/Boolean wrapper objects store primitive values in 'valueOf' property
 * - Function objects as constructors: SUPPORTED (blank method name with function object)
 * - User-defined constructors: SUPPORTED (method property containing function object)
 * - 'this' binding: SUPPORTED for DefineFunction2, limited for DefineFunction
 * - Constructor return value: Discarded per spec (always returns new object)
 *
 * Remaining limitations:
 * - Prototype chains not implemented (requires __proto__ property support)
 * - DefineFunction (type 1) has limited 'this' context support
 */
void actionNewMethod(SWFAppContext* app_context)
{
	// Pop in order: method_name, object, num_args, then args
	
	// 1. Pop method name (string)
	char str_buffer[17];
	convertString(app_context, str_buffer);
	const char* method_name = (const char*) VAL(u64, &STACK_TOP_VALUE);
	u32 method_name_len = STACK_TOP_N;
	POP();
	
	// 2. Pop object reference
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// 3. Pop number of arguments
	convertFloat(app_context);
	ActionVar num_args_var;
	popVar(app_context, &num_args_var);
	u32 num_args = (u32) VAL(float, &num_args_var.value);
	
	// 4. Pop arguments from stack (store them temporarily)
	// Limit to 16 arguments for simplicity
	ActionVar args[16];
	if (num_args > 16)
	{
		num_args = 16;
	}
	
	// Pop arguments in reverse order (first arg is deepest on stack)
	for (int i = (int)num_args - 1; i >= 0; i--)
	{
		popVar(app_context, &args[i]);
	}
	
	// 5. Get the method property from the object
	const char* ctor_name = NULL;
	
	// Check for blank/empty method name (SWF spec: treat object as function)
	if (method_name == NULL || method_name_len == 0 || method_name[0] == '\0')
	{
		// Blank method name: object should be invoked as function/constructor
		// The object should be a function object (ACTION_STACK_VALUE_FUNCTION)
		if (obj_var.type == ACTION_STACK_VALUE_FUNCTION)
		{
			ASFunction* func = (ASFunction*) obj_var.value;
			
			if (func != NULL)
			{
				// Create new object for 'this' context
				ASObject* new_obj = allocObject(app_context, 8);
				
				// TODO: Set up prototype chain (new_obj.__proto__ = func.prototype)
				// This requires prototype support in the object system
				
				// Call function as constructor with 'this' binding
				ActionVar return_value;
				
				if (func->function_type == 2)
				{
					// DefineFunction2 with full register support
					ActionVar* registers = NULL;
					if (func->register_count > 0) {
						registers = (ActionVar*) HCALLOC(func->register_count, sizeof(ActionVar));
					}
					
					// Create local scope for function
					ASObject* local_scope = allocObject(app_context, 8);
					if (scope_depth < MAX_SCOPE_DEPTH) {
						scope_chain[scope_depth++] = local_scope;
					}
					
					// Call with 'this' context set to new object
					return_value = func->advanced_func(app_context, args, num_args, registers, new_obj);
					
					// Pop local scope
					if (scope_depth > 0) {
						scope_depth--;
					}
					releaseObject(app_context, local_scope);
					
					if (registers != NULL) FREE(registers);
				}
				else
				{
					// Simple DefineFunction (type 1)
					// Push arguments onto stack for the function
					for (u32 i = 0; i < num_args; i++)
					{
						pushVar(app_context, &args[i]);
					}
					
					// Call simple function
					// Note: Simple functions don't have 'this' context support in current implementation
					func->simple_func(app_context);
					
					// Pop return value if one was pushed
					if (SP < INITIAL_SP)
					{
						popVar(app_context, &return_value);
					}
					else
					{
						return_value.type = ACTION_STACK_VALUE_UNDEFINED;
						return_value.value = 0;
					}
				}
				
				// According to SWF spec: constructor return value should be discarded
				// Always return the newly created object
				// (unless constructor explicitly returns an object, but we simplify here)
				PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
				return;
			}
		}
		
		// If not a function object, push undefined
		pushUndefined(app_context);
		return;
	}
	
	ASFunction* user_ctor_func = NULL;
	
	if (obj_var.type == ACTION_STACK_VALUE_OBJECT)
	{
		ASObject* obj = (ASObject*) obj_var.value;
		
		if (obj != NULL)
		{
			// Look up the method property
			ActionVar* method_prop = getProperty(obj, method_name, method_name_len);
			
			if (method_prop != NULL)
			{
				if (method_prop->type == ACTION_STACK_VALUE_STRING)
				{
					// Get constructor name from the property (for built-in constructors)
					ctor_name = method_prop->owns_memory ?
						method_prop->heap_ptr :
						(const char*) method_prop->value;
				}
				else if (method_prop->type == ACTION_STACK_VALUE_FUNCTION)
				{
					// Property is a user-defined function - use it as constructor
					user_ctor_func = (ASFunction*) method_prop->value;
				}
			}
		}
	}
	
	// 6. Create new object based on constructor name
	void* new_obj = NULL;
	
	if (ctor_name != NULL && strcmp(ctor_name, "Array") == 0)
	{
		// Handle Array constructor
		if (num_args == 0)
		{
			// new Array() - empty array
			ASArray* arr = allocArray(app_context, 4);
			arr->length = 0;
			new_obj = arr;
		}
		else if (num_args == 1 &&
		         (args[0].type == ACTION_STACK_VALUE_F32 ||
		          args[0].type == ACTION_STACK_VALUE_F64))
		{
			// new Array(length) - array with specified length
			float length_f = (args[0].type == ACTION_STACK_VALUE_F32) ?
				VAL(float, &args[0].value) :
				(float) VAL(double, &args[0].value);
			u32 length = (u32) length_f;
			ASArray* arr = allocArray(app_context, length > 0 ? length : 4);
			arr->length = length;
			new_obj = arr;
		}
		else
		{
			// new Array(elem1, elem2, ...) - array with elements
			ASArray* arr = allocArray(app_context, num_args);
			arr->length = num_args;
			for (u32 i = 0; i < num_args; i++)
			{
				arr->elements[i] = args[i];
				// Retain if object/array
				if (args[i].type == ACTION_STACK_VALUE_OBJECT)
				{
					retainObject((ASObject*) args[i].value);
				}
				else if (args[i].type == ACTION_STACK_VALUE_ARRAY)
				{
					retainArray((ASArray*) args[i].value);
				}
			}
			new_obj = arr;
		}
		PUSH(ACTION_STACK_VALUE_ARRAY, (u64) new_obj);
	}
	else if (ctor_name != NULL && strcmp(ctor_name, "Object") == 0)
	{
		// Handle Object constructor
		ASObject* obj = allocObject(app_context, 8);
		new_obj = obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
	}
	else if (ctor_name != NULL && strcmp(ctor_name, "Date") == 0)
	{
		// Handle Date constructor (simplified)
		ASObject* date = allocObject(app_context, 4);
		new_obj = date;
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj);
	}
	else if (ctor_name != NULL && strcmp(ctor_name, "String") == 0)
	{
		// Handle String constructor
		// new String() or new String(value)
		ASObject* str_obj = allocObject(app_context, 4);
		
		if (num_args > 0)
		{
			// Convert first argument to string and store it
			// Store the string value so it can be retrieved with valueOf() or toString()
			ActionVar string_value = args[0];
			
			// If not already a string, we'd need to convert it
			// For now, store the value as-is with property name "valueOf"
			setProperty(app_context, str_obj, "valueOf", 7, &string_value);
		}
		else
		{
			// new String() with no arguments - store empty string
			ActionVar empty_str;
			empty_str.type = ACTION_STACK_VALUE_STRING;
			empty_str.value = (u64) "";
			setProperty(app_context, str_obj, "valueOf", 7, &empty_str);
		}
		
		new_obj = str_obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, VAL(u64, new_obj));
	}
	else if (ctor_name != NULL && strcmp(ctor_name, "Number") == 0)
	{
		// Handle Number constructor
		// new Number() or new Number(value)
		ASObject* num_obj = allocObject(app_context, 4);
		
		if (num_args > 0)
		{
			// Store the numeric value
			ActionVar num_value = args[0];
			
			// Convert to float if not already numeric
			if (num_value.type != ACTION_STACK_VALUE_F32 &&
			    num_value.type != ACTION_STACK_VALUE_F64)
			{
				// For strings, convert to number
				if (num_value.type == ACTION_STACK_VALUE_STRING)
				{
					const char* str = num_value.owns_memory ?
						num_value.heap_ptr :
						(const char*) num_value.value;
					float fval = (float) atof(str);
					num_value.type = ACTION_STACK_VALUE_F32;
					num_value.value = VAL(u64, &fval);
				}
				else
				{
					// Default to 0 for other types
					float zero = 0.0f;
					num_value.type = ACTION_STACK_VALUE_F32;
					num_value.value = VAL(u64, &zero);
				}
			}
			
			setProperty(app_context, num_obj, "valueOf", 7, &num_value);
		}
		else
		{
			// new Number() with no arguments - store 0
			ActionVar zero_val;
			float zero = 0.0f;
			zero_val.type = ACTION_STACK_VALUE_F32;
			zero_val.value = VAL(u64, &zero);
			setProperty(app_context, num_obj, "valueOf", 7, &zero_val);
		}
		
		new_obj = num_obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, VAL(u64, new_obj));
	}
	else if (ctor_name != NULL && strcmp(ctor_name, "Boolean") == 0)
	{
		// Handle Boolean constructor
		// new Boolean() or new Boolean(value)
		ASObject* bool_obj = allocObject(app_context, 4);
		
		if (num_args > 0)
		{
			// Convert first argument to boolean
			// In ActionScript/JavaScript, false values are: false, 0, NaN, "", null, undefined
			ActionVar bool_value;
			bool truthy = true;  // Default to true
			
			if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				float fval = VAL(float, &args[0].value);
				truthy = (fval != 0.0f && !isnan(fval));
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				double dval = VAL(double, &args[0].value);
				truthy = (dval != 0.0 && !isnan(dval));
			}
			else if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				const char* str = args[0].owns_memory ?
					args[0].heap_ptr :
					(const char*) args[0].value;
				truthy = (str != NULL && str[0] != '\0');
			}
			else if (args[0].type == ACTION_STACK_VALUE_UNDEFINED)
			{
				truthy = false;
			}
			
			// Store as a number (1.0 for true, 0.0 for false)
			float bool_as_float = truthy ? 1.0f : 0.0f;
			bool_value.type = ACTION_STACK_VALUE_F32;
			bool_value.value = VAL(u64, &bool_as_float);
			setProperty(app_context, bool_obj, "valueOf", 7, &bool_value);
		}
		else
		{
			// new Boolean() with no arguments - store false (0)
			ActionVar false_val;
			float zero = 0.0f;
			false_val.type = ACTION_STACK_VALUE_F32;
			false_val.value = VAL(u64, &zero);
			setProperty(app_context, bool_obj, "valueOf", 7, &false_val);
		}
		
		new_obj = bool_obj;
		PUSH(ACTION_STACK_VALUE_OBJECT, VAL(u64, new_obj));
	}
	else if (user_ctor_func != NULL)
	{
		// User-defined constructor function from object property
		// Create new object for 'this' context
		ASObject* new_obj_inst = allocObject(app_context, 8);
		
		// TODO: Set up prototype chain (new_obj.__proto__ = func.prototype)
		
		// Call function as constructor with 'this' binding
		ActionVar return_value;
		
		if (user_ctor_func->function_type == 2)
		{
			// DefineFunction2 with full register support
			ActionVar* registers = NULL;
			if (user_ctor_func->register_count > 0) {
				registers = (ActionVar*) calloc(user_ctor_func->register_count, sizeof(ActionVar));
			}
			
			// Create local scope for function
			ASObject* local_scope = allocObject(app_context, 8);
			if (scope_depth < MAX_SCOPE_DEPTH) {
				scope_chain[scope_depth++] = local_scope;
			}
			
			// Call with 'this' context set to new object
			return_value = user_ctor_func->advanced_func(app_context, args, num_args, registers, new_obj_inst);
			
			// Pop local scope
			if (scope_depth > 0) {
				scope_depth--;
			}
			releaseObject(app_context, local_scope);
			
			if (registers != NULL) FREE(registers);
		}
		else
		{
			// Simple DefineFunction (type 1)
			// Push arguments onto stack for the function
			for (u32 i = 0; i < num_args; i++)
			{
				pushVar(app_context, &args[i]);
			}
			
			// Call simple function
			// Note: Simple functions don't have 'this' context support
			user_ctor_func->simple_func(app_context);
			
			// Pop return value if one was pushed
			if (SP < INITIAL_SP)
			{
				popVar(app_context, &return_value);
			}
			else
			{
				return_value.type = ACTION_STACK_VALUE_UNDEFINED;
				return_value.value = 0;
			}
		}
		
		// According to SWF spec: constructor return value should be discarded
		// Always return the newly created object
		// (unless constructor explicitly returns an object, but we simplify here)
		PUSH(ACTION_STACK_VALUE_OBJECT, (u64) new_obj_inst);
	}
	else
	{
		// Method not found or not a valid constructor - push undefined
		pushUndefined(app_context);
	}
}

// ==================================================================
// WITH Statement Implementation
// ==================================================================

void actionWithStart(SWFAppContext* app_context)
{
	// Pop object from stack
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	if (obj_var.type == ACTION_STACK_VALUE_OBJECT)
	{
		// Get the object pointer
		ASObject* obj = (ASObject*) obj_var.value;
		
		// Push onto scope chain (if valid and space available)
		if (obj != NULL && scope_depth < MAX_SCOPE_DEPTH)
		{
			scope_chain[scope_depth++] = obj;
#ifdef DEBUG
			printf("[DEBUG] actionWithStart: pushed object %p onto scope chain (depth=%u)\n", (void*)obj, scope_depth);
#endif
		}
		else
		{
			if (obj == NULL)
			{
				// Push null marker to maintain balance
				scope_chain[scope_depth++] = NULL;
#ifdef DEBUG
				printf("[DEBUG] actionWithStart: object is null, pushed null marker (depth=%u)\n", scope_depth);
#endif
			}
			else
			{
				fprintf(stderr, "ERROR: Scope chain overflow (depth=%u, max=%u)\n", scope_depth, MAX_SCOPE_DEPTH);
			}
		}
	}
	else
	{
		// Non-object type - push null marker to maintain balance
		if (scope_depth < MAX_SCOPE_DEPTH)
		{
			scope_chain[scope_depth++] = NULL;
#ifdef DEBUG
			printf("[DEBUG] actionWithStart: non-object type %d, pushed null marker (depth=%u)\n", obj_var.type, scope_depth);
#endif
		}
	}
}

void actionWithEnd(SWFAppContext* app_context)
{
	// Pop from scope chain
	if (scope_depth > 0)
	{
		scope_depth--;
#ifdef DEBUG
		printf("[DEBUG] actionWithEnd: popped from scope chain (depth=%u)\n", scope_depth);
#endif
	}
	else
	{
		fprintf(stderr, "ERROR: actionWithEnd called with empty scope chain\n");
	}
}

// ============================================================================
// Exception Handling (Try-Catch-Finally)
// ============================================================================

// Exception state structure
#include <setjmp.h>

// TODO: Current setjmp/longjmp implementation has a critical flaw!
// The problem: setjmp is called inside actionTryExecute(), which is called from within
// the if statement. When longjmp is triggered, it returns to setjmp inside actionTryExecute,
// which then returns false. However, the C runtime is still executing inside the try block's
// code body, so execution continues from where longjmp was called rather than jumping to
// the catch block.
//
// Solution needed: Generate code that places setjmp at the script function level, not inside
// a helper function. The generated code should look like:
//
//   if (setjmp(exception_handler) == 0) {
//       // try block
//   } else {
//       // catch block
//   }
//
// This requires modifying the SWFRecomp translator to emit setjmp inline rather than
// calling actionTryExecute().

typedef struct {
	bool exception_thrown;
	ActionVar exception_value;
	int handler_depth;
	jmp_buf exception_handler;
	int has_jmp_buf;
} ExceptionState;

static ExceptionState g_exception_state = {false, {0}, 0, {0}, 0};

void actionThrow(SWFAppContext* app_context)
{
	// Pop value to throw
	ActionVar throw_value;
	popVar(app_context, &throw_value);
	
	// Set exception state
	g_exception_state.exception_thrown = true;
	g_exception_state.exception_value = throw_value;
	
	// Check if we're in a try block
	if (g_exception_state.handler_depth == 0) {
		// Uncaught exception - print error message and exit
		printf("[Uncaught exception: ");
		
		if (throw_value.type == ACTION_STACK_VALUE_STRING) {
			const char* str = (const char*) VAL(u64, &throw_value.value);
			printf("%s", str);
		} else if (throw_value.type == ACTION_STACK_VALUE_F32) {
			float val = VAL(float, &throw_value.value);
			printf("%g", val);
		} else if (throw_value.type == ACTION_STACK_VALUE_F64) {
			double val = VAL(double, &throw_value.value);
			printf("%g", val);
		} else {
			printf("(type %d)", throw_value.type);
		}
		
		printf("]\n");
		
		// Exit to stop script execution
		exit(1);
	}
	
	// Inside a try block - jump to catch handler using longjmp
	// NOTE: Due to current implementation flaw (see TODO above), this doesn't
	// properly skip remaining try block code. Fix requires inline setjmp in generated code.
	if (g_exception_state.has_jmp_buf) {
		longjmp(g_exception_state.exception_handler, 1);
	}
}

void actionTryBegin(SWFAppContext* app_context)
{
	// Push exception handler onto handler stack
	g_exception_state.handler_depth++;
	
	// Clear exception flag for new try block
	g_exception_state.exception_thrown = false;
	g_exception_state.has_jmp_buf = 0;
}

bool actionTryExecute(SWFAppContext* app_context)
{
	// Set up exception handler using setjmp
	// This will be called again when longjmp is triggered
	// WARNING: This function-based approach has a control flow flaw (see TODO above)
	int exception_occurred = setjmp(g_exception_state.exception_handler);
	g_exception_state.has_jmp_buf = 1;
	
	// If exception occurred (longjmp was called), return false to execute catch block
	if (exception_occurred != 0) {
		g_exception_state.exception_thrown = true;
		return false;
	}
	
	// No exception yet, execute try block
	return true;
}

jmp_buf* actionGetExceptionJmpBuf(SWFAppContext* app_context)
{
	// Return pointer to the exception handler jump buffer
	// This allows setjmp to be called inline in generated code
	g_exception_state.has_jmp_buf = 1;
	return &g_exception_state.exception_handler;
}

void actionCatchToVariable(SWFAppContext* app_context, const char* var_name)
{
	// Store caught exception in named variable
	if (g_exception_state.exception_thrown)
	{
		// Get or create the variable by name
		ActionVar* var = getVariable(app_context, (char*)var_name, strlen(var_name));
		if (var) {
			*var = g_exception_state.exception_value;
		}
		g_exception_state.exception_thrown = false;
	}
}

void actionCatchToRegister(SWFAppContext* app_context, u8 reg_num)
{
	// Store caught exception in register
	if (g_exception_state.exception_thrown)
	{
#ifdef DEBUG
		printf("[DEBUG] actionCatchToRegister: storing exception in register %d\n", reg_num);
#endif
		// Validate register number
		if (reg_num >= MAX_REGISTERS) {
			fprintf(stderr, "ERROR: Invalid register number %d for catch\n", reg_num);
			g_exception_state.exception_thrown = false;
			return;
		}
		
		// Store exception value in the specified register
		g_registers[reg_num] = g_exception_state.exception_value;
		
		// Clear the exception flag
		g_exception_state.exception_thrown = false;
	}
}

void actionTryEnd(SWFAppContext* app_context)
{
	// Pop exception handler from handler stack
	g_exception_state.handler_depth--;
	
	// Clear jmp_buf flag
	g_exception_state.has_jmp_buf = 0;
	
	if (g_exception_state.handler_depth == 0)
	{
		// Clear exception if at top level
		g_exception_state.exception_thrown = false;
	}
	
#ifdef DEBUG
	printf("[DEBUG] actionTryEnd: handler_depth=%d\n", g_exception_state.handler_depth);
#endif
}

// ============================================================================

void actionDefineFunction(SWFAppContext* app_context, const char* name, void (*func)(SWFAppContext*), u32 param_count)
{
	// Create function object
	ASFunction* as_func = (ASFunction*) malloc(sizeof(ASFunction));
	if (as_func == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for function\n");
		return;
	}
	
	// Initialize function object
	strncpy(as_func->name, name, 255);
	as_func->name[255] = '\0';
	as_func->function_type = 1;  // Simple function
	as_func->param_count = param_count;
	as_func->simple_func = (SimpleFunctionPtr) func;
	as_func->advanced_func = NULL;
	as_func->register_count = 0;
	as_func->flags = 0;
	
	// Register function
	if (function_count < MAX_FUNCTIONS) {
		function_registry[function_count++] = as_func;
	} else {
		fprintf(stderr, "ERROR: Function registry full\n");
		free(as_func);
		return;
	}
	
	// If named, store in variable
	if (strlen(name) > 0) {
		ActionVar func_var;
		func_var.type = ACTION_STACK_VALUE_FUNCTION;
		func_var.str_size = 0;
		func_var.value = (u64) as_func;
		ActionVar* var = getVariable(app_context, (char*)name, strlen(name));
		if (var) {
			*var = func_var;
		}
	} else {
		// Anonymous function: push to stack
		PUSH(ACTION_STACK_VALUE_FUNCTION, (u64) as_func);
	}
}

void actionDefineFunction2(SWFAppContext* app_context, const char* name, Function2Ptr func, u32 param_count, u8 register_count, u16 flags)
{
	// Create function object
	ASFunction* as_func = (ASFunction*) malloc(sizeof(ASFunction));
	if (as_func == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for function\n");
		return;
	}
	
	// Initialize function object
	strncpy(as_func->name, name, 255);
	as_func->name[255] = '\0';
	as_func->function_type = 2;  // Advanced function
	as_func->param_count = param_count;
	as_func->simple_func = NULL;
	as_func->advanced_func = func;
	as_func->register_count = register_count;
	as_func->flags = flags;
	
	// Register function
	if (function_count < MAX_FUNCTIONS) {
		function_registry[function_count++] = as_func;
	} else {
		fprintf(stderr, "ERROR: Function registry full\n");
		free(as_func);
		return;
	}
	
	// If named, store in variable
	if (strlen(name) > 0) {
		ActionVar func_var;
		func_var.type = ACTION_STACK_VALUE_FUNCTION;
		func_var.str_size = 0;
		func_var.value = (u64) as_func;
		ActionVar* var = getVariable(app_context, (char*)name, strlen(name));
		if (var) {
			*var = func_var;
		}
	} else {
		// Anonymous function: push to stack
		PUSH(ACTION_STACK_VALUE_FUNCTION, (u64) as_func);
	}
}

void actionCallFunction(SWFAppContext* app_context, char* str_buffer)
{
	// 1. Pop function name (string) from stack
	char func_name_buffer[17];
	convertString(app_context, func_name_buffer);
	const char* func_name = (const char*) VAL(u64, &STACK_TOP_VALUE);
	u32 func_name_len = STACK_TOP_N;
	POP();
	
	// 2. Pop number of arguments
	ActionVar num_args_var;
	popVar(app_context, &num_args_var);
	u32 num_args = 0;
	
	if (num_args_var.type == ACTION_STACK_VALUE_F32)
	{
		num_args = (u32) VAL(float, &num_args_var.value);
	}
	else if (num_args_var.type == ACTION_STACK_VALUE_F64)
	{
		num_args = (u32) VAL(double, &num_args_var.value);
	}
	
	// 3. Pop arguments from stack (in reverse order)
	ActionVar* args = NULL;
	if (num_args > 0)
	{
		args = (ActionVar*) HALLOC(sizeof(ActionVar) * num_args);
		for (u32 i = 0; i < num_args; i++)
		{
			popVar(app_context, &args[num_args - 1 - i]);
		}
	}
	
	// 4. Check for built-in global functions first
	int builtin_handled = 0;
	
	// parseInt(string) - Parse string to integer
	if (func_name_len == 8 && strncmp(func_name, "parseInt", 8) == 0)
	{
		if (num_args > 0)
		{
			// Convert first argument to string
			char arg_buffer[17];
			const char* str_value = NULL;
			
			if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				str_value = (const char*) args[0].value;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				// Convert float to string
				float fval = VAL(float, &args[0].value);
				snprintf(arg_buffer, 17, "%.15g", fval);
				str_value = arg_buffer;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				// Convert double to string
				double dval = VAL(double, &args[0].value);
				snprintf(arg_buffer, 17, "%.15g", dval);
				str_value = arg_buffer;
			}
			else
			{
				// Undefined or other types -> NaN
				str_value = "NaN";
			}
			
			// Parse integer from string
			float result = (float) atoi(str_value);
			if (args != NULL) FREE(args);
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
			builtin_handled = 1;
		}
		else
		{
			// No arguments - return NaN
			if (args != NULL) FREE(args);
			float nan_val = 0.0f / 0.0f;
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &nan_val));
			builtin_handled = 1;
		}
	}
	// parseFloat(string) - Parse string to float
	else if (func_name_len == 10 && strncmp(func_name, "parseFloat", 10) == 0)
	{
		if (num_args > 0)
		{
			// Convert first argument to string
			char arg_buffer[17];
			const char* str_value = NULL;
			
			if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				str_value = (const char*) args[0].value;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				// Convert float to string
				float fval = VAL(float, &args[0].value);
				snprintf(arg_buffer, 17, "%.15g", fval);
				str_value = arg_buffer;
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				// Convert double to string
				double dval = VAL(double, &args[0].value);
				snprintf(arg_buffer, 17, "%.15g", dval);
				str_value = arg_buffer;
			}
			else
			{
				// Undefined or other types -> NaN
				str_value = "NaN";
			}
			
			// Parse float from string
			float result = (float) atof(str_value);
			if (args != NULL) FREE(args);
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
			builtin_handled = 1;
		}
		else
		{
			// No arguments - return NaN
			if (args != NULL) FREE(args);
			float nan_val = 0.0f / 0.0f;
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &nan_val));
			builtin_handled = 1;
		}
	}
	// isNaN(value) - Check if value is NaN
	else if (func_name_len == 5 && strncmp(func_name, "isNaN", 5) == 0)
	{
		if (num_args > 0)
		{
			// Convert to number and check if NaN
			float val = 0.0f;
			if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				val = VAL(float, &args[0].value);
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				val = (float) VAL(double, &args[0].value);
			}
			else if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				// Try to parse as number
				const char* str = (const char*) args[0].value;
				val = (float) atof(str);
			}
			
			float result = (val != val) ? 1.0f : 0.0f;  // NaN != NaN is true
			if (args != NULL) FREE(args);
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
			builtin_handled = 1;
		}
		else
		{
			// No arguments - isNaN(undefined) = true
			if (args != NULL) FREE(args);
			float result = 1.0f;
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
			builtin_handled = 1;
		}
	}
	// isFinite(value) - Check if value is finite
	else if (func_name_len == 8 && strncmp(func_name, "isFinite", 8) == 0)
	{
		if (num_args > 0)
		{
			// Convert to number and check if finite
			float val = 0.0f;
			if (args[0].type == ACTION_STACK_VALUE_F32)
			{
				val = VAL(float, &args[0].value);
			}
			else if (args[0].type == ACTION_STACK_VALUE_F64)
			{
				val = (float) VAL(double, &args[0].value);
			}
			else if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				const char* str = (const char*) args[0].value;
				val = (float) atof(str);
			}
			
			// Check if finite (not NaN and not infinity)
			float result = (val == val && val != INFINITY && val != -INFINITY) ? 1.0f : 0.0f;
			if (args != NULL) FREE(args);
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
			builtin_handled = 1;
		}
		else
		{
			// No arguments - isFinite(undefined) = false
			if (args != NULL) FREE(args);
			float result = 0.0f;
			PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
			builtin_handled = 1;
		}
	}
	
	// If not a built-in function, look up user-defined functions
	if (!builtin_handled)
	{
		ASFunction* func = lookupFunctionByName(func_name, func_name_len);
		
		if (func != NULL)
		{
			if (func->function_type == 2)
			{
				// DefineFunction2 with registers and this context
				ActionVar* registers = NULL;
				if (func->register_count > 0) {
					registers = (ActionVar*) HCALLOC(func->register_count, sizeof(ActionVar));
				}
				
				// Create local scope object for function-local variables
				// Start with capacity for a few local variables
				ASObject* local_scope = allocObject(app_context, 8);
				
				// Push local scope onto scope chain
				if (scope_depth < MAX_SCOPE_DEPTH) {
					scope_chain[scope_depth++] = local_scope;
				}
				
				ActionVar result = func->advanced_func(app_context, args, num_args, registers, NULL);
				
				// Pop local scope from scope chain
				if (scope_depth > 0) {
					scope_depth--;
				}
				
				// Clean up local scope object
				// Release decrements refcount and frees if refcount reaches 0
				releaseObject(app_context, local_scope);
				
				if (registers != NULL) FREE(registers);
				if (args != NULL) FREE(args);
				
				pushVar(app_context, &result);
			}
			else
			{
				// Simple DefineFunction (type 1)
				// Simple functions expect arguments on the stack, not in an array
				// We need to push arguments back onto stack in correct order
				
				// Remember stack position BEFORE pushing arguments
				// After function executes (pops args + pushes return), sp should be sp_before + 24
				u32 sp_before_args = SP;
				
				// Push arguments onto stack in order (first to last)
				// The function will pop them and bind to parameter names
				for (u32 i = 0; i < num_args; i++)
				{
					pushVar(app_context, &args[i]);
				}
				
				// Free args array before calling function
				if (args != NULL) FREE(args);
				
				// Call the simple function
				// It will pop parameters, execute body, and may push a return value
				func->simple_func(app_context);
				
				// Check if a return value was pushed
				// After function pops all args, sp should be back to sp_before_args
				// If function pushed a return, sp should be sp_before_args + 24
				if (SP == sp_before_args)
				{
					// No return value was pushed - push undefined
					// In ActionScript, functions that don't explicitly return push undefined
					pushUndefined(app_context);
				}
				// else: return value (or multiple values) already on stack - keep it
			}
		}
		else
		{
			// Function not found - push undefined
			if (args != NULL) FREE(args);
			pushUndefined(app_context);
		}
	}
}

// Helper function to call built-in string methods
// Returns 1 if method was handled, 0 if not found
static int callStringPrimitiveMethod(SWFAppContext* app_context, char* str_buffer,
                                      const char* str_value, u32 str_len,
                                      const char* method_name, u32 method_name_len,
                                      ActionVar* args, u32 num_args)
{
	// toUpperCase() - no arguments
	if (method_name_len == 11 && strncmp(method_name, "toUpperCase", 11) == 0)
	{
		// Convert string to uppercase
		int i;
		for (i = 0; i < str_len && i < 16; i++)
		{
			char c = str_value[i];
			if (c >= 'a' && c <= 'z')
			{
				str_buffer[i] = c - ('a' - 'A');
			}
			else
			{
				str_buffer[i] = c;
			}
		}
		str_buffer[i] = '\0';
		PUSH_STR(str_buffer, i);
		return 1;
	}
	
	// toLowerCase() - no arguments
	if (method_name_len == 11 && strncmp(method_name, "toLowerCase", 11) == 0)
	{
		// Convert string to lowercase
		int i;
		for (i = 0; i < str_len && i < 16; i++)
		{
			char c = str_value[i];
			if (c >= 'A' && c <= 'Z')
			{
				str_buffer[i] = c + ('a' - 'A');
			}
			else
			{
				str_buffer[i] = c;
			}
		}
		str_buffer[i] = '\0';
		PUSH_STR(str_buffer, i);
		return 1;
	}
	
	// charAt(index) - 1 argument
	if (method_name_len == 6 && strncmp(method_name, "charAt", 6) == 0)
	{
		int index = 0;
		if (num_args > 0 && args[0].type == ACTION_STACK_VALUE_F32)
		{
			index = (int)VAL(float, &args[0].value);
		}
		
		// Bounds check
		if (index < 0 || index >= str_len)
		{
			str_buffer[0] = '\0';
			PUSH_STR(str_buffer, 0);
		}
		else
		{
			str_buffer[0] = str_value[index];
			str_buffer[1] = '\0';
			PUSH_STR(str_buffer, 1);
		}
		return 1;
	}
	
	// substr(start, length) - 2 arguments
	if (method_name_len == 6 && strncmp(method_name, "substr", 6) == 0)
	{
		int start = 0;
		int length = str_len;
		
		if (num_args > 0 && args[0].type == ACTION_STACK_VALUE_F32)
		{
			start = (int)VAL(float, &args[0].value);
		}
		if (num_args > 1 && args[1].type == ACTION_STACK_VALUE_F32)
		{
			length = (int)VAL(float, &args[1].value);
		}
		
		// Handle negative start (count from end)
		if (start < 0)
		{
			start = str_len + start;
			if (start < 0) start = 0;
		}
		
		// Bounds check
		if (start >= str_len || length <= 0)
		{
			str_buffer[0] = '\0';
			PUSH_STR(str_buffer, 0);
		}
		else
		{
			if (start + length > str_len)
			{
				length = str_len - start;
			}
			
			int i;
			for (i = 0; i < length && i < 16; i++)
			{
				str_buffer[i] = str_value[start + i];
			}
			str_buffer[i] = '\0';
			PUSH_STR(str_buffer, i);
		}
		return 1;
	}
	
	// substring(start, end) - 2 arguments (different from substr!)
	if (method_name_len == 9 && strncmp(method_name, "substring", 9) == 0)
	{
		int start = 0;
		int end = str_len;
		
		if (num_args > 0 && args[0].type == ACTION_STACK_VALUE_F32)
		{
			start = (int)VAL(float, &args[0].value);
		}
		if (num_args > 1 && args[1].type == ACTION_STACK_VALUE_F32)
		{
			end = (int)VAL(float, &args[1].value);
		}
		
		// Clamp to valid range
		if (start < 0) start = 0;
		if (end < 0) end = 0;
		if (start > str_len) start = str_len;
		if (end > str_len) end = str_len;
		
		// Swap if start > end
		if (start > end)
		{
			int temp = start;
			start = end;
			end = temp;
		}
		
		int length = end - start;
		if (length <= 0)
		{
			str_buffer[0] = '\0';
			PUSH_STR(str_buffer, 0);
		}
		else
		{
			int i;
			for (i = 0; i < length && i < 16; i++)
			{
				str_buffer[i] = str_value[start + i];
			}
			str_buffer[i] = '\0';
			PUSH_STR(str_buffer, i);
		}
		return 1;
	}
	
	// indexOf(searchString, startIndex) - 1-2 arguments
	if (method_name_len == 7 && strncmp(method_name, "indexOf", 7) == 0)
	{
		const char* search_str = "";
		int search_len = 0;
		int start_index = 0;
		
		if (num_args > 0)
		{
			if (args[0].type == ACTION_STACK_VALUE_STRING)
			{
				search_str = (const char*)args[0].value;
				search_len = args[0].str_size;
			}
		}
		if (num_args > 1 && args[1].type == ACTION_STACK_VALUE_F32)
		{
			start_index = (int)VAL(float, &args[1].value);
			if (start_index < 0) start_index = 0;
		}
		
		// Search for substring
		int found_index = -1;
		if (search_len == 0)
		{
			found_index = start_index <= str_len ? start_index : -1;
		}
		else
		{
			for (int i = start_index; i <= str_len - search_len; i++)
			{
				int match = 1;
				for (int j = 0; j < search_len; j++)
				{
					if (str_value[i + j] != search_str[j])
					{
						match = 0;
						break;
					}
				}
				if (match)
				{
					found_index = i;
					break;
				}
			}
		}
		
		float result = (float)found_index;
		PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &result));
		return 1;
	}
	
	// Method not found
	return 0;
}

void actionCallMethod(SWFAppContext* app_context, char* str_buffer)
{
	// 1. Pop method name (string) from stack
	char method_name_buffer[17];
	convertString(app_context, method_name_buffer);
	const char* method_name = (const char*) VAL(u64, &STACK_TOP_VALUE);
	u32 method_name_len = STACK_TOP_N;
	POP();
	
	// 2. Pop object (receiver/this) from stack
	ActionVar obj_var;
	popVar(app_context, &obj_var);
	
	// 3. Pop number of arguments
	ActionVar num_args_var;
	popVar(app_context, &num_args_var);
	u32 num_args = 0;
	
	if (num_args_var.type == ACTION_STACK_VALUE_F32)
	{
		num_args = (u32) VAL(float, &num_args_var.value);
	}
	else if (num_args_var.type == ACTION_STACK_VALUE_F64)
	{
		num_args = (u32) VAL(double, &num_args_var.value);
	}
	
	// 4. Pop arguments from stack (in reverse order)
	ActionVar* args = NULL;
	if (num_args > 0)
	{
		args = (ActionVar*) HALLOC(sizeof(ActionVar) * num_args);
		for (u32 i = 0; i < num_args; i++)
		{
			popVar(app_context, &args[num_args - 1 - i]);
		}
	}
	
	// 5. Check for empty/blank method name - invoke object as function
	if (method_name_len == 0 || (method_name_len == 1 && method_name[0] == '\0'))
	{
		// Empty method name - invoke the object itself as a function
		if (obj_var.type == ACTION_STACK_VALUE_FUNCTION)
		{
			// Object is a function - invoke it
			ASFunction* func = lookupFunctionFromVar(&obj_var);
			
			if (func != NULL && func->function_type == 2)
			{
				// Invoke DefineFunction2
				ActionVar* registers = NULL;
				if (func->register_count > 0) {
					registers = (ActionVar*) HCALLOC(func->register_count, sizeof(ActionVar));
				}
				
				// No 'this' binding for direct function call (pass NULL)
				ActionVar result = func->advanced_func(app_context, args, num_args, registers, NULL);
				
				if (registers != NULL) FREE(registers);
				if (args != NULL) FREE(args);
				
				pushVar(app_context, &result);
				return;
			}
			else
			{
				// Simple function or invalid - push undefined
				if (args != NULL) FREE(args);
				pushUndefined(app_context);
				return;
			}
		}
		else
		{
			// Object is not a function - cannot invoke, push undefined
			if (args != NULL) FREE(args);
			pushUndefined(app_context);
			return;
		}
	}
	
	// 6. Look up the method on the object and invoke it
	if (obj_var.type == ACTION_STACK_VALUE_OBJECT)
	{
		ASObject* obj = (ASObject*) obj_var.value;
		
		if (obj == NULL)
		{
			// Null object - push undefined
			if (args != NULL) FREE(args);
			pushUndefined(app_context);
			return;
		}
		
		// Look up the method property
		ActionVar* method_prop = getProperty(obj, method_name, method_name_len);
		
		if (method_prop != NULL && method_prop->type == ACTION_STACK_VALUE_FUNCTION)
		{
			// Get function object
			ASFunction* func = lookupFunctionFromVar(method_prop);
			
			if (func != NULL && func->function_type == 2)
			{
				// Invoke DefineFunction2 with 'this' binding
				ActionVar* registers = NULL;
				if (func->register_count > 0) {
					registers = (ActionVar*) HCALLOC(func->register_count, sizeof(ActionVar));
				}
				
				ActionVar result = func->advanced_func(app_context, args, num_args, registers, (void*) obj);
				
				if (registers != NULL) FREE(registers);
				if (args != NULL) FREE(args);
				
				pushVar(app_context, &result);
			}
			else
			{
				// Simple function or invalid - push undefined
				if (args != NULL) FREE(args);
				pushUndefined(app_context);
			}
		}
		else
		{
			// Method not found or not a function - push undefined
			if (args != NULL) FREE(args);
			pushUndefined(app_context);
			return;
		}
	}
	else if (obj_var.type == ACTION_STACK_VALUE_STRING)
	{
		// String primitive - call built-in string methods
		const char* str_value = (const char*) obj_var.value;
		u32 str_len = obj_var.str_size;
		
		int handled = callStringPrimitiveMethod(app_context, str_buffer,
		                                         str_value, str_len,
		                                         method_name, method_name_len,
		                                         args, num_args);
		
		if (args != NULL) FREE(args);
		
		if (!handled)
		{
			// Method not found - push undefined
			pushUndefined(app_context);
		}
		return;
	}
	else
	{
		// Not an object or string - push undefined
		if (args != NULL) FREE(args);
		pushUndefined(app_context);
		return;
	}
}