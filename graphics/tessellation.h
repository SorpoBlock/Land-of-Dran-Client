#ifndef TESSELLATION_H_INCLUDED
#define TESSELLATION_H_INCLUDED

#include "code/graphics/uniformsBasic.h"
#include "code/graphics/texture.h"

namespace syj
{
    struct tessellation
    {
        struct tessellationSettings
        {
            /*unsigned int numPatchesX = 50;
            unsigned int numPatchesZ = 50;
            float patchSizeX = 50.0;
            float patchSizeZ = 50.0;*/

            unsigned int numPatchesX = 1;
            unsigned int numPatchesZ = 1;
            float patchSizeX = 2500.0;
            float patchSizeZ = 2500.0;
            float heightMapYScale = 200;
            float yOffset = 0.0;
        } prefs;

        texture *heightMapTex = 0;
        GLuint vao;
        GLuint vertexBuffer;
        void render(uniformsHolder &tess,bool bindsOnly = false);
        tessellation(float _yOffset);
        tessellation(std::string heightMapFilePath);
        ~tessellation();

        private:

        void init();
    };

}

#endif // TESSELLATION_H_INCLUDED
