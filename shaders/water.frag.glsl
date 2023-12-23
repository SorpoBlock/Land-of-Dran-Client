#version 330 core

uniform vec3 cameraPlayerPosition;
uniform vec3 cameraPlayerDirection;

uniform vec3 fogColor;
uniform float fogDistance;

uniform vec4 sunColor;
uniform vec3 sunDirection;

uniform sampler2D normalTexture;
uniform sampler2D refractionTexture;
uniform sampler2D reflectionTexture;
uniform sampler2D postProcessingTexture;
uniform sampler2D shadowNearMap;

uniform bool niceWater;
uniform sampler2D heightMapTexture;
uniform vec3 tessellationScale;

in vec2 UV;
in vec4 clipSpace;
uniform float waterDelta;

in vec3 FS_Position;

out vec4 color;

float getHeight(vec2 worldPosition)
{
	vec2 uv = worldPosition / tessellationScale.xz;
	vec3 texColor = texture2D(heightMapTexture,uv).rgb;
	return ((texColor.r / 256.0) + texColor.g) * tessellationScale.y - 300;
}

float getHeight(float x,float z)
{
	return getHeight(vec2(x,z));
}

void main() 
{
	if(!niceWater) {
   		vec2 p = mod(UV*6.28318530718, 6.28318530718)-250.0;
		vec2 i = vec2(p);
		float c = 1.0;
		float inten = .005;

		for (int n = 0; n < 4; n++) 
		{
			float t = waterDelta * (1.0 - (3.5 / float(n+1)));
			i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
			c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
		}
		c /= float(4);
		c = 1.17-pow(c, 1.4);
		vec3 colour = vec3(pow(abs(c), 8.0));
		colour = clamp(colour + vec3(0.0, 0.35, 0.5), 0.0, 1.0);
		color = vec4(colour, 0.25 + (colour.r + colour.g + colour.b) / 5.0);
		return;
	}

	float dist = abs(length(cameraPlayerPosition - FS_Position));
	
	vec2 ndc = (clipSpace.xy/clipSpace.w)/2.0 + 0.5;
	vec2 refractTexCoords = vec2(ndc.x,ndc.y);
	vec2 reflectTexCoords = vec2(ndc.x,-ndc.y);
	
	/*float near = 0.5;
	float far = 1000.0;
	float texDepth = texture(shadowNearMap,ndc).r;
	float fragDepth = gl_FragCoord.z;
	texDepth =  2.0 * near * far / (far + near - (2.0 * texDepth - 1.0) * (far - near));
	fragDepth = 2.0 * near * far / (far + near - (2.0 * fragDepth - 1.0) * (far - near));
	float relWaterDepth = texDepth-fragDepth;
	float waterDepth = relWaterDepth;//300-getHeight(FS_Position.xz);
	float waterDepthScale = clamp(relWaterDepth,0,10) / 10.0;*/
	
	vec2 predistort = vec2(UV.x,UV.y) * 2.0;
	vec2 distort1 = texture(normalTexture,predistort+vec2(waterDelta / 4.0,0)).rg * 2.0 - 1.0;
	vec2 distort2 = texture(normalTexture,predistort+vec2(0,waterDelta / 4.0)).rg * 2.0 - 1.0;
	vec2 distort = distort1 + distort2;
	
	float distScale = clamp(dist,100.0,1000.0) / 1000.0;
	distort /= 10.0;
	distort *= distScale;
	
	reflectTexCoords += distort;// * waterDepthScale;
	reflectTexCoords.x = clamp(reflectTexCoords.x,0.001,0.999);
	reflectTexCoords.y = clamp(reflectTexCoords.y,-0.999,-0.001);
	
	refractTexCoords += distort;
	refractTexCoords = clamp(refractTexCoords,0.001,0.999);

	vec4 reflectColor = texture(reflectionTexture,reflectTexCoords);
	vec4 refractColor = texture(refractionTexture,refractTexCoords);
	
	float nonLinearAlbedoF = 1.0;
	reflectColor.rgb = pow(reflectColor.rgb,vec3(1.0 + 1.2 * nonLinearAlbedoF));
	refractColor.rgb = pow(refractColor.rgb,vec3(1.0 + 1.2 * nonLinearAlbedoF));
	
	vec3 camDir = normalize(cameraPlayerPosition - FS_Position);
	float refractionPortion = dot(camDir,vec3(0.0,1.0,0.0));//pow(dot(camDir,vec3(0.0,1.0,0.0)),0.8);
	color = mix(reflectColor,refractColor,clamp(refractionPortion,0.05,0.95));
	
	//Tone maping
	color.rgb = color.rgb / (color.rgb + vec3(1.0));
	//Gamma correction
	color.rgb = pow(color.rgb, vec3(1.0/2.2)); 
	
	color.a = 1.0;
}