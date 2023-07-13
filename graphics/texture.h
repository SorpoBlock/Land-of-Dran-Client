#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include "code/utility/logger.h"
#include "code/graphics/program.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include "code/external/bitmap_image.hpp"

namespace syj
{
    void getImageDims(std::string filepath,int *x,int *y,int *c);
    bool isHDR(std::string filename);
    float *loadImageF(std::string filename,int &tWidth,int &tHeight,int &tChannels,int desiredChannels);
    unsigned char *loadImageU(std::string filename,int &tWidth,int &tHeight,int &tChannels,int desiredChannels);

    enum channel
    {
        red = 0,
        green = 1,
        blue = 2,
        alpha = 3
    };

    class texture
    {
        public:

            bool created = false;
            bool mipMaps = true;
            bool isHdr = false;
            int resX = -1,resY = -1,layers=1;
            int currentChannel = 0;
            float *hdrData = nullptr;
            unsigned char *ldrData = nullptr;

            GLenum magFilter =   GL_LINEAR;
            GLenum minFilter =   GL_LINEAR_MIPMAP_LINEAR;
            GLenum texWrapS =    GL_CLAMP_TO_EDGE;
            GLenum texWrapT =    GL_CLAMP_TO_EDGE;
            GLenum texWrapR =    GL_CLAMP_TO_EDGE;
            GLenum textureType = GL_TEXTURE_2D;

            GLuint handle;

            void setParams(GLenum _textureType,int w,int h);

        public:

            void allocateDepth(unsigned int width,unsigned int height,unsigned int _layers,GLenum internalFormat,GLenum format,GLenum type);
            void allocateDepth(unsigned int width,unsigned int height,GLenum internalFormat = GL_DEPTH24_STENCIL8,GLenum format = GL_DEPTH_STENCIL,GLenum type = GL_UNSIGNED_INT_24_8);
            //void useForDepthbuffer(GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT);
            void useForFramebuffer(GLenum attachmentNumber);
            void allocate(bool hdr,unsigned int width,unsigned int height,unsigned int channels);
            void allocate(bool hdr,unsigned int width,unsigned int height,unsigned int _layers,unsigned int channels);

            void addChannel(std::string fileName,unsigned int channels = 0,channel fileChannel = red);
            void dummyChannel();
            void dummyChannel(float value);
            void dummyChannel(unsigned char value);
            void finalize();

            void setFilter(GLenum _magFilter,GLenum _minFilter);
            void setWrapping(GLenum texWrap);
            void setWrapping(GLenum _texWrapS,GLenum _texWrapT,GLenum _texWrapR);
            void useMipMaps(bool value);

            glm::vec2 getResolution();
            void bind(textureLocations loc);
            void bind(brickShaderTexture loc);
            bool createFromFile(std::string filePath);

            texture(GLenum type = GL_TEXTURE_2D);
            ~texture();
    };

}

#endif // TEXTURE_H_INCLUDED
