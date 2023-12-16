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

uniform vec3 cameraPlayerPosition;

in vec3 modelPos; //Used only for living bricks
in vec2 uv;
in vec2 verts;
in vec3 normal;
in vec3 worldPos;
in vec4 finalColor;
in vec2 swapDimensions;
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

uniform bool pointLightUsed[8];
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform bool pointLightIsSpotlight[8];
uniform vec4 pointLightDirection[8];

in vec4 shadowPos[3];
in float brickMaterial;
in float textureToUse;

vec3 getNormalFromMapTop(sampler2D tex,vec2 realUV)
{
    vec3 tangentNormal = texture(tex, realUV).xyz * 2.0 - 1.0;
	
    //Extra code so that if useNormal(map) is 0, texture map normal defaults to 0,1,0 aka it's just the interpolated vertex normal
	//if(!useNormal)
	//	tangentNormal = vec3(0,1,0);


	mat3 TBN;		

	if(true)		//As of 03feb we still need to calc these cause actual tangent/bitangent is messed up
	{
		//Honeslty don't even need this because assImp calcs TBN for us
		vec3 Q1  = dFdx(worldPos);
		vec3 Q2  = dFdy(worldPos);
		vec2 st1 = dFdx(uv);
		vec2 st2 = dFdy(uv);
		vec3 N   = normalize(normal);
		vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
		vec3 B  = -normalize(cross(N, T));
		TBN = mat3(T,B,N);
	}

    return normalize(TBN * tangentNormal);
}

bool floatCompare(float a,float b)
{
	return abs(a-b) < 0.05;
}

vec3 getNormalFromMap(sampler2D tex,vec2 realUV)
{
    vec3 tangentNormal = texture(tex, realUV).xyz * 2.0 - 1.0;
	
    //Extra code so that if useNormal(map) is 0, texture map normal defaults to 0,1,0 aka it's just the interpolated vertex normal
	//if(!useNormal)
	//	tangentNormal = vec3(0,1,0);


	mat3 TBN;		

	if(true)		//As of 03feb we still need to calc these cause actual tangent/bitangent is messed up
	{
		//Honeslty don't even need this because assImp calcs TBN for us
		vec3 Q1  = dFdx(worldPos);
		vec3 Q2  = dFdy(worldPos);
		vec2 st1 = dFdx(uv);
		vec2 st2 = dFdy(uv);
		vec3 N   = normalize(normal);
		vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
		vec3 B  = -normalize(cross(N, T));
		TBN = mat3(T,B,N);
	}

    return normalize(TBN * tangentNormal);
}


