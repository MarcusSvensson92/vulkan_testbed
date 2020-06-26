#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 OutTexCoord;

void main()
{
	gl_Position = vec4(((gl_VertexIndex << 1) & 2) * 2.0 - 1.0, (gl_VertexIndex & 2) * -2.0 + 1.0, 0.0, 1.0);
	OutTexCoord = gl_Position.xy * 0.5 + 0.5;
}