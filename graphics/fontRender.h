#ifndef FONTRENDER_H_INCLUDED
#define FONTRENDER_H_INCLUDED

#include "code/graphics/uniformsBasic.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace syj
{
    struct fontChar
    {
        GLuint texture;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
        bool used = false;
    };

    struct fontRenderer
    {
        GLuint squareVAO,squareVBO;
        fontChar glyphs[128];
        FT_Face face;
        void naiveRender(uniformsHolder &unis,std::string text,glm::vec3 position,float scale,glm::vec3 color);
        fontRenderer(FT_Library &ft,std::string filename,GLenum wrap = GL_CLAMP_TO_EDGE,GLenum minFilter = GL_LINEAR,GLenum magFilter = GL_LINEAR);
    };
}

#endif // FONTRENDER_H_INCLUDED
