# String Variable Storage - Test Suite

This directory contains a comprehensive unit test suite for the string variable storage implementation in SWFModernRuntime.

## Test Files

- **`test_variables_simple.c`** - Main test suite with 24 comprehensive tests
- **`Makefile.test_simple`** - Build configuration for the test suite

## Running Tests

### Quick Start

```bash
# Build and run tests
make -f Makefile.test_simple test

# Run with memory leak detection
make -f Makefile.test_simple valgrind
```

### Individual Commands

```bash
# Build
make -f Makefile.test_simple

# Clean
make -f Makefile.test_simple clean

# Run tests
./test_variables_simple

# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all ./test_variables_simple
```

## Test Coverage

The test suite covers all critical functionality:

### 1. Basic String Variable Storage
- Tests simple string assignment to variables
- Verifies heap allocation and ownership tracking
- Confirms correct string size tracking

### 2. STR_LIST Materialization
- Tests concatenation of multiple strings (STR_LIST → heap string)
- Verifies proper materialization of string lists to single heap strings
- Tests the core functionality needed for StringAdd → variable storage

### 3. Variable Reassignment
- Tests memory management when reassigning variables
- Verifies old values are properly freed
- Ensures no memory leaks during reassignment

### 4. Mixed Type Variables
- Tests both numeric (F32) and string variables
- Verifies union-based storage works correctly
- Ensures type information is preserved

### 5. Multiple Independent Variables
- Tests that multiple variables don't interfere with each other
- Verifies independent memory management
- Tests variable map functionality

### 6. Edge Cases
- Empty strings
- Long strings (1023 bytes)
- STR_LIST with many parts (10 strings)

### 7. Direct Function Testing
- Tests `materializeStringList()` directly
- Verifies both STR_LIST and single string paths
- Tests return value handling

## Test Results

```
Total:  24 tests
Passed: 24 tests
Failed: 0 tests
```

## Memory Safety

Valgrind confirms perfect memory management:

```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 28 allocs, 28 frees, 8,394,927 bytes allocated

All heap blocks were freed -- no leaks are possible

ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

- ✅ Zero memory leaks
- ✅ Zero uninitialized value errors
- ✅ All allocations properly freed

## What's Being Tested

This test suite validates the implementation described in `docs/STRING_VARIABLE_NEXT_STEPS.md`:

1. **String Materialization**: When a STR_LIST (from StringAdd) is stored to a variable, it's materialized into a single heap-allocated string

2. **Memory Ownership**: Variables properly track whether they own heap memory via the `owns_memory` flag

3. **Proper Cleanup**: Old values are freed when variables are reassigned, and all memory is freed on cleanup

4. **Type Safety**: The union-based storage correctly handles both numeric and string types

## Implementation Files Tested

- `src/actionmodern/variables.c`:
  - `materializeStringList()` - Converts STR_LIST to heap string
  - `setVariableWithValue()` - Stores values with proper memory management
  - `getVariable()` - Creates and initializes variables
  - `freeMap()` - Cleans up all variables

- `include/actionmodern/variables.h`:
  - `ActionVar` structure with union-based storage
  - String ownership tracking (`owns_memory` flag)

## Notes

- This test suite is independent of the full SWFModernRuntime build
- Only links necessary components (variables.c, utils.c, hashmap)
- Avoids SDL and other heavy dependencies
- Can be run quickly during development

## Future Enhancements

Potential additional tests:

1. **Integration tests** with full SWFModernRuntime and actual SWF files
2. **Performance tests** comparing STR_LIST optimization vs materialization
3. **Stress tests** with thousands of variables
4. **Concurrent access tests** (if threading is added)
