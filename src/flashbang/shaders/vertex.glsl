#version 460

layout(location = 0) in vec2 a_position;
layout(location = 1) in uvec2 style;
layout(location = 0) flat out uint v_style_type;
layout(location = 1) flat out uint v_style_id;
layout(location = 2) out vec4 v_args;

layout(std430, set = 0, binding = 0) readonly buffer Transforms
{
	mat4 transforms[];
};

layout(std430, set = 0, binding = 1) readonly buffer Colors
{
	vec4 colors[];
};

layout(std430, set = 0, binding = 2) readonly buffer InverseGradientMatrices
{
	mat4 inv_gradmats[];
};

layout(set = 1, binding = 0) uniform StageTransform
{
	mat4 stage_to_ndc;
};

layout(set = 1, binding = 1) uniform CurrentTransformID
{
	uint transform_id;
};

void main()
{
	mat4 transform = transforms[transform_id];
	vec4 pos = vec4(a_position, 0.0f, 1.0f);
	
	v_style_type = style.x;
	v_style_id = style.y;
	
	gl_Position = stage_to_ndc*transform*pos;
	
	vec2 v_uv = (inv_gradmats[v_style_id]*pos).xy;
	
	float start = v_uv.x + 16384.0f;
	float t = start/32768.0f;
	
	v_args = (v_style_type == 0x00) ? colors[v_style_id] :
									  vec4(t, 0.0f, 0.0f, 0.0f);
}