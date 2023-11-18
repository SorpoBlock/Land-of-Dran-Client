#ifndef MATERIAL_H_INCLUDED
#define MATERIAL_H_INCLUDED

#include "code/graphics/texture.h"
#include "code/graphics/uniformsBasic.h"
#include <assimp/scene.h>
#include "code/utility/preference.h"

namespace syj
{
    class material
    {
        public:
            texture textures[3];
            bool albedoUsed         = false;
            bool normalUsed         = false;
            bool metalnessUsed      = false;
            bool roughnessUsed      = false;
            bool displacementUsed   = false;
            bool occlusionUsed      = false;

            std::string tryFile(std::string type,std::string name,preferenceFile &settings);
            void create(std::string name,preferenceFile &settings);

        public:
            void use(uniformsHolder *uniforms);
            void useManuelOffset(brickShaderTexture albedoOffset,brickShaderTexture normalOffset,brickShaderTexture mohrOffset);
            material(std::string name,preferenceFile &settings);
            material(std::string filePath);

            material(){};
            void addOnlyAlbedo(std::string filename);

            ~material();
    };
}

#endif // MATERIAL_H_INCLUDED
