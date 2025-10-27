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
