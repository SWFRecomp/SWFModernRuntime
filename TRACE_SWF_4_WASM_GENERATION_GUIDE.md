# Complete Guide: Generating trace_swf_4 WASM File

## Overview

This document provides a comprehensive explanation of the complete process used to generate the `trace_swf_4.wasm` WebAssembly file from a Flash SWF file. This process bridges Flash/ActionScript technology with modern web browsers through static recompilation.

## High-Level Pipeline

```
┌─────────────┐
│  test.swf   │  Original Flash SWF file (80 bytes, SWF v4)
│  (Flash)    │  Contains: ActionScript bytecode to print "sup from SWF 4"
└──────┬──────┘
       │
       ▼
┌─────────────────────┐
│    SWFRecomp        │  Static Recompiler (C++ application)
│  (C++ Tool)         │  Location: ../SWFRecomp
└──────┬──────────────┘
       │
       │  Generates C source files:
       │  ├── RecompiledScripts/script_0.c (ActionScript → C)
       │  ├── RecompiledScripts/script_defs.c
       │  ├── RecompiledScripts/script_decls.h
       │  ├── RecompiledTags/tagMain.c (Frame logic)
       │  ├── RecompiledTags/constants.c (String literals)
       │  └── RecompiledTags/draws.c (Graphics data)
       │
       ▼
┌─────────────────────┐
│  Generated C Code   │  Portable C17 code
│  + Runtime Files    │  + runtime.c, main.c, recomp.h
└──────┬──────────────┘
       │
       ▼
┌─────────────────────┐
│    Emscripten       │  LLVM-based compiler toolchain
│  (emcc compiler)    │  Compiles C → LLVM IR → WebAssembly
└──────┬──────────────┘
       │
       │  Generates:
       │  ├── trace_swf.wasm (7.6 KB binary)
       │  └── trace_swf.js (12 KB JS loader)
       │
       ▼
┌─────────────────────┐
│   Web Browser       │  JavaScript loads WASM module
│  (Chrome, etc.)     │  Executes ActionScript in WASM sandbox
└─────────────────────┘
```

## Detailed Step-by-Step Process

### Phase 1: SWF Recompilation (SWFRecomp Tool)

**Location:** `../SWFRecomp` (sibling directory)

#### 1.1 Input Files

- **test.swf** - The original Flash file (80 bytes, SWF version 4)
  - Contains ActionScript bytecode
  - Contains frame/tag data
  - Contains string constants

- **config.toml** - Configuration file:
  ```toml
  [input]
  path_to_swf = "test.swf"
  output_tags_folder = "RecompiledTags"
  output_scripts_folder = "RecompiledScripts"
  ```

#### 1.2 SWFRecomp Execution

**Command:**
```bash
cd ../SWFRecomp/tests/trace_swf_4
../../build/SWFRecomp config.toml
```

**What SWFRecomp Does:**

The recompiler is a C++ application that performs static analysis and code generation:

1. **SWF File Parsing** (`src/swf.cpp`)
   - Opens and reads the binary SWF file
   - Detects compression format (uncompressed/zlib/LZMA)
   - Decompresses if needed using zlib or lzma libraries
   - Parses SWF header:
     - Signature (FWS/CWS/ZWS)
     - Version number (4 in this case)
     - File length
     - Frame size (RECT structure)
     - Frame rate
     - Frame count

2. **Tag Parsing** (`src/tag.cpp`)
   - Reads tag header: tag type (10 bits) + length (6 bits or extended)
   - Processes each tag type:
     - **DoAction (tag 12)**: Contains ActionScript bytecode
     - **SetBackgroundColor (tag 9)**: RGB color values
     - **ShowFrame (tag 1)**: End of frame marker
     - **DefineShape/DefineShape2**: Vector graphics data
     - And many others...

3. **ActionScript Bytecode Translation** (`src/action/action.cpp`)
   - Parses ActionScript bytecode stream
   - For each action opcode:
     - Identifies action type (8-bit opcode)
     - Reads length field if present (actions ≥ 0x80)
     - Reads action-specific data
   - Converts bytecode to equivalent C code
   - Example for `trace_swf_4`:
     ```
     ActionScript Bytecode:
     0x96 (PUSH) - Push string constant
     0x26 (TRACE) - Print to console
     0x00 (END) - End of actions

     Generated C Code:
     PUSH_STR(str_0, 14);
     actionTrace(stack, sp);
     ```

