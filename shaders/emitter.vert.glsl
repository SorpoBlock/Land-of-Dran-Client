#version 330 core

layout(location = 0) in vec2 vertexPosition;
//From particle:
layout(location = 1) in vec4 startingPosAndTime;
layout(location = 2) in vec4 startingVelAndLifeRandomness;

//Camera:
uniform vec3 cameraPlayerRight;
uniform vec3 cameraPlayerUp;
uniform vec3 cameraPlayerPosition;
uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;

//From particleType:
uniform float lifetimeMS;
uniform vec3 drag;
uniform vec3 gravity;
uniform float spinSpeed;
uniform float sizes[4];
uniform vec4 colors[4];
uniform float times[4];
uniform bool useInvAlpha;
uniform bool calculateMovement;

uniform float currentTimeMS;

flat out int frameOne;
flat out int frameTwo;
out float frameInterpolate;
out vec2 uvs;

mat4 spinVerts(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
	
	return mat4(c,-s,0.0,0.0,
				s,c,0.0,0.0,
				0.0,0.0,oc+c,0.0,
				0.0,0.0,0.0,1.0);
}

void main()
{
	if(startingPosAndTime.w <= 0.0)
	{
		gl_Position = vec4(0,0,0,0);
		return;
	}
	
	uvs = vertexPosition + 0.5;

	float lifetimeVariance = startingVelAndLifeRandomness.w;
	float particleCreationTime = startingPosAndTime.w;
	float particleAge = (currentTimeMS - particleCreationTime);
	float lifeProgress = clamp(particleAge / (lifetimeMS+lifetimeVariance),0,1);
	
	//Avoid branching at any cost hur dur ahh
	frameOne = 0;									//0 and 1
	frameOne += lifeProgress > times[1] ? 1 : 0;	//1 and 2
	frameOne += lifeProgress > times[2] ? 1 : 0;	//2 and 3
	frameTwo = frameOne+1;
	frameInterpolate = (lifeProgress - times[frameOne]) / (times[frameTwo] - times[frameOne]);
	
	particleAge *= 0.001;
	vec2 spunVerts = (spinVerts(spinSpeed * particleAge * 6.28) * vec4(vertexPosition,0,1)).xy;
	vec3 particlePosition = vec3(0,0,0);
	
	if(calculateMovement)
	{
		vec3 startingVel = startingVelAndLifeRandomness.xyz;
		vec3 fixedDrag = drag;
		/*fixedDrag.x += (fixedDrag.x >= 0.0 ? 0.001 : -0.001);	//Prevent devide by zero
		fixedDrag.y += (fixedDrag.y >= 0.0 ? 0.001 : -0.001);	//Prevent devide by zero
		fixedDrag.z += (fixedDrag.z >= 0.0 ? 0.001 : -0.001);	//Prevent devide by zero*/
		particlePosition =  exp(-fixedDrag*particleAge);		//This equation is basically from wolfram alpha, I'm not sure I totally understand it, but it kinda works
		particlePosition *= gravity * exp(fixedDrag*particleAge) * (fixedDrag * particleAge - 1.0) + gravity + fixedDrag*exp(fixedDrag*particleAge) * (startingPosAndTime.xyz*fixedDrag + startingVel) - fixedDrag*startingVel;
		particlePosition /= fixedDrag * fixedDrag;
	}
	else
		particlePosition = startingPosAndTime.xyz;

	float size = mix(sizes[frameOne],sizes[frameTwo],frameInterpolate);

	vec3 endPos = particlePosition.xyz;
	endPos += cameraPlayerRight * spunVerts.x * size + cameraPlayerUp * spunVerts.y * size;

	vec4 finalPos = projectionPlayerMatrix * viewPlayerMatrix * vec4(endPos,1.0);
	if(finalPos.z < 1.0)
		gl_Position = vec4(0,0,0,0);
	else
		gl_Position = finalPos;
}