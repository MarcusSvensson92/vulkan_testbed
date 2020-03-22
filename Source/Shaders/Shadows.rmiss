#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT bool Payload;

void main()
{
	Payload = true;
}