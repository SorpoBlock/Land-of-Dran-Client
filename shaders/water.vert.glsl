#version 330 core

uniform vec3 cameraPlayerPosition;
uniform vec3 tessellationScale;

layout(location = 0) in vec3 verts;

out vec3 CS_Position;

void main()
{
	CS_Position = verts + floor(vec3(cameraPlayerPosition.x,0,cameraPlayerPosition.z)/50.0)*50.0 - vec3(tessellationScale.x,0,tessellationScale.z)/2.0;
}
