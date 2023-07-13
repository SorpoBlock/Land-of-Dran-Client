#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include "code/utility/logger.h"
#define GLEW_STATIC
#define GLEW_NO_GLU
#include <GL/glew.h>
#include "code/utility/fileOpeartions.h"

namespace syj
{

    struct shader
    {
        GLenum shaderType = 0;
        GLuint glShader = 0;
        shader(std::string filePath,GLenum type);
        ~shader();
    };

}

#endif // SHADER_H_INCLUDED
