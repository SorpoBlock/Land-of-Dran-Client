#version 330 core

out vec4 color;

in float portion;

uniform vec3 bulletTrailStart;
uniform vec3 bulletTrailEnd;
uniform vec3 bulletTrailColor;
uniform float bulletTrailProgress;

void main()
{	
	if(portion < 0 || portion > 1)
		discard;

	float dist = portion - bulletTrailProgress;
	if(dist < 0.05)
	{
		color.rgb = mix(vec3(1),bulletTrailColor,dist/0.05);
		color.a = dist / 0.05;
	}
	else
		discard;
}
