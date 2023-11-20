#version 330 core

layout(location = 0) in vec3 verts;

out vec3 uv;

void main()
{
	uv = (verts + 1.0) / 2.0;
	gl_Position = vec4(verts.xy,0.0,1.0);
	return;
} 