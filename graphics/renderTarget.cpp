#include "renderTarget.h"

namespace syj
{

    renderTarget::renderTarget(renderTargetSettings &frameBufferPrefs)
    {
        scope("renderTarget::renderTarget");
        settings = frameBufferPrefs;

        glGenFramebuffers(1,&frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

        std::cout<<"Layers: "<<frameBufferPrefs.layers<<"\n";

        if(settings.useColor)
        {
            if(frameBufferPrefs.layers != 0)
                colorResult = new texture(GL_TEXTURE_2D_ARRAY);
            else
                colorResult = new texture(GL_TEXTURE_2D);

            colorResult->setWrapping(settings.texWrapping);
            colorResult->setFilter(settings.magFilter,settings.minFilter);
            if(frameBufferPrefs.layers != 0)
                colorResult->allocate(settings.HDR,settings.resX,settings.resY,frameBufferPrefs.layers,settings.numColorChannels);
            else
                colorResult->allocate(settings.HDR,settings.resX,settings.resY,settings.numColorChannels);
            colorResult->useForFramebuffer(GL_COLOR_ATTACHMENT0);    //glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentNumber, GL_TEXTURE_2D, handle,0);
            drawBuffers =  new GLenum[1];
            drawBuffers[0] = {GL_COLOR_ATTACHMENT0};
            glDrawBuffers(1, drawBuffers);
        }
        else
            glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if(settings.depthBufferType != GL_FALSE)
        {
            if(settings.useDepth)
            {

                std::cout<<"Using depth, layers: "<<frameBufferPrefs.layers<<"\n";
                if(frameBufferPrefs.layers!=0)
                {
                    depthResult = new texture(GL_TEXTURE_2D_ARRAY);
                    glBindTexture(GL_TEXTURE_2D_ARRAY,depthResult->handle);
                    glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_DEPTH_COMPONENT32F,settings.resX,settings.resY,frameBufferPrefs.layers,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthResult->handle, 0);

                }
                else
                {
                    if(frameBufferPrefs.layers != 0)
                        depthResult = new texture(GL_TEXTURE_2D_ARRAY);
                    else
                        depthResult = new texture(GL_TEXTURE_2D);

                    depthResult->setWrapping(settings.texWrapping);
                    depthResult->setFilter(settings.magFilter,settings.minFilter);
                    if(frameBufferPrefs.layers != 0)
                        depthResult->allocateDepth(settings.resX,settings.resY,frameBufferPrefs.layers,settings.depthBufferType,settings.depthBufferType,GL_FLOAT);        //glTexImage2D
                    else
                        depthResult->allocateDepth(settings.resX,settings.resY,settings.depthBufferType,settings.depthBufferType,GL_FLOAT);        //glTexImage2D
                    depthResult->useForFramebuffer(settings.useStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT);
                }

            }
            else
            {
                glGenRenderbuffers(1, &depthBuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
                glRenderbufferStorage(GL_RENDERBUFFER, settings.depthBufferType, settings.resX,settings.resY);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, settings.useStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
            }
        }
        else if(settings.useDepth)
            error("useDepth needs to be false if settings.depthBufferType is GL_FALSE");


        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            error("Framebuffer creation error: " + std::to_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)) + " : " + std::to_string(glGetError()));
        else
            debug("Framebuffer created!");


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    renderTarget::~renderTarget()
    {
        glDeleteFramebuffers(1,&frameBuffer);
        glDeleteRenderbuffers(1,&depthBuffer);
        delete colorResult;
        delete depthResult;
        delete drawBuffers;
    }

    void renderTarget::bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER,frameBuffer);
        glViewport(0,0,settings.resX,settings.resY);
        glClearColor( settings.clearColor.r,settings.clearColor.g,settings.clearColor.b,settings.clearColor.a );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void renderTarget::unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }

}


