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
uniform sampler2DArray shadowColorMap;

uniform samplerCube cubeMapRadiance;
uniform samplerCube cubeMapIrradiance;
uniform sampler2D brdfTexture;

uniform vec3 cameraPlayerPosition;

in vec3 tanViewPos;
in vec3 tanWorldPos;
in vec3 modelPos; //Used only for living bricks
in vec2 uv;
in vec2 roughUVs;
in vec2 edgeSize;
in vec2 verts;
in vec3 normal;
in vec3 worldPos;
in vec4 finalColor;
flat in vec3 dimension;
flat in int direction;
uniform bool specialBricks;
uniform float materialCutoff;
uniform bool useColoredShadows;
uniform int shadowSoftness;
uniform vec3 sunDirection;
uniform vec4 sunColor;
uniform vec3 fogColor;
uniform bool sunAboveHorizon;
uniform float deltaT;
uniform bool isPrint;
uniform bool useShadows;
uniform bool useIBL;

uniform bool pointLightUsed[8];
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform bool pointLightIsSpotlight[8];
uniform vec4 pointLightDirection[8];

in mat3 TBN;
in vec4 shadowPos[3];
flat in int brickMaterial;
flat in int textureToUse;

vec3 getNormalFromMapGrad(sampler2D tex,vec2 UV,vec2 dx,vec2 dy)
{
    vec3 tangentNormal = textureGrad(tex, UV,dx,dy).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);

    vec3 N   = normalize(normal);
    vec3 T  = normalize(Q1*dy.t - Q2*dx.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

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
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------

vec2 ParallaxMapping(sampler2D tex,vec2 texCoords, vec3 viewDir)
{ 
	return texCoords;
/*
	float heightScale = 0.15;

    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
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
    float currentDepthMapValue = texture(tex, currentTexCoords).b;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(tex, currentTexCoords).b;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(tex, prevTexCoords).b - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;*/
}

void main()
{		
	vec3 tanViewVector = normalize(tanViewPos - tanWorldPos);
	vec3 viewVector = normalize(cameraPlayerPosition - worldPos);
	float finalAlpha = finalColor.a;
	vec3 finalColorChangeable = finalColor.rgb;
	float dist = abs(length(cameraPlayerPosition - worldPos));
	
	vec2 uvGradX = dFdx(uv);
	vec2 uvGradY = dFdy(uv);
	vec2 roughDx = dFdx(uv);
	vec2 roughDy = dFdy(uv);
	vec3 normalDx = dFdx(normal);
	vec3 normalDy = dFdy(normal);
	 
    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec3 N1   = normalize(normal);
    vec3 T  = normalize(Q1*uvGradY.t - Q2*uvGradX.t);
    vec3 B  = -normalize(cross(N1, T));
    mat3 TBN2 = transpose(mat3(T, B, N1));

	if(brickMaterial == 1)
	{
		color.r = finalColorChangeable.r + 0.3 * (sin((deltaT/15.0)+worldPos.x*8.0) * cos(deltaT/20.0));
		color.g = finalColorChangeable.g + 0.3 * (cos(worldPos.z*8.0) * sin(deltaT/20.0));
		color.b = finalColorChangeable.b + 0.3 * (cos((deltaT/30.0)+worldPos.y*8.0) * sin(deltaT/20.0));
		color.a = min(finalColor.a,0.5);
		return;
	}

	if(finalAlpha < 0.01)
		discard;
		
	float roughness = 0.8;
	float metalness = 0.0;
	float occlusion = 1;
	vec3 newNormal = normal;
	
	vec3 albedo = pow(finalColorChangeable.rgb,vec3(1.0 + 1.2 * 1.0)); 
	
	if(specialBricks)
	{	
		vec2 or = vec2(0,0);
		if(textureToUse == 0)
		{
			vec2 topUv = ParallaxMapping(mohrTexture,uv,normalize(TBN2 * cameraPlayerPosition - TBN2 * worldPos));
			or = textureGrad(topMohrTexture,topUv,uvGradX,uvGradY).ga;
			newNormal = getNormalFromMapGrad(topNormalTexture,topUv,uvGradX,uvGradY);
		}
		else if(textureToUse == 1)
		{
			or = textureGrad(sideMohrTexture,uv,uvGradX,uvGradY).ga;
			newNormal = getNormalFromMapGrad(sideNormalTexture,uv,uvGradX,uvGradY);
		}
		else if(textureToUse == 2)
		{
			or = textureGrad(bottomMohrTexture,uv,uvGradX,uvGradY).ga;
			newNormal = getNormalFromMapGrad(bottomNormalTexture,uv,uvGradX,uvGradY);
		}
		else if(textureToUse == 3)
		{
			or = textureGrad(rampMohrTexture,uv,uvGradX,uvGradY).ga;
			newNormal = getNormalFromMapGrad(rampNormalTexture,uv,uvGradX,uvGradY);
			or.y = sqrt(or.y);
		}
		else if(isPrint)//Special print brick or just no texture
		{
			or = textureGrad(printTexture,uv*0.3,uvGradX*0.3,uvGradY*0.3).ga;
			newNormal = getNormalFromMapGrad(printTextureSpecialBrickNormal,uv*0.3,uvGradX*0.3,uvGradY*0.3);
		}
		else
		{
			or = vec2(occlusion,roughness);
		}
			
		occlusion = or.x;
		roughness = or.y;
	}
	else
	{		
		vec2 uvs = uv;
		//Calulcate normal map and AO map UVs for the sides of bricks
		if(direction == 0)
			uvs = ParallaxMapping(mohrTexture,uv,normalize(TBN2 * cameraPlayerPosition - TBN2 * worldPos));
		else if(direction > 1)
		{
			float edgeWidth = 0.0273; //How many pixels is the border / how many pixels total for the various textures (56/2048 in this case)
			
			if(uv.x <= edgeSize.x)
				uvs.x = mix(0.0,edgeWidth,uv.x/edgeSize.x);
			else if( uv.x >= 1.0 - edgeSize.x)
			{
				float progress = (uv.x - (1.0-edgeSize.x)) / edgeSize.x;
				uvs.x = mix(1.0 - edgeWidth,1.0,progress);
			}
			else
				uvs.x = mix(edgeWidth*1.1,1.0-(edgeWidth*1.1),uv.x);
			
			if(uv.y <= edgeSize.y)
				uvs.y = mix(0.0,edgeWidth,uv.y/edgeSize.y);
			else if(uv.y >= 1.0 - edgeSize.y)
			{
				float progress = (uv.y - (1.0-edgeSize.y)) / edgeSize.y;
				uvs.y = mix(1.0 - edgeWidth,1.0,progress);
			}
			else
				uvs.y = mix(edgeWidth*1.1,1.0-(edgeWidth*1.1),uv.y);
		}
		
		if(isPrint)
		{
			vec4 print = texture(albedoTexture,verts);
			print.rgb = pow(print.rgb,vec3(1.0 + 1.2 * 1.0));
			albedo = mix(albedo,print.rgb,print.a);
		}
		else //dist check?
		{
			if(brickMaterial == 8)
			{
				roughness = 0;
				newNormal = normal;
				occlusion = 1;
			}
			else
			{
				roughness = textureGrad(mohrTexture,roughUVs,roughDx,roughDy).a;
				newNormal = getNormalFromMapGrad(normalTexture,uvs,uvGradX,uvGradY);
				occlusion = textureGrad(mohrTexture,uvs,uvGradX,uvGradY).g;	
			}
		}
	}
	
	if(brickMaterial == 6)
	{
		finalAlpha = abs(sin(uv.x*8.0 + deltaT/20.0));
		if(finalAlpha < 0.01)
			discard;
	}

	if(brickMaterial == 9)
	{
		//float otherFactor = min((sin(0.5 * cameraPlayerDirection.x) + sin(0.5 * cameraPlayerDirection.y))/2.0,0.5);
		//otherFactor *= (sin(0.5 * worldPos.x) + sin(0.5 * worldPos.y) + sin(0.5 * worldPos.z)) / 3.0;
		float otherFactor = ((1.0 + sin(gl_FragCoord.z*0.01))/2.0) * sin(dist * 0.03);
		float oneFactor = (1.0+dot(normal,viewVector))/2.0;
		float iridescent = (1.0+sin(40.0 * oneFactor * otherFactor))/2.0;
		if(iridescent > 0.5)
			finalColorChangeable.rgb = mix(vec3(1.0,0.5,0.15),vec3(0.5,1.0,0.15),(iridescent-0.5)*2.0);
		else
			finalColorChangeable.rgb = mix(vec3(0.15,0.5,1.0),vec3(1.0,0.5,0.15),iridescent*2.0);
			
		albedo = pow(finalColorChangeable.rgb,vec3(1.0 + 1.2 * 1.0)); 
		
		metalness = 1.0;
	}
	
	if(brickMaterial == 2)
		metalness = 0.5;
	if(brickMaterial == 3)
		metalness = 1.0;
	if(brickMaterial == 5)
	{
		albedo *= (0.5 + ((1.0+sin(deltaT/15.0)) * 0.5));
	}
	
	vec3 windowTint = vec3(0); 
	float shadowCoverage = 0.0;
	
	if(useShadows && sunAboveHorizon)
	{
		for(int i = 0; i<3; i++)
		{
			vec3 shadowCoords = shadowPos[i].xyz / shadowPos[i].w;
			shadowCoords = shadowCoords * 0.5 + 0.5;
			
			if(shadowCoords.x < 0 || shadowCoords.x > 1 || shadowCoords.y < 0 || shadowCoords.y > 1 || shadowCoords.z < 0 || shadowCoords.z > 1)
				continue;
			
			float bias = 0.0002;
			
			float fragDepth = shadowCoords.z;
				
			int samplesTaken = 0;
			//float sTex = 500.0;
			//vec2 sTexSize = vec2(1.0/sTex,1.0/sTex);
			vec2 sTexSize = 1.0 / vec2(textureSize(shadowNearMap, 0));
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
	
	float specularShadowEffect = 1.0 - (metalness * 0.5);
	specularShadowEffect *= 1.0 - length(windowTint);
	
	float forceShadow = -1.0 * dot(sunDirection,newNormal);
	forceShadow = clamp(forceShadow,0.0,0.3);
	forceShadow *= (1.0/0.3);
	
	shadowCoverage = max(shadowCoverage,forceShadow);
	
	float totalFaceAwayFromSunAmountDiffuse = clamp(1.0-(shadowCoverage*0.5),0,1);
	float totalFaceAwayFromSunAmountSpecular = clamp(1.0-(shadowCoverage*specularShadowEffect),0,1);
	
	if(brickMaterial == 4)
	{
		totalFaceAwayFromSunAmountDiffuse = 1.0;
		totalFaceAwayFromSunAmountSpecular = 1.0;
		albedo *= 3.0;
	}
	
    // input lighting data
    vec3 N = newNormal;
    vec3 V = normalize(cameraPlayerPosition - worldPos);
    vec3 R = reflect(-V, N); 

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 8; ++i) 
    {
		if(!pointLightUsed[i])
			continue;
			
        // calculate per-light radiance
        vec3 L = normalize(pointLightPos[i] - worldPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLightPos[i] - worldPos);
		
		if(distance > 200.0)
			continue;

		float thetaStrength = 1.0;
			
		if(pointLightIsSpotlight[i])
		{
			vec3 spotLightDirection = pointLightDirection[i].xyz;
			float phi = pointLightDirection[i].w;
			float theta = dot(L,normalize(-spotLightDirection));
			if(theta < phi)
				continue;		   //phi = 0.5, theta = 0.5, diff = 0.0, div = 0.0
			if(theta - 0.25 <= phi) //phi = 0.5, theta = 0.35, diff = 0.15, div = 1.0
				thetaStrength = (phi-theta) / -0.25;
		}
			
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = pointLightColor[i] * attenuation;
		radiance *= thetaStrength;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalness;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;	

	if(useIBL)
	{
		vec3 irradiance = textureGrad(cubeMapIrradiance, vec3(1,-1,1)*N,normalDx,normalDy).rgb;
		windowTint *= (irradiance.r + irradiance.g + irradiance.b);
		vec3 diffuse      = irradiance * albedo * totalFaceAwayFromSunAmountDiffuse;
		diffuse += albedo * (irradiance + windowTint);
	   
		// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
		const float MAX_REFLECTION_LOD = 4.0;
		vec3 prefilteredColor = textureLod(cubeMapRadiance, normalize(vec3(1,-1,1)*R), roughness * MAX_REFLECTION_LOD).rgb;    
		vec2 brdf  = textureGrad(brdfTexture, vec2(max(dot(N, V), 0.0), roughness),uvGradX,uvGradY).rg;
		vec3 specular = prefilteredColor * (F * brdf.x + brdf.y) * totalFaceAwayFromSunAmountSpecular;

		vec3 ambient = (kD * diffuse + specular) * occlusion;
		
		color.rgb = ambient + Lo;
	}
	else
	{
		float NdotL = max(dot(normalize(N), normalize(sunDirection)), 0.0);  
		float NDF = DistributionGGX(N, normalize(V+sunDirection), roughness);   
		float G   = GeometrySmith(N,V,sunDirection,roughness);   
		
		vec3 numerator    = NDF * G * F; 
		float denominator = 4 * dot(N,V) * NdotL + 0.001; // 0.001 to prevent divide by zero
		vec3 specular = numerator / denominator;
		
		//The contentious part
		float totalFaceAwayFromSunAmount = min(1.0-(shadowCoverage*0.8),NdotL);
		
		specular *= sqrt(clamp(vec3(1.0 - shadowCoverage),0.1,1));
		vec3 windowRadiance = windowTint.rgb * sunColor.rgb;
		color.rgb += (kD * albedo / PI + specular) * sunColor.rgb * totalFaceAwayFromSunAmount;
		color.rgb += (kD * albedo / PI + specular) * windowRadiance * NdotL;
		color.rgb += vec3(0.2) * albedo * occlusion * normalize(sunColor.rgb);
	}

    // HDR tonemapping
    color.rgb = color.rgb / (color.rgb + vec3(1.0));
    // gamma correct
    color.rgb = pow(color.rgb, vec3(1.0/2.2)); 
	
	float fogDist = clamp(dist,500,1200);
	fogDist -= 500.0;
	fogDist /= 700.0;
	vec3 tmpFogColor = vec3(172,170,190) / 255.0;
	color.rgb = mix(color.rgb,tmpFogColor,fogDist);

    color.a = finalAlpha;
}
