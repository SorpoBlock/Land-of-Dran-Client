#ifndef RENDERCONTEXT_H_INCLUDED
#define RENDERCONTEXT_H_INCLUDED

#include "code/utility/logger.h"
#define GLEW_STATIC
#define GLEW_NO_GLU
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>

namespace syj
{
    struct renderContextOptions
    {
        int glMajorVersion = 4;
        int glMinorVersion = 0;
        int multiSampleBuffers = 1;
        int multiSampleSamples = 8;
        bool useComptabilityProfile = false;
        int startingResX = 1000;
        int startingResY = 1000;
        bool useFullscreen = false;
        bool useVSync = false;
        bool debug = true;
    };

    class renderContext
    {
        public:

            bool isActive()
            {
                return startUpOkay;
            }

            glm::vec2 getResolution()
            {
                return glm::vec2(width,height);
            }

            float getWidth()
            {
                return width;
            }

            float getHeight()
            {
                return height;
            }

            bool getMouseLocked()
            {
                return mouseLocked;
            }

            void setMouseLock(bool toLock)
            {
                mouseLocked = toLock;
                SDL_SetRelativeMouseMode(toLock ? SDL_TRUE : SDL_FALSE);
            }

            renderContext(renderContextOptions &options);
            ~renderContext();

            void swap();
            void select();
            void clear(float r,float g,float b,float a = 1.0,bool depth = true);
            void clear(glm::vec3 rgb,bool depth = true);

        private:

            SDL_GLContext context;
            SDL_Window *window;
            renderContextOptions options;
            bool startUpOkay = false;
            float width,height;
            bool mouseLocked = false;
    };
}

#endif // RENDERCONTEXT_H_INCLUDED