4. **Code Generation** (`src/recompilation.cpp`)

   Creates multiple output files:

   **a) RecompiledScripts/script_0.c** - Translated ActionScript:
   ```c
   #include <recomp.h>
   #include "script_decls.h"

   void script_0(char* stack, u32* sp)
   {
       // Push (String)
       PUSH_STR(str_0, 14);
       // Trace
       actionTrace(stack, sp);
   }
   ```

   **b) RecompiledTags/tagMain.c** - Frame execution logic:
   ```c
   #include <recomp.h>
   #include <out.h>
   #include "draws.h"

   void frame_0()
   {
       tagSetBackgroundColor(255, 255, 255);
       script_0(stack, &sp);
       tagShowFrame();
       quit_swf = 1;
   }

   typedef void (*frame_func)();

   frame_func frame_funcs[] = {
       frame_0,
   };

   void tagInit() {
   }
   ```

   **c) RecompiledTags/constants.c** - String literals:
   ```c
   char str_0[] = "sup from SWF 4";
   ```

   **d) RecompiledTags/constants.h** - String declarations:
   ```c
   #pragma once
   extern char str_0[];
   ```

   **e) RecompiledTags/draws.c** - Graphics data (empty for this test):
   ```c
   #include "recomp.h"
   #include "draws.h"
   ```

   **f) RecompiledScripts/script_defs.c** - Empty definitions file

   **g) RecompiledScripts/script_decls.h** - Function declarations:
   ```c
   #pragma once
   void script_0(char* stack, u32* sp);
   ```

#### 1.3 Output Structure

After SWFRecomp completes, you have:

```
tests/trace_swf_4/
├── RecompiledTags/
│   ├── tagMain.c        # Frame execution: frame_0(), frame_funcs[]
│   ├── constants.c      # String data: str_0[] = "sup from SWF 4"
│   ├── constants.h      # String declarations
│   ├── draws.c          # Graphics/shape data (empty for this test)
│   └── draws.h          # Graphics declarations
└── RecompiledScripts/
    ├── script_0.c       # Translated ActionScript
    ├── script_defs.c    # Script definitions (empty)
    ├── script_decls.h   # Script function declarations
    └── out.h            # Output declarations
```

### Phase 2: WASM Runtime Preparation

**Location:** `wasm/examples/trace-swf-test/`

The generated C code needs runtime support to execute. These files are manually created or copied:

#### 2.1 Runtime Implementation Files

**a) runtime.c** - Core runtime functionality (77 lines):

```c
#include "recomp.h"

// Global variables
char stack[4096];        // 4KB stack for ActionScript operations
u32 sp = 0;             // Stack pointer
int quit_swf = 0;       // Exit flag
int manual_next_frame = 0;
int next_frame = 0;

// Helper: Pop string from stack
static char* pop_string(char* stack, u32* sp_ptr) {
    // Walks backward through stack to find string boundaries
    // Strings are null-terminated on stack
    u32 current = *sp_ptr;
    while (current > 0 && stack[current - 1] != '\0') {
        current--;
    }
    if (current > 0) {
        current--;
    }
    u32 start = current;
    while (start > 0 && stack[start - 1] != '\0') {
        start--;
    }
    *sp_ptr = start;
    return &stack[start];
}

// ActionScript function: trace()
void actionTrace(char* stack, u32* sp_ptr) {
    char* str = pop_string(stack, sp_ptr);
    printf("%s\n", str);
}

// Tag function: Set background color
void tagSetBackgroundColor(u8 r, u8 g, u8 b) {
    printf("[Tag] SetBackgroundColor(%d, %d, %d)\n", r, g, b);
}

// Tag function: Show frame
void tagShowFrame() {
    printf("[Tag] ShowFrame()\n");
}

// Main runtime loop
void swfStart(frame_func* funcs) {
    printf("=== SWF Execution Started ===\n");

    tagInit();  // Initialize tags (from tagMain.c)

    int current_frame = 0;
    while (!quit_swf && current_frame < 100) {
        printf("\n[Frame %d]\n", current_frame);

        if (funcs[current_frame]) {
            funcs[current_frame]();  // Execute frame function
        } else {
            printf("No function for frame %d, stopping.\n", current_frame);
            break;
        }

        if (manual_next_frame) {
            current_frame = next_frame;
            manual_next_frame = 0;
        } else {
            current_frame++;
        }
    }

    printf("\n=== SWF Execution Completed ===\n");
}
```

