#version 330 core

//new

uniform mat4 viewShadowMatrix;
uniform mat4 angleShadowMatrix;
uniform mat4 projectionShadowMatrix;
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform mat4 scaleMatrix;
uniform bool renderingSun;
uniform vec3 sunDirection;
uniform mat4 boneTransforms[100];
uniform int isAnimated;
uniform bool doingGodRayPass;

layout(location = 0) in vec3  positions;
layout(location = 1) in vec3  normals;
layout(location = 2) in vec2  uvs;
layout(location = 3) in vec4  colors;
layout(location = 4) in vec3  tangents;
layout(location = 5) in vec3  bitangents;
layout(location = 6) in ivec4 boneIDs;
layout(location = 7) in vec4  boneWeights;

out vec2 uv;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec3 worldPos;
out vec4 shadowPos;
out vec4 trans;

void main()
{
	//trans = vec4(0.0,0.0,0.0,1.0);
	trans = colors;
	
	if(renderingSun)
	{
		gl_Position = projectionShadowMatrix * angleShadowMatrix * vec4(positions + normalize(sunDirection) * 20.0,1.0);
		return;
	}
	
	if(isAnimated == 1)
	{
		int numWeights = 0;
		vec4 totalPosition = vec4(0.0);
		vec4 totalNormal = vec4(0.0);
		vec4 totalTangent = vec4(0.0);
		vec4 totalBitangent = vec4(0.0);
		for(int i = 0 ; i < 4 ; i++)
		{
			if(boneIDs[i] == -1) 
			{
				continue;
			}
			numWeights++;
			if(boneIDs[i] >= 50) 
			{
				//boneWeightsDebug = vec4(1,1,1,1);
				totalPosition = vec4(positions,1.0f);
				break;
			} 
			vec4 localPosition = boneTransforms[boneIDs[i]] * vec4(positions,1.0f);
			vec4 localNormal = boneTransforms[boneIDs[i]] * vec4(normals,0.0f);
			vec4 localTangent = boneTransforms[boneIDs[i]] * vec4(tangents,0.0f);
			vec4 localBitangent = boneTransforms[boneIDs[i]] * vec4(bitangents,0.0f);
        	totalPosition += localPosition * boneWeights[i];
			totalNormal += localNormal * boneWeights[i];
			totalTangent += localTangent * boneWeights[i];
			totalBitangent += localBitangent * boneWeights[i];
   		}
		totalPosition = vec4(positions,1.0);
		totalNormal = vec4(normals,1.0);
		totalTangent = vec4(tangents,1.0);
		totalBitangent = vec4(bitangents,1.0);
		if(doingGodRayPass)
			gl_Position = projectionShadowMatrix * viewShadowMatrix * translateMatrix * rotateMatrix * scaleMatrix * totalPosition;
		else
			gl_Position = translateMatrix * rotateMatrix * scaleMatrix * totalPosition;
		return;
	}
	
	vec4 endPos = vec4(positions,1.0);
	worldPos = (translateMatrix * rotateMatrix * scaleMatrix * endPos).xyz;
	
	if(doingGodRayPass)
		gl_Position = projectionShadowMatrix * viewShadowMatrix * vec4(worldPos,1.0);
	else
		gl_Position = vec4(worldPos,1.0);
}
 