#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 0) out vec4 v_color;

layout(std430, set = 0, binding = 0) readonly buffer GlobalTransforms
{
	mat4 transforms[];
};

void main()
{
	mat4 transform = transforms[gl_InstanceIndex];
	vec4 pos = vec4(a_position, 1.0f);
	gl_Position = transform*pos;
	v_color = a_color;
}