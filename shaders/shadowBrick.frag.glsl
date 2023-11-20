#version 330 core

uniform bool useAlbedo;
uniform sampler2D albedoTexture;
uniform bool renderingSun;
uniform bool doingGodRayPass;

in vec2 uv;
in vec4 geoTrans;
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
		
	if(!doingGodRayPass)
	{
		if(geoTrans.a < 0.05)
			discard;
		if(geoTrans.a > 0.98)
			color = vec4(0,0,0,1);
		else
			color = vec4(geoTrans.rgb,1);
	}
	else
		color = vec4(0,0,0,1);
}
