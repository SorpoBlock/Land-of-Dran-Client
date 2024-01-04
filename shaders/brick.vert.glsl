#version 330 core

layout(location = 0) in vec3 vertexPosition; //only x and y are set for basic bricks, special bricks use all 3 dims
layout(location = 1) in vec4 brickPositionAndMat;
layout(location = 2) in vec4 brickRotation;
layout(location = 3) in vec4 paintColorIn;
layout(location = 4) in uvec4 brickDimensions;
layout(location = 5) in int  faceDirection; //0=up,1=down,2=north,3=south,4=east,5=west
layout(location = 6) in vec3 customNormal;
layout(location = 7) in vec3 customUVandTex;
layout(location = 8) in vec4 customVertexColor;

uniform vec3 cameraPlayerPosition;
uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;
uniform mat4 lightSpaceMatricies[3];

uniform bool livingBricks;
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform float deltaT;
uniform float clipHeight;
uniform bool doingGeomShadows;
uniform bool specialBricks;

out mat3 TBN;
out vec3 tanViewPos;
out vec3 tanWorldPos;
out vec3 modelPos;
out vec3 worldPos;
out vec3 normal;
out vec2 uv;
out vec2 roughUVs;
out vec2 edgeSize;
out vec2 verts;
out vec4 finalColor;
flat out vec3 dimension;
flat out int direction;
out vec4 shadowPos[3];
flat out int brickMaterial;
flat out int textureToUse;

mat4 makeRotateMatrix(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(                       c         ,   0							     ,                                  s,  0.0,
                0						  		 ,	oc                   + c         ,  0								 ,  0.0,
                                               -s,  0								 ,                         c         ,  0.0,
                0.0,                                0.0,                                0.0,                                1.0);
				
}

bool floatCompare(float a,float b)
{
	return abs(a-b) < 0.05;
}

