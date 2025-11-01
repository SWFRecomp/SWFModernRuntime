# String ID Optimization - Proof of Concept

**Date:** 2025-11-01
**Status:** Design Complete - Ready for Implementation

This document shows the **exact code changes** needed to implement LittleCube's string ID optimization.

---

## Overview

**Goal:** Use string IDs for O(1) variable access instead of O(n) string hashing.

**Changes Required:**
1. SWFRecomp: Pass string IDs through PUSH_STR
2. SWFModernRuntime: Array-based variable storage
3. SWFModernRuntime: Implement actionGetVariable/actionSetVariable

---

## Part 1: SWFRecomp Changes

### File: `src/action/action.cpp`

**Change 1: Update String Push Code Generation**

Location: Line ~275

```cpp
// BEFORE:
out_script << "\t" << "PUSH_STR(str_" << to_string(next_str_i - 1) << ", " << push_str_len << ");" << endl;

// AFTER:
out_script << "\t" << "PUSH_STR_ID(str_" << to_string(next_str_i - 1) << ", "
           << push_str_len << ", " << to_string(next_str_i - 1) << ");" << endl;
```

**Change 2: Add String Deduplication (Optional but Recommended)**

Location: Top of file (new member variable in SWFAction class)

```cpp
// In include/action/action.hpp - add to SWFAction class:
class SWFAction
{
public:
    size_t next_str_i;
    std::map<std::string, size_t> string_to_id;  // NEW: Track declared strings

    // ... rest of class
};
```

Location: In `declareString()` function (~line 356)

```cpp
// BEFORE:
void SWFAction::declareString(Context& context, char* str)
{
    context.out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
    context.out_script_decls << endl << "extern char* str_" << next_str_i << ";";
    next_str_i += 1;
}

// AFTER:
void SWFAction::declareString(Context& context, char* str)
{
    // Check if this string was already declared
    auto it = string_to_id.find(str);
    if (it != string_to_id.end()) {
        // String already exists - don't create duplicate
        return;
    }

    // New string - assign ID and declare
    string_to_id[str] = next_str_i;
    context.out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
    context.out_script_decls << endl << "extern char* str_" << next_str_i << ";";
    next_str_i += 1;
}
```

**Change 3: Get String ID When Needed**

Add helper function:

```cpp
// In SWFAction class (action.cpp)
size_t SWFAction::getStringId(char* str)
{
    auto it = string_to_id.find(str);
    if (it != string_to_id.end()) {
        return it->second;
    }

    // This shouldn't happen if declareString was called first
    return 0;  // Return 0 for "no ID"
}
```

Update PUSH_STR generation to use correct ID:

```cpp
// In PUSH action handler:
declareString(context, (char*) push_value);
size_t str_id = getStringId((char*) push_value);  // Get the actual ID
out_script << "\t" << "PUSH_STR_ID(str_" << str_id << ", "
           << push_str_len << ", " << str_id << ");" << endl;
```

**Change 4: Generate MAX_STRING_ID Constant**

At the end of script generation (in `parseActions` or in main recompilation loop):

```cpp
// After all actions are parsed:
context.out_script_defs << endl << endl
                        << "#define MAX_STRING_ID " << next_str_i << endl;
context.out_script_decls << endl
                         << "#define MAX_STRING_ID " << next_str_i << endl;
```

---

## Part 2: SWFModernRuntime Changes

### File: `include/actionmodern/action.h`

**Change 1: Add PUSH_STR_ID Macro**

Location: After existing PUSH_STR macro (~line 14)

```c
// NEW: Push string with ID
#define PUSH_STR_ID(v, n, id) \
	oldSP = *sp; \
	*sp -= 4 + 4 + 8 + 8; \
	*sp &= ~7; \
	stack[*sp] = ACTION_STACK_VALUE_STRING; \
	VAL(u32, &stack[*sp + 4]) = id; \
	VAL(u32, &stack[*sp + 8]) = n; \
	VAL(char*, &stack[*sp + 16]) = v;

// MODIFIED: PUSH_STR now calls PUSH_STR_ID with id=0
#define PUSH_STR(v, n) PUSH_STR_ID(v, n, 0)
```

**Change 2: Add Function Declarations**

Location: End of file (~line 77)

```c
// NEW: Variable access runtime functions
void actionGetVariable(char* stack, u32* sp);
void actionSetVariable(char* stack, u32* sp);
```

### File: `include/actionmodern/variables.h`

**Change 1: Add Array Storage**

Location: After existing declarations (~line 19)

```c
// NEW: Array-based variable storage for constant string IDs
extern ActionVar** var_array;
extern size_t var_array_size;

// NEW: Initialization function
void initVarArray(size_t max_string_id);

// NEW: Fast variable access by ID
ActionVar* getVariableById(u32 string_id);

// EXISTING: Keep for dynamic strings
ActionVar* getVariable(char* var_name, size_t key_size);
```

