#version 460

layout(location = 0) flat in uint v_style_type;
layout(location = 1) flat in uint v_style_id;
layout(location = 2) in vec4 v_args;
layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2DArray gradient_tex;

void main()
{
	FragColor = (v_style_type == 0x00) ? v_args :
										 texture(gradient_tex, vec3(v_args.x, 0.5f, float(v_style_id)));
}