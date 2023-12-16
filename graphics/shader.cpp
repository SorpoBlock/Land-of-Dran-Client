#include "code/graphics/shader.h"

namespace syj
{
    shader::shader(std::string filePath,GLenum type)
    {
        scope("shader::shader");

        info("Trying to create shader with filePath " + filePath);

        shaderType = type;
        switch(shaderType)
        {
            case GL_VERTEX_SHADER: break;
            case GL_TESS_CONTROL_SHADER: break;
            case GL_TESS_EVALUATION_SHADER: break;
            case GL_GEOMETRY_SHADER: break;
            case GL_FRAGMENT_SHADER: break;
            default:
                error("Invalid shader type requested: " + std::to_string(type));
                return;
        }

        glShader = glCreateShader(type);
        if(!glShader)
        {
            error("glCreateShader failed for " + filePath);
            error(std::to_string(glGetError()));
            return;
        }

        //It's a text file, but we're just copying it all as one big buffer for openGL to look at
        std::ifstream shaderFile(filePath.c_str(),std::ios::ate | std::ios::binary);
        if(shaderFile.is_open())
        {
            //Figure out how big file is then return to start position
            int size = shaderFile.tellg();
            shaderFile.seekg(0);

            char *data = new char[size];
            shaderFile.read(data,size);

            glShaderSource(glShader,1,&data,&size);
            glCompileShader(glShader);

            shaderFile.close();
            delete data;

            //We need to get the expected length of the shader, then get the shader itself
            //length and actualLength should not differ
            int length,actualLength = 0;
            glGetShaderiv(glShader,GL_INFO_LOG_LENGTH,&length);
            data = new char[length];
            glGetShaderInfoLog(glShader,length,&actualLength,data);

            //If the length of the openGL error output is 0 then it succeeded
            if(!length)
                debug("Shader compiled successfully.");
            else
            {
                error("Error while compiling " + filePath + " see associated error file for more info!");
                std::string logPath = "Logs/ " + getFileFromPath(filePath) + ".log";
                std::ofstream shaderLog(logPath);
                if(shaderLog.is_open())
                {
                    error(data);
                    shaderLog<<data;
                    shaderLog.close();
                }
                else
                    error("Could not open " + logPath + ".log for output");
            }

            delete data;
        }
        else
            error("Could not open " + filePath);
    }

    shader::~shader()
    {
        glDeleteShader(glShader);
    }
}
