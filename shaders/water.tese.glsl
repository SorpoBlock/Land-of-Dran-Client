#version 330 core
#extension GL_ARB_tessellation_shader : require

uniform float deltaT;

uniform mat4 viewPlayerMatrix;
uniform mat4 projectionPlayerMatrix;

uniform mat4 scaleMatrix;
uniform mat4 translateMatrix;
uniform mat4 rotateMatrix;

uniform sampler2D heightMapTexture;
uniform vec3 tessellationScale;
uniform float waterLevel;

float getHeight(vec2 worldPosition)
{
	vec2 uv = worldPosition / tessellationScale.xz;
	vec3 texColor = texture(heightMapTexture,uv).rgb;
	return ((texColor.r / 256.0) + texColor.g) * tessellationScale.y - 300;
}

float getHeight(float x,float z)
{
	return getHeight(vec2(x,z));
}


layout(quads, equal_spacing, cw) in;

in vec3 ES_Position[];

out vec2 UV;
out vec3 FS_Position;
out vec4 clipSpace;

void main() 
{
	FS_Position.xz = mix(ES_Position[0].xz,ES_Position[2].xz,gl_TessCoord.xy);
	FS_Position.y = ES_Position[0].y + waterLevel;
	//float waterDepth = abs(300-getHeight(FS_Position.xz));
	//float waterDepthScale = (clamp(waterDepth,10,50) - 10.0) / 40.0;
	//FS_Position.y -= 1 * (sin(FS_Position.x/10.0-deltaT) + sin(FS_Position.z/10.0-deltaT));

	FS_Position = vec3((translateMatrix * rotateMatrix * scaleMatrix * vec4(FS_Position,1)).xyz);

	UV = FS_Position.xz / 128.0;

	gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(FS_Position,1.0);
	clipSpace = gl_Position;
}





