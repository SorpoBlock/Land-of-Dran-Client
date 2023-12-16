#version 330 core

layout(location = 0) in vec3  positions;

uniform mat4 anglePlayerMatrix;
uniform mat4 projectionPlayerMatrix;
uniform vec3 sunDirection;
uniform float sunDistance;

out vec4 sunPos;
out vec2 uv;

void main()
{
	vec4 pos = projectionPlayerMatrix * anglePlayerMatrix * vec4(positions + normalize(sunDirection) * 20.0 * sunDistance,1.0);
	pos.z = 0.5;
	sunPos = pos;
	
	uv = (positions.xy + 1.0) / 2.0;
	gl_Position = vec4(positions,1.0);
}