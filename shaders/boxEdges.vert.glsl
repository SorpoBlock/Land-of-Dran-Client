#version 330 core

uniform mat4 projectionPlayerMatrix;
uniform mat4 viewPlayerMatrix;
uniform vec3 start;
uniform vec3 end;
uniform bool drawingGraph;
uniform bool drawingRopes;

layout(location = 0) in vec3  positions;
layout(location = 1) in float dist;

out float interpolateDist;
 
void main()
{
	/*if(drawingGraph)
	{
		gl_Position.xyz = (positions * 2.0) - 1.0;
		gl_Position.w = 1.0;
	}*/
	interpolateDist = 1;
	if(drawingRopes)
	{
		interpolateDist = dist;
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(positions,1);
	}
	else
	{
		vec3 worldPos = (end-start) * (positions+vec3(0.5,0.5,0.5)) + start;
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(worldPos,1);
		gl_Position.z -= 0.01;
	}
}
 