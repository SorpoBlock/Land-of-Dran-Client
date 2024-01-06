#version 330 core

//new
out vec4 color;

uniform bool useAlbedo;
uniform bool useNormal;
uniform bool useMetalness;
uniform bool useAO;
uniform bool useHeight;
uniform bool useRoughness;
uniform bool calcTBN;
uniform bool renderingSky;
uniform bool renderingSun;

uniform vec3 cameraPlayerPosition;
uniform vec3 cameraPlayerDirection;

uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D mohrTexture;
uniform sampler2DArray shadowNearMap;
uniform sampler2DArray shadowColorMap;
uniform sampler2D refractionTexture;
uniform sampler2D reflectionTexture;
uniform samplerCube cubeMapEnvironment;

uniform bool useShadows;
uniform bool useColoredShadows;
uniform int shadowSoftness;
uniform bool sunAboveHorizon;
uniform vec3 fogColor;
uniform vec3 sunDirection;
uniform vec4 sunColor;
uniform bool previewTexture;
uniform float dncInterpolation;
uniform vec3 skyColor;
uniform bool drawingDebugLocations;
uniform vec3 debugColor;
uniform vec3 tint;
uniform bool useTint;
uniform bool skipCamera;
uniform bool bottomLand;

uniform bool avatarSelectorLighting;
uniform int currentMesh;
/*uniform bool useNodeColor;
uniform vec3 nodeColor;*/

uniform bool pointLightUsed[8];
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform bool pointLightIsSpotlight[8];
uniform vec4 pointLightDirection[8];

uniform samplerCube cubeMapRadiance;
uniform samplerCube cubeMapIrradiance;
uniform sampler2D brdfTexture;

in vec2 uv;
in vec3 normal;
in vec3 bitangent;
in vec3 tangent;
in vec3 worldPos;
in vec3 tanFragPos;
in vec3 tanViewPos;
in vec4 shadowPos[3];
in vec3 nodeColor;

flat in int usePickingColor;
flat in int pickingColor;
 
//Start tutorial code
//https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap(vec2 realUV)
{
    vec3 tangentNormal = textureLod(normalTexture, realUV,0).xyz * 2.0 - 1.0;
	
    //Extra code so that if useNormal(map) is 0, texture map normal defaults to 0,1,0 aka it's just the interpolated vertex normal
	if(!useNormal)
		tangentNormal = vec3(0,1,0);


	mat3 TBN;

	if(calcTBN)
	{
		//Honeslty don't even need this because assImp calcs TBN for us
		vec3 Q1  = dFdx(worldPos);
		vec3 Q2  = dFdy(worldPos);
		vec2 st1 = dFdx(realUV);
		vec2 st2 = dFdy(realUV);
		vec3 N   = normalize(normal);
		vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
		vec3 B  = -normalize(cross(N, T));
		TBN = mat3(T,B,N);
	}
	else
		TBN = mat3(
			normalize(tangent),
			normalize(bitangent),
			normalize(normal)
		);
	
	

    return normalize(TBN * tangentNormal);
}
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

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
   float heightScale = 0.3;

   float doReverse = 1.0;

    // number of depth layers
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = doReverse - texture(mohrTexture, currentTexCoords).b;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = doReverse - texture(mohrTexture, currentTexCoords).b;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (doReverse - texture(mohrTexture, prevTexCoords).b) - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec2 simpleParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
   float height_scale = 0.01;
    float height =  texture(mohrTexture, texCoords).b;    
    vec2 p = viewDir.xy / viewDir.z * (height * height_scale);
    return texCoords - p;    
} 
//End tutorial code

