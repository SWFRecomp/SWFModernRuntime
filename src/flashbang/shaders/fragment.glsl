#version 460

#define LINEAR_T(v_args) ((v_args.x + 16384.0f)/32768.0f)
#define RADIAL_T(v_args) (distance(v_args.xy, vec2(0.0f, 0.0f))/16384.0f)
#define BITMAP_UV(v_args) (v_args.xy)

layout(location = 0) flat in uint v_style_type;
layout(location = 1) flat in uint v_style_id;
layout(location = 2) in vec4 v_args;
layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2DArray gradient_tex;
layout(set = 2, binding = 1) uniform sampler2DArray bitmap_tex;

void main()
{
	FragColor = (v_style_type == 0x00) ? v_args :
				(v_style_type == 0x10) ? texture(gradient_tex, vec3(LINEAR_T(v_args), 0.5f, float(v_style_id))) :
				(v_style_type == 0x12) ? texture(gradient_tex, vec3(RADIAL_T(v_args), 0.5f, float(v_style_id))) :
				(v_style_type == 0x41) ? texture(bitmap_tex, vec3(BITMAP_UV(v_args), float(v_style_id))) :
										 vec4(0.0f);
}