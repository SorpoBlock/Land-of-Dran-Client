#version 330 core

//new

uniform mat4 anglePlayerMatrix;
uniform mat4 projectionPlayerMatrix;
uniform vec3 sunDirection;
uniform float sunDistance;

layout(location = 0) in vec3  positions;

void main()
{
	gl_Position = projectionPlayerMatrix * anglePlayerMatrix * vec4(positions + normalize(sunDirection) * 20.0 * sunDistance,1.0);
}
 