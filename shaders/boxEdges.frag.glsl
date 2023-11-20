#version 330 core

uniform bool drawingRopes;
uniform vec4 boxColor;

out vec4 color;

in float interpolateDist;

void main()
{
	if(drawingRopes)
	{
		color = vec4(0.588,0.341,0.2,1);
		if(mod(interpolateDist,1) < 0.2)
			color.rgb /= 2.5;
	}
	else
		color = boxColor;
}


 



















