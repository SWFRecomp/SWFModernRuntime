.PHONY: all clean

all: vertex.spv fragment.spv compute.spv

%.spv: %.glsl
	glslc -fshader-stage=$* $< -o $@

clean:
	rm *.spv