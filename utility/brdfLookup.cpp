#include "brdfLookup.h"

namespace syj
{
    texture *generateBDRF(GLuint quadVAO)
    {
        debug("Creating BDRF look-up texture.");

        program bdrfProgram;
        shader bdrfVertex("shaders/bdrf.vert.glsl",GL_VERTEX_SHADER);
        shader bdrfFragment("shaders/bdrf.frag.glsl",GL_FRAGMENT_SHADER);
        bdrfProgram.bindShader(&bdrfVertex);
        bdrfProgram.bindShader(&bdrfFragment);
        bdrfProgram.compile();
        if(!bdrfProgram.isCompiled())
            return 0;

        renderTarget::renderTargetSettings pref;
        pref.resX = 2048;
        pref.resY = 2048;
        pref.HDR = true;
        pref.minFilter = GL_LINEAR;
        pref.magFilter = GL_LINEAR;
        pref.texWrapping = GL_CLAMP_TO_EDGE;
        pref.useColor = true;
        pref.useDepth = false;
        renderTarget *ret = new renderTarget(pref);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        ret->bind();
        bdrfProgram.use();

        //Literally all we have to do is draw a quad, the shader takes care of the rest
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);

        ret->unbind();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        return ret->colorResult;
    }

}
