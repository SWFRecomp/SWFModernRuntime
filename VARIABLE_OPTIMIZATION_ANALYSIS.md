# Variable Access Optimization Analysis

**Date:** 2025-11-01
**Status:** Analysis Complete - Recommendations Ready

## Executive Summary

This document analyzes the current variable access implementation and LittleCube's proposed **String ID optimization** for constant string variable names. The analysis covers:

1. Current GetVariable/SetVariable implementation
2. String ID system already present in SWFRecomp
3. Proposed optimization architecture
4. Implementation recommendations

---

## Current Implementation Review

### 1. Code Generation (SWFRecomp)

**Generated Code Pattern:**
```c
// Example from tests/dyna_string_vars_swf_4/RecompiledScripts/script_0.c

// Push variable name
PUSH_STR(str_0, 1);        // str_0 = "x"
// Push variable value
PUSH_STR(str_1, 10);       // str_1 = "string_var"
// Store to variable
actionSetVariable(stack, sp);

// Later: retrieve variable
PUSH_STR(str_2, 1);        // str_2 = "x" (same variable)
actionGetVariable(stack, sp);
```

**Key Observations:**
- ✅ SWFRecomp already assigns unique IDs to all constant strings (`str_0`, `str_1`, etc.)
- ✅ Variable names are pushed as strings with known IDs
- ❌ The ID information is **lost** when the string reaches the runtime
- ❌ Runtime must perform hashmap lookup by string content every time

### 2. Runtime Implementation (SWFModernRuntime)

**Current Flow:**
```c
// In actionSetVariable/actionGetVariable (stub runtime):
void actionSetVariable(char* stack, u32* sp_ptr) {
    char* value_str = pop_string(stack, sp_ptr);
    char* var_name = pop_string(stack, sp_ptr);    // ← String lookup
    setVariableString(var_name, value_str, strlen(value_str));
}
```

**Implementation Path:**
```
actionSetVariable/actionGetVariable (test stubs)
    ↓
Currently uses string-based hashmap lookup
    ↓
variables.c: getVariable(char* var_name, size_t key_size)
    ↓
hashmap_get(var_map, var_name, key_size, ...)  // ← String comparison overhead
```

**Performance Characteristics:**
- ❌ Every variable access requires string hashing
- ❌ Every variable access requires string comparison in hashmap
- ❌ Variable names like "x", "player", "score" are re-hashed repeatedly
- ✅ Handles dynamic variable names (rare in AS1/AS2)

---

## String ID System (Already Exists!)

### In SWFRecomp

**String Declaration:**
```cpp
// src/action/action.cpp:356-358
void SWFAction::declareString(Context& context, char* str) {
    context.out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
    context.out_script_decls << endl << "extern char* str_" << next_str_i << ";";
    next_str_i += 1;  // Assign unique ID
}
```

**Generated Output:**
```c
// script_defs.c
char* str_0 = "x";           // ID = 0
char* str_1 = "string_var";  // ID = 1
char* str_2 = "x";           // ID = 2 (duplicate content, but unique constant)
char* str_3 = "string_var value";  // ID = 3

// script_0.c
PUSH_STR(str_0, 1);  // Pushes "x" with implicit ID 0
```

**Key Insight:**
SWFRecomp **already knows** that `str_0`, `str_2`, and `str_4` all contain `"x"` and could be **deduplicated** or at minimum have their IDs tracked.

---

## LittleCube's Proposed Optimization

From Discord conversation:

> "I actually have the perfect optimization for most of the string-based variable name cases, which is string ids. The recompiler will evaluate all string constants in all actions in the entire swf and assign each one an id where 0 is reserved to represent no id. Whenever a string is pushed onto the stack, its string id is also pushed with it. When calling GetVariable, if the string id is not 0 then access an array of variables using the id."

### Architecture

**Compile-Time (SWFRecomp):**
1. Assign unique ID to each constant string (already done via `next_str_i`)
2. Generate PUSH_STR calls that include the string ID
3. Pass string ID through the stack alongside the string pointer

**Runtime (SWFModernRuntime):**
1. Extend stack entry to include string ID field
2. Maintain **array-based variable storage** indexed by string ID (for constants)
3. Fall back to hashmap for dynamic strings (ID = 0)

