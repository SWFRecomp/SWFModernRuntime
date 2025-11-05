#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers
#include <stackvalue.h>
#include <variables.h>

// Forward declarations
void actionStringAdd(char* stack, u32* sp, char* a_str, char* b_str);
void actionTrace(char* stack, u32* sp);

// Macros from action.h - we need to define them with proper oldSP handling
#define INITIAL_STACK_SIZE 8388608
#define INITIAL_SP INITIAL_STACK_SIZE

#define VAL(type, x) *((type*) x)

// Test result tracking
typedef struct {
    int total;
    int passed;
    int failed;
} TestResults;

TestResults results = {0, 0, 0};

// Helper to check test results
void assert_string_equals(const char* test_name, const char* expected, const char* actual) {
    results.total++;
    if (strcmp(expected, actual) == 0) {
        results.passed++;
        printf("  ✓ %s: PASS\n", test_name);
    } else {
        results.failed++;
        printf("  ✗ %s: FAIL\n", test_name);
        printf("    Expected: '%s'\n", expected);
        printf("    Got:      '%s'\n", actual);
    }
}

void assert_true(const char* test_name, bool condition) {
    results.total++;
    if (condition) {
        results.passed++;
        printf("  ✓ %s: PASS\n", test_name);
    } else {
        results.failed++;
        printf("  ✗ %s: FAIL\n", test_name);
    }
}

// Test helper functions
void push_string(char* stack, u32* sp, const char* str) {
    u32 oldSP = *sp;
    u32 len = strlen(str);

    *sp -= 4 + 4 + 8 + 8;
    *sp &= ~7;
    stack[*sp] = ACTION_STACK_VALUE_STRING;
    VAL(u32, &stack[*sp + 4]) = oldSP;
    VAL(u32, &stack[*sp + 8]) = len;
    VAL(char*, &stack[*sp + 16]) = (char*)str;
}

void push_float(char* stack, u32* sp, float value) {
    u32 oldSP = *sp;

    *sp -= 4 + 4 + 8 + 8;
    *sp &= ~7;
    stack[*sp] = ACTION_STACK_VALUE_F32;
    VAL(u32, &stack[*sp + 4]) = oldSP;
    VAL(u64, &stack[*sp + 16]) = VAL(u32, &value);
}

void pop_stack(char* stack, u32* sp) {
    *sp = VAL(u32, &stack[*sp + 4]);
}

char* get_stack_string(char* stack, u32 sp) {
    if (stack[sp] == ACTION_STACK_VALUE_STRING) {
        return (char*) VAL(u64, &stack[sp + 16]);
    }
    return NULL;
}

float get_stack_float(char* stack, u32 sp) {
    if (stack[sp] == ACTION_STACK_VALUE_F32) {
        u64 val = VAL(u64, &stack[sp + 16]);
        return VAL(float, &val);
    }
    return 0.0f;
}

ActionStackValueType get_stack_type(char* stack, u32 sp) {
    return stack[sp];
}

void push_var_to_stack(char* stack, u32* sp, ActionVar* var) {
    u32 oldSP = *sp;

    switch (var->type) {
        case ACTION_STACK_VALUE_F32:
        case ACTION_STACK_VALUE_F64:
        {
            *sp -= 4 + 4 + 8 + 8;
            *sp &= ~7;
            stack[*sp] = var->type;
            VAL(u32, &stack[*sp + 4]) = oldSP;
            VAL(u64, &stack[*sp + 16]) = var->data.numeric_value;
            break;
        }
        case ACTION_STACK_VALUE_STRING:
        {
            char* str_ptr = var->data.string_data.owns_memory ?
                var->data.string_data.heap_ptr :
                (char*) var->data.numeric_value;

            *sp -= 4 + 4 + 8 + 8;
            *sp &= ~7;
            stack[*sp] = ACTION_STACK_VALUE_STRING;
            VAL(u32, &stack[*sp + 4]) = oldSP;
            VAL(u32, &stack[*sp + 8]) = var->str_size;
            VAL(char*, &stack[*sp + 16]) = str_ptr;
            break;
        }
    }
}

