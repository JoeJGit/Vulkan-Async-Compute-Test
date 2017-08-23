#version 430 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;


layout (std430, binding = 0) buffer storage_block // uniform_block 
{
	mat4 MVP;
};





layout (location = 0) out vec4 outColor;


out gl_PerVertex 
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main() 
{
	outColor = inColor;
	gl_PointSize = 8.0;
	gl_Position = MVP * inPos;
}
