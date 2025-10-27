# SWFModernRuntime-WASM

WebAssembly port of [SWFModernRuntime](https://github.com/SWFRecomp/SWFModernRuntime) by LittleCube.

## What is This?

This fork adds **WebAssembly compilation support** to SWFModernRuntime, enabling Flash SWF files to run natively in web browsers without Flash Player or emulation.

**Live Demos:** https://peerinfinity.github.io/SWFModernRuntime-WASM/

## Quick Demo

The `trace_swf_4` example is already working! It demonstrates:
- SWF bytecode to C code to WebAssembly compilation
- ActionScript execution in browser
- String operations and console output

[Try it live!](https://peerinfinity.github.io/SWFModernRuntime-WASM/examples/trace-swf-test/)

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
SWFModernRuntime-WASM/
├── src/                    # Runtime source (mostly from upstream)
│   ├── libswf/            # Core SWF execution (unchanged)
│   ├── actionmodern/      # ActionScript VM (unchanged)
│   ├── flashbang/         # Native GPU rendering (unchanged)
│   └── rendering/         # NEW - Rendering abstraction layer
├── wasm/                   # NEW - All WASM-specific code
│   ├── examples/          # Working test cases
│   ├── shell-templates/   # HTML templates
│   └── README.md          # WASM documentation
├── docs/                   # GitHub Pages content
└── WASM_PROJECT_PLAN.md   # Detailed development plan
```

## Building

### Prerequisites

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git ~/tools/emsdk
cd ~/tools/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Build an Example

```bash
cd wasm/examples/trace-swf-test
./build.sh

# Test locally
python3 -m http.server 8000
# Open http://localhost:8000/index.html
```

## Current Status

| Feature | Status | Notes |
|---------|--------|-------|
| **ActionScript VM** | Working | String ops, math, variables |
| **Frame execution** | Working | Frame-by-frame playback |
| **Canvas2D rendering** | In Progress | Basic shapes next |
| **WebGL2 rendering** | Planned | Optional Phase 2 |
| **SDL3 WebGPU** | Future | Waiting on SDL3 |

## Roadmap

See [WASM_PROJECT_PLAN.md](WASM_PROJECT_PLAN.md) for complete details.

- **Phase 1 (Current):** Canvas2D backend - Proof of concept
- **Phase 2 (Optional):** WebGL2 backend - GPU acceleration
- **Phase 3 (Future):** SDL3 WebGPU - Use upstream code unmodified

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

## Contact

- **Issues:** https://github.com/PeerInfinity/SWFModernRuntime-WASM/issues
- **Upstream Issues:** https://github.com/SWFRecomp/SWFModernRuntime/issues

---

**Note:** This is a community fork. The upstream project (by LittleCube) focuses on native runtime development. WASM support is maintained independently but syncs regularly with upstream improvements.
