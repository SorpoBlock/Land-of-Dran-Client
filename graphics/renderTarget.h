#ifndef RENDERTARGET_H_INCLUDED
#define RENDERTARGET_H_INCLUDED

#include "texture.h"

namespace syj
{
    struct renderTarget
    {
        struct renderTargetSettings
        {
            int resX = 800;
            int resY = 800;
            unsigned int layers = 0; //set to 0 for GL_TEXTURE_2D
            bool HDR = false;
            int numColorChannels = 3;
            bool useStencil = false;
            bool useColor = true;
            bool useDepth = true;
            GLenum depthBufferType = GL_DEPTH_COMPONENT;
            glm::vec4 clearColor = glm::vec4(0,0,0,1);
            GLenum texWrapping = GL_CLAMP_TO_EDGE;
            GLenum minFilter = GL_LINEAR;
            GLenum magFilter = GL_LINEAR;
        } settings;

        texture *colorResult = 0;
        texture *depthResult = 0;
        GLuint frameBuffer = 0;
        GLuint depthBuffer = 0;
        GLenum *drawBuffers = 0;

        renderTarget(renderTargetSettings &frameBufferPrefs);
        ~renderTarget();

        void bind();
        void unbind();
    };
}

#endif // RENDERTARGET_H_INCLUDED
