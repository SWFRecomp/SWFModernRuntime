.PHONY: all clean

all: vertex.spv fragment.spv

vertex.spv: vertex.glsl
	glslc -fshader-stage=vertex $< -o $@

fragment.spv: fragment.glsl
	glslc -fshader-stage=fragment $< -o $@

clean:
	rm *.spv