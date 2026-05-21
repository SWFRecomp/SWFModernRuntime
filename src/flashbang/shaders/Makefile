.PHONY: all clean

all: vertex.spv fragment.spv compute.spv multiply.spv

multiply.spv: multiply.glsl
	glslc -fshader-stage=compute $< -o $@

%.spv: %.glsl
	glslc -fshader-stage=$* $< -o $@

clean:
	rm *.spv