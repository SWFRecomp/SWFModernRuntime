.PHONY: all clean

all: compute.spv vertex.spv fragment.spv

%.spv: %.glsl
	glslc -fshader-stage=$* $< -o $@

clean:
	rm *.spv