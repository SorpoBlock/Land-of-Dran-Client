#include "uniformsBasic.h"

namespace syj
{
    std::vector<uniformsHolder*> loadAllShaders(std::string &errorString,std::string shadersListFilePath)
    {
        std::vector<uniformsHolder*> programUnis;

        bool shaderFailedToCompile = false;
        program *lastProgram = 0;
        std::string lastProgramName = "";

        std::ifstream shadersList(shadersListFilePath.c_str());

        if(shadersList.is_open())
        {
            std::string line = "";

            while(!shadersList.eof())
            {
                lodGetLine(shadersList,line);

                if(line.length() < 1)
                    continue;

                if(line.substr(0,1) == "#")
                    continue;

                int firstTab = line.find("\t");
                int secondTab = line.find("\t",firstTab+1);

                if(firstTab == std::string::npos || secondTab == std::string::npos)
                {
                    error("Malformed shadersList.txt line: " + line);
                    continue;
                }

                std::string programName = line.substr(0,firstTab);
                std::string shaderType = line.substr(firstTab+1,secondTab - (firstTab+1));
                std::string filePath = line.substr(secondTab+1,line.length() - (secondTab-1));

                if(programName.length() < 1 || shaderType.length() < 1 || filePath.length() < 1)
                {
                    error("Malformed shadersList.txt line: " + line);
                    continue;
                }

                if(lastProgramName != programName)
                {
                    if(lastProgram != 0)
                    {
                        lastProgram->compile();
                        if(!lastProgram->isCompiled())
                        {
                            errorString = lastProgramName + " shader failed to compile. Check logs folder. This will cause severe graphics issues.";
                            //notify("Shader Failed to Compile",lastProgramName + " shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
                            shaderFailedToCompile = true;
                        }
                        uniformsHolder *lastUnis = new uniformsHolder(*lastProgram);
                        lastUnis->name = lastProgramName;
                        programUnis.push_back(lastUnis);
                    }

                    lastProgram = new program;
                }

                lastProgramName = programName;

                GLenum shaderEnum = 0;
                if(shaderType == "vert")
                    shaderEnum = GL_VERTEX_SHADER;
                else if(shaderType == "frag")
                    shaderEnum = GL_FRAGMENT_SHADER;
                else if(shaderType == "geom")
                    shaderEnum = GL_GEOMETRY_SHADER;
                else if(shaderType == "tesc")
                    shaderEnum = GL_TESS_CONTROL_SHADER;
                else if(shaderType == "tese")
                    shaderEnum = GL_TESS_EVALUATION_SHADER;

                if(shaderEnum == 0)
                {
                    error("Invalid shader type: " + shaderType);
                    continue;
                }
                else
                {
                    shader *tmp = new shader("shaders/" + filePath,shaderEnum);
                    lastProgram->bindShader(tmp);
                }
            }

            shadersList.close();
        }
        else
        {
            error("Could not open shaders/shadersList.txt!");
            return programUnis;
        }

        if(lastProgram != 0)
        {
            lastProgram->compile();
            if(!lastProgram->isCompiled())
            {
                errorString = lastProgramName + " shader failed to compile. Check logs folder. This will cause severe graphics issues.";
                //notify("Shader Failed to Compile",lastProgramName + " shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
                shaderFailedToCompile = true;
            }
            uniformsHolder *lastUnis = new uniformsHolder(*lastProgram);
            lastUnis->name = lastProgramName;
            programUnis.push_back(lastUnis);
        }

        return programUnis;
    }

