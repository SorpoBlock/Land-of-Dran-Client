#version 330 core

//new
out vec4 color;

uniform vec3 cameraPlayerPosition;
uniform float waterLevel;

uniform float godRayDensity;
uniform float godRayExposure;
uniform float godRayDecay;
uniform float godRayWeight;
uniform int godRaySamples;

uniform vec3 vignetteColor;
uniform float vignetteStrength;

uniform sampler2D albedoTexture;

in vec2 uv;
in vec4 sunPos;

void main()
{	
	color = vec4(vignetteColor,0);
	
	float vignetteDist = (abs(uv.x-0.5) + abs(uv.y-0.5))/2.0;
	
	color.a = vignetteDist*vignetteStrength;

	if(cameraPlayerPosition.y < waterLevel)
	{
		color = vec4(0.2,0.2,0.8,0.5);
		return;
	}
	
	if(godRaySamples > 1)
	{
		color = vec4(0,0,0,0);
		float center = abs(uv.x-0.5) + abs(uv.y-0.5);
		if(center > 0.005 && center < 0.01)
			color = vec4(1,1,1,0.5);
		//color.gb = (sunPos.xy + 1.0) / 2.0;
		vec3 sunCoords = sunPos.xyz / sunPos.w;
		vec2 realSunPos = sunCoords.xy * 0.5 + 0.5;//(sunCoords.xy + 1.0) / 2.0;
		

		vec2 textCoo = uv;
		vec2 deltaTextCoord = vec2( textCoo - realSunPos );
		deltaTextCoord *= 1.0 /  float(godRaySamples) * godRayDensity;
		float illuminationDecay = 1.0;
		float endColor = 0;
	
		for(int i=0; i < godRaySamples ; i++)
		{
				textCoo -= deltaTextCoord;
				float mySample = texture(albedoTexture, textCoo ).r;
		
				mySample *= illuminationDecay * godRayWeight;

				endColor += mySample;

				illuminationDecay *= godRayDecay;
		 }

		endColor *= godRayExposure;
		color.a = endColor;
		color.rgb = vec3(1.0,1.0,0.7);
	}
}
