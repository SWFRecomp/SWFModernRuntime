#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <stackvalue.h>
#include <variables.h>
#include <action.h>

// VAL macro is defined in action.h, so don't redefine it
#define INITIAL_STACK_SIZE 8388608
#define INITIAL_SP INITIAL_STACK_SIZE

// Test result tracking
typedef struct {
    int total;
    int passed;
    int failed;
} TestResults;

TestResults results = {0, 0, 0};

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

// Test 1: Array-based variable access with string IDs
void test_array_based_variable() {
    printf("\n[TEST 1] Array-based Variable Access (String ID = 5)\n");

    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    u32 sp_val = INITIAL_SP;
    u32* sp = &sp_val;
    u32 oldSP;

    // Push variable name "x" with ID = 5
    PUSH_STR_ID("x", 1, 5);
    // Push value "hello"
    PUSH_STR_ID("hello", 5, 6);
    // Set variable
    actionSetVariable(stack, sp);

    // Get variable back
    PUSH_STR_ID("x", 1, 5);
    actionGetVariable(stack, sp);

    // Check result
    char* result = (char*) VAL(u64, &stack[*sp + 16]);
    assert_string_equals("Array-based get/set", "hello", result);

    free(stack);
}

// Test 2: Hashmap-based variable access (ID = 0)
void test_hashmap_based_variable() {
    printf("\n[TEST 2] Hashmap-based Variable Access (String ID = 0)\n");

    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    u32 sp_val = INITIAL_SP;
    u32* sp = &sp_val;
    u32 oldSP;

    char dynamic_name[] = "dynamic_var";

    // Push variable name (dynamic, ID = 0)
    PUSH_STR(dynamic_name, strlen(dynamic_name));
    // Push value
    PUSH_STR_ID("world", 5, 0);
    // Set variable
    actionSetVariable(stack, sp);

    // Get variable back
    PUSH_STR(dynamic_name, strlen(dynamic_name));
    actionGetVariable(stack, sp);

    // Check result
    char* result = (char*) VAL(u64, &stack[*sp + 16]);
    assert_string_equals("Hashmap-based get/set", "world", result);

    free(stack);
}

// Test 3: Same ID accesses same variable (deduplication test)
void test_same_id_same_variable() {
    printf("\n[TEST 3] Same ID = Same Variable\n");

    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    u32 sp_val = INITIAL_SP;
    u32* sp = &sp_val;
    u32 oldSP;

    // Set variable with ID = 5
    PUSH_STR_ID("first_name", 10, 5);
    PUSH_STR_ID("value1", 6, 0);
    actionSetVariable(stack, sp);

    // Access same variable with different name but same ID
    PUSH_STR_ID("completely_different_name", 25, 5);  // Same ID!
    actionGetVariable(stack, sp);

    // Should get the same variable value
    char* result = (char*) VAL(u64, &stack[*sp + 16]);
    assert_string_equals("Same ID = same variable", "value1", result);

    free(stack);
}

// Test 4: String materialization still works
void test_string_materialization() {
    printf("\n[TEST 4] String Materialization (STR_LIST → heap)\n");

    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    u32 sp_val = INITIAL_SP;
    u32* sp = &sp_val;
    u32 oldSP;

    // Create a STR_LIST manually
    char* strings[] = {"Hello ", "World", "!"};
    u32 total_len = 12;

    (*sp) -= 4 + 4 + 8 + sizeof(u64) * 4;
    (*sp) &= ~7;
    stack[*sp] = ACTION_STACK_VALUE_STR_LIST;
    VAL(u32, &stack[*sp + 4]) = 0;  // String ID = 0 for STR_LIST
    VAL(u32, &stack[*sp + 8]) = total_len;
    u64* str_list = (u64*) &stack[*sp + 16];
    str_list[0] = 3;  // Number of strings
    str_list[1] = (u64)strings[0];
    str_list[2] = (u64)strings[1];
    str_list[3] = (u64)strings[2];

    // Push variable name
    u32 old_sp = *sp;
    PUSH_STR_ID("concat_var", 10, 7);

    // Swap to get [value] [name] order
    u32 temp_sp = VAL(u32, &stack[*sp + 4]);
    VAL(u32, &stack[*sp + 4]) = temp_sp;
    VAL(u32, &stack[temp_sp + 4]) = *sp;
    *sp = temp_sp;

    // Set variable (should materialize STR_LIST to heap)
    actionSetVariable(stack, sp);

    // Get it back
    PUSH_STR_ID("concat_var", 10, 7);
    actionGetVariable(stack, sp);

    // Check materialized result
    char* result = (char*) VAL(u64, &stack[*sp + 16]);
    assert_string_equals("STR_LIST materialization", "Hello World!", result);

    free(stack);
}

// Test 5: Performance comparison
void test_performance() {
    printf("\n[TEST 5] Performance Comparison\n");

    char* stack = (char*) calloc(1, INITIAL_STACK_SIZE);
    u32 sp;
    u32 oldSP;
    clock_t start, end;
    double cpu_time_array, cpu_time_hashmap;

    // Setup: Create a variable with ID = 3
    sp = INITIAL_SP;
    PUSH_STR_ID("test_var", 8, 3);
    PUSH_STR_ID("value", 5, 0);
    actionSetVariable(stack, sp);

    // Test 1: Array-based access (ID = 3)
    sp = INITIAL_SP;
    start = clock();
    for (int i = 0; i < 100000; i++) {
        PUSH_STR_ID("test_var", 8, 3);
        actionGetVariable(stack, sp);
        POP();
    }
    end = clock();
    cpu_time_array = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("  Array-based:   100K accesses in %.4f seconds\n", cpu_time_array);

    // Test 2: Hashmap-based access (ID = 0)
    sp = INITIAL_SP;
    start = clock();
    for (int i = 0; i < 100000; i++) {
        PUSH_STR("test_var", 8);  // ID = 0
        actionGetVariable(stack, sp);
        POP();
    }
    end = clock();
    cpu_time_hashmap = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("  Hashmap-based: 100K accesses in %.4f seconds\n", cpu_time_hashmap);

    double speedup = cpu_time_hashmap / cpu_time_array;
    printf("  Speedup: %.2fx faster with array-based access\n", speedup);

    assert_true("Array is faster than hashmap", speedup > 1.0);

    free(stack);
}

int main() {
    printf("==========================================================\n");
    printf("  String ID Optimization - Test Suite\n");
    printf("==========================================================\n");

    // Initialize both storage systems
    printf("\nInitializing variable storage...\n");
    printf("  Max String ID: 10\n");
    initVarArray(10);
    initMap();

    // Run all tests
    test_array_based_variable();
    test_hashmap_based_variable();
    test_same_id_same_variable();
    test_string_materialization();
    test_performance();

    // Cleanup
    freeMap();

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