**b) main.c** - Entry point with Emscripten integration (28 lines):

```c
#include "recomp.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Export for JavaScript to call
EMSCRIPTEN_KEEPALIVE
void runSWF() {
    printf("Starting SWF execution from JavaScript...\n");
    swfStart(frame_funcs);  // frame_funcs from tagMain.c
}
#endif

int main() {
    printf("WASM SWF Runtime Loaded!\n");
    printf("This is a recompiled Flash SWF running in WebAssembly.\n\n");

#ifndef __EMSCRIPTEN__
    // Native mode - run immediately
    swfStart(frame_funcs);
#else
    // WASM mode - wait for JavaScript to call runSWF()
    printf("Call runSWF() from JavaScript to execute the SWF.\n");
#endif

    return 0;
}
```

**c) recomp.h** - Type definitions and API (1.3 KB):

```c
#pragma once

#include <stdio.h>

// Type definitions for cross-platform compatibility
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

// Stack manipulation macros
#define PUSH_STR(str, len) \
    do { \
        for (int i = 0; i < len; i++) { \
            stack[sp++] = str[i]; \
        } \
        stack[sp++] = '\0'; \
    } while(0)

// Global variables (defined in runtime.c)
extern char stack[4096];
extern u32 sp;
extern int quit_swf;
extern int manual_next_frame;
extern int next_frame;

// Frame function pointer type
typedef void (*frame_func)();

// Frame functions (defined in tagMain.c)
extern frame_func frame_funcs[];

// ActionScript action functions (implemented in runtime.c)
void actionTrace(char* stack, u32* sp);
void actionAdd(char* stack, u32* sp);
void actionSubtract(char* stack, u32* sp);
// ... more action functions ...

// Tag functions (implemented in runtime.c)
void tagSetBackgroundColor(u8 r, u8 g, u8 b);
void tagShowFrame();
void tagInit();
// ... more tag functions ...

// Runtime entry point (implemented in runtime.c)
void swfStart(frame_func* funcs);

// String constants (from constants.c)
extern char str_0[];
```

**d) stackvalue.h** - Stack value type definitions (minimal for this test)

**e) out.h** - Additional declarations

#### 2.2 Copy Generated Files

The recompiled code from SWFRecomp is copied to the WASM directory:

```bash
# Copy generated files to WASM project
cp RecompiledScripts/*.c wasm/examples/trace-swf-test/
cp RecompiledScripts/*.h wasm/examples/trace-swf-test/
cp RecompiledTags/*.c wasm/examples/trace-swf-test/
cp RecompiledTags/*.h wasm/examples/trace-swf-test/
```

Final file structure:
```
wasm/examples/trace-swf-test/
├── main.c              # Entry point (WASM/native)
├── runtime.c           # Runtime implementation
├── recomp.h            # API header
├── stackvalue.h        # Type definitions
├── out.h               # Declarations
├── script_0.c          # Generated ActionScript (from SWFRecomp)
├── script_defs.c       # Generated definitions (from SWFRecomp)
├── script_decls.h      # Generated declarations (from SWFRecomp)
├── tagMain.c           # Generated frame logic (from SWFRecomp)
├── constants.c         # Generated strings (from SWFRecomp)
├── constants.h         # Generated string headers (from SWFRecomp)
├── draws.c             # Generated graphics (from SWFRecomp)
├── draws.h             # Generated graphics headers (from SWFRecomp)
├── build.sh            # Emscripten build script
└── index.html          # Web interface
```

### Phase 3: WebAssembly Compilation (Emscripten)

**Location:** `wasm/examples/trace-swf-test/`

#### 3.1 Prerequisites

Install Emscripten SDK:
```bash
# Clone emsdk (if not already installed)
git clone https://github.com/emscripten-core/emsdk.git ~/tools/emsdk
cd ~/tools/emsdk

# Install latest version
./emsdk install latest
./emsdk activate latest

# Activate for current shell
source ~/tools/emsdk/emsdk_env.sh
```

Verify installation:
```bash
emcc --version
# Should output: emscripten X.X.X (commit hash)
```

