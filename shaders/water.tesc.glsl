#version 330 core
#extension GL_ARB_tessellation_shader : require

uniform vec3 camPosition;

layout(vertices = 4) out;

in vec3 CS_Position[];

out vec3 ES_Position[];

void main()
{
	ES_Position[gl_InvocationID] = CS_Position[gl_InvocationID];

	float dist = length(abs(camPosition - ES_Position[gl_InvocationID]));
	int level = 16;
	
	gl_TessLevelOuter[0] = level;
	gl_TessLevelOuter[1] = level;
	gl_TessLevelOuter[2] = level;
	gl_TessLevelOuter[3] = level;
	gl_TessLevelInner[0] = level;
	gl_TessLevelInner[1] = level;
}