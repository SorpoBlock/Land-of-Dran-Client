//https://learnopengl.com/In-Practice/Text-Rendering
#version 330 core
in vec2 uv;
out vec4 color;

uniform sampler2D albedo;
uniform vec3 textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(albedo, uv).r);
	if(sampled.a < 0.02)
		discard;
    color = vec4(textColor, 1.0) * sampled;
}