#version 460

layout (location = 0) in vec2 a_position;
layout (location = 1) in uvec2 style;
layout (location = 0) out vec4 v_color;

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
	uint style_id = style.y;
	gl_Position = stage_to_ndc*transform*pos;
	v_color = colors[style_id];
}