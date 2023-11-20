#version 330 core

uniform vec3 bulletTrailStart;
uniform vec3 bulletTrailEnd;
uniform vec3 bulletTrailColor;
uniform float bulletTrailProgress;

uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;
uniform vec3 cameraPlayerRight;
uniform vec3 cameraPlayerUp;

layout(location = 0) in vec3  positions;

out float portion;

void main()
{	
	vec3 endPos;
	if(positions.x > 0.5)
		endPos = bulletTrailStart;
	else
		endPos = bulletTrailEnd;

	portion = positions.x;

	gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(endPos,1);
}