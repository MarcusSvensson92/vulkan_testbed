#version 460
#extension GL_NV_ray_tracing : enable

layout(location = 0) rayPayloadInNV bool Payload;

void main()
{
	Payload = true;
}