#version 460

layout (location = 0) in vec2 a_position;
layout (location = 1) in uvec2 style;
layout (location = 0) out vec4 v_color;

layout(std430, set = 0, binding = 0) readonly buffer GlobalTransforms
{
	mat4 transforms[];
};

layout(std430, set = 0, binding = 1) readonly buffer GlobalColors
{
	vec4 colors[];
};

layout(set = 1, binding = 0) uniform StageTransform {
	mat4 stage_to_ndc;
};

layout(set = 1, binding = 1) uniform CurrentTransformID {
	uint transform_id;
};

void main()
{
	mat4 transform = transforms[transform_id];
	vec4 pos = vec4(a_position, 0.0f, 1.0f);
	gl_Position = stage_to_ndc*transform*pos;
	v_color = colors[style.y];
}