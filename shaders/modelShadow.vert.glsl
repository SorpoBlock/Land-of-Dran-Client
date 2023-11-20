#version 330 core

layout(location = 0) in vec3  positions;
layout(location = 1) in vec2  uvs;
layout(location = 2) in vec3  normals;
layout(location = 3) in vec3  tangents;
layout(location = 4) in vec3  bitangents;
//Buffer 5 is index
layout(location = 7) in vec3  perMeshColor;
layout(location = 8) in mat4 perMeshTransform;
//Slots 8,9, and 10 are also taken up by perMeshTransform

//Basic transformations:
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform mat4 scaleMatrix;

//Fragment shader inputs:
out vec3 worldPos;
out vec4 trans;

void main()
{	
	trans = vec4(0.0,0.0,0.0,1.0);
	vec4 endPos = perMeshTransform * vec4(positions,1.0);
	worldPos = (translateMatrix * rotateMatrix * scaleMatrix * endPos).xyz;
	gl_Position = vec4(worldPos,1.0);
}
