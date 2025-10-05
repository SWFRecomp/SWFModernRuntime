#version 460

#define INV_POS(id) (inv_mats[id]*pos)

#define V_GRAD_UV(g_id) (INV_POS(g_id).xy)
#define V_BITMAP_UV(mat_id, sizes) (vec2(INV_POS(mat_id).x/float(sizes.x), INV_POS(mat_id).y/float(sizes.y)))

layout(location = 0) in vec2 position;
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

layout(std430, set = 0, binding = 2) readonly buffer InverseMatrices
{
	mat4 inv_mats[];
};

layout(std430, set = 0, binding = 3) readonly buffer BitmapSizes
{
	uvec2 bitmap_sizes[];
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
	vec4 pos = vec4(position, 0.0f, 1.0f);
	
	v_style_type = style.x;
	v_style_id = style.y & 0xFFFF;
	uint style_upper = ((style.y >> 16) & 0xFFFF);
	
	gl_Position = stage_to_ndc*transform*pos;
	
	v_args = (v_style_type == 0x00) ? colors[v_style_id] :
			 ((v_style_type & 0xF0) == 0x10) ? vec4(V_GRAD_UV(v_style_id), 0.0f, 0.0f) :
			 ((v_style_type & 0xF0) == 0x40) ? vec4(V_BITMAP_UV(style_upper, bitmap_sizes[v_style_id]), 0.0f, 0.0f) :
											   vec4(0.0f);
}