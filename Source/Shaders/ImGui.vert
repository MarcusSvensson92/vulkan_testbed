#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants
{
    vec2 Scale;
    vec2 Translation;
};

layout(location = 0) in vec2 InPosition;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in vec4 InColor;

layout(location = 0) out vec2 OutTexCoord;
layout(location = 1) out vec4 OutColor;

void main()
{
    gl_Position = vec4(InPosition * Scale + Translation, 0.0, 1.0);
    OutTexCoord = InTexCoord;
    OutColor = InColor;
}