    uniformsHolder::uniformsHolder(program &toAdd)
    {
        target = &toAdd;

        //Basic:

        /*angleMatrix         = toAdd.registerUniformMat("angleMatrix"         ,true,glm::mat4(1.0));
        viewMatrix          = toAdd.registerUniformMat("viewMatrix"         ,true,glm::mat4(1.0));
        projectionMatrix    = toAdd.registerUniformMat("projectionMatrix"   ,true,glm::mat4(1.0));*/
        translateMatrix     = toAdd.registerUniformMat("translateMatrix"    ,true,glm::mat4(1.0));
        rotateMatrix        = toAdd.registerUniformMat("rotateMatrix"       ,true,glm::mat4(1.0));
        scaleMatrix         = toAdd.registerUniformMat("scaleMatrix"        ,true,glm::mat4(1.0));
        modelMatrix         = toAdd.registerUniformMat("modelMatrix"        ,true,glm::mat4(1.0));

        useAlbedo           = toAdd.registerUniformBool("useAlbedo"         ,true,false);
        useNormal           = toAdd.registerUniformBool("useNormal"         ,true,false);
        useMetalness        = toAdd.registerUniformBool("useMetalness"      ,true,false);
        useRoughness        = toAdd.registerUniformBool("useRoughness"      ,true,false);
        useHeight           = toAdd.registerUniformBool("useHeight"         ,true,false);
        useAO               = toAdd.registerUniformBool("useAO"             ,true,false);

        isAnimated          = toAdd.registerUniformBool("isAnimated"        ,true,false);
        deltaT              = toAdd.registerUniformFloat("deltaT"           ,true);

        /*cameraDirection     = toAdd.registerUniformVec3("cameraDirection"    ,true);
        cameraPosition      = toAdd.registerUniformVec3("cameraPosition"    ,true);*/

        albedoTexture       = toAdd.registerUniformInt("albedoTexture"      ,true,albedo);
        normalTexture       = toAdd.registerUniformInt("normalTexture"      ,true,normal);
        mohrTexture         = toAdd.registerUniformInt("mohrTexture"        ,true,mohr);
        brdfTexture         = toAdd.registerUniformInt("brdfTexture"        ,true,brdf);
        shadowNearMapTexture = toAdd.registerUniformInt("shadowNearMap"          ,true,shadowNearMap);
        refractionTexture   = toAdd.registerUniformInt("refractionTexture"  ,true,refraction);
        reflectionTexture   = toAdd.registerUniformInt("reflectionTexture"  ,true,reflection);
        shadowFarMapTexture = toAdd.registerUniformInt("shadowFarMap"          ,true,shadowFarMap);
        shadowColorTexture  = toAdd.registerUniformInt("shadowColorMap"        ,true,shadowColorMap);
        shadowNearTransMapTexture = toAdd.registerUniformInt("shadowNearTransMap",true,shadowNearTransMap);
        cubeMapEnvironmentUni = toAdd.registerUniformInt("cubeMapEnvironment",true,cubeMapEnvironment);
        cubeMapRadianceUni = toAdd.registerUniformInt("cubeMapRadiance",true,cubeMapRadiance);
        cubeMapIrradianceUni = toAdd.registerUniformInt("cubeMapIrradiance",true,cubeMapIrradiance);

        calcTBN             = toAdd.registerUniformBool("calcTBN"           ,true);

        //Brick textures:

         brickTopNormalTexture = toAdd.registerUniformInt("topNormalTexture"      ,true,topNormal);
         brickTopMohrTexture = toAdd.registerUniformInt("topMohrTexture"          ,true,topMohr);
         brickSideNormalTexture = toAdd.registerUniformInt("sideNormalTexture"    ,true,sideNormal);
         brickSideMohrTexture = toAdd.registerUniformInt("sideMohrTexture"        ,true,sideMohr);
         brickBottomNormalTexture = toAdd.registerUniformInt("bottomNormalTexture",true,bottomNormal);
         brickBottomMohrTexture = toAdd.registerUniformInt("bottomMohrTexture"    ,true,bottomMohr);
         brickRampMohrTexture   = toAdd.registerUniformInt("rampMohrTexture"      ,true,rampMohr);
         brickRampMohrTexture   = toAdd.registerUniformInt("rampNormalTexture"      ,true,rampNormal);
         brickPrintTexture   = toAdd.registerUniformInt("printTexture"            ,true,printTexture);
         printTextureSpecialBrickNormalTexture = toAdd.registerUniformInt("printTextureSpecialBrickNormal",true,printTextureSpecialBrickNormal);

         //Environment:

        sunDirection            = toAdd.registerUniformVec3("sunDirection"      ,false);
        sunDistance             = toAdd.registerUniformFloat("sunDistance"      ,false);
        sunColor                = toAdd.registerUniformVec3("sunColor"          ,false);
        skyColor                = toAdd.registerUniformVec3("skyColor"          ,false);
        fogMaxDist              = toAdd.registerUniformFloat("fogMaxDist"          ,false);
        fogMinDist              = toAdd.registerUniformFloat("fogMinDist"          ,false);
        fogColor                = toAdd.registerUniformVec3("fogColor"          ,false);
        godRayDensity           = toAdd.registerUniformFloat("godRayDensity"          ,false);
        godRayExposure          = toAdd.registerUniformFloat("godRayExposure"          ,false);
        godRayDecay             = toAdd.registerUniformFloat("godRayDecay"          ,false);
        godRayWeight            = toAdd.registerUniformFloat("godRayWeight"          ,false);
        godRaySamples           = toAdd.registerUniformInt("godRaySamples"          ,false);
        dncInterpolation        = toAdd.registerUniformFloat("dncInterpolation"          ,false);
        renderingSun            = toAdd.registerUniformBool("renderingSun"          ,false);
        renderingSky            = toAdd.registerUniformBool("renderingSky"          ,false);
        sunAboveHorizon         = toAdd.registerUniformBool("sunAboveHorizon"       ,false);
        renderingRays           = toAdd.registerUniformBool("renderingRays"         ,false);
        doingGodRayPass         = toAdd.registerUniformBool("doingGodRayPass"       ,false);
        useIBL                  = toAdd.registerUniformBool("useIBL"                ,false);

        //Lighting:

        for(int a = 0; a<8; a++)
        {
            pointLightUsed[a] = toAdd.registerUniformBool("pointLightUsed[" + std::to_string(a) + "]",true,false);
            pointLightPos[a] = toAdd.registerUniformVec3("pointLightPos[" + std::to_string(a) + "]",false);
            pointLightColor[a] = toAdd.registerUniformVec3("pointLightColor[" + std::to_string(a) + "]",false);
            pointLightIsSpotlight[a] = toAdd.registerUniformBool("pointLightIsSpotlight[" + std::to_string(a) + "]",true,false);
            pointLightDirection[a] = toAdd.registerUniformVec3("pointLightDirection[" + std::to_string(a) + "]",false);
        }

        //Emitter:

         lifetimeMS             = toAdd.registerUniformFloat("lifetimeMS"       ,false);
         drag                   = toAdd.registerUniformVec3("drag"             ,false);
         gravity                = toAdd.registerUniformVec3("gravity"          ,false);
         spinSpeed              = toAdd.registerUniformFloat("spinSpeed"        ,false);
         for(int a = 0; a<4; a++)
         {
             sizes[a] = toAdd.registerUniformFloat("sizes["+std::to_string(a)+"]",false);
             colors[a] = toAdd.registerUniformVec3("colors["+std::to_string(a)+"]",false);
             times[a] = toAdd.registerUniformFloat("times["+std::to_string(a)+"]",false);
         }
         useInvAlpha            = toAdd.registerUniformBool("useInvAlpha"      ,false);
         currentTimeMS          = toAdd.registerUniformFloat("currentTimeMS"    ,false);
         calculateMovement      = toAdd.registerUniformBool("calculateMovement",false);

        //Tessellation:

        tessellationScale = toAdd.registerUniformVec3("tessellationScale"      ,true,glm::vec3(1,1,1));
        heightMapTexture  = toAdd.registerUniformInt("heightMapTexture"        ,true,heightMap);

        //Other:

        clipHeight        = toAdd.registerUniformFloat("clipHeight"            ,true,0);
        previewTexture    = toAdd.registerUniformBool("previewTexture"         ,true,false);
        waterDelta        = toAdd.registerUniformFloat("waterDelta"            ,true,0);

         //Bullet trails:

         bulletTrailStart = toAdd.registerUniformVec3("bulletTrailStart");
         bulletTrailEnd = toAdd.registerUniformVec3("bulletTrailEnd");
         bulletTrailColor = toAdd.registerUniformVec3("bulletTrailColor");
         bulletTrailProgress = toAdd.registerUniformFloat("bulletTrailProgress");
    }

