#include "code/graphics/renderContext.h"

namespace syj
{
    void renderContext::select()
    {
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,width,height);
    }

    void renderContext::clear(float r,float g,float b,float a,bool depth)
    {
        glClearColor(r,g,b,a);
        if(depth)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        else
            glClear(GL_COLOR_BUFFER_BIT);
    }

    void renderContext::clear(glm::vec3 rgb,bool depth)
    {
            clear(rgb.r,rgb.g,rgb.b,depth);
    }

    renderContext::renderContext(renderContextOptions &options)
    {
        scope("renderContext::renderContext");

        debug("Initializing SDL");
        //if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER))
        {
            error("Could not Initialize SDL: " + std::string(SDL_GetError()));
            return;
        }

        debug("Setting preferences");

        if(SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 ) != 0)
            error("Setting attribute SDL_GL_DEPTH_SIZE failed: " + std::string(SDL_GetError()));

        if(SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, options.glMajorVersion ) != 0)
            error("Setting attribute SDL_GL_CONTEXT_MAJOR_VERSION failed: " + std::string(SDL_GetError()));

        if(SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, options.glMinorVersion ) != 0)
            error("Setting attribute SDL_GL_CONTEXT_MINOR_VERSION failed: " + std::string(SDL_GetError()));

        if(SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, options.multiSampleBuffers ) != 0)
            error("Setting attribute SDL_GL_MULTISAMPLEBUFFERS failed: " + std::string(SDL_GetError()));

        if(SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, options.multiSampleSamples ) != 0)
            error("Setting attribute SDL_GL_MULTISAMPLESAMPLES failed: " + std::string(SDL_GetError()));

        if(SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK,
                                options.useComptabilityProfile ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY : SDL_GL_CONTEXT_PROFILE_CORE
                                ) != 0)
            error("Setting attribute SDL_GL_CONTEXT_PROFILE_MASK failed: " + std::string(SDL_GetError()));

        if(options.debug)
        {
            if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG) != 0)
                error("SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG) failed");
        }

        debug("Creating window");

        width = options.startingResX;
        height = options.startingResY;
        int flag = options.useFullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;
        std::string name = "Land of Dran v0.002      ";
        name += options.name;
        window = SDL_CreateWindow( name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            options.startingResX, options.startingResY, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | flag);

        if(!window)
        {
            error("Failed to create window " + std::string(SDL_GetError()));
            return;
        }

        debug("Creating glContext");

        context = SDL_GL_CreateContext(window);

        if(!context)
        {
            error("Failed to create context: " + std::string(SDL_GetError()) + " | " + std::to_string(glGetError()));
            return;
        }

        debug("Starting GLEW");

        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();

        if(glewError != GLEW_OK)
        {
            error("glewInit failed" + std::string(SDL_GetError()) + " | " + std::to_string(glGetError()));
            return;
        }

        if(SDL_GL_SetSwapInterval(options.useVSync) != 0)
            error("SDL_GL_SetSwapInterval failed " + std::string(SDL_GetError()));

        if(options.debug)
            glEnable(GL_DEBUG_OUTPUT);

        startUpOkay = true;

        info("Graphics context created successfully.");
    }

    renderContext::~renderContext()
    {
        SDL_Quit();
    }

    void renderContext::swap()
    {
        SDL_GL_SwapWindow(window);
    }
}
