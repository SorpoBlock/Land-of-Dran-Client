#include "code/graphics/program.h"

namespace syj
{
    void glUniformMatArray(std::string location,program *prog,glm::mat4 *array,int size)
    {
        for(unsigned int a = 0; a<size; a++)
        {
            GLint loc = prog->getUniformLocation(location + "[" + std::to_string(a) + "]");
            if(loc == -1)
                continue;
            glm::mat4 tmp = array[a];
            glUniformMatrix4fv(loc,1,GL_FALSE,&tmp[0][0]);
        }
    }

    void glUniformMat(GLint location,glm::mat4 mat)
    {
        glUniformMatrix4fv(location,1,GL_FALSE,&mat[0][0]);
    }

    void glUniform3vec(GLint location,glm::vec3 vec)
    {
        glUniform3f(location,vec.x,vec.y,vec.z);
    }

    void glUniform4vec(GLint location,glm::vec4 vec)
    {
        glUniform4f(location,vec.r,vec.g,vec.b,vec.a);
    }

    GLint program::registerUniformVec3  (std::string name,bool useDefault,glm::vec3 defaul)
    {
        scope("program::registerUniformVec3");
        GLint uniform = getUniformLocation(name);
        uniforms.push_back(uniform);
        useDefaults.push_back(useDefault);
        defaultVec3s.push_back(defaul);
        defaultTypes.push_back(isVec3);
        defaultMats.push_back(glm::mat4(1.0));
        defaultInts.push_back(0);
        defaultFloats.push_back(0);
        return uniform;
    }

    GLint program::registerUniformMat  (std::string name,bool useDefault,glm::mat4 defaul)
    {
        scope("program::registerUniformMat");
        GLint uniform = getUniformLocation(name);
        uniforms.push_back(uniform);
        defaultMats.push_back(defaul);
        useDefaults.push_back(useDefault);
        defaultTypes.push_back(isMat);
        defaultInts.push_back(0);
        defaultFloats.push_back(0);
        defaultVec3s.push_back(glm::vec3(0,0,0));
        return uniform;
    }

    GLint program::registerUniformBool (std::string name,bool useDefault,bool defaul)
    {
        return registerUniformInt(name,useDefault,defaul);
    }

    GLint program::registerUniformInt  (std::string name,bool useDefault,int   defaul)
    {
        scope("program::registerUniformInt");
        GLint uniform = getUniformLocation(name);

        uniforms.push_back(uniform);

        defaultInts.push_back(defaul);
        useDefaults.push_back(useDefault);
        defaultTypes.push_back(isInt);
        defaultMats.push_back(glm::mat4(1.0));
        defaultFloats.push_back(0);
        defaultVec3s.push_back(glm::vec3(0,0,0));

        return uniform;
    }

    GLint program::registerUniformFloat(std::string name,bool useDefault,float defaul)
    {
        scope("program::registerUniformFloat");
        GLint uniform = getUniformLocation(name);

        uniforms.push_back(uniform);
        defaultFloats.push_back(defaul);
        useDefaults.push_back(useDefault);
        defaultTypes.push_back(isFloat);
        defaultMats.push_back(glm::mat4(1.0));
        defaultInts.push_back(0);
        defaultVec3s.push_back(glm::vec3(0,0,0));

        return uniform;
    }

    void program::bindShader(shader *toBind)
    {
        scope("program::bindShader");

        switch(toBind->shaderType)
        {
            case GL_VERTEX_SHADER:
                vertex = toBind;
                break;
            case GL_FRAGMENT_SHADER:
                fragment = toBind;
                break;
            case GL_TESS_CONTROL_SHADER:
                tessControl = toBind;
                break;
            case GL_TESS_EVALUATION_SHADER:
                tessEval = toBind;
                break;
            case GL_GEOMETRY_SHADER:
                geometry = toBind;
                break;

            default:
                error("Invalid shader type!");
                return;
        }
    }

    void program::compile()
    {
        scope("program::compile");

        debug("Creating program!");

        handle = glCreateProgram();
        if(handle == 0)
        {
            error("glCreateProgram failed: " + std::to_string(glGetError()));
            return;
        }

        if(vertex)
            glAttachShader(handle,vertex->glShader);
        if(fragment)
            glAttachShader(handle,fragment->glShader);
        if(tessEval)
            glAttachShader(handle,tessEval->glShader);
        if(tessControl)
            glAttachShader(handle,tessControl->glShader);
        if(geometry)
            glAttachShader(handle,geometry->glShader);

        glLinkProgram(handle);
        GLint programSuccess = GL_FALSE;
        glGetProgramiv( handle, GL_LINK_STATUS, &programSuccess );

        if(programSuccess != GL_TRUE)
        {
            error("Could not create program! ");

            int length,actualLength;
            glGetProgramiv( handle, GL_INFO_LOG_LENGTH, &length );
            char *data = new char[length];
            glGetProgramInfoLog( handle, length, &actualLength, data );

            if(length < 1)
                error("Could not get error log for program compilation.");
            else
            {
                std::ofstream programLog("logs/shaderProgram.log");

                if(programLog.is_open())
                {
                    programLog<<data;
                    error(data);
                    programLog.close();
                }
                else
                    error("Could not open logs/shaderProgram.log for write!");
            }

            delete data;

            return;
        }

        info("Program created succesfully.");

        _isCompiled = true;
    }

    void program::resetUniforms()
    {
        for(unsigned int a = 0; a<uniforms.size(); a++)
        {
            if(useDefaults[a])
            {
                if(uniforms[a] != -1)
                {
                    switch(defaultTypes[a])
                    {
                        case isMat:
                            glUniformMatrix4fv(uniforms[a],1,GL_FALSE,&defaultMats.at(a)[0][0]);
                            break;
                        case isFloat:
                            glUniform1f(uniforms[a],defaultFloats[a]);
                            break;
                        case isInt:
                            glUniform1i(uniforms[a],defaultInts[a]);
                            break;
                        case isVec3:
                            glUniform3f(uniforms[a],defaultVec3s[a].x,defaultVec3s[a].y,defaultVec3s[a].z);
                            break;
                    }
                }
            }
        }
    }

    void program::use(bool reset)
    {
        glUseProgram(handle);
        if(reset)
            resetUniforms();
    }

    GLint program::getUniformLocation(std::string name)
    {
        scope("program::getUniformLocation");
        GLint ret = glGetUniformLocation(handle,name.c_str());
        //if(ret == -1)
          //  error(name + " not found, or probably just not used in the shader anywhere!");
        return ret;
    }
}
