#version 330 core

uniform bool useAlbedo;
uniform sampler2D albedoTexture;
uniform bool renderingSun;
uniform bool doingGodRayPass;

in vec2 uv;
out vec4 color;

void main()
{			
	/*if(useAlbedo)
	{
		vec4 albedo_ = texture(albedoTexture,uv);
		if(albedo_.a < 0.5)
			discard;
	}*/
	
	if(renderingSun)
	{
		color = vec4(1,1,1,1);
		return;
	}
		
	color = vec4(0,0,0,1);
}
