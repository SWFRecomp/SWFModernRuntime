#version 460

#define OUT_OFFSET (offset + 2*size + gl_GlobalInvocationID.x)

layout(local_size_x = 64) in;
layout(local_size_y = 1) in;
layout(local_size_z = 1) in;

layout(std430, set = 1, binding = 0) readonly buffer Matrices
{
	mat4 mats[];
};

layout(set = 2, binding = 0) uniform StartOffset
{
	uint offset;
};

layout(set = 2, binding = 1) uniform Size
{
	uint size;
};

void main()
{
	mats[OUT_OFFSET] = mats[offset + 2*gl_GlobalInvocationID.x]*mats[offset + 2*gl_GlobalInvocationID.x + 1];
}