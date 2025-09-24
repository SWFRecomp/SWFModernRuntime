#version 460

layout(local_size_x = 64) in;

layout(std430, set = 0, binding = 0) readonly buffer GradientMatrices
{
	mat4 gradmats[];
};

layout(std430, set = 1, binding = 0) writeonly buffer InverseGradientMatrices
{
	mat4 inv_gradmats[];
};

void main()
{
	uint mat_i = gl_GlobalInvocationID.x;
	mat4 gradmat = gradmats[mat_i];
	inv_gradmats[mat_i] = inverse(gradmat);
}