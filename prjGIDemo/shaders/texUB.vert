
#version 430 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inUV;


layout (std430, binding = 0) buffer uniform_block 
{
	mat4 MVP[];
};





layout (location = 0) out vec4 texcoord;

out gl_PerVertex 
{
	vec4 gl_Position;
};



void main() 
{
	texcoord = inUV;
	gl_Position = MVP[floatBitsToUint(inPos.w)] * vec4(inPos.xyz, 1.0);
	//gl_Position = MVP[gl_VertexIndex / (12*3)] * inPos;
}
