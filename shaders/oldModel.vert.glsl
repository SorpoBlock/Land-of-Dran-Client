#version 330 core

//new

uniform vec3 cameraPlayerPosition;
uniform mat4 anglePlayerMatrix;
uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;
uniform mat4 lightSpaceMatricies[3];
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform mat4 scaleMatrix;
uniform mat4 modelMatrix;
uniform mat4 additionalMatrix;
uniform bool previewTexture;
uniform mat4 boneTransforms[100];
uniform int isAnimated;
uniform bool renderingSky;
uniform bool drawingDebugLocations;
uniform vec3 debugLoc;
uniform bool skipCamera;
uniform vec3 nodeColorOld;

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
out vec4 shadowPos[3];
out vec3 tanFragPos;
out vec3 tanViewPos;
out vec3 skyDir;
out vec3 nodeColor;

uniform float clipHeight;

void main()
{
	nodeColor = nodeColorOld;
	uv = uvs;
	
	if(drawingDebugLocations)
	{
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(debugLoc + positions,1.0);
		return;
	}
	
	if(renderingSky)
	{
		gl_Position = projectionPlayerMatrix * anglePlayerMatrix * vec4(positions,1);
		gl_Position.z = gl_Position.w;
		skyDir = normalize(positions * vec3(1,-1,1));
		return;
	}
	
	if(previewTexture)
	{
		gl_Position = vec4(positions*vec3(0.2)+vec3(-0.78,0.78,0),1.0); //vec4(positions*0.5+vec3(-0.5,1,0),1.0);
		return;
	}

	vec4 endPos = vec4(positions,1.0);
	//mat3 TBN = transpose(mat3(tangents,bitangents,normals));
	normal =     normalize(rotateMatrix * vec4(normals,0)).xyz;
	tangent =    normalize(rotateMatrix * vec4(tangents,0)).xyz;	
	bitangent = normalize(rotateMatrix * vec4(bitangents,0)).xyz;
	mat3 TBN = transpose(mat3(tangent,bitangent,normal));

	worldPos = (translateMatrix * rotateMatrix * scaleMatrix * endPos).xyz;
	
	tanFragPos = TBN * worldPos;
	tanViewPos = TBN * cameraPlayerPosition;
	
	for(int i = 0; i<3; i++)
		shadowPos[i] = lightSpaceMatricies[i] * vec4(worldPos,1.0);
		
	if(skipCamera)
	{
		gl_Position = modelMatrix * endPos;
	}
	else
	{
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(worldPos,1.0);
		if(clipHeight > 0.0)
			gl_ClipDistance[0] = worldPos.y - clipHeight;
		else
			gl_ClipDistance[0] = abs(clipHeight) - worldPos.y; 
	}
}
 
 
 