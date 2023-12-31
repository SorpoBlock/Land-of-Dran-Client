#ifndef BLOCKLANDBRICKTYPE_H_INCLUDED
#define BLOCKLANDBRICKTYPE_H_INCLUDED

#include "code/networking/common.h"
#include "code/graphics/renderContext.h"
#include "code/graphics/uniformsBasic.h"
#include "code/physics/bulletIncludes.h"
#include "code/graphics/brickStructure.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace syj
{
    //Holds up to 7 special faces and some collision data
    struct specialBrickType
    {
        unsigned int serverID = 0;

        bool hasTransparency = false;

        //The name of the file we loaded it from
        //Unfortunately Blockland uses the ui name in saves, and that's nowhere in the bricktype files we are loading:
        std::string fileName = "";
        std::string uiName = "";
        bool hasPrint = false;

        //Render data:
        unsigned int vertexCount = 0;
        //The 8 buffers from perTextureBuffers, but I don't think dimensionBuffer and directionBuffer would ever really be used in the shader:
        GLuint buffers[9]={0,0,0,0,0,0,0,0,0};
        void initFromVectors(
                             std::vector<glm::vec3> &vertPositions,
                             std::vector<glm::vec3> &normals,
                             std::vector<glm::vec3> &uvsAndTex,
                             std::vector<glm::vec4> &vertColors);

        //Collision data:
        bool isModTerrain = false;
        btCollisionShape *shape = 0;
        int width,height,length;

        void initModTerrain(std::vector<glm::vec3> &verts);

        specialBrickType(std::string blbFile,bool customMesh);
        ~specialBrickType();

        specialBrickType(const specialBrickType&) = delete; //Disable copy constructor
    };
}

#endif // BLOCKLANDBRICKTYPE_H_INCLUDED
