#version 330 core
  
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
    
uniform mat4 lightSpaceMatricies[3];

in vec4 trans[];
out vec4 geoTrans;
     
void main()
{      
	for (int invoc = 0; invoc<3; ++invoc)
	{
    
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = lightSpaceMatricies[invoc] * gl_in[i].gl_Position;
        gl_Layer = invoc;
		geoTrans = trans[i];
        EmitVertex();
    }
    EndPrimitive();
	
	}
}  