// ============================================================================
// TEST CASES
// ============================================================================

void test_basic_string_variable(char* stack, u32* sp) {
    printf("\n[TEST 1] Basic String Variable\n");

    // Push a string
    push_string(stack, sp, "hello");

    // Store in variable
    ActionVar* var = getVariable("test_var", 8);
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    // Retrieve and verify
    push_var_to_stack(stack, sp, var);
    char* result = get_stack_string(stack, *sp);

    assert_string_equals("Basic string storage", "hello", result);
    assert_true("Variable owns memory", var->data.string_data.owns_memory);

    pop_stack(stack, sp);
}

void test_string_concat_to_variable(char* stack, u32* sp) {
    printf("\n[TEST 2] String Concatenation to Variable\n");

    // Push two strings
    char* str1 = "Hello ";
    char* str2 = "World";
    push_string(stack, sp, str2);
    push_string(stack, sp, str1);

    // Concatenate (creates STR_LIST)
    actionStringAdd(stack, sp, str1, str2);

    // Store result in variable
    ActionVar* var = getVariable("concat_var", 10);
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    // Retrieve and verify
    push_var_to_stack(stack, sp, var);
    char* result = get_stack_string(stack, *sp);

    assert_string_equals("Concatenated string", "Hello World", result);
    assert_true("Variable owns memory", var->data.string_data.owns_memory);
    assert_true("Result is heap allocated", result == var->data.string_data.heap_ptr);

    pop_stack(stack, sp);
}

void test_chained_concat(char* stack, u32* sp) {
    printf("\n[TEST 3] Chained String Concatenation\n");

    // First concat: "a" + "b"
    char* str_a = "a";
    char* str_b = "b";
    push_string(stack, sp, str_b);
    push_string(stack, sp, str_a);
    actionStringAdd(stack, sp, str_a, str_b);

    // Second concat: result + "c"
    char* str_c = "c";
    push_string(stack, sp, str_c);
    actionStringAdd(stack, sp, NULL, str_c);  // NULL means use stack values

    // Store in variable
    ActionVar* var = getVariable("chain_var", 9);
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    // Retrieve and verify
    push_var_to_stack(stack, sp, var);
    char* result = get_stack_string(stack, *sp);

    assert_string_equals("Chained concatenation", "abc", result);

    pop_stack(stack, sp);
}

void test_variable_reassignment(char* stack, u32* sp) {
    printf("\n[TEST 4] Variable Reassignment (Memory Leak Test)\n");

    // First assignment
    push_string(stack, sp, "first value");
    ActionVar* var = getVariable("reassign_var", 12);
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    char* first_ptr = var->data.string_data.heap_ptr;

    // Second assignment (should free first)
    push_string(stack, sp, "second value");
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    char* second_ptr = var->data.string_data.heap_ptr;

    // Verify second value
    push_var_to_stack(stack, sp, var);
    char* result = get_stack_string(stack, *sp);

    assert_string_equals("Reassignment value", "second value", result);
    assert_true("New pointer allocated", first_ptr != second_ptr);

    pop_stack(stack, sp);
}

void test_mixed_types(char* stack, u32* sp) {
    printf("\n[TEST 5] Mixed Type Variables\n");

    // Create numeric variable
    float num = 42.5f;
    push_float(stack, sp, num);
    ActionVar* num_var = getVariable("num_var", 7);
    setVariableWithValue(num_var, stack, *sp);
    pop_stack(stack, sp);

    // Create string variable
    push_string(stack, sp, "text");
    ActionVar* str_var = getVariable("str_var", 7);
    setVariableWithValue(str_var, stack, *sp);
    pop_stack(stack, sp);

    // Verify numeric variable
    push_var_to_stack(stack, sp, num_var);
    float num_result = get_stack_float(stack, *sp);
    assert_true("Numeric value correct", num_result == 42.5f);
    pop_stack(stack, sp);

    // Verify string variable
    push_var_to_stack(stack, sp, str_var);
    char* str_result = get_stack_string(stack, *sp);
    assert_string_equals("String value correct", "text", str_result);
    pop_stack(stack, sp);
}

