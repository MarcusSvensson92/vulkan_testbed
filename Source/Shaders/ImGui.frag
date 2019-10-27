#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 InTexCoord;
layout(location = 1) in vec4 InColor;

layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform sampler2D Texture;

void main()
{
    OutColor = texture(Texture, InTexCoord) * InColor;
}