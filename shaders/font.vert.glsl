//https://learnopengl.com/In-Practice/Text-Rendering
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 uv;

uniform mat4 projectionPlayerMatrix;
uniform mat4 viewPlayerMatrix;
uniform vec3 cameraPlayerDirection;
uniform vec3 cameraPlayerRight;
uniform vec3 cameraPlayerUp;
uniform vec3 textOffset;

void main()
{
	vec3 vertPosition = vec3(0,0,0);
	vertPosition += vertex.x * cameraPlayerRight;
	vertPosition += vertex.y * cameraPlayerUp;
	//vertPosition += vertex.z * cameraPlayerDirection;
	vertPosition += textOffset;
	
    gl_Position = projectionPlayerMatrix * viewPlayerMatrix * vec4(vertPosition, 1.0);
    uv = vertex.zw;
}  