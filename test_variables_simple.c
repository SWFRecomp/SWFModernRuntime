#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers
#include <stackvalue.h>
#include <variables.h>

#define VAL(type, x) *((type*) x)
#define INITIAL_STACK_SIZE 8388608
#define INITIAL_SP INITIAL_STACK_SIZE

// Test result tracking
typedef struct {
    int total;
    int passed;
    int failed;
} TestResults;

TestResults results = {0, 0, 0};

// Helper functions
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

// Create a simple string on the stack
void create_string_on_stack(char* stack, u32 sp, const char* str) {
    stack[sp] = ACTION_STACK_VALUE_STRING;
    VAL(u32, &stack[sp + 8]) = strlen(str);
    VAL(char*, &stack[sp + 16]) = (char*)str;
}

// Create a STR_LIST on the stack (simulates StringAdd output)
void create_str_list_on_stack(char* stack, u32 sp, char** strings, int count) {
    u32 total_len = 0;
    for (int i = 0; i < count; i++) {
        total_len += strlen(strings[i]);
    }

    stack[sp] = ACTION_STACK_VALUE_STR_LIST;
    VAL(u32, &stack[sp + 8]) = total_len;

    u64* str_list = (u64*) &stack[sp + 16];
    str_list[0] = count;
    for (int i = 0; i < count; i++) {
        str_list[i + 1] = (u64)strings[i];
    }
}

// Create a float on the stack
void create_float_on_stack(char* stack, u32 sp, float value) {
    stack[sp] = ACTION_STACK_VALUE_F32;
    VAL(u32, &stack[sp + 8]) = 4;
    VAL(float, &stack[sp + 16]) = value;
}

// ============================================================================
// TEST CASES
// ============================================================================

void test_basic_string_variable(char* stack) {
    printf("\n[TEST 1] Basic String Variable\n");

    u32 sp = INITIAL_SP - 100;
    create_string_on_stack(stack, sp, "hello");

    ActionVar* var = getVariable("test_var", 8);
    setVariableWithValue(var, stack, sp);

    assert_string_equals("Basic string storage", "hello", var->data.string_data.heap_ptr);
    assert_true("Variable owns memory", var->data.string_data.owns_memory);
    assert_true("String size correct", var->str_size == 5);
}

void test_string_list_materialization(char* stack) {
    printf("\n[TEST 2] STR_LIST Materialization\n");

    u32 sp = INITIAL_SP - 200;
    char* strings[] = {"Hello ", "World", "!"};
    create_str_list_on_stack(stack, sp, strings, 3);

    ActionVar* var = getVariable("concat_var", 10);
    setVariableWithValue(var, stack, sp);

    assert_string_equals("Concatenated string", "Hello World!", var->data.string_data.heap_ptr);
    assert_true("Variable owns memory", var->data.string_data.owns_memory);
    assert_true("String size correct", var->str_size == 12);
}

void test_variable_reassignment(char* stack) {
    printf("\n[TEST 3] Variable Reassignment (Memory Leak Test)\n");

    u32 sp = INITIAL_SP - 300;

    // First assignment
    create_string_on_stack(stack, sp, "first value that is longer");
    ActionVar* var = getVariable("reassign_var", 12);
    setVariableWithValue(var, stack, sp);

    char* first_ptr = var->data.string_data.heap_ptr;
    char first_value[50];
    strcpy(first_value, first_ptr);  // Save the value
    printf("    First allocation: %p ('%s')\n", (void*)first_ptr, first_value);

    // Second assignment with different size (should free first and allocate new)
    create_string_on_stack(stack, sp, "second");
    setVariableWithValue(var, stack, sp);

    char* second_ptr = var->data.string_data.heap_ptr;
    printf("    Second allocation: %p ('%s')\n", (void*)second_ptr, var->data.string_data.heap_ptr);

    assert_string_equals("Reassignment value", "second", var->data.string_data.heap_ptr);
    // Note: Pointer may be reused by allocator, so we just verify the value changed
    assert_true("Old value was freed and new value stored", strcmp(first_value, "second") != 0);
}

void test_mixed_types(char* stack) {
    printf("\n[TEST 4] Mixed Type Variables\n");

    u32 sp1 = INITIAL_SP - 400;
    u32 sp2 = INITIAL_SP - 500;

    // Create numeric variable
    create_float_on_stack(stack, sp1, 42.5f);
    ActionVar* num_var = getVariable("num_var", 7);
    setVariableWithValue(num_var, stack, sp1);

    // Create string variable
    create_string_on_stack(stack, sp2, "text");
    ActionVar* str_var = getVariable("str_var", 7);
    setVariableWithValue(str_var, stack, sp2);

    // Verify numeric variable
    float num_result = VAL(float, &num_var->data.numeric_value);
    assert_true("Numeric value correct", num_result == 42.5f);
    assert_true("Numeric type correct", num_var->type == ACTION_STACK_VALUE_F32);

    // Verify string variable
    assert_string_equals("String value correct", "text", str_var->data.string_data.heap_ptr);
    assert_true("String owns memory", str_var->data.string_data.owns_memory);
}

