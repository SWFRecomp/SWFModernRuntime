#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <stackvalue.h>
#include <variables.h>

static SWFAppContext app_context =
{
	.frame_funcs = NULL  // Will be set in main()
#ifndef NO_GRAPHICS
	,
	.width = 800,
	.height = 600,
	.stage_to_ndc = NULL,
	.bitmap_count = 0,
	.bitmap_highest_w = 0,
	.bitmap_highest_h = 0,
	.shape_data = NULL,
	.shape_data_size = 0,
	.transform_data = NULL,
	.transform_data_size = 0,
	.color_data = NULL,
	.color_data_size = 0,
	.uninv_mat_data = NULL,
	.uninv_mat_data_size = 0,
	.gradient_data = NULL,
	.gradient_data_size = 0,
	.bitmap_data = NULL,
	.bitmap_data_size = 0
#endif
};

int main()
{
    printf("==========================================================\n");
    printf("  String ID Optimization - Simple Test\n");
    printf("==========================================================\n");
    
    // Initialize variable storage
    printf("\nInitializing variable storage...\n");
    printf("  Max String ID: 10\n");
    heap_init(app_context, 1024*1024*1024);
    initVarArray(app_context, 10);
    initMap();
    
    printf("\n[TEST 1] Array-based variable access (ID = 5)\n");
    ActionVar* var1 = getVariableById(5);
    
    if (!var1)
    {
        printf("  ✗ FAIL: Could not get variable by ID\n");
        return 1;
    }
    
    printf("  ✓ PASS: Got variable by ID 5\n");
    
    printf("\n[TEST 2] Hashmap-based variable access\n");
    ActionVar* var2 = getVariable("dynamic_var", 11);
    
    if (!var2)
    {
        printf("  ✗ FAIL: Could not get variable by name\n");
        return 1;
    }
    
    printf("  ✓ PASS: Got variable by name 'dynamic_var'\n");
    
    printf("\n[TEST 3] Same ID returns same variable\n");
    ActionVar* var3 = getVariableById(5);
    
    if (var3 != var1)
    {
        printf("  ✗ FAIL: Different pointers for same ID\n");
        return 1;
    }
    printf("  ✓ PASS: Same ID returns same variable pointer\n");
    
    printf("\n[TEST 4] Different IDs return different variables\n");
    ActionVar* var4 = getVariableById(3);
    
    if (var4 == var1)
    {
        printf("  ✗ FAIL: Same pointer for different IDs\n");
        return 1;
    }
    
    printf("  ✓ PASS: Different IDs return different pointers\n");
    
    printf("\n[TEST 5] ID 0 returns NULL (dynamic strings)\n");
    ActionVar* var5 = getVariableById(0);
    
    if (var5 != NULL)
    {
        printf("  ✗ FAIL: ID 0 should return NULL\n");
        return 1;
    }
    
    printf("  ✓ PASS: ID 0 returns NULL as expected\n");
    
    printf("\n[TEST 6] ID >= array_size returns NULL\n");
    ActionVar* var6 = getVariableById(100);
    
    if (var6 != NULL)
    {
        printf("  ✗ FAIL: Out of bounds ID should return NULL\n");
        return 1;
    }
    
    printf("  ✓ PASS: Out of bounds ID returns NULL\n");
    
    // Cleanup
    freeMap();
    heap_shutdown(app_context);
    
    printf("\n==========================================================\n");
    printf("  All tests passed!\n");
    printf("==========================================================\n\n");
    
    return 0;
}