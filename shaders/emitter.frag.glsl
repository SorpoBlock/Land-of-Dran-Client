#version 330 core

out vec4 color;

uniform sampler2D albedoTexture;

uniform vec4 colors[4];
uniform bool calculateMovement;

flat in int frameOne;
flat in int frameTwo;
in float frameInterpolate;
in vec2 uvs;

void main()
{	
	color = texture(albedoTexture,uvs);
	
	color *= mix(colors[frameOne],colors[frameTwo],frameInterpolate);
	
	if(calculateMovement)
	{
		if(color.a < 0.5)
			discard;
		else
			color.a = 1.0;
	}
	else if(color.a < 0.05)
		discard;
}