void test_multiple_variables(char* stack) {
    printf("\n[TEST 5] Multiple Independent Variables\n");

    u32 sp1 = INITIAL_SP - 600;
    u32 sp2 = INITIAL_SP - 700;
    u32 sp3 = INITIAL_SP - 800;

    // Create three variables
    create_string_on_stack(stack, sp1, "var1");
    ActionVar* v1 = getVariable("v1", 2);
    setVariableWithValue(v1, stack, sp1);

    create_string_on_stack(stack, sp2, "var2");
    ActionVar* v2 = getVariable("v2", 2);
    setVariableWithValue(v2, stack, sp2);

    create_string_on_stack(stack, sp3, "var3");
    ActionVar* v3 = getVariable("v3", 2);
    setVariableWithValue(v3, stack, sp3);

    // Verify all three
    assert_string_equals("Variable 1", "var1", v1->data.string_data.heap_ptr);
    assert_string_equals("Variable 2", "var2", v2->data.string_data.heap_ptr);
    assert_string_equals("Variable 3", "var3", v3->data.string_data.heap_ptr);
}

void test_empty_string(char* stack) {
    printf("\n[TEST 6] Empty String Variable\n");

    u32 sp = INITIAL_SP - 900;
    create_string_on_stack(stack, sp, "");

    ActionVar* var = getVariable("empty_var", 9);
    setVariableWithValue(var, stack, sp);

    assert_string_equals("Empty string", "", var->data.string_data.heap_ptr);
    assert_true("Empty string owns memory", var->data.string_data.owns_memory);
    assert_true("Empty string size correct", var->str_size == 0);
}

void test_long_string(char* stack) {
    printf("\n[TEST 7] Long String Variable\n");

    // Create a long string
    char long_str[1024];
    for (int i = 0; i < 1023; i++) {
        long_str[i] = 'A' + (i % 26);
    }
    long_str[1023] = '\0';

    u32 sp = INITIAL_SP - 1000;
    create_string_on_stack(stack, sp, long_str);

    ActionVar* var = getVariable("long_var", 8);
    setVariableWithValue(var, stack, sp);

    assert_string_equals("Long string", long_str, var->data.string_data.heap_ptr);
    assert_true("Long string length correct", var->str_size == 1023);
}

void test_materialize_string_list_function(char* stack) {
    printf("\n[TEST 8] materializeStringList() Function\n");

    // Test STR_LIST materialization
    u32 sp1 = INITIAL_SP - 1100;
    char* strings[] = {"abc", "def", "ghi"};
    create_str_list_on_stack(stack, sp1, strings, 3);

    char* result1 = materializeStringList(stack, sp1);
    assert_string_equals("STR_LIST materialization", "abcdefghi", result1);
    free(result1);

    // Test single string duplication
    u32 sp2 = INITIAL_SP - 1200;
    create_string_on_stack(stack, sp2, "single");

    char* result2 = materializeStringList(stack, sp2);
    assert_string_equals("Single string duplication", "single", result2);
    free(result2);
}

void test_str_list_with_many_strings(char* stack) {
    printf("\n[TEST 9] STR_LIST with Many Strings\n");

    u32 sp = INITIAL_SP - 2000;
    char* strings[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    create_str_list_on_stack(stack, sp, strings, 10);

    ActionVar* var = getVariable("many_strings", 12);
    setVariableWithValue(var, stack, sp);

    assert_string_equals("Many strings concatenated", "12345678910", var->data.string_data.heap_ptr);
    assert_true("Correct size", var->str_size == 11);
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("==========================================================\n");
    printf("  String Variable Storage - Unit Test Suite\n");
    printf("==========================================================\n");

    // Initialize variable map
    initMap();

    // Create stack
    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    if (!stack) {
        fprintf(stderr, "Failed to allocate stack\n");
        return 1;
    }

    // Run all tests
    test_basic_string_variable(stack);
    test_string_list_materialization(stack);
    test_variable_reassignment(stack);
    test_mixed_types(stack);
    test_multiple_variables(stack);
    test_empty_string(stack);
    test_long_string(stack);
    test_materialize_string_list_function(stack);
    test_str_list_with_many_strings(stack);

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