### File: `src/actionmodern/variables.c`

**Change 1: Add Global Variables**

Location: After existing var_map declaration (~line 10)

```c
hashmap* var_map = NULL;

// NEW: Array-based storage
ActionVar** var_array = NULL;
size_t var_array_size = 0;
```

**Change 2: Add Initialization Function**

Location: After initMap() (~line 15)

```c
void initMap()
{
	var_map = hashmap_create();
}

// NEW: Initialize variable array
void initVarArray(size_t max_string_id)
{
	var_array_size = max_string_id;
	var_array = (ActionVar**) calloc(var_array_size, sizeof(ActionVar*));

	if (!var_array)
	{
		EXC("Failed to allocate variable array\n");
		exit(1);
	}
}
```

**Change 3: Add Array-Based Variable Access**

Location: After getVariable() (~line 64)

```c
// NEW: Get variable by string ID (O(1) array access)
ActionVar* getVariableById(u32 string_id)
{
	if (string_id == 0 || string_id >= var_array_size)
	{
		// Invalid ID or dynamic string
		return NULL;
	}

	// Lazy allocation
	if (!var_array[string_id])
	{
		ActionVar* var = (ActionVar*) malloc(sizeof(ActionVar));
		if (!var)
		{
			EXC("Failed to allocate variable\n");
			return NULL;
		}

		// Initialize with unset type
		var->type = ACTION_STACK_VALUE_STRING;
		var->str_size = 0;
		var->data.string_data.heap_ptr = NULL;
		var->data.string_data.owns_memory = false;

		var_array[string_id] = var;
	}

	return var_array[string_id];
}
```

**Change 4: Update Cleanup**

Location: freeMap() function (~line 31)

```c
void freeMap()
{
	if (var_map)
	{
		hashmap_iterate(var_map, free_variable_callback, NULL);
		hashmap_free(var_map);
		var_map = NULL;
	}

	// NEW: Free array-based variables
	if (var_array)
	{
		for (size_t i = 0; i < var_array_size; i++)
		{
			if (var_array[i])
			{
				// Free heap-allocated strings
				if (var_array[i]->type == ACTION_STACK_VALUE_STRING &&
				    var_array[i]->data.string_data.owns_memory)
				{
					free(var_array[i]->data.string_data.heap_ptr);
				}
				free(var_array[i]);
			}
		}
		free(var_array);
		var_array = NULL;
	}
}
```

### File: `src/actionmodern/action.c`

**Change 1: Implement actionGetVariable**

Location: After existing action functions (e.g., after actionTrace ~line 680)

```c
// NEW: Get variable value and push to stack
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
```

**Change 2: Implement actionSetVariable**

Location: After actionGetVariable

```c
// NEW: Set variable value from stack
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
```

---

## Part 3: Integration Changes

### File: Generated script initialization

Currently scripts don't have an initialization function. We need to add one.

**Option A: Add to main.c or runtime initialization**

```c
// In main.c or wherever SWF is loaded:
void initialize_swf()
{
    initVarArray(MAX_STRING_ID);  // MAX_STRING_ID from generated code
    initMap();
    // ... other initialization
}
```

**Option B: Generate init function in each script**

```cpp
// In SWFRecomp, generate:
context.out_script_defs << endl << endl
    << "void script_init() {" << endl
    << "\tinitVarArray(MAX_STRING_ID);" << endl
    << "\tinitMap();" << endl
    << "}" << endl;
```

---

## Part 4: Testing

### Test 1: Update Existing Test

Modify `tests/dyna_string_vars_swf_4/runtime/native/runtime.c`:

**REMOVE** the stub implementations:
```c
// DELETE THESE:
void actionGetVariable(char* stack, u32* sp_ptr) { ... }
void actionSetVariable(char* stack, u32* sp_ptr) { ... }
```

The runtime will now use the real implementations from SWFModernRuntime!

### Test 2: Add String ID Test

