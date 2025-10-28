# GitHub Pages Content

This directory contains the GitHub Pages website for SWFModernRuntime.

## Structure

```
docs/
├── index.html           # Main landing page
├── favicon.svg          # Site favicon
└── examples/            # Working WASM examples
    └── trace-swf-test/  # trace_swf_4 example
        ├── index.html
        ├── trace_swf.js
        ├── trace_swf.wasm
        └── (other generated files)
```

## Deployment

The site will be available at: https://peerinfinity.github.io/SWFModernRuntime/

## Adding New Examples

When you create a new working example:

1. Build the WASM files in `wasm/examples/your-example/`
2. Copy the built example to `docs/examples/your-example/`
3. Update `docs/index.html` to link to the new example
4. Commit and push

Note: Only copy the necessary files (.html, .js, .wasm) - don't copy source files or build scripts to docs.