**Performance Benefit:**
```
Before:
    Variable access = O(strlen(name)) for hash + O(1) hashmap lookup

After:
    Constant variable access = O(1) array index
    Dynamic variable access = O(strlen(name)) for hash + O(1) hashmap lookup (same as before)
```

For typical AS1/AS2 code with mostly constant variable names: **Massive speedup**

---

## Current vs. Proposed Architecture

### Current Stack Entry (24 bytes)

```c
// Offset 0: Type (1 byte)
// Offset 4: Old SP (4 bytes)
// Offset 8: String size OR numeric size (4 bytes)
// Offset 16: Value pointer (8 bytes)

typedef enum {
    ACTION_STACK_VALUE_F32,
    ACTION_STACK_VALUE_F64,
    ACTION_STACK_VALUE_STRING,
    ACTION_STACK_VALUE_STR_LIST
} ActionStackValueType;
```

### Proposed Stack Entry (24 bytes - no size change!)

```c
// Offset 0: Type (1 byte)
// Offset 4: String ID (4 bytes) ← NEW: repurpose old SP field or add new
// Offset 8: String size OR numeric size (4 bytes)
// Offset 16: Value pointer (8 bytes)
```

**OR maintain 24 bytes by repurposing:**
```c
// For strings only:
// Offset 4: String ID (4 bytes) instead of oldSP
```

### Proposed Variable Storage

```c
// variables.h
typedef struct {
    ActionStackValueType type;
    u32 str_size;
    union {
        u64 numeric_value;
        struct {
            char* heap_ptr;
            bool owns_memory;
        } string_data;
    } data;
} ActionVar;

// NEW: Dual storage
extern ActionVar** var_array;      // Array indexed by string ID (fast)
extern size_t var_array_size;      // Size of array
extern hashmap* var_map;           // Hashmap for dynamic strings (existing)
```

---

## Implementation Plan

### Phase 1: String ID Tracking in Stack

**SWFRecomp Changes:**

1. **Update PUSH_STR macro generation** (src/action/action.cpp)
   ```cpp
   // Current:
   out_script << "PUSH_STR(str_" << to_string(next_str_i - 1) << ", " << push_str_len << ");" << endl;

   // Proposed:
   out_script << "PUSH_STR_ID(str_" << to_string(next_str_i - 1) << ", "
              << push_str_len << ", " << to_string(next_str_i - 1) << ");" << endl;
   ```

**SWFModernRuntime Changes:**

2. **Add PUSH_STR_ID macro** (include/actionmodern/action.h)
   ```c
   #define PUSH_STR_ID(v, n, id) \
       oldSP = *sp; \
       *sp -= 4 + 4 + 8 + 8; \
       *sp &= ~7; \
       stack[*sp] = ACTION_STACK_VALUE_STRING; \
       VAL(u32, &stack[*sp + 4]) = id;  /* Store string ID instead of oldSP */ \
       VAL(u32, &stack[*sp + 8]) = n; \
       VAL(char*, &stack[*sp + 16]) = v;

   // Keep PUSH_STR for dynamic strings (ID = 0)
   #define PUSH_STR(v, n) PUSH_STR_ID(v, n, 0)
   ```

### Phase 2: Array-Based Variable Storage

**SWFModernRuntime Changes:**

3. **Update variables.h**
   ```c
   // Add array storage
   extern ActionVar** var_array;
   extern size_t var_array_size;

   // Add new functions
   void initVarArray(size_t max_string_id);
   ActionVar* getVariableById(u32 string_id);
   ActionVar* getVariableByName(char* var_name, size_t key_size);
   ```

4. **Update variables.c**
   ```c
   ActionVar** var_array = NULL;
   size_t var_array_size = 0;

   void initVarArray(size_t max_string_id) {
       var_array_size = max_string_id + 1;
       var_array = (ActionVar**) calloc(var_array_size, sizeof(ActionVar*));
   }

   ActionVar* getVariableById(u32 string_id) {
       if (string_id >= var_array_size) {
           return NULL;  // Invalid ID
       }

       if (!var_array[string_id]) {
           // Lazy allocation
           var_array[string_id] = (ActionVar*) malloc(sizeof(ActionVar));
           var_array[string_id]->type = ACTION_STACK_VALUE_STRING;
           var_array[string_id]->str_size = 0;
           var_array[string_id]->data.string_data.heap_ptr = NULL;
           var_array[string_id]->data.string_data.owns_memory = false;
       }

       return var_array[string_id];
   }

   // Keep existing getVariable for backward compatibility and dynamic names
   ActionVar* getVariableByName(char* var_name, size_t key_size) {
       // Existing implementation (unchanged)
       // ...
   }
   ```

