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

uniform vec4 sunColor;
uniform bool useIBL;
uniform bool sunAboveHorizon;

uniform sampler2D albedoTexture;

in vec2 uv;
in vec4 sunPos;

void main()
{	
	if(cameraPlayerPosition.y < waterLevel)
		color = vec4(0.2,0.2,0.8,0.5);
	else if(sunAboveHorizon)
	{
		vec3 sunCoords = sunPos.xyz / sunPos.w;
		vec2 realSunPos = sunCoords.xy * 0.5 + 0.5;//(sunCoords.xy + 1.0) / 2.0;
			
		if(godRaySamples > 1)
		{
			color = vec4(0,0,0,0);
			
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
			color.a = clamp(endColor,0,1);
			color.rgb = sunColor.rgb;
		}
		else if(!useIBL)
		{	
			float mySample = texture(albedoTexture, uv ).r;
			if(mySample > 0.5)
				color = vec4(sunColor.rgb,1);
		}
	}
	else
		color = vec4(0,0,0,0);
	
	float vignetteDist = (abs(uv.x-0.5) + abs(uv.y-0.5))/2.0;
	vec4 vignette = vec4(vignetteColor,clamp(vignetteDist * vignetteStrength,0,1));
	
	color.rgb = color.rgb * color.a + vignette.rgb * vignette.a;//mix(color.rgb,vignette.rgb,vignette.a);
	color.a = max(vignette.a,color.a);
	
	//if(vignetteDist*vignetteStrength > 0.1)
		//color = vec4(vignetteColor,vignetteDist*vignetteStrength);
}