Create `test_string_id_vars.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <stackvalue.h>
#include <variables.h>
#include <action.h>

#define VAL(type, x) *((type*) x)
#define INITIAL_STACK_SIZE 8388608
#define INITIAL_SP INITIAL_STACK_SIZE

int main() {
    // Initialize
    initVarArray(10);  // Support string IDs 0-9
    initMap();

    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    u32 sp = INITIAL_SP;
    u32 oldSP;

    // Test 1: Array-based variable (ID = 5)
    printf("Test 1: Array-based variable access\n");

    // Push variable name "x" with ID = 5
    PUSH_STR_ID("x", 1, 5);
    // Push value "hello"
    PUSH_STR_ID("hello", 5, 6);
    // Set variable
    actionSetVariable(stack, &sp);

    // Get variable back
    PUSH_STR_ID("x", 1, 5);
    actionGetVariable(stack, &sp);

    // Check result
    char* result = (char*) VAL(u64, &stack[sp + 16]);
    printf("  Result: '%s'\n", result);
    printf("  Expected: 'hello'\n");
    printf("  %s\n\n", strcmp(result, "hello") == 0 ? "PASS" : "FAIL");

    // Test 2: Hashmap-based variable (ID = 0, dynamic)
    printf("Test 2: Hashmap-based variable access\n");

    char dynamic_name[] = "dynamic_var";
    PUSH_STR(dynamic_name, strlen(dynamic_name));  // ID = 0
    PUSH_STR_ID("world", 5, 0);
    actionSetVariable(stack, &sp);

    PUSH_STR(dynamic_name, strlen(dynamic_name));
    actionGetVariable(stack, &sp);

    result = (char*) VAL(u64, &stack[sp + 16]);
    printf("  Result: '%s'\n", result);
    printf("  Expected: 'world'\n");
    printf("  %s\n\n", strcmp(result, "world") == 0 ? "PASS" : "FAIL");

    // Test 3: Verify same ID accesses same variable
    printf("Test 3: Same ID = same variable\n");

    PUSH_STR_ID("different_name", 14, 5);  // Same ID as test 1!
    actionGetVariable(stack, &sp);

    result = (char*) VAL(u64, &stack[sp + 16]);
    printf("  Result: '%s'\n", result);
    printf("  Expected: 'hello' (from test 1)\n");
    printf("  %s\n\n", strcmp(result, "hello") == 0 ? "PASS" : "FAIL");

    // Cleanup
    freeMap();
    free(stack);

    return 0;
}
```

### Test 3: Build New Test

Create `Makefile.test_string_id`:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -Iinclude/actionmodern -Iinclude/libswf -Ilib/c-hashmap
LDFLAGS = -lm

SOURCES = test_string_id_vars.c \
          src/actionmodern/variables.c \
          src/actionmodern/action.c \
          src/utils.c \
          lib/c-hashmap/map.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = test_string_id_vars

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET)

.PHONY: all clean test
```

---

## Part 5: Expected Results

### Before Optimization

```
Variable access: getVariable("x", 1)
    ↓
hashmap_get(var_map, "x", 1, ...)
    ↓
Compute hash of "x" (O(1) for single char, O(n) in general)
    ↓
Find bucket (O(1))
    ↓
Compare strings (O(n))
    ↓
Return variable (total: O(n) where n = strlen)
```

### After Optimization (for constant strings)

```
Variable access: getVariableById(5)
    ↓
return var_array[5]
    ↓
Return variable (total: O(1))
```

### Performance Measurement

Add timing to test:

```c
#include <time.h>

clock_t start, end;
double cpu_time_used;

// Array-based (ID = 5)
start = clock();
for (int i = 0; i < 1000000; i++) {
    PUSH_STR_ID("x", 1, 5);
    actionGetVariable(stack, &sp);
    POP();
}
end = clock();
cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
printf("Array-based: 1M accesses in %.4f seconds\n", cpu_time_used);

// Hashmap-based (ID = 0)
start = clock();
for (int i = 0; i < 1000000; i++) {
    PUSH_STR("x", 1);
    actionGetVariable(stack, &sp);
    POP();
}
end = clock();
cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
printf("Hashmap-based: 1M accesses in %.4f seconds\n", cpu_time_used);
```

**Expected:** Array-based should be 2-5x faster

---

## Summary of Changes

### SWFRecomp (4 files)
1. `include/action/action.hpp` - Add string_to_id map
2. `src/action/action.cpp` - String deduplication + PUSH_STR_ID generation
3. Generated `script_defs.c` - Add MAX_STRING_ID
4. Generated `script_0.c` - Use PUSH_STR_ID instead of PUSH_STR

### SWFModernRuntime (4 files)
1. `include/actionmodern/action.h` - Add PUSH_STR_ID macro + function declarations
2. `include/actionmodern/variables.h` - Add array storage declarations
3. `src/actionmodern/variables.c` - Implement array storage + cleanup
4. `src/actionmodern/action.c` - Implement actionGetVariable/actionSetVariable

### Total Lines Changed
- SWFRecomp: ~50 lines added/modified
- SWFModernRuntime: ~150 lines added/modified
- Tests: ~100 lines for new test

**Total: ~300 lines of code**

---

## Next Steps

1. **Get LittleCube's approval** on architecture
2. **Implement SWFRecomp changes** first (string ID generation)
3. **Implement SWFModernRuntime changes** (array storage + runtime functions)
4. **Test with existing SWF files**
5. **Benchmark performance improvement**
6. **Document and merge**

**Ready to implement!**
