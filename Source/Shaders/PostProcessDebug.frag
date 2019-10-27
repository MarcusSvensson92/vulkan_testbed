#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform sampler2D Texture;

void main()
{
	OutColor = vec4(texelFetch(Texture, ivec2(gl_FragCoord.xy), 0).rgb, 1.0);
}