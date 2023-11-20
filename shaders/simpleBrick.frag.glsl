#version 330 core

out vec4 color;

uniform bool useAlbedo;
uniform bool useNormal;
uniform bool useMetalness;
uniform bool useAO;
uniform bool useHeight;
uniform bool useRoughness;

//Either use these for basic bricks...
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D mohrTexture;
//Or these for special bricks...
uniform sampler2D topNormalTexture;
uniform sampler2D topMohrTexture;
uniform sampler2D sideNormalTexture;
uniform sampler2D sideMohrTexture;
uniform sampler2D bottomNormalTexture;
uniform sampler2D bottomMohrTexture;
uniform sampler2D rampNormalTexture;
uniform sampler2D rampMohrTexture;
uniform sampler2D printTexture;
uniform sampler2D printTextureSpecialBrickNormal;

uniform sampler2DArray shadowNearMap;
uniform sampler2D shadowColorMap;
uniform sampler2D shadowNearTransMap;

uniform samplerCube cubeMapRadiance;
uniform samplerCube cubeMapIrradiance;
uniform sampler2D brdfTexture;

uniform vec3 cameraPlayerPosition;

uniform bool useShadows;
uniform int shadowSoftness;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 fogColor;
uniform bool sunAboveHorizon;
uniform float deltaT;
uniform bool isPrint;

in vec3 modelPos; //Used only for living bricks
in vec2 uv;
in vec2 verts;
in vec3 normal;
in vec3 worldPos;
in vec4 finalColor;
flat in vec2 swapDimensions;
flat in vec3 dimension;
flat in int direction;
uniform bool specialBricks;
in float brickMaterial;
in float textureToUse;

uniform int debugMode;

//Start tutorial code
//https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float NdotV,float NdotL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

bool floatCompare(float a,float b)
{
	return abs(a-b) < 0.05;
}

void main()
{	
	if(debugMode == 1)
	{
		color = finalColor;
		color.rgb *= clamp(dot(sunDirection,normal),0.2,1.0);
		return;
	}

	float finalAlpha = finalColor.a;
	float dist = abs(length(cameraPlayerPosition - worldPos));

	if(floatCompare(brickMaterial,1))
	{
		color.r = finalColor.r + 0.3 * (sin((deltaT/15.0)+worldPos.x*8.0) * cos(deltaT/20.0));
		color.g = finalColor.g + 0.3 * (cos(worldPos.z*8.0) * sin(deltaT/20.0));
		color.b = finalColor.b + 0.3 * (cos((deltaT/30.0)+worldPos.y*8.0) * sin(deltaT/20.0));
		color.a = min(finalColor.a,0.5);
		return;
	}

	if(finalAlpha < 0.01)
		discard;
		
	vec2 uvs = uv;
	float roughness = 0.8;
	float metalness = 0.0;
	float occlusion = 1;
	vec3 newNormal = normal;
	vec3 albedo = pow(finalColor.rgb,vec3(1.0 + 1.2 * 1.0)); 
	
	if(floatCompare(brickMaterial,6))
	{
		finalAlpha = abs(sin(uv.x*8.0 + deltaT/20.0));
		if(finalAlpha < 0.01)
			discard;
	}
	if(floatCompare(brickMaterial,2))
		metalness = 0.5;
	if(floatCompare(brickMaterial,3))
		metalness = 1.0;
	if(floatCompare(brickMaterial,5))
	{
		albedo *= (0.5 + ((1.0+sin(deltaT/15.0)) * 0.5));
	}
	
	vec4 windowTint = vec4(0); 
	float shadowCoverage = 0.5;
	
	vec3 viewVector = normalize(cameraPlayerPosition - worldPos);
	vec3 halfVector = normalize(viewVector + sunDirection);     
	float NdotV = max(dot(newNormal, viewVector), 0.0);	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);
    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;
	
	//New IBL lighting code:
	float specularShadowEffect = 1.0 - (metalness * 0.5);
	float totalFaceAwayFromSunAmountDiffuse = clamp(1.0-(shadowCoverage*0.5),0,1);
	float totalFaceAwayFromSunAmountSpecular = clamp(1.0-(shadowCoverage*specularShadowEffect),0,1);
	
	if(floatCompare(brickMaterial,4))
	{
		totalFaceAwayFromSunAmountDiffuse = 1.0;
		totalFaceAwayFromSunAmountSpecular = 1.0;
		albedo *= 3.0;
	}
	
	color = vec4(0,0,0,1);
	
	if(debugMode == 2)
	{
		vec3 lightDirection = sunDirection;
		vec3 lightHalfVector = normalize(viewVector + lightDirection);
		
		float NdotL = max(dot(newNormal, lightDirection), 0.0) * (1.0-shadowCoverage); 
		
		vec3 radiance = vec3(10,6,5);
		float NDF = DistributionGGX(newNormal, lightHalfVector, roughness);            
		float G   = GeometrySmith(NdotV,NdotL,roughness);
		vec3 lightF = fresnelSchlick(max(dot(lightHalfVector,viewVector),0.0),F0);
		vec3 numerator    = NDF * G * lightF;
		float denominator = 4.0 * NdotV * NdotL + 0.0001;
		vec3 lightSpecular     = numerator / denominator;       
		
		color.rgb += (kD * albedo / PI + lightSpecular) * radiance * NdotL; 
		
		NdotL = 1.0 - NdotL;
		
		radiance = vec3(1,2,4);
		NDF = DistributionGGX(newNormal, lightHalfVector, roughness);            
		G   = GeometrySmith(NdotV,NdotL,roughness);
		lightF = fresnelSchlick(max(dot(lightHalfVector,viewVector),0.0),F0);
		numerator    = NDF * G * lightF;
		denominator = 4.0 * NdotV * NdotL + 0.0001;
		lightSpecular     = numerator / denominator;       
		
		color.rgb += (kD * albedo / PI + lightSpecular) * radiance * NdotL; 
	}
	else
	{
		vec3 R = reflect(-viewVector,newNormal);
		vec3 irradiance = textureLod(cubeMapIrradiance, vec3(1,-1,1)*newNormal,0).rgb;
		vec3 diffuse      = irradiance * albedo;
		diffuse += albedo * irradiance * windowTint.rgb * 10;
		const float MAX_REFLECTION_LOD = 10.0;
		vec3 prefilteredColor = textureLod(cubeMapRadiance, vec3(1,-1,1)*R,  roughness * MAX_REFLECTION_LOD).rgb;    
		vec2 brdf  = texture(brdfTexture, vec2(max(dot(newNormal, viewVector), 0.0), roughness)).rg;
		vec3 specular = prefilteredColor * (F * brdf.x + brdf.y) * totalFaceAwayFromSunAmountSpecular;
		vec3 ambient = (kD * diffuse + specular) * occlusion * totalFaceAwayFromSunAmountDiffuse;
		color.rgb += ambient;
	}
	
	//Tone maping
	color.rgb = color.rgb / (color.rgb + vec3(1.0));
	//Gamma correction
	color.rgb = pow(color.rgb, vec3(1.0/2.2)); 
	color.a = 1.0;
	
	float fogDist = clamp(dist,500,1200);
	fogDist -= 500.0;
	fogDist /= 700.0;
	vec3 tmpFogColor = vec3(172,170,190) / 255.0;
	color.rgb = mix(color.rgb,tmpFogColor,fogDist);
	
	color.a = finalAlpha;
}