void test_multiple_variables(char* stack, u32* sp) {
    printf("\n[TEST 6] Multiple Independent Variables\n");

    // Create three variables
    push_string(stack, sp, "var1");
    ActionVar* v1 = getVariable("v1", 2);
    setVariableWithValue(v1, stack, *sp);
    pop_stack(stack, sp);

    push_string(stack, sp, "var2");
    ActionVar* v2 = getVariable("v2", 2);
    setVariableWithValue(v2, stack, *sp);
    pop_stack(stack, sp);

    push_string(stack, sp, "var3");
    ActionVar* v3 = getVariable("v3", 2);
    setVariableWithValue(v3, stack, *sp);
    pop_stack(stack, sp);

    // Verify all three
    push_var_to_stack(stack, sp, v1);
    char* r1 = get_stack_string(stack, *sp);
    assert_string_equals("Variable 1", "var1", r1);
    pop_stack(stack, sp);

    push_var_to_stack(stack, sp, v2);
    char* r2 = get_stack_string(stack, *sp);
    assert_string_equals("Variable 2", "var2", r2);
    pop_stack(stack, sp);

    push_var_to_stack(stack, sp, v3);
    char* r3 = get_stack_string(stack, *sp);
    assert_string_equals("Variable 3", "var3", r3);
    pop_stack(stack, sp);
}

void test_empty_string(char* stack, u32* sp) {
    printf("\n[TEST 7] Empty String Variable\n");

    push_string(stack, sp, "");
    ActionVar* var = getVariable("empty_var", 9);
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    push_var_to_stack(stack, sp, var);
    char* result = get_stack_string(stack, *sp);

    assert_string_equals("Empty string", "", result);
    assert_true("Empty string owns memory", var->data.string_data.owns_memory);

    pop_stack(stack, sp);
}

void test_long_string(char* stack, u32* sp) {
    printf("\n[TEST 8] Long String Variable\n");

    // Create a long string
    char long_str[1024];
    for (int i = 0; i < 1023; i++) {
        long_str[i] = 'A' + (i % 26);
    }
    long_str[1023] = '\0';

    push_string(stack, sp, long_str);
    ActionVar* var = getVariable("long_var", 8);
    setVariableWithValue(var, stack, *sp);
    pop_stack(stack, sp);

    push_var_to_stack(stack, sp, var);
    char* result = get_stack_string(stack, *sp);

    assert_string_equals("Long string", long_str, result);
    assert_true("Long string length correct", var->str_size == 1023);

    pop_stack(stack, sp);
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("==========================================================\n");
    printf("  String Variable Storage - Comprehensive Test Suite\n");
    printf("==========================================================\n");

    // Initialize variable map
    initMap();

    // Create stack
    char* stack = (char*) malloc(INITIAL_STACK_SIZE);
    if (!stack) {
        fprintf(stderr, "Failed to allocate stack\n");
        return 1;
    }

    u32 sp_val = INITIAL_SP;
    u32* sp = &sp_val;

    // Run all tests
    test_basic_string_variable(stack, sp);
    test_string_concat_to_variable(stack, sp);
    test_chained_concat(stack, sp);
    test_variable_reassignment(stack, sp);
    test_mixed_types(stack, sp);
    test_multiple_variables(stack, sp);
    test_empty_string(stack, sp);
    test_long_string(stack, sp);

    // Cleanup
    freeMap();
    free(stack);

    // Print results
    printf("\n==========================================================\n");
    printf("  Test Results\n");
    printf("==========================================================\n");
    printf("  Total:  %d\n", results.total);
    printf("  Passed: %d\n", results.passed);
    printf("  Failed: %d\n", results.failed);
    printf("==========================================================\n");

    if (results.failed == 0) {
        printf("\n✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n\n");
        return 1;
    }
}