vec3 getNormalFromMapGrad(sampler2D tex,vec2 realUV,vec2 gradx,vec2 grady)
{
    vec3 tangentNormal = textureGrad(tex, realUV,gradx,grady).xyz * 2.0 - 1.0;
	
    //Extra code so that if useNormal(map) is 0, texture map normal defaults to 0,1,0 aka it's just the interpolated vertex normal
	//if(!useNormal)
	//	tangentNormal = vec3(0,1,0);


	mat3 TBN;		

	if(true)		//As of 03feb we still need to calc these cause actual tangent/bitangent is messed up
	{
		//Honeslty don't even need this because assImp calcs TBN for us
		vec3 Q1  = dFdx(worldPos);
		vec3 Q2  = dFdy(worldPos);
		vec2 st1 = dFdx(uv);
		vec2 st2 = dFdy(uv);
		vec3 N   = normalize(normal);
		vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
		vec3 B  = -normalize(cross(N, T));
		TBN = mat3(T,B,N);
	}

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


void main()
{	
	vec3 viewVector = normalize(cameraPlayerPosition - worldPos);
	float finalAlpha = finalColor.a;
	vec3 finalColorChangeable = finalColor.rgb;
	float dist = abs(length(cameraPlayerPosition - worldPos));

	if(floatCompare(brickMaterial,1))
	{
		color.r = finalColorChangeable.r + 0.3 * (sin((deltaT/15.0)+worldPos.x*8.0) * cos(deltaT/20.0));
		color.g = finalColorChangeable.g + 0.3 * (cos(worldPos.z*8.0) * sin(deltaT/20.0));
		color.b = finalColorChangeable.b + 0.3 * (cos((deltaT/30.0)+worldPos.y*8.0) * sin(deltaT/20.0));
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
	
	vec3 albedo = pow(finalColorChangeable.rgb,vec3(1.0 + 1.2 * 1.0)); 
	
	float howSideways = clamp(1.0 - abs(dot(normal,vec3(0.0,1.0,0.0))),0.35,1.0);
	
	if(dist < howSideways * materialCutoff)
	{
		if(specialBricks)
		{	
			vec2 or = vec2(0,0);
			if(floatCompare(textureToUse,0))
			{
				or = texture(topMohrTexture,uvs).ga;
				newNormal = getNormalFromMapTop(topNormalTexture,uvs);
			}
			else if(floatCompare(textureToUse,1))
			{
				or = texture(sideMohrTexture,uvs).ga;
				newNormal = getNormalFromMap(sideNormalTexture,uvs);
			}
			else if(floatCompare(textureToUse,2))
			{
				or = texture(bottomMohrTexture,uvs).ga;
				newNormal = getNormalFromMap(bottomNormalTexture,uvs);
			}
			else if(floatCompare(textureToUse,3))
			{
				or = texture(rampMohrTexture,uvs).ga;
				newNormal = getNormalFromMap(rampNormalTexture,uvs);
				or.y = sqrt(or.y);
			}
			else if(isPrint)//Special print brick or just no texture
			{
				or = texture(printTexture,uvs*0.1).ga;
				newNormal = getNormalFromMap(printTextureSpecialBrickNormal,uvs*0.1);
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
			//Used for calculating universal roughness UVs
			float roughZoom = 30.0;
			vec3 rotPos = modelPos;
			rotPos = abs(mod(rotPos,roughZoom)) / roughZoom;
			rotPos.xz = vec2(rotPos.x * swapDimensions.x + (1.0-swapDimensions.x) * rotPos.z,rotPos.z * swapDimensions.x + (1.0-swapDimensions.x) * rotPos.x);
			vec2 roughUVs;
			
			//Used for calculating normal and AO UVs on the sides of bricks
			vec2 edgeSize = vec2(0.05);
			
			//For both normal map and AO map side face UVs as well as for universal roughness map UVs
			if(direction == 0 || direction == 1)	//top and bottom faces
			{
				roughUVs = rotPos.xz;
				
				//Repeat stud texture across tops and bottoms of bricks:
				/*if(!specialBricks)
					uvs *= dimension.xz;*/
			}
			else if(direction == 2 || direction == 3)	//north and south faces
			{
				roughUVs = rotPos.xy;
				
				edgeSize /= dimension.xy;
			}
			else									//east and west faces
			{
				roughUVs = rotPos.zy;
				
				edgeSize /= dimension.zy;
			}
			
			//Calulcate normal map and AO map UVs for the sides of bricks
			if(!specialBricks && direction > 1)
			{
				float edgeWidth = 0.0273; //How many pixels is the border / how many pixels total for the various textures (56/2048 in this case)
				edgeSize.y *= 3.0;
				
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
				if(floatCompare(brickMaterial,8))
				{
					roughness = 0;
					newNormal = normal;
					occlusion = 1;
				}
				else
				{
					vec2 gradx = dFdx(uv);
					vec2 grady = dFdy(uv);
					roughness = texture(mohrTexture,roughUVs).a;
					newNormal = getNormalFromMapGrad(normalTexture,uvs,gradx,grady);
					occlusion = textureGrad(mohrTexture,uvs,gradx,grady).g;	
				}
			}
		}
		
		if(floatCompare(brickMaterial,6))
		{
			finalAlpha = abs(sin(uv.x*8.0 + deltaT/20.0));
			if(finalAlpha < 0.01)
				discard;
		}
	
		if(floatCompare(brickMaterial,9))
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
	
		
		if(floatCompare(brickMaterial,2))
			metalness = 0.5;
		if(floatCompare(brickMaterial,3))
			metalness = 1.0;
		if(floatCompare(brickMaterial,5))
		{
			albedo *= (0.5 + ((1.0+sin(deltaT/15.0)) * 0.5));
		}
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
	
	vec3 halfVector = normalize(viewVector + sunDirection);     
	float NdotV = max(dot(newNormal, viewVector), 0.0);	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);
    //vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
	vec3 F    = fresnelSchlick(max(dot(halfVector, viewVector), 0.0), F0);
	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;
	
	color = vec4(0,0,0,1);
	
	//Point light
	
	for(int i = 0; i<8; i++)
	{
		if(!pointLightUsed[i])
			continue;
			
		float distance    = length(pointLightPos[i] - worldPos);
			
		if(distance > 200.0)
			continue;
			
		vec3 lightDirection = normalize(pointLightPos[i] - worldPos);
		float thetaStrength = 1.0;
			
		if(pointLightIsSpotlight[i])
		{
			vec3 spotLightDirection = pointLightDirection[i].xyz;
			float phi = pointLightDirection[i].w;
			float theta = dot(lightDirection,normalize(-spotLightDirection));
			if(theta < phi)
				continue;		   //phi = 0.5, theta = 0.5, diff = 0.0, div = 0.0
			if(theta - 0.25 <= phi) //phi = 0.5, theta = 0.35, diff = 0.15, div = 1.0
				thetaStrength = (phi-theta) / -0.25;
		}
	
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
	//End point light	
	
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
	
	//Tone maping
	color.rgb = color.rgb / (color.rgb + vec3(1.0));
	//Gamma correction
	color.rgb = pow(color.rgb, vec3(1.0/2.2)); 
	color.a = 1.0;
	
	float fogDist = clamp(dist,500,1000);
	fogDist -= 500.0;
	fogDist /= 500.0;
	color.rgb = mix(color.rgb,fogColor,fogDist);
	
	color.a = finalColor.a;
}