#### 3.2 Build Script (build.sh)

```bash
#!/bin/bash
set -e

echo "=== Building trace_swf_4 to WASM ==="
echo ""

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten (emcc) not found!"
    echo "Run: source ~/tools/emsdk/emsdk_env.sh"
    exit 1
fi

echo "✓ Emscripten found: $(emcc --version | head -n1)"
echo ""

# Compile all C files
echo "Compiling SWF recompiled code to WASM..."
emcc \
    main.c \
    runtime.c \
    tagMain.c \
    script_0.c \
    script_defs.c \
    -I. \
    -o trace_swf.js \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_main","_runSWF"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -O2

echo ""
echo "✅ Build complete!"
echo ""
echo "Generated files:"
ls -lh trace_swf.js trace_swf.wasm 2>/dev/null
echo ""
echo "To test:"
echo "  1. python3 -m http.server 8000"
echo "  2. Open http://localhost:8000/index.html"
echo ""
```

#### 3.3 Emscripten Compilation Process

**Command Breakdown:**

```bash
emcc \                                              # Emscripten compiler
    main.c \                                        # Entry point
    runtime.c \                                     # Runtime functions
    tagMain.c \                                     # Frame execution
    script_0.c \                                    # Recompiled ActionScript
    script_defs.c \                                 # Script definitions
    -I. \                                          # Include current directory
    -o trace_swf.js \                              # Output filename (generates .js + .wasm)
    -s WASM=1 \                                    # Enable WebAssembly output
    -s EXPORTED_FUNCTIONS='["_main","_runSWF"]' \  # Functions callable from JS
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \  # JS-C bridge methods
    -s ALLOW_MEMORY_GROWTH=1 \                     # Allow dynamic memory allocation
    -O2                                             # Optimization level 2
```

**What Emscripten Does:**

1. **C Preprocessing**
   - Processes `#ifdef __EMSCRIPTEN__` directives
   - Includes Emscripten-specific headers
   - Resolves all `#include` statements

2. **C Compilation to LLVM IR**
   - Compiles each `.c` file to LLVM Intermediate Representation
   - Type checking and semantic analysis
   - Optimizations at IR level

3. **Linking**
   - Links all object files together
   - Resolves function references
   - Links Emscripten runtime library
   - Resolves external symbols (printf, etc.)

4. **LLVM IR to WebAssembly**
   - Converts LLVM IR to WebAssembly binary format (.wasm)
   - Applies WASM-specific optimizations
   - Generates function table
   - Creates memory layout

5. **JavaScript Glue Code Generation**
   - Creates `trace_swf.js` loader
   - Implements exported functions (`_main`, `_runSWF`)
   - Creates `ccall`/`cwrap` helpers for JS-WASM communication
   - Sets up memory management
   - Implements standard library shims (printf → console.log)
   - Handles WASM module loading and initialization

#### 3.4 Output Files

**a) trace_swf.wasm** (7.6 KB)
- Binary WebAssembly module
- Contains compiled machine code
- Platform-independent bytecode
- Executed by browser's WASM VM

**Structure:**
```
Magic number: 0x00 0x61 0x73 0x6d (binary format identifier)
Version: 0x01 0x00 0x00 0x00 (version 1)

Sections:
- Type section: Function signatures
- Import section: Imported functions (e.g., env.printf)
- Function section: Function indices
- Table section: Function pointer table
- Memory section: Linear memory definition (initial size, max size)
- Global section: Global variables
- Export section: Exported functions (_main, _runSWF)
- Code section: Function bytecode
- Data section: Initialized data (str_0, etc.)
```

**b) trace_swf.js** (12 KB)
- JavaScript loader and runtime
- Minified/compressed code
- Handles WASM module loading
- Provides JS-WASM bridge

**Key components:**
```javascript
// Module initialization
var Module = {
    preRun: [],
    postRun: [],
    print: function(text) {
        console.log(text);
    },
    printErr: function(text) {
        console.error(text);
    },
    // ... memory management, file system, etc.
};

// Load WASM binary
WebAssembly.instantiate(wasmBinary, imports).then(function(output) {
    Module.asm = output.instance.exports;
    // ... initialize runtime
});

// Exported functions
Module._main = function() {
    return Module.asm._main();
};

Module._runSWF = function() {
    return Module.asm._runSWF();
};

// Helper methods
Module.ccall = function(ident, returnType, argTypes, args) {
    // Call C function from JavaScript
};

Module.cwrap = function(ident, returnType, argTypes) {
    // Wrap C function for JavaScript
};
```

