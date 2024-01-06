#version 330 core

layout(location = 0) in vec3  positions;
layout(location = 1) in vec2  uvs;
layout(location = 2) in vec3  normals;
layout(location = 3) in vec3  tangents;
layout(location = 4) in vec3  bitangents;
//Buffer 5 is index
layout(location = 6) in int perMeshFlags;
layout(location = 7) in vec3  perMeshColor;
layout(location = 8) in mat4 perMeshTransform;
//Slots 8,9, and 10 are also taken up by perMeshTransform

//Camera details:
uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;
uniform vec3 cameraPlayerPosition;

//For water rendering:
uniform float clipHeight;

//Shadows:
uniform mat4 lightSpaceMatricies[3];

//Basic transformations:
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform mat4 scaleMatrix;
uniform mat4 modelMatrix;

uniform bool bottomLand;

//Fragment shader inputs:
out vec2 uv;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec3 worldPos;
out vec4 shadowPos[3];
out vec3 tanFragPos;
out vec3 tanViewPos;
out vec3 nodeColor;

flat out int usePickingColor;
flat out int pickingColor;

void main()
{
	if(bottomLand)
	{
		worldPos = vec3(cameraPlayerPosition.x+positions.x*500.0,0,cameraPlayerPosition.z+positions.y*500.0);
		normal = vec3(0,1,0);
		tangent = vec3(0,0,1);
		bitangent = vec3(1,0,0);
		uv = positions.xy;
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(worldPos,1);
		if(clipHeight > 0.0)
			gl_ClipDistance[0] = worldPos.y - clipHeight;
		else
			gl_ClipDistance[0] = abs(clipHeight) - worldPos.y; 
		return;
	}

	//Instances of instanced meshes that we want hidden will have perMeshTransform set to glm::mat4(nan(""))
	if(isnan(perMeshTransform[0][0]))
	{
		gl_Position = vec4(perMeshTransform[0][0]);
		return;
	}
		

	usePickingColor = perMeshFlags & 256;
	if(usePickingColor > 0)
		usePickingColor = 1;
	pickingColor = perMeshFlags & 255;

	uv = uvs;
	nodeColor = perMeshColor;
	
	vec4 endPos = perMeshTransform * vec4(positions,1.0);
	normal =     normalize(rotateMatrix * perMeshTransform * vec4(normals,0)).xyz;
	tangent =    normalize(rotateMatrix * perMeshTransform * vec4(tangents,0)).xyz;	
	bitangent = normalize(rotateMatrix * perMeshTransform * vec4(bitangents,0)).xyz;
	mat3 TBN = transpose(mat3(tangent,bitangent,normal));

	worldPos = (translateMatrix * rotateMatrix * scaleMatrix * endPos).xyz;
	
	tanFragPos = TBN * worldPos;
	tanViewPos = TBN * cameraPlayerPosition;
	
	for(int i = 0; i<3; i++)
		shadowPos[i] = lightSpaceMatricies[i] * vec4(worldPos,1.0);
		
	if((perMeshFlags & 512) == 512)
		gl_Position = modelMatrix * endPos;
	else
	{
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(worldPos,1.0);
		if(clipHeight > 0.0)
			gl_ClipDistance[0] = worldPos.y - clipHeight;
		else
			gl_ClipDistance[0] = abs(clipHeight) - worldPos.y; 
	}
}
