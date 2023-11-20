#version 330 core

layout(location = 0) in vec3 vertexPosition; //only x and y are set, z = 0 so why is it here?
layout(location = 1) in vec4 brickPositionAndMat;
layout(location = 2) in vec4 brickRotation;
layout(location = 3) in vec4 paintColorIn;
layout(location = 4) in ivec4 brickDimensions;
layout(location = 5) in int  faceDirection; //0=up,1=down,2=north,3=south,4=east,5=west
layout(location = 6) in vec3 customNormal;
layout(location = 7) in vec2 customUV;
layout(location = 8) in vec4 customVertexColor;

//uniform mat4 viewShadowMatrix;
//uniform mat4 projectionShadowMatrix;
uniform bool specialBricks;

uniform bool livingBricks;
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;
uniform float deltaT;

uniform bool doingGodRayPass;
uniform mat4 viewShadowMatrix;
uniform mat4 projectionShadowMatrix;

out vec3 modelPos;
out vec3 worldPos;
out vec3 normal;
out vec2 uv;
out vec4 finalColor;
out vec2 swapDimensions;
flat out vec3 dimension;
flat out int direction;
out vec4 trans;

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

void main()
{
	float brickMaterial = brickPositionAndMat.w;
	
	vec3 brickSide = vec3(0,0,0);
	
	if(!specialBricks)
	{
		if(faceDirection == 0) //top
		{
			if((brickDimensions.w & 1) != 0)
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.xz;
			brickSide = vec3(vertexPosition.x * brickDimensions.x,brickDimensions.y / 2.0,-vertexPosition.y * brickDimensions.z);
			normal = vec3(0,1,0);
		}
		else if(faceDirection == 1) //bottom
		{
			if((brickDimensions.w & 2) != 0)
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.xz;
			brickSide = vec3(vertexPosition.x * brickDimensions.x,-brickDimensions.y / 2.0,vertexPosition.y * brickDimensions.z);
			normal = vec3(0,-1,0);
		}
		else if(faceDirection == 2) //north
		{
			if((brickDimensions.w & 4) != 0)
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.xy;
			brickSide = vec3(-vertexPosition.x * brickDimensions.x,-vertexPosition.y * brickDimensions.y,brickDimensions.z / 2.0);
			normal = vec3(0,0,1);
		}
		else if(faceDirection == 3) //south
		{
			if((brickDimensions.w & 8) != 0)
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.xy;
			brickSide = vec3(-vertexPosition.x * brickDimensions.x,vertexPosition.y * brickDimensions.y,-brickDimensions.z / 2.0);
			normal = vec3(0,0,-1);
		}
		else if(faceDirection == 4) //east
		{
			if((brickDimensions.w & 16) != 0)
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.zy;
			brickSide = vec3(brickDimensions.x / 2.0,-vertexPosition.y * brickDimensions.y,vertexPosition.x * brickDimensions.z);
			normal = vec3(1,0,0);
		}
		else if(faceDirection == 5) //west
		{
			if((brickDimensions.w & 32) != 0)
			{
				gl_Position = vec4(0,0,0,0);
				return;
			}
			uv = (0.5 + vertexPosition.xy);// * brickDimensions.zy;
			brickSide = vec3(-brickDimensions.x / 2.0,vertexPosition.y * brickDimensions.y,vertexPosition.x * brickDimensions.z);
			normal = vec3(-1,0,0);
		}
		brickSide.y /= 2.5;
	}
	else
	{
		brickSide = vertexPosition;
		uv = customUV;
		normal = customNormal;
	}
	
	direction = faceDirection;
	dimension = brickDimensions.xyz;
	finalColor = vec4(mix(paintColorIn.rgb,customVertexColor.rgb,customVertexColor.a),max(paintColorIn.a,customVertexColor.a));
		
	mat4 rot = makeRotateMatrix(brickRotation.xyz,brickRotation.w);
	swapDimensions = (rot * vec4(1,0,0,0)).xz;
	normal = (rot * vec4(normal,0)).xyz;
	worldPos = brickPositionAndMat.xyz + (rot * vec4(brickSide,1.0)).xyz;
	
	if(brickMaterial >= 2000)
	{
		brickMaterial -= 2000;
		vec3 distort = vec3(0,abs(max(0,brickSide.y)*(1.0+sin(deltaT/5.0))*2),0);
		distort *= vec3(0,0.3,0);
		worldPos += distort;
	}
	else if(brickMaterial >= 1000)
	{
		brickMaterial -= 1000;
		vec3 distort = vec3(
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
	
	if(customVertexColor.a > 0 && customVertexColor.a < 1)
		trans = customVertexColor;
	else
		trans = paintColorIn;
		
	if(doingGodRayPass)
		gl_Position = projectionShadowMatrix * viewShadowMatrix * vec4(worldPos,1.0);
	else
		gl_Position = vec4(worldPos,1.0);
} 