#### 3.5 Build Execution

```bash
cd wasm/examples/trace-swf-test

# Activate Emscripten (if not already active)
source ~/tools/emsdk/emsdk_env.sh

# Run build
./build.sh
```

**Build output:**
```
=== Building trace_swf_4 to WASM ===

✓ Emscripten found: emscripten 3.1.XX (commit XXXXXXX)

Compiling SWF recompiled code to WASM...

✅ Build complete!

Generated files:
-rw-r--r-- 1  12K Oct 27 14:23 trace_swf.js
-rwxr-xr-x 1 7.6K Oct 27 14:23 trace_swf.wasm

To test:
  1. python3 -m http.server 8000
  2. Open http://localhost:8000/index.html
```

### Phase 4: Web Integration

#### 4.1 HTML Interface (index.html)

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>trace_swf_4 - WASM Test</title>
    <style>
        body {
            font-family: monospace;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
        }
        #output {
            background: #f0f0f0;
            border: 1px solid #ccc;
            padding: 10px;
            min-height: 200px;
            white-space: pre-wrap;
            font-family: monospace;
        }
        button {
            background: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            cursor: pointer;
            font-size: 16px;
            margin: 10px 5px;
        }
        button:hover {
            background: #45a049;
        }
    </style>
</head>
<body>
    <h1>trace_swf_4 WASM Test</h1>
    <p>This is a Flash SWF file (ActionScript 1) recompiled to WebAssembly.</p>

    <button onclick="runSWF()">Run SWF</button>
    <button onclick="clearOutput()">Clear Output</button>

    <h2>Console Output:</h2>
    <div id="output"></div>

    <script>
        var outputElement = document.getElementById('output');

        // Capture console output
        var Module = {
            print: function(text) {
                outputElement.textContent += text + '\n';
            },
            printErr: function(text) {
                outputElement.textContent += 'ERROR: ' + text + '\n';
            }
        };

        function clearOutput() {
            outputElement.textContent = '';
        }

        function runSWF() {
            outputElement.textContent += '>>> Running SWF...\n';
            Module.ccall('runSWF', 'void', [], []);
        }
    </script>

    <!-- Load the Emscripten-generated JavaScript -->
    <script src="trace_swf.js"></script>
</body>
</html>
```

#### 4.2 Running the Application

**Start local web server:**
```bash
cd wasm/examples/trace-swf-test
python3 -m http.server 8000
```

**Open in browser:**
```
http://localhost:8000/index.html
```

**User interaction:**
1. Page loads `trace_swf.js`
2. JavaScript loads `trace_swf.wasm` module
3. WASM module initializes (calls `main()`)
4. Console shows: "WASM SWF Runtime Loaded!"
5. User clicks "Run SWF" button
6. JavaScript calls `Module.ccall('runSWF', ...)`
7. WASM executes the recompiled Flash code
8. Output appears in browser console/page

**Expected output:**
```
WASM SWF Runtime Loaded!
This is a recompiled Flash SWF running in WebAssembly.

Call runSWF() from JavaScript to execute the SWF.
>>> Running SWF...
Starting SWF execution from JavaScript...
=== SWF Execution Started ===

[Frame 0]
[Tag] SetBackgroundColor(255, 255, 255)
sup from SWF 4
[Tag] ShowFrame()

=== SWF Execution Completed ===
```

## Complete File Dependency Graph

```
test.swf (80 bytes)
    │
    ▼
[SWFRecomp Tool]
    │
    ├─▶ RecompiledScripts/script_0.c ────┐
    ├─▶ RecompiledScripts/script_defs.c ─┤
    ├─▶ RecompiledScripts/script_decls.h ┤
    ├─▶ RecompiledTags/tagMain.c ────────┤
    ├─▶ RecompiledTags/constants.c ──────┤
    ├─▶ RecompiledTags/constants.h ──────┤
    ├─▶ RecompiledTags/draws.c ──────────┤
    └─▶ RecompiledTags/draws.h ──────────┤
                                         │
    ┌────────────────────────────────────┘
    │
    ├─▶ runtime.c (manual) ──────────────┐
    ├─▶ main.c (manual) ─────────────────┤
    ├─▶ recomp.h (manual) ───────────────┤
    ├─▶ stackvalue.h (manual) ───────────┤
    └─▶ out.h (manual) ──────────────────┤
                                         │
                                         ▼
                                   [Emscripten]
                                         │
                                         ├─▶ trace_swf.wasm (7.6 KB)
                                         └─▶ trace_swf.js (12 KB)
                                              │
                                              ▼
                                         index.html
                                              │
                                              ▼
                                         Web Browser