void main()
{
	normal = vec3(0,1,0);

	vec3 brickSide = vec3(0,0,0);
	brickMaterial = int(brickPositionAndMat.w);
	
	if(specialBricks)
	{
		brickSide = vertexPosition;
		textureToUse = int(customUVandTex.z);
		uv = customUVandTex.xy;
		normal = customNormal;
	}
	else
	{
		vec3 signedDims = brickDimensions.xyz;
		if(faceDirection == 0) //top
		{
			if((brickDimensions.w & uint(1)) != uint(0))
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy) * brickDimensions.xz;// * brickDimensions.xz;
			verts = (0.5 + vertexPosition.xy);
			verts.y = 1.0 - verts.y;
			brickSide = vec3(vertexPosition.x * signedDims.x,signedDims.y / 2.0,-vertexPosition.y * signedDims.z);
			normal = vec3(0,1,0);
		}
		else if(faceDirection == 1) //bottom
		{
			if((brickDimensions.w & uint(2)) != uint(0))
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy) * brickDimensions.xz;// * brickDimensions.xz;
			verts = (0.5 + vertexPosition.xy);
			verts.y = 1.0 - verts.y;
			brickSide = vec3(vertexPosition.x * signedDims.x,-signedDims.y / 2.0,vertexPosition.y * signedDims.z);
			normal = vec3(0,-1,0);
		}
		else if(faceDirection == 2) //north
		{
		
			if((brickDimensions.w & uint(4)) != uint(0))
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.xy;
			verts = (0.5 + vertexPosition.xy);
			verts.y = 1.0 - verts.y;
			brickSide = vec3(-vertexPosition.x * signedDims.x,-vertexPosition.y * signedDims.y,signedDims.z / 2.0);
			normal = vec3(0,0,1);
		}
		else if(faceDirection == 3) //south
		{
			if((brickDimensions.w & uint(8)) != uint(0))
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.xy;
			verts = (0.5 + vertexPosition.xy);
			verts.y = 1.0 - verts.y;
			brickSide = vec3(-vertexPosition.x * signedDims.x,vertexPosition.y * signedDims.y,-signedDims.z / 2.0);
			normal = vec3(0,0,-1);
		}
		else if(faceDirection == 4) //east
		{
			if((brickDimensions.w & uint(16)) != uint(0))
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.zy;
			verts = uv;
			verts.y = 1.0 - verts.y;
			brickSide = vec3(signedDims.x / 2.0,-vertexPosition.y * signedDims.y,vertexPosition.x * signedDims.z);
			normal = vec3(1,0,0);
		}
		else if(faceDirection == 5) //west
		{
			if((brickDimensions.w & uint(32)) != uint(0))
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.zy;
			verts = uv;
			verts.y = 1.0 - verts.y;
			brickSide = vec3(-signedDims.x / 2.0,vertexPosition.y * signedDims.y,vertexPosition.x * signedDims.z);
			normal = vec3(-1,0,0);
		}
		brickSide.y /= 2.5;
	}
	
	direction = faceDirection;
	dimension = brickDimensions.xyz;
	
	if(paintColorIn.a < 0.01)
	{
		gl_Position = vec4(0,0,0,0);
		return;
	}
		
	if(customVertexColor.a < 0.01)
		finalColor = paintColorIn;
	else
		finalColor = customVertexColor;
		
	mat4 rot = makeRotateMatrix(brickRotation.w);
	
	vec2 swapDimensions = (rot * vec4(1,0,0,0)).xz;
	swapDimensions = round(swapDimensions);
	
	normal = (rot * vec4(normal,0)).xyz;
	worldPos = brickPositionAndMat.xyz + (rot * vec4(brickSide,1.0)).xyz;
	
	vec3 distort = vec3(0,0,0);
	if(brickMaterial >= 2000)
	{
		brickMaterial -= 2000;
		distort = vec3(0,abs(max(0,brickSide.y)*(1.0+sin(deltaT/5.0))*2),0);
		distort *= vec3(0,0.3,0);
		worldPos += distort;
	}
	else if(brickMaterial >= 1000)
	{
		brickMaterial -= 1000;
		distort = vec3(
							sin(worldPos.x) * sin(deltaT/7.0),
							cos(worldPos.y) * cos(deltaT/7.0),
							sin(worldPos.z) * cos(deltaT/7.0)
							);
		worldPos += distort;
	}
		
	modelPos = worldPos;
	
	if(livingBricks)
	{
		worldPos = (translateMatrix * rotateMatrix * vec4(worldPos,1.0)).xyz;
		normal = (rotateMatrix * vec4(normal,0.0)).xyz;
	}
	
	if(clipHeight > 0.0)
		gl_ClipDistance[0] = worldPos.y - clipHeight;
	else
		gl_ClipDistance[0] = abs(clipHeight) - worldPos.y; 
			
	//Used for calculating universal roughness UVs
	float roughZoom = 10.0;
	vec3 rotPos = modelPos;
	//rotPos = abs(mod(rotPos,roughZoom)) / roughZoom;
	rotPos.xz = vec2(rotPos.x * swapDimensions.x + (1.0-swapDimensions.x) * rotPos.z,rotPos.z * swapDimensions.x + (1.0-swapDimensions.x) * rotPos.x);
	
	//Used for calculating normal and AO UVs on the sides of bricks
	edgeSize = vec2(0.05);
	
	//For both normal map and AO map side face UVs as well as for universal roughness map UVs
	if(direction == 0 || direction == 1)	//top and bottom faces
	{
		roughUVs = rotPos.xz;
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
	
	roughUVs /= roughZoom;
	
	edgeSize *= 3.0;
	
	vec3 tangent = cross(normal,vec3(0,1,0));
	vec3 bitangent = normalize(cross(normal,tangent));
	tangent = normalize(cross(normal,bitangent));
	
    TBN = transpose(mat3(tangent,bitangent,normal));
	tanViewPos = TBN * cameraPlayerPosition;
	tanWorldPos = TBN * worldPos;
	
	if(doingGeomShadows)
	{
		gl_Position = vec4(worldPos,1.0);
	}
	else
	{
		gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(worldPos,1.0);
		for(int i = 0; i<3; i++)
			shadowPos[i] = lightSpaceMatricies[i] * vec4(worldPos,1.0);
		
		if(brickMaterial == 1)
		{
			gl_Position.z -= 0.01;
		}
	}
} 