void main()
{	
	if(usePickingColor == 1)
	{
		float pickingFloat = pickingColor;
		color = vec4(vec3(pickingFloat/255.0),1);
		color.a = 1;
		return;
	}

	if(drawingDebugLocations)
	{
		color = vec4(debugColor,1);
		return;
	}

	float dist = abs(length(cameraPlayerPosition - worldPos));
	
	if(renderingSun)
	{
		color = vec4(1,1,1,1);
		return;
	}
	 
	color.a = 0.0;
	
	if(previewTexture)
	{
		//color = texture(normalTexture,uv);
		color = texture(normalTexture,uv);
		color.a = 1.0;
		//if(color.a < 0.1)
			//discard;
		return;
	}
	
	vec3 viewVector = normalize(cameraPlayerPosition - worldPos);
	vec3 tanViewVector = normalize(tanViewPos - tanFragPos);
	
	vec2 realUV = uv;
	if(bottomLand)
		realUV = vec2(mod(worldPos.x,50)/50,mod(worldPos.z,50)/50);
	
	if(useHeight)
	{
		float xScale = 10.0;
		float yScale = 10.0;
		realUV = ParallaxMapping(uv*vec2(xScale,yScale),tanViewVector);
		if(realUV.x > xScale || realUV.y > yScale || realUV.x < 0 || realUV.y < 0)
			discard;
	}
		
	//color = vec4(realUV,0,1);
	//return;

	vec4 albedo_ = vec4(nodeColor,1);
	if(useAlbedo)
		albedo_ = texture(albedoTexture,realUV);
	/*if(useNodeColor)
	{ 
		if(useAlbedo)
		{
			albedo_ = texture(albedoTexture,realUV);
			float textureAlpha = albedo_.a * float(useAlbedo);
			albedo_.rgb = mix(nodeColor,albedo_.rgb,textureAlpha);
		}
		else
			albedo_.rgb = nodeColor;
	}
	else
	{
		albedo_ = texture(albedoTexture,realUV);
		if(useTint)
			albedo_.rgb *= tint;
	}*/
	
	if(albedo_.a < 0.1)
		discard;
	vec3 newNormal = getNormalFromMap(realUV);	
	float nonLinearAlbedoF = 1.0;											//Honestly no clue if this applies to bricks...
	vec3 albedo = pow(albedo_.rgb,vec3(1.0 + 1.2 * nonLinearAlbedoF));
	
	vec3 windowTint = vec3(0); 
	float bias = max(0.01 * (1.0 - dot(newNormal, normalize(sunDirection))), 0.001);  
	float shadowCoverage = 0.0;
	
	if(useShadows && sunAboveHorizon && !skipCamera)
	{
		for(int i = 0; i<3; i++)
		{
			vec3 shadowCoords = shadowPos[i].xyz / shadowPos[i].w;
			shadowCoords = shadowCoords * 0.5 + 0.5;
			if(shadowCoords.x < 0 || shadowCoords.x > 1 || shadowCoords.y < 0 || shadowCoords.y > 1 || shadowCoords.z < 0 || shadowCoords.z > 1)
				continue;
			
			float bias = 0.0001;
	
			float fragDepth = shadowCoords.z;
				
			int samplesTaken = 0;
			float sTex = 500.0;
			vec2 sTexSize = vec2(1.0/sTex,1.0/sTex);
			int pcf = shadowSoftness;
			if(i > 1)
				pcf = 0;
			for(int x = -pcf; x <= pcf; ++x)
			{
				for(int y = -pcf; y <= pcf; ++y)
				{
					samplesTaken++;
					vec2 offset = vec2(x,y) * sTexSize;
					float depth =		texture(shadowNearMap,vec3(shadowCoords.xy+offset,i)).r;
					shadowCoverage += ((fragDepth - bias) > depth) ? 1.0 : 0.0;
					
					if(useColoredShadows)
					{
						if((fragDepth - bias)  > depth)
							windowTint += texture(shadowColorMap,vec3(shadowCoords.xy+offset,i)).rgb;
					}
				}
			}
			shadowCoverage /= samplesTaken;
			windowTint /= samplesTaken;
			
			break;
		}
	}
	
	vec4 mohr = texture(mohrTexture,realUV);
	
	//Metalness defaults to 0
	float metalness = mohr.r;
	if(!useMetalness)
		metalness = 0;
		
	//AO defaults to 1 (no occlusion)
	float occlusion = mohr.g;
	if(!useAO)
		occlusion = 1.0;
		
	//Roughness defaults to 0.5
	float roughness = mohr.a;
	if(!useRoughness)
		roughness = 0.5;
		
	if(bottomLand)
	{
		metalness = 0.0;
		roughness = 1.0;
	}
	
	//New IBL lighting code:	
	windowTint *= 0.6;
	float specularShadowEffect = 1.0 - (metalness * 0.5);
	specularShadowEffect *= 1.0 - length(windowTint);
	float totalFaceAwayFromSunAmountDiffuse = clamp(1.0-(shadowCoverage*0.5),0,1);
	float totalFaceAwayFromSunAmountSpecular = clamp(1.0-(shadowCoverage*specularShadowEffect),0,1);
	
	float NdotV = max(dot(newNormal, viewVector), 0.0);	
	vec3 halfVector = normalize(viewVector + sunDirection);     
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);
    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

	if(avatarSelectorLighting)
	{
		vec3 lightDirection = normalize(vec3(-1,-0.5,-0.25));
		vec3 radiance     = vec3(10.0); 
		vec3 lightHalfVector = normalize(viewVector + lightDirection);
		float NdotL = max(dot(newNormal, lightDirection), 0.0); 
		float NDF = DistributionGGX(newNormal, lightHalfVector, roughness);            
		float G   = GeometrySmith(NdotV,NdotL,roughness);
		vec3 lightF = fresnelSchlick(max(dot(lightHalfVector,viewVector),0.0),F0);
		vec3 numerator    = NDF * G * lightF;
		float denominator = 4.0 * NdotV * NdotL + 0.0001;
		vec3 lightSpecular     = numerator / denominator;              
		color.rgb += (kD * albedo / PI + lightSpecular) * radiance * max(NdotL,0.13);
		color.a = 1.0;
	
		//Tone maping
		color.rgb = color.rgb / (color.rgb + vec3(1.0));
		//Gamma correction
		color.rgb = pow(color.rgb, vec3(1.0/2.2)); 
		return;
	}
	
	color = vec4(0,0,0,1);
	
	for(int i = 0; i<8; i++)
	{
		if(!pointLightUsed[i])
			continue;
			
		float distance    = length(pointLightPos[i] - worldPos);
			
		if(distance > 200)
			continue;
			
		vec3 lightDirection = normalize(pointLightPos[i] - worldPos);
		float thetaStrength = 1.0;
			
		if(pointLightIsSpotlight[i])
		{
			vec3 spotLightDirection = pointLightDirection[i].xyz;
			float phi = pointLightDirection[i].w;
			float theta = dot(lightDirection,normalize(-spotLightDirection));
			if(theta < phi)
				continue;
			if(theta - 0.25 <= phi)
				thetaStrength = (phi-theta) / -0.25;
		}
	
		//vec3 pointLightColor = vec3(900,900,400);
		//vec3 pointLightPos = vec3(1020,37,227);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance     = pointLightColor[i] * attenuation; 
		vec3 lightHalfVector = normalize(viewVector + lightDirection);
		radiance *= thetaStrength;
		
		float NdotL = max(dot(newNormal, lightDirection), 0.0); 
		float NDF = DistributionGGX(newNormal, lightHalfVector, roughness);            
		float G   = GeometrySmith(NdotV,NdotL,roughness);
		vec3 lightF = fresnelSchlick(max(dot(lightHalfVector,viewVector),0.0),F0);
		vec3 numerator    = NDF * G * lightF;
		float denominator = 4.0 * NdotV * NdotL + 0.0001;
		vec3 lightSpecular     = numerator / denominator;              
		color.rgb += (kD * albedo / PI + lightSpecular) * radiance * NdotL; 
	}
		

	float NdotL = max(dot(normalize(newNormal), normalize(sunDirection)), 0.0);  
	float NDF = DistributionGGX(newNormal, halfVector, roughness);   
	float G   = GeometrySmith(NdotV,NdotL,roughness);   
	
	vec3 numerator    = NDF * G * F; 
	float denominator = 4 * NdotV * NdotL + 0.001; // 0.001 to prevent divide by zero
	vec3 specular = numerator / denominator;
	
	//The contentious part
	float totalFaceAwayFromSunAmount = min(1.0-(shadowCoverage*0.8),NdotL);
	
	specular *= sqrt(clamp(vec3(1.0 - shadowCoverage),0.1,1));
	vec3 windowRadiance = vec3(0); //windowTint.rgb * length(sunColor) * windowTint.a * 5;
	color.rgb += (kD * albedo / PI + specular) * sunColor.rgb * totalFaceAwayFromSunAmount;
	color.rgb += (kD * albedo / PI + specular) * windowRadiance * NdotL;
	color.rgb += vec3(0.2) * albedo * occlusion * normalize(sunColor.rgb);
	//...
	
	if(!skipCamera)
	{
		float fogDist = clamp(dist,500,1000);
		fogDist -= 500.0;
		fogDist /= 500.0;
		color.rgb = mix(color.rgb,fogColor,fogDist);
	}
	
	//Tone maping
	color.rgb = color.rgb / (color.rgb + vec3(1.0));
	//Gamma correction
	color.rgb = pow(color.rgb, vec3(1.0/2.2)); 
	
	color.a = 1.0;
}


 



















