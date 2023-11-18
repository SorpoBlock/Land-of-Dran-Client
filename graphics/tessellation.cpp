#include "tessellation.h"

namespace syj
{
    void tessellation::init()
    {
        glCreateVertexArrays(1,&vao);
        glCreateBuffers(1,&vertexBuffer);

        std::vector<glm::vec3> verts;
        for(unsigned int x = 0; x<prefs.numPatchesX; x++)
        {
            for(unsigned int z = 0; z<prefs.numPatchesZ; z++)
            {
                verts.push_back(glm::vec3(x * prefs.patchSizeX,prefs.yOffset,z * prefs.patchSizeZ));
                verts.push_back(glm::vec3((x+1) * prefs.patchSizeX,prefs.yOffset,z * prefs.patchSizeZ));
                verts.push_back(glm::vec3((x+1) * prefs.patchSizeX,prefs.yOffset,(z+1) * prefs.patchSizeZ));

                verts.push_back(glm::vec3((x+1) * prefs.patchSizeX,prefs.yOffset,(z+1) * prefs.patchSizeZ));
                verts.push_back(glm::vec3(x * prefs.patchSizeX,prefs.yOffset,(z+1) * prefs.patchSizeZ));
                verts.push_back(glm::vec3(x * prefs.patchSizeX,prefs.yOffset,z * prefs.patchSizeZ));
            }
        }

        std::cout<<verts.size() * sizeof(glm::vec3)<<"\n";
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,verts.size() * sizeof(glm::vec3),&verts[0][0],GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
            error("OpenGL Error: " + std::to_string(err));
    }

    tessellation::tessellation(float _yOffset)
    {
        scope("Tessellation::tessellation");
        glGetError();

        prefs.yOffset = _yOffset;
        init();
    }

    tessellation::tessellation(std::string heightMapFilePath)
    {
        scope("Tessellation::tessellation heightMapLoad");
        glGetError();

        std::cout<<"Made a texture.\n";

        heightMapTex = new texture;
        heightMapTex->setWrapping(GL_REPEAT);
        heightMapTex->createFromFile(heightMapFilePath);

        init();
    }

    void tessellation::render(uniformsHolder *tess,bool bindsOnly)
    {
        glm::vec3 scale = glm::vec3(prefs.numPatchesX * prefs.patchSizeX,prefs.heightMapYScale,prefs.numPatchesZ * prefs.patchSizeZ);
        glUniform3vec(tess->tessellationScale,scale);
        if(heightMapTex)
            heightMapTex->bind(heightMap);

        //std::cout<<"Patches: "<<prefs.numPatchesX * prefs.numPatchesZ * 6<<"\n";
        //std::cout<<"Scale: "<<scale.x<<","<<scale.y<<","<<scale.z<<"\n";

        if(!bindsOnly)
        {
            glPatchParameteri(GL_PATCH_VERTICES,3);
            glBindVertexArray(vao);
            //std::cout<<prefs.numPatchesX * prefs.numPatchesZ * 6<<"\n";
            glDrawArrays(GL_PATCHES,0,prefs.numPatchesX * prefs.numPatchesZ * 6);
            glBindVertexArray(0);
        }
    }

    tessellation::~tessellation()
    {
        glDeleteVertexArrays(1,&vao);
        glDeleteBuffers(1,&vertexBuffer);
        delete heightMapTex;
    }

}