```

## Technical Details

### Memory Layout

**WASM Linear Memory:**
```
0x0000 - 0x0FFF: Stack space (4096 bytes)
0x1000+       : Heap (dynamically grows)
               : Static data (str_0, etc.)
               : Code (in separate code section)
```

**Stack Layout (ActionScript stack in C):**
```
stack[0] ... stack[sp-1]: Used space
stack[sp] ... stack[4095]: Free space

Example during execution:
stack[0..13]: "sup from SWF 4"
stack[14]: '\0'
sp = 15
```

### Execution Flow

**Initialization:**
```
1. Browser loads index.html
2. index.html loads trace_swf.js
3. trace_swf.js fetches trace_swf.wasm
4. WebAssembly.instantiate() creates WASM instance
5. WASM module initialization runs
6. _main() is called automatically
7. main() prints startup messages
8. main() waits for JavaScript to call runSWF()
```

**SWF Execution (when runSWF() called):**
```
JavaScript: Module.ccall('runSWF', ...)
    │
    ▼
WASM: runSWF()
    │
    ▼
WASM: swfStart(frame_funcs)
    │
    ▼
WASM: tagInit() [empty]
    │
    ▼
WASM: frame_0()
    │
    ├─▶ tagSetBackgroundColor(255, 255, 255)
    │       └─▶ printf("[Tag] SetBackgroundColor(...)")
    │               └─▶ JavaScript console
    │
    ├─▶ script_0(stack, &sp)
    │       │
    │       ├─▶ PUSH_STR(str_0, 14)
    │       │       └─▶ Copies "sup from SWF 4" to stack
    │       │
    │       └─▶ actionTrace(stack, &sp)
    │               │
    │               ├─▶ pop_string(stack, &sp)
    │               └─▶ printf("%s\n", str)
    │                       └─▶ JavaScript console: "sup from SWF 4"
    │
    ├─▶ tagShowFrame()
    │       └─▶ printf("[Tag] ShowFrame()")
    │
    └─▶ quit_swf = 1
        │
        ▼
WASM: Loop exits
    │
    ▼
WASM: printf("=== SWF Execution Completed ===")
    │
    ▼
Return to JavaScript
```

### Data Flow Diagram

```
┌─────────────┐
│  Flash SWF  │ (Binary ActionScript bytecode)
└──────┬──────┘
       │
       │ [SWFRecomp Static Analysis]
       │
       ▼
┌──────────────────────────────────────┐
│  Intermediate Representation (C17)   │
│  - Procedural code                   │
│  - Stack-based operations            │
│  - Function calls to runtime API     │
└──────┬───────────────────────────────┘
       │
       │ [Emscripten LLVM Compilation]
       │
       ▼
┌──────────────────────────────────────┐
│  WebAssembly Binary (.wasm)          │
│  - WASM bytecode instructions        │
│  - Linear memory model               │
│  - Import/Export tables              │
└──────┬───────────────────────────────┘
       │
       │ [Browser WASM VM]
       │
       ▼
