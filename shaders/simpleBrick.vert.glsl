#version 330 core

layout(location = 0) in vec3 vertexPosition; //only x and y are set, z = 0 so why is it here?
layout(location = 1) in vec4 brickPositionAndMat;
layout(location = 2) in vec4 brickRotation;
layout(location = 3) in vec4 paintColorIn;
layout(location = 4) in uvec4 brickDimensions;
layout(location = 5) in int  faceDirection; //0=up,1=down,2=north,3=south,4=east,5=west
layout(location = 6) in vec3 customNormal;
layout(location = 7) in vec3 customUVandTex;
layout(location = 8) in vec4 customVertexColor;

uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;

uniform bool specialBricks;
uniform bool livingBricks;
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform float deltaT;
uniform float clipHeight;

out vec3 modelPos;
out vec3 worldPos;
out vec3 normal;
out vec2 uv;
out vec2 verts;
out vec4 finalColor;
flat out vec2 swapDimensions;
flat out vec3 dimension;
flat out int direction;
out vec4 shadowPos[3];
out float brickMaterial;
out float textureToUse;

mat4 makeRotateMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec2 rotateVec2(vec2 ina,float angle)
{
	float c = cos(angle);
	float s = sin(angle);
	return vec2(ina.x * c - ina.y * s,ina.x * s + ina.y * c);
}

bool floatCompare(float a,float b)
{
	return abs(a-b) < 0.05;
}

void main()
{
	vec3 brickSide = vec3(0,0,0);
	brickMaterial = brickPositionAndMat.w;
	
	if(!specialBricks)
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
	else
	{
		brickSide = vertexPosition;
		textureToUse = customUVandTex.z;
		uv = customUVandTex.xy;
		normal = customNormal;
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
		
	mat4 rot = makeRotateMatrix(brickRotation.xyz,brickRotation.w);
	swapDimensions = (rot * vec4(1,0,0,0)).xz;
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
	
	if(livingBricks)
	{
		modelPos = worldPos;
		worldPos = (translateMatrix * rotateMatrix * vec4(worldPos,1.0)).xyz;
		normal = (rotateMatrix * vec4(normal,0.0)).xyz;
	}
	else
		modelPos = worldPos;
	
	if(clipHeight > 0.0)
		gl_ClipDistance[0] = worldPos.y - clipHeight;
	else
		gl_ClipDistance[0] = abs(clipHeight) - worldPos.y; 
	
	gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(worldPos,1.0);
	
	if(floatCompare(brickMaterial,1))
	{
		gl_Position.z -= 0.01;
	}
} 