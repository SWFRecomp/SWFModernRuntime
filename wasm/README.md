# WebAssembly Support for SWFModernRuntime

This directory contains all WebAssembly-specific code and examples.

## Project Status

This is a community fork adding WASM support to [SWFModernRuntime](https://github.com/SWFRecomp/SWFModernRuntime) by LittleCube.

**Current Status:** Phase 1 - Canvas2D Prototype (In Progress)

- **Working:** ActionScript execution in WASM
- **In Progress:** Canvas2D rendering backend
- **Future:** SDL3 WebGPU support

## Directory Structure

```
wasm/
├── examples/           # Working test cases compiled to WASM
│   └── trace-swf-test/ # Simple ActionScript test (WORKING!)
├── shell-templates/    # HTML templates for hosting WASM
└── README.md          # This file
```

## Live Demos

Visit the [GitHub Pages](https://peerinfinity.github.io/SWFModernRuntime/) to see working demos!

## Building Examples

Each example has its own `build.sh`:

```bash
# Install Emscripten first
source ~/tools/emsdk/emsdk_env.sh

# Build an example
cd examples/trace-swf-test
./build.sh

# Test locally
python3 -m http.server 8000
# Open http://localhost:8000/index.html
```

## How It Works

1. **SWFRecomp** translates SWF bytecode → C code (runs natively)
2. **Emscripten** compiles C code → WebAssembly
3. **This runtime** provides ActionScript execution + rendering in browser

## Current Examples

### trace-swf-test (trace_swf_4)
- **Status:** Working
- **Features:** ActionScript execution, string operations, console output
- **Size:** ~20KB (WASM + JS)
- **Demo:** [Live Demo](https://peerinfinity.github.io/SWFModernRuntime/examples/trace-swf-test/)

## Roadmap

See [WASM_PROJECT_PLAN.md](../WASM_PROJECT_PLAN.md) in the project root for detailed roadmap.

- **Phase 1 (Current):** Canvas2D rendering backend
- **Phase 2 (Optional):** WebGL2 rendering backend
- **Phase 3 (Future):** SDL3 WebGPU support

## Credits

- **Upstream:** [SWFModernRuntime](https://github.com/SWFRecomp/SWFModernRuntime) by LittleCube
- **Inspiration:** [N64Recomp](https://github.com/N64Recomp/N64Recomp) by Wiseguy

## License

Same as upstream (check main repository LICENSE file).