┌──────────────────────────────────────┐
│  Native Execution                    │
│  - JIT compilation to machine code   │
│  - Sandboxed memory access           │
│  - JavaScript interop via imports    │
└──────────────────────────────────────┘
```

## Key Technologies

### SWFRecomp (C++ Application)
- **Language:** C++17
- **Build System:** CMake
- **Dependencies:**
  - zlib (SWF decompression)
  - lzma (SWF decompression)
  - earcut (polygon triangulation)
  - tomlplusplus (config parsing)
- **Purpose:** Static recompiler (SWF → C)

### Emscripten (Compiler Toolchain)
- **Based on:** LLVM/Clang
- **Input:** C/C++ source code
- **Output:** WebAssembly + JavaScript
- **Features:**
  - Full C standard library support
  - POSIX compatibility layer
  - OpenGL ES → WebGL translation
  - SDL → HTML5 APIs

### WebAssembly (Binary Format)
- **Format:** Binary instruction format
- **VM:** Stack-based virtual machine
- **Memory:** Linear memory model
- **Security:** Sandboxed execution
- **Performance:** Near-native speed

## Optimization Levels

The build uses `-O2` optimization:

**Compiler Optimizations:**
- Function inlining
- Dead code elimination
- Constant folding
- Loop unrolling
- Common subexpression elimination

**WASM-Specific Optimizations:**
- Compact binary encoding
- Efficient memory layout
- Optimized import/export tables
- Minimal JavaScript glue code

**Result:**
- Original SWF: 80 bytes
- Generated WASM: 7.6 KB (includes full runtime)
- JavaScript loader: 12 KB
- Total: ~20 KB (compared to Flash Player: ~15-20 MB)

## Limitations and Future Work

### Current Limitations

1. **Graphics Rendering:** Not implemented in this example
   - `draws.c` is empty
   - No Canvas2D/WebGL rendering
   - Only console output works

2. **ActionScript Support:** SWF v4 only
   - No ActionScript 2.0/3.0 support
   - Limited action set
   - No complex control flow (functions, classes)

3. **SWF Features:** Minimal
   - No MovieClips
   - No user input handling
   - No sound
   - No video

### Future Enhancements

**Phase 1: Canvas2D Rendering**
- Implement shape rendering in draws.c
- Use HTML5 Canvas 2D API
- Translate fills, strokes, gradients

**Phase 2: WebGL2 Rendering**
- GPU-accelerated rendering
- Shader-based graphics pipeline
- Better performance for complex scenes

**Phase 3: SDL3 WebGPU**
- Use upstream SWFModernRuntime rendering
- WebGPU backend for maximum performance
- Full compatibility with native version

## Summary

The complete process of generating `trace_swf_4.wasm`:

1. **SWFRecomp** statically analyzes Flash SWF bytecode
2. Generates portable C17 source code
3. Manual runtime implementation provides execution environment
4. **Emscripten** compiles C to LLVM IR
5. LLVM backend generates WebAssembly binary
6. JavaScript glue code provides browser integration
7. HTML page loads and executes WASM module
8. Flash content runs natively in browser without Flash Player

**Key Innovation:** This is **static recompilation**, not emulation. The Flash bytecode is permanently translated to native code (WebAssembly), enabling Flash games to run forever without Flash Player.

## References

- **SWFRecomp:** `../SWFRecomp` (sibling directory)
- **SWFModernRuntime:** `.` (this project)
- **Working Example:** `wasm/examples/trace-swf-test/`
- **Live Demo:** https://peerinfinity.github.io/SWFModernRuntime/examples/trace-swf-test/

## Build Commands Quick Reference

```bash
# Step 1: Recompile SWF to C (from SWFModernRuntime directory)
cd ../SWFRecomp/tests/trace_swf_4
../../build/SWFRecomp config.toml

# Step 2: Copy generated files to WASM directory
cd ../../../SWFModernRuntime
cp ../SWFRecomp/tests/trace_swf_4/RecompiledScripts/*.c wasm/examples/trace-swf-test/
cp ../SWFRecomp/tests/trace_swf_4/RecompiledScripts/*.h wasm/examples/trace-swf-test/
cp ../SWFRecomp/tests/trace_swf_4/RecompiledTags/*.c wasm/examples/trace-swf-test/
cp ../SWFRecomp/tests/trace_swf_4/RecompiledTags/*.h wasm/examples/trace-swf-test/

# Step 3: Compile to WASM
cd wasm/examples/trace-swf-test
source ~/tools/emsdk/emsdk_env.sh
./build.sh

# Step 4: (Optional) Deploy to GitHub Pages
cp trace_swf.wasm ../../../docs/examples/trace-swf-test/
cp trace_swf.js ../../../docs/examples/trace-swf-test/

# Step 5: Test locally
python3 -m http.server 8000
# Open http://localhost:8000/index.html
# Or if testing from docs: cd ../../../docs && python3 -m http.server 8000
# Then open http://localhost:8000/examples/trace-swf-test/index.html
```
