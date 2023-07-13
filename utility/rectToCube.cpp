#include "rectToCube.h"

namespace syj
{
    GLuint processEquirectangularMap(program &rectToCube,GLuint cubeVAO,std::string fileName,bool mipMaps)
    {
        unsigned int width=2048,height=2048;

        //Our normal renderTarget code doens't support cubemaps

        texture sourceRect;
        sourceRect.createFromFile(fileName);

        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        GLuint envCubemap;
        glGenTextures(1, &envCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            // note that we store each face with 16 bit floating point values
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                            width, height, 0, GL_RGB, GL_FLOAT, nullptr);
            /*int mipLevel = 0;
            while((1<<mipLevel) <= std::max(width,height))
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel, GL_RGB16F,
                            width/(1<<mipLevel), height/(1<<mipLevel), 0, GL_RGB, GL_FLOAT, nullptr);
                mipLevel++;
            }*/
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        if(mipMaps)
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        else
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        // convert HDR equirectangular environment map to cubemap equivalent
        rectToCube.use();
        glUniform1i(rectToCube.getUniformLocation("albedoTexture"),0);
        glUniformMat(rectToCube.getUniformLocation("projection"),captureProjection);
        sourceRect.bind(albedo);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, width, height); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindVertexArray(cubeVAO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMat(rectToCube.getUniformLocation("view"),captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //The code from learnopengl uses a cube from 0-1 I think but we're using a cube from -0.5 to 0.5
            glDrawArrays(GL_TRIANGLES,0,36);
        }
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        if(mipMaps)
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        glBindTexture(GL_TEXTURE_CUBE_MAP,0);

        return envCubemap;
    }
}
