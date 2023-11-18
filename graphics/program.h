#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include "code/graphics/shader.h"
#include <glm/glm.hpp>

namespace syj
{
    void glUniformMat(GLint location,glm::mat4 mat);
    void glUniform3vec(GLint location,glm::vec3 vec);
    void glUniform4vec(GLint location,glm::vec4 vec);

    enum brickShaderTexture { topNormal=0,topMohr=1,sideNormal=2,sideMohr=4,bottomNormal=6,bottomMohr=7,rampNormal=14,rampMohr=15,printTexture=16,printTextureSpecialBrickNormal=11,doNotUseBrickTexture=999};

    enum textureLocations
    {
        albedo = 0,
        normal = 1,
        mohr   = 2,
        brdf   = 3,
        heightMap = 4,
        shadowNearMap = 5,
        refraction = 6,
        reflection = 7,
        shadowFarMap = 8,
        shadowColorMap = 9,
        shadowNearTransMap = 10,
        cubeMapEnvironment = 11,
        cubeMapRadiance = 12,
        cubeMapIrradiance = 13
    };

    enum layout
    {
        positions               = 0,
        normals                 = 1,
        uvs                     = 2,
        colors                  = 3,
        tangents                = 4,
        bitangents              = 5,
        boneIDs                 = 6,
        boneWeightsOrSprites    = 7,
        index                   = 8
    };

    enum uniformType {isMat = 0,isFloat = 1,isInt = 2,isVec3 = 3};

    class program
    {
        private:
            bool _isCompiled = false;
            shader *vertex = 0;
            shader *fragment = 0;
            shader *tessEval = 0;
            shader *tessControl = 0;
            shader *geometry = 0;

            GLuint handle;

            std::vector<GLint>       uniforms;
            std::vector<bool>        useDefaults;
            std::vector<uniformType> defaultTypes;
            std::vector<glm::mat4>   defaultMats;
            std::vector<float>       defaultFloats;
            std::vector<int>         defaultInts;
            std::vector<glm::vec3>   defaultVec3s;

        public:
            GLint registerUniformMat  (std::string name,bool useDefault = false,glm::mat4 defaul = glm::mat4(1.0));
            GLint registerUniformInt  (std::string name,bool useDefault = false,int   defaul = 0);
            GLint registerUniformBool (std::string name,bool useDefault = false,bool  defaul = false);
            GLint registerUniformFloat(std::string name,bool useDefault = false,float defaul = 0);
            GLint registerUniformVec3 (std::string name,bool useDefault = false,glm::vec3 defaul = glm::vec3(0,0,0));

            void resetUniforms();
            void bindShader(shader *toBind);
            void compile();
            bool isCompiled() { return _isCompiled; }
            void use(bool reset = true);
            GLint getUniformLocation(std::string name);
    };

    void glUniformMatArray(std::string location,program *prog,glm::mat4 *array,int size);
}

#endif // PROGRAM_H_INCLUDED