    GLint uniformsHolder::cameraRight(std::string name)
    {
        return target->getUniformLocation("camera" + name + "Right");
    }

    GLint uniformsHolder::cameraUp(std::string name)
    {
        return target->getUniformLocation("camera" + name + "Up");
    }

     GLint uniformsHolder::cameraPosition(std::string uname)
     {
         GLint ret = target->getUniformLocation("camera" + uname + "Position");
         /*if(ret == -1)
            error(name);*/
         return ret;
     }

     GLint uniformsHolder::cameraDirection(std::string uname)
     {
         GLint ret = target->getUniformLocation("camera" + uname + "Direction");
         /*if(ret == -1)
            error(name);*/
         return ret;
     }

     GLint uniformsHolder::angleMatrix(std::string uname)
     {
         GLint ret = target->getUniformLocation("angle" + uname + "Matrix");
         /*if(ret == -1)
            error(name);*/
         return ret;
     }

     GLint uniformsHolder::viewMatrix(std::string uname)
     {
         GLint ret = target->getUniformLocation("view" + uname + "Matrix");
         /*if(ret == -1)
            error(name);*/
         return ret;
     }

     GLint uniformsHolder::projectionMatrix(std::string uname)
     {
         GLint ret = target->getUniformLocation("projection" + uname + "Matrix");
         /*if(ret == -1)
            error(name);*/
         return ret;
     }

     void uniformsHolder::setModelMatrix(glm::mat4 matrix)
     {
        glm::vec3 scale,skew,trans;
        glm::vec4 perspective;
        glm::quat rot;
        glm::decompose(matrix,scale,rot,trans,skew,perspective);
        glm::mat4 m_trans = glm::translate(trans);
        glm::mat4 m_rot = glm::mat4(rot);
        glm::mat4 m_scale = glm::scale(scale);
        glUniformMat(translateMatrix,m_trans);
        glUniformMat(rotateMatrix,m_rot);
        glUniformMat(scaleMatrix,m_scale);
        glUniformMat(modelMatrix,matrix);
     }

     void uniformsHolder::setModelMatrix(glm::mat4 scale,glm::mat4 trans,glm::mat4 rot)
     {
        glUniformMat(translateMatrix,trans);
        glUniformMat(rotateMatrix,rot);
        glUniformMat(scaleMatrix,scale);
        glUniformMat(modelMatrix,trans * rot * scale);
     }
}
