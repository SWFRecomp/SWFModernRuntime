# SWFModernRuntime

WebAssembly port of [SWFModernRuntime](https://github.com/SWFRecomp/SWFModernRuntime) by LittleCube.

## What is This?

This fork adds **WebAssembly compilation support** to SWFModernRuntime, enabling Flash SWF files to run natively in web browsers without Flash Player or emulation.

**Live Demos:** https://swfrecomp.github.io/SWFRecompDocs/

## Documentation

See the [SWFRecompDocs](https://github.com/SWFRecomp/SWFRecompDocs) repository for comprehensive documentation:

- **[Live Demos](https://swfrecomp.github.io/SWFRecompDocs/)** - See working examples
- **[Reference Guides](https://github.com/SWFRecomp/SWFRecompDocs/tree/master/reference)** - Technical documentation
- **[Implementation Guides](https://github.com/SWFRecomp/SWFRecompDocs/tree/master/guides)** - Step-by-step guides

**Related Repositories:**
- **[SWFRecomp](https://github.com/SWFRecomp/SWFRecomp)** - The static recompiler
- **[Upstream SWFModernRuntime](https://github.com/SWFRecomp/SWFModernRuntime)** - Original runtime by LittleCube

## Quick Demo

The `trace_swf_4` example is working! It demonstrates:
- SWF bytecode → C code → WebAssembly compilation pipeline
- ActionScript execution in browser
- String operations and console output
- Native performance without Flash Player

**[Try it live!](https://swfrecomp.github.io/SWFRecompDocs/examples/trace-swf-test/)**

## Project Goals

- Compile SWFRecomp-generated C code to WebAssembly
- Add Canvas2D rendering backend (in progress)
- Support SDL3 WebGPU (when available)
- Maintain 100% compatibility with native runtime

## Architecture

```
Flash SWF → SWFRecomp → C Code → Emscripten → WASM → Browser
                                      ↓
                            Canvas2D/WebGL2/WebGPU
```

The generated C code is **100% portable** - it compiles to both native and WASM with the same source.

## Repository Structure

```
SWFModernRuntime/
├── src/                    # Runtime source
│   ├── libswf/            # Core SWF execution
│   ├── actionmodern/      # ActionScript VM with variable storage improvements
│   └── flashbang/         # Native GPU rendering
├── include/               # Header files
├── test_*.c               # Test suite for variable storage
└── Makefile.test*         # Build files for tests
```

## Building

### Run Tests

This repository contains a comprehensive test suite for the variable storage system:

```bash
# Build and run all tests
make -f Makefile.test

# Or run individual test suites
make -f Makefile.test_simple              # Basic variable tests
make -f Makefile.test_string_id           # String ID optimization tests
make -f Makefile.test_simple_string_id    # Simple ID tests
```

### Integration with SWFRecomp

This runtime is used by [SWFRecomp](https://github.com/SWFRecomp/SWFRecomp) when generating WASM builds. See the SWFRecomp repository for complete build instructions and working examples.

## Current Status

| Feature | Status | Notes |
|---------|--------|-------|
| **ActionScript VM** | ✅ Working | String ops, math, variables |
| **Variable Storage** | ✅ Working | Copy-on-Store, array optimization |
| **Memory Management** | ✅ Working | Proper cleanup, no leaks |
| **Test Suite** | ✅ Complete | 1,100+ lines of tests |
| **Frame execution** | ✅ Working | Frame-by-frame playback |
| **Native GPU Rendering** | ✅ Working | SDL3 + WebGPU (upstream) |

## Key Features in This Fork

### Variable Storage Improvements
- **Copy-on-Store**: Variables own their string data
- **Array Optimization**: O(1) lookup for constant strings by ID
- **Memory Management**: Proper heap allocation and cleanup
- **Ownership Tracking**: No memory leaks or dangling pointers

See the [branch differences document](https://github.com/SWFRecomp/SWFRecompDocs/blob/master/merge/swfmodernruntime-branch-differences.md) for complete technical details.

## Upstream Sync

This fork regularly syncs with [upstream](https://github.com/SWFRecomp/SWFModernRuntime):

```bash
git fetch upstream
git merge upstream/master
```

All WASM code is in separate directories to minimize merge conflicts.

## License

Same as upstream SWFModernRuntime (check upstream LICENSE file).

## Credits

- **Upstream:** [SWFModernRuntime](https://github.com/SWFRecomp/SWFModernRuntime) by LittleCube
- **Upstream:** [SWFRecomp](https://github.com/SWFRecomp/SWFRecomp) by LittleCube
- **Inspiration:** [N64Recomp](https://github.com/N64Recomp/N64Recomp) by Wiseguy

---

**Note:** This is a community fork. The upstream project (by LittleCube) focuses on native runtime development. WASM support is maintained independently but syncs regularly with upstream improvements.