### Phase 3: Update actionGetVariable/actionSetVariable

**Option A: Create in SWFModernRuntime**

These don't exist in SWFModernRuntime yet! They're only in test stubs.

5. **Add to src/actionmodern/action.c**
   ```c
   void actionGetVariable(char* stack, u32* sp) {
       u32 oldSP;

       // Get variable name from stack
       u32 string_id = VAL(u32, &stack[*sp + 4]);  // NEW: Read string ID
       char* var_name = (char*) VAL(u64, &stack[*sp + 16]);
       u32 var_name_len = VAL(u32, &stack[*sp + 8]);

       // Pop variable name
       POP();

       // Get variable (fast path for constants)
       ActionVar* var;
       if (string_id != 0) {
           var = getVariableById(string_id);  // O(1) array access
       } else {
           var = getVariableByName(var_name, var_name_len);  // O(n) hashmap
       }

       // Push variable value to stack
       PUSH_VAR(var);
   }

   void actionSetVariable(char* stack, u32* sp) {
       // Get value from stack top
       ActionStackValueType value_type = STACK_TOP_TYPE;
       u32 value_sp = *sp;

       // Get variable name from second stack entry
       u32 var_name_sp = SP_SECOND_TOP;
       u32 string_id = VAL(u32, &stack[var_name_sp + 4]);  // NEW: Read string ID
       char* var_name = (char*) VAL(u64, &stack[var_name_sp + 16]);
       u32 var_name_len = VAL(u32, &stack[var_name_sp + 8]);

       // Get variable (fast path for constants)
       ActionVar* var;
       if (string_id != 0) {
           var = getVariableById(string_id);  // O(1) array access
       } else {
           var = getVariableByName(var_name, var_name_len);  // O(n) hashmap
       }

       // Set variable value (materialize strings as needed)
       setVariableWithValue(var, stack, value_sp);

       // Pop both value and variable name
       POP_2();
   }
   ```

6. **Add to include/actionmodern/action.h**
   ```c
   void actionGetVariable(char* stack, u32* sp);
   void actionSetVariable(char* stack, u32* sp);
   ```

### Phase 4: String Constant Deduplication (Optional)

**SWFRecomp Enhancement:**

Currently, identical strings get different IDs:
```c
char* str_0 = "x";  // ID 0
char* str_2 = "x";  // ID 2 (duplicate!)
char* str_4 = "x";  // ID 4 (duplicate!)
```

**Optimization:**
```cpp
// In SWFAction class, add:
std::map<std::string, size_t> string_to_id;

void SWFAction::declareString(Context& context, char* str) {
    // Check if string already declared
    auto it = string_to_id.find(str);
    if (it != string_to_id.end()) {
        // Reuse existing ID - don't increment next_str_i
        return;
    }

    // New string - assign ID
    string_to_id[str] = next_str_i;
    context.out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
    context.out_script_decls << endl << "extern char* str_" << next_str_i << ";";
    next_str_i += 1;
}
```

**Benefit:**
- Smaller var_array size
- All references to "x" would use the same ID (and thus same variable slot)

---

## Comparison with Current Implementation

### Current Implementation (STRING_VARIABLE_NEXT_STEPS.md)

The implemented solution focuses on:
- ✅ Proper string materialization (STR_LIST → heap string)
- ✅ Memory ownership tracking
- ✅ Proper cleanup on reassignment
- ✅ Works via SET_VAR macro and setVariableWithValue()

**Does NOT include:**
- ❌ String ID optimization (not mentioned in original doc)
- ❌ Array-based variable storage
- ❌ actionGetVariable/actionSetVariable runtime functions

