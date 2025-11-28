#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <actionmodern/action.h>
#include <actionmodern/variables.h>
#include <stackvalue.h>

#define VAL(type, x) *((type*) x)

int main() {
    // Initialize variable map
    initMap();

    // Create stack
    char* stack = (char*) malloc(INITIAL_STACK_SIZE);
    u32 sp_val = INITIAL_SP;
    u32* sp = &sp_val;
    u32 oldSP;

    printf("Test 1: Simple string variable assignment\n");

    // Push string "Hello "
    char* str1 = "Hello ";
    PUSH_STR(str1, strlen(str1));

    // Get variable and set it
    ActionVar* var1 = getVariable("test_var", 8);
    setVariableWithValue(var1, stack, *sp);
    POP();

    // Push the variable back and verify
    PUSH_VAR(var1);

    if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING) {
        char* result = (char*) STACK_TOP_VALUE;
        printf("Variable value: '%s'\n", result);
        if (strcmp(result, "Hello ") == 0) {
            printf("✓ Test 1 PASSED\n\n");
        } else {
            printf("✗ Test 1 FAILED: Expected 'Hello ', got '%s'\n\n", result);
        }
    } else {
        printf("✗ Test 1 FAILED: Wrong type\n\n");
    }
    POP();

    printf("Test 2: String concatenation stored in variable\n");

    // Push two strings and concatenate
    char* str2 = "World";
    char* str3 = "Hello ";
    PUSH_STR(str2, strlen(str2));
    PUSH_STR(str3, strlen(str3));

    // Concatenate (creates STR_LIST)
    actionStringAdd(stack, sp, str3, str2);

    // Store in variable
    ActionVar* var2 = getVariable("concat_var", 10);
    setVariableWithValue(var2, stack, *sp);
    POP();

    // Push variable back and verify
    PUSH_VAR(var2);

    if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING) {
        char* result = (char*) STACK_TOP_VALUE;
        printf("Concatenated value: '%s'\n", result);
        if (strcmp(result, "Hello World") == 0) {
            printf("✓ Test 2 PASSED\n\n");
        } else {
            printf("✗ Test 2 FAILED: Expected 'Hello World', got '%s'\n\n", result);
        }
    } else {
        printf("✗ Test 2 FAILED: Wrong type\n\n");
    }
    POP();

    printf("Test 3: Variable reassignment (memory leak test)\n");

    // Reassign var2 with a new value
    char* str4 = "New Value";
    PUSH_STR(str4, strlen(str4));
    setVariableWithValue(var2, stack, *sp);
    POP();

    // Verify new value
    PUSH_VAR(var2);

    if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING) {
        char* result = (char*) STACK_TOP_VALUE;
        printf("Reassigned value: '%s'\n", result);
        if (strcmp(result, "New Value") == 0) {
            printf("✓ Test 3 PASSED\n\n");
        } else {
            printf("✗ Test 3 FAILED: Expected 'New Value', got '%s'\n\n", result);
        }
    } else {
        printf("✗ Test 3 FAILED: Wrong type\n\n");
    }
    POP();

    printf("Test 4: Numeric variable\n");

    // Store a float
    float num = 42.5f;
    PUSH(ACTION_STACK_VALUE_F32, VAL(u32, &num));

    ActionVar* var3 = getVariable("num_var", 7);
    setVariableWithValue(var3, stack, *sp);
    POP();

    // Retrieve and verify
    PUSH_VAR(var3);

    if (STACK_TOP_TYPE == ACTION_STACK_VALUE_F32) {
        float result = VAL(float, &STACK_TOP_VALUE);
        printf("Numeric value: %.1f\n", result);
        if (result == 42.5f) {
            printf("✓ Test 4 PASSED\n\n");
        } else {
            printf("✗ Test 4 FAILED: Expected 42.5, got %.1f\n\n", result);
        }
    } else {
        printf("✗ Test 4 FAILED: Wrong type\n\n");
    }
    POP();

    // Cleanup
    freeMap();
    free(stack);

    printf("All tests completed!\n");

    return 0;
}
