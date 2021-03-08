#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT float Payload;

void main()
{
	Payload = gl_HitTEXT;
}