**Key Difference:**
The current implementation **fixed the correctness issue** (string materialization).
LittleCube's optimization **improves performance** (fast variable access).

These are **complementary**, not conflicting!

---

## Recommendations

### Recommendation 1: Implement String ID Optimization

**Why:**
- Aligns with LittleCube's vision
- Provides significant performance improvement for typical AS1/AS2 code
- Builds on existing string ID infrastructure in SWFRecomp
- Maintains backward compatibility (dynamic strings still work via hashmap)

**Scope:**
- Implement Phases 1-3 (String ID tracking + Array storage + Runtime functions)
- Phase 4 (deduplication) is optional but recommended

**Effort Estimate:**
- Phase 1: 1-2 hours (macro updates)
- Phase 2: 2-3 hours (array storage implementation)
- Phase 3: 2-3 hours (runtime function implementation)
- Phase 4: 1-2 hours (deduplication)
- Testing: 2-3 hours
- **Total: 8-13 hours**

### Recommendation 2: Coordinate String ID Size

**Current:**
- SWFRecomp must communicate max string ID to runtime
- Runtime needs to allocate var_array with correct size

**Options:**

**A. Compile-Time Constant (Simple)**
```c
// Generated in script_defs.c
#define MAX_STRING_ID 100
```

**B. Runtime Initialization (Flexible)**
```c
// Generated in script initialization
void script_init() {
    initVarArray(100);  // Max string ID from this SWF
    initMap();
}
```

**Recommendation:** Use Option B for flexibility

### Recommendation 3: Preserve Current String Materialization

The string materialization logic (materializeStringList, setVariableWithValue) is **critical** and should be **preserved exactly as-is**.

The string ID optimization is purely about **how variables are looked up**, not about **how strings are stored**.

**Integration:**
```c
void actionSetVariable(char* stack, u32* sp) {
    // ... get variable via ID or name ...

    // Use existing string materialization (unchanged!)
    setVariableWithValue(var, stack, value_sp);

    // ... cleanup ...
}
```

---

## Testing Strategy

### 1. Unit Tests

Extend `test_variables_simple.c`:
- ✅ Test array-based variable access
- ✅ Test ID-based vs. name-based access
- ✅ Test string ID = 0 (dynamic strings)
- ✅ Test string deduplication if implemented

### 2. Integration Tests

- ✅ Run dyna_string_vars_swf_4 test (already exists)
- ✅ Create new test with many variables (stress test array storage)
- ✅ Create test with dynamic variable names (hash map fallback)

### 3. Performance Benchmarks

Compare before/after for typical AS1/AS2 code:
- Variable-heavy scripts (GetVariable/SetVariable in loops)
- Measure time per variable access

---

## Open Questions

1. **Stack Entry Size:** Keep at 24 bytes or expand?
   - **Recommendation:** Keep at 24 bytes, repurpose oldSP field for string ID when type is STRING

2. **String ID 0:** Reserve for "no ID" or use for first string?
   - **Recommendation:** Reserve 0 for "no ID" (dynamic strings)

3. **Array Growth:** Fixed size or dynamic?
   - **Recommendation:** Fixed size determined at SWF compile time (simpler, faster)

4. **Deduplication:** Implement in Phase 1 or later?
   - **Recommendation:** Implement in Phase 1 for maximum benefit

---

## Next Steps

1. **Discuss with LittleCube:**
   - Confirm architecture aligns with their vision
   - Agree on stack entry layout
   - Confirm string ID = 0 convention

2. **Implementation Priority:**
   - Start with Phase 1 (string ID tracking)
   - Then Phase 3 (runtime functions)
   - Then Phase 2 (array storage)
   - Finally Phase 4 (deduplication)

3. **Coordinate Repositories:**
   - SWFRecomp changes (code generation)
   - SWFModernRuntime changes (runtime functions)
   - Update documentation

---

## Conclusion

The current implementation **correctly handles string materialization** (the original problem).
LittleCube's string ID optimization **adds significant performance improvements**.

These two features are **complementary** and should both be implemented for optimal results:
- **String materialization:** Ensures correctness (strings don't get corrupted)
- **String ID optimization:** Ensures performance (O(1) variable access)

**Status:** Ready to implement pending LittleCube's approval of architecture.
