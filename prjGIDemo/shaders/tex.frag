#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 2) uniform sampler2D tex;

layout (location = 0) in vec4 texcoord;
layout (location = 0) out vec4 uFragColor;

void main() 
{
   uFragColor = texture(tex, texcoord.xy);
   //vec4 t = texture(tex, texcoord.xy);
   //uFragColor = vec4(t.w, t.w, t.w, t.w);
   //uFragColor = vec4(0,0,1,1);
}
































/*

#version 400

in vec4 Color;
layout (location = 0) out vec4 FragColor;

void main() 
{
	FragColor = Color;
}
*/