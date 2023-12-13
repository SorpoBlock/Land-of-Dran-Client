#include "environment.h"

namespace syj
{
    void environment::renderGodRays(uniformsHolder *uniforms)
    {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        glUniform1i(uniforms->renderingRays,true);
        godRayPass->colorResult->bind(albedo);
        passUniforms(uniforms);

        glBindVertexArray(godRaySquareVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);

        glUniform1i(uniforms->renderingRays,false);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void environment::drawSun(uniformsHolder *uniforms)
    {
        glDepthMask(0);
        glDisable(GL_CULL_FACE);
        glUniform1i(uniforms->renderingSun,true);

        sun->render(uniforms,glm::mat4(1.0),true);

        glUniform1i(uniforms->renderingSun,false);
        glEnable(GL_CULL_FACE);
        glDepthMask(1);
    }

    void environment::drawSky(uniformsHolder *uniforms)
    {
        if(useIBL)
        {
            glActiveTexture(GL_TEXTURE0 + cubeMapEnvironment);
            glBindTexture(GL_TEXTURE_CUBE_MAP,IBL);
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        uniforms->setModelMatrix(glm::mat4(1.0));
        glUniform1i(uniforms->renderingSky,true);

        glDepthMask(0);
        for(int a = 0; a<6; a++)
        {
            //For some reason I made it just reuse the top texture for the bottom texture idk lol
            if(!useIBL)
            {
                skyTexturesSideDay[a!=5 ? a : 0].bind(albedo);
                skyTexturesSideNight[a!=5 ? a : 0].bind(normal);
            }
            glBindVertexArray(skyBoxFacesVAO[a]);
            glDrawArrays(GL_TRIANGLES,0,6);
            glBindVertexArray(0);
        }

        glUniform1i(uniforms->renderingSky,false);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);


    }

    bool environment::loadDaySkyBox(std::string path)
    {
        for(int a = 0; a<5; a++)
        {
            skyTexturesSideDay[a].createFromFile(path + std::to_string(a) + ".png");
        }
    }

    bool environment::loadNightSkyBox(std::string path)
    {
        for(int a = 0; a<5; a++)
            skyTexturesSideNight[a].createFromFile(path + std::to_string(a) + ".png");
    }

    bool environment::loadSunModel(std::string path)
    {
        sun = new model(path);
    }

    environment::~environment()
    {
        delete lightSpaceMatricies;
        delete godRayPass;
        glDeleteVertexArrays(6,skyBoxFacesVAO);
        glDeleteBuffers(6,skyBoxVertBuffer);
        glDeleteBuffers(6,skyBoxUVBuffer);
        delete shadowBuffer;
        if(sun)
            delete sun;
    }

    environment::environment(int resX,int resY)
    {
        lightSpaceMatricies = new glm::mat4[numShadowLayers];

        sunDirection = glm::vec3(0.235702,0.942809,0.235702);
        sunColor = glm::vec4(0.7,0.6,0.5,1.0/30.0) * glm::vec4(30.0);

        /*shadowNearCamera.name = "ShadowNear";
        //shadowNearCamera.orthoBoundNear = glm::vec3(-200,-200,-300);
        //shadowNearCamera.orthoBoundFar = glm::vec3(200,200,300);
        shadowNearCamera.orthoBoundNear = glm::vec3(-100,-100,300);
        shadowNearCamera.orthoBoundFar = glm::vec3(100,100,-300);
        shadowNearCamera.setPosition(glm::vec3(0,0,0));

        shadowFarCamera.name = "ShadowFar";
        //shadowFarCamera.orthoBoundNear = glm::vec3(-800,-800,-600);
        //shadowFarCamera.orthoBoundFar = glm::vec3(800,800,600);
        shadowFarCamera.orthoBoundNear = glm::vec3(-500,-500,500);
        shadowFarCamera.orthoBoundFar = glm::vec3(500,500,-500);
        shadowFarCamera.setPosition(glm::vec3(0,0,0));*/

        /*renderTarget::renderTargetSettings shadowBufferSettings;
        shadowBufferSettings.resX = resX;
        shadowBufferSettings.resY = resY;
        shadowBufferSettings.useColor = false;
        shadowFarBuffer = new renderTarget(shadowBufferSettings);
        shadowNearBuffer = new renderTarget(shadowBufferSettings);*/

        renderTarget::renderTargetSettings shadowBufferSettings;
        shadowBufferSettings.resX = resX;
        shadowBufferSettings.resY = resY;
        shadowBufferSettings.useColor = true;
        shadowBufferSettings.useDepth = true;
        shadowBufferSettings.numColorChannels = 4;
        shadowBufferSettings.layers = numShadowLayers;
        shadowBuffer = new renderTarget(shadowBufferSettings);

        //shadowBufferSettings.numColorChannels = 4;
        //shadowBufferSettings.useColor = true;
        //shadowNearTransparentBuffer = new renderTarget(shadowBufferSettings);

        renderTarget::renderTargetSettings godRaySettings;
        godRaySettings.resX = shadowBufferSettings.resX;
        godRaySettings.resY = shadowBufferSettings.resY;
        godRayPass = new renderTarget(godRaySettings);
        godRaySquareVAO = createQuadVAO();

        glGenVertexArrays(6,skyBoxFacesVAO);
        std::vector<glm::vec3> skyBoxVerts[6];
        for(unsigned int a = 0; a<6; a++)
            for(unsigned int b = 0; b<6; b++)
                skyBoxVerts[b].push_back(glm::vec3(0,0,0));

        //0 1 1 2 2 stars 4 hori flip, 2 vert flip
        //0 1 3 1 0 clouds

        float adjust = 1.0;

        //Top
        skyBoxVerts[0].at(0) = glm::vec3(-adjust,adjust,-adjust);
        skyBoxVerts[0].at(1) = glm::vec3(adjust,adjust,-adjust);
        skyBoxVerts[0].at(2) = glm::vec3(adjust,adjust,adjust);
        skyBoxVerts[0].at(3) = glm::vec3(adjust,adjust,adjust);
        skyBoxVerts[0].at(4) = glm::vec3(-adjust,adjust,adjust);
        skyBoxVerts[0].at(5) = glm::vec3(-adjust,adjust,-adjust);

        //Right
        skyBoxVerts[1].at(0) = glm::vec3(adjust,-adjust,-adjust);
        skyBoxVerts[1].at(1) = glm::vec3(adjust,-adjust,adjust);
        skyBoxVerts[1].at(2) = glm::vec3(adjust,adjust,adjust);
        skyBoxVerts[1].at(3) = glm::vec3(adjust,adjust,adjust);
        skyBoxVerts[1].at(4) = glm::vec3(adjust,adjust,-adjust);
        skyBoxVerts[1].at(5) = glm::vec3(adjust,-adjust,-adjust);

        //Left
        skyBoxVerts[2].at(0) = glm::vec3(-adjust, adjust,   adjust);
        skyBoxVerts[2].at(1) = glm::vec3(-adjust,-adjust,   adjust);
        skyBoxVerts[2].at(2) = glm::vec3(-adjust,-adjust,   -adjust);
        skyBoxVerts[2].at(3) = glm::vec3(-adjust, adjust,   adjust);
        skyBoxVerts[2].at(4) = glm::vec3(-adjust,-adjust,   -adjust);
        skyBoxVerts[2].at(5) = glm::vec3(-adjust, adjust,   -adjust);

        //Front
        skyBoxVerts[3].at(0) = glm::vec3(-adjust,-adjust,adjust);
        skyBoxVerts[3].at(1) = glm::vec3(-adjust,adjust,adjust);
        skyBoxVerts[3].at(2) = glm::vec3(adjust,adjust,adjust);
        skyBoxVerts[3].at(3) = glm::vec3(adjust,adjust,adjust);
        skyBoxVerts[3].at(4) = glm::vec3(adjust,-adjust,adjust);
        skyBoxVerts[3].at(5) = glm::vec3(-adjust,-adjust,adjust);

        //Back
        skyBoxVerts[4].at(0) = glm::vec3(adjust, adjust,   -adjust);
        skyBoxVerts[4].at(1) = glm::vec3(-adjust, adjust,  -adjust);
        skyBoxVerts[4].at(2) = glm::vec3(-adjust,  -adjust,   -adjust);
        skyBoxVerts[4].at(3) = glm::vec3(adjust, adjust,    -adjust);
        skyBoxVerts[4].at(4) = glm::vec3(-adjust, -adjust,  -adjust);
        skyBoxVerts[4].at(5) = glm::vec3(adjust,  -adjust,  -adjust);

        //Bottom
        skyBoxVerts[5].at(0) = glm::vec3(-adjust,-adjust,-adjust);
        skyBoxVerts[5].at(1) = glm::vec3(adjust,-adjust,-adjust);
        skyBoxVerts[5].at(2) = glm::vec3(adjust,-adjust,adjust);
        skyBoxVerts[5].at(3) = glm::vec3(adjust,-adjust,adjust);
        skyBoxVerts[5].at(4) = glm::vec3(-adjust,-adjust,adjust);
        skyBoxVerts[5].at(5) = glm::vec3(-adjust,-adjust,-adjust);

        glm::vec2 uvsa[6];
        uvsa[0] = glm::vec2(0,0);
        uvsa[1] = glm::vec2(0,1);
        uvsa[2] = glm::vec2(1,1);
        uvsa[3] = glm::vec2(1,1);
        uvsa[4] = glm::vec2(1,0);
        uvsa[5] = glm::vec2(0,0);

        glm::vec2 uvsflip[6];
        uvsflip[0] = glm::vec2(1,1);
        uvsflip[1] = glm::vec2(0,1);
        uvsflip[2] = glm::vec2(0,0);
        uvsflip[3] = glm::vec2(1,1);
        uvsflip[4] = glm::vec2(0,0);
        uvsflip[5] = glm::vec2(1,0);

        glGenBuffers(6,skyBoxVertBuffer);
        glGenBuffers(6,skyBoxUVBuffer);

        for(unsigned int a = 0; a<6; a++)
        {
            glBindVertexArray(skyBoxFacesVAO[a]);
            glEnableVertexAttribArray(positions);
            glBindBuffer(GL_ARRAY_BUFFER, skyBoxVertBuffer[a]);
            glBufferData(GL_ARRAY_BUFFER, skyBoxVerts[a].size() * sizeof(glm::vec3), &skyBoxVerts[a].at(0), GL_STATIC_DRAW);
            glVertexAttribPointer(
                    positions,                  // attribute
                    3,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
                    0,                  // stride
                    (void*)0            // array buffer offset
            );

            glEnableVertexAttribArray(uvs);
            glBindBuffer(GL_ARRAY_BUFFER, skyBoxUVBuffer[a]);
            glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec2), (a == 2 || a == 4) ? uvsflip : uvsa, GL_STATIC_DRAW);
            glVertexAttribPointer(
                    uvs,                  // attribute
                    2,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
                    0,                  // stride
                    (void*)0            // array buffer offset
            );
        }
    }

    std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
    {
        const auto inv = glm::inverse(proj * view);

        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x)
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt =
                        inv * glm::vec4(
                            2.0f * x - 1.0f,
                            2.0f * y - 1.0f,
                            2.0f * z - 1.0f,
                            1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }

        return frustumCorners;
    }

    void environment::shadowsCalc(perspectiveCamera *playerCamera,glm::vec3 shadowDir)
    {
        if(useIBL)
            solarElevation = 10.0;

        float manualFracs[4] = {0,0.05,0.3,1};
        float manualZMults[3] = {20,10,7};

        for(unsigned int a = 0; a<numShadowLayers; a++)
        {
            //float startFrac = ((float)a) * frac;        //i.e. 0 = 0, 2 = 0.6666
            //float endFrac = ((float)a+1) * frac;       //i.e. 0 = 0.3333, 2 = 1.0
            float startFrac = manualFracs[a];
            float endFrac = manualFracs[a+1];
            float near = glm::mix(playerCamera->getNearPlane(),playerCamera->getFarPlane(),startFrac);
            float far = glm::mix(playerCamera->getNearPlane(),playerCamera->getFarPlane(),endFrac);

            glm::mat4 playerCameraNearMapProjSlice = glm::perspective(glm::radians(playerCamera->getFieldOfVision()),playerCamera->getAspectRatio(),near,far);
            std::vector<glm::vec4> frustumCorners = getFrustumCornersWorldSpace(playerCameraNearMapProjSlice,playerCamera->getViewMatrix());
            glm::vec3 playerViewCenter = glm::vec3(0, 0, 0);
            for (const auto& v : frustumCorners)
                playerViewCenter += glm::vec3(v);
            playerViewCenter /= frustumCorners.size();
            glm::mat4 lightView = glm::lookAt(playerViewCenter + shadowDir,playerViewCenter,glm::vec3(0,1,0));
            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();
            float minZ = std::numeric_limits<float>::max();
            float maxZ = std::numeric_limits<float>::lowest();
            for (const auto& v : frustumCorners)
            {
                const auto trf = lightView * v;
                minX = std::min(minX, trf.x);
                maxX = std::max(maxX, trf.x);
                minY = std::min(minY, trf.y);
                maxY = std::max(maxY, trf.y);
                minZ = std::min(minZ, trf.z);
                maxZ = std::max(maxZ, trf.z);
            }

            float zMult = manualZMults[a];
            if (minZ < 0)
            {
                minZ *= zMult;
            }
            else
            {
                minZ /= zMult;
            }
            if (maxZ < 0)
            {
                maxZ /= zMult;
            }
            else
            {
                maxZ *= zMult;
            }

            lightSpaceMatricies[a] = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ) * lightView;
        }
    }

    void environment::calc(float deltaMS,perspectiveCamera *playerCamera)
    {
        if(useIBL)
        {
            shadowsCalc(playerCamera,sunDirection);
            glActiveTexture(GL_TEXTURE0 + cubeMapRadiance);
            glBindTexture(GL_TEXTURE_CUBE_MAP,IBLRad);
            glActiveTexture(GL_TEXTURE0 + cubeMapIrradiance);
            glBindTexture(GL_TEXTURE_CUBE_MAP,IBLIrr);

            return;
        }

        //time = 0.0;
        //+/-1 = midnight
        //-0.5 = dawn
        //0 = noon
        //0.5 = dusk
        //Actually it's like 0.6 is dusk and -0.6 is dawn for the given lat and decl

        currentTime += deltaMS / (cycle.secondsInDay * 1000.0);
        if(currentTime >= 1.0)
            currentTime -= 2.0;

        int dncToUse     = 0;
        int nextDncToUse = 0;
        double dncInterpolateProgress = 0.0;
        skyboxInterpolate = 0.0;

        if(currentTime < cycle.dawnStart || currentTime > cycle.duskEnd)
        {
            //Night time
            dncToUse = 0;
            nextDncToUse = 0;
            dncInterpolateProgress = 0.0;
            skyboxInterpolate = 1.0;
        }
        else if(currentTime > cycle.dawnEnd && currentTime < cycle.duskStart)
        {
            //Day time
            dncToUse = 2;
            nextDncToUse = 2;
            dncInterpolateProgress = 0.0;
            skyboxInterpolate = 0.0;
        }
        else if(currentTime > cycle.dawnStart && currentTime < cycle.dawnEnd)
        {
            //Dawn
            skyboxInterpolate = 1.0 - ((currentTime - cycle.dawnStart) / (cycle.dawnEnd - cycle.dawnStart));

            double dawnRange = ((cycle.dawnEnd - cycle.dawnStart) / 2.0);
            double dawnMiddle = cycle.dawnStart + dawnRange;
            if(currentTime < dawnMiddle)
            {
                //Night to dawn
                dncToUse = 0;
                nextDncToUse = 1;
                dncInterpolateProgress = (currentTime - cycle.dawnStart) / dawnRange;
            }
            else
            {
                //Dawn to day
                dncToUse = 1;
                nextDncToUse = 2;
                dncInterpolateProgress = 1.0 - ((cycle.dawnEnd - currentTime) / dawnRange);
            }
        }
        else if(currentTime > cycle.duskStart && currentTime < cycle.duskEnd)
        {
            //Dusk
            skyboxInterpolate = (currentTime - cycle.duskStart) / (cycle.duskEnd - cycle.duskStart);

            double duskRange = ((cycle.duskEnd - cycle.duskStart) / 2.0);
            double duskMiddle = cycle.duskStart + duskRange;
            if(currentTime < duskMiddle)
            {
                //day to dusk
                dncToUse = 2;
                nextDncToUse = 3;
                dncInterpolateProgress = (currentTime - cycle.duskStart) / duskRange;
            }
            else
            {
                //dusk to night
                dncToUse = 3;
                nextDncToUse = 0;
                dncInterpolateProgress = 1.0 - ((cycle.duskEnd - currentTime) / duskRange);
            }
        }

        sunColor =        glm::mix(cycle.dncSunColors[dncToUse],   cycle.dncSunColors[nextDncToUse],   dncInterpolateProgress);
        skyColor =        glm::mix(cycle.dncSkyColors[dncToUse],   cycle.dncSkyColors[nextDncToUse],   dncInterpolateProgress);
        fogColor =        glm::mix(cycle.dncFogColors[dncToUse],   cycle.dncFogColors[nextDncToUse],   dncInterpolateProgress);

        double lat  = 45.0; //equator
        double decl = 18.0; //-23.44 * cos((360/365) * (213+10) * (pi/180))

        lat  *= 0.0174532925; //to rads
        decl *= 0.0174532925;


        solarElevation = asin(
                                    sin(lat) * sin(decl)
                                    +
                                    cos(lat) * cos(decl) * cos(currentTime * 3.1415)
                                  );

        double azimuth = acos(
                                (sin(decl) * cos(lat)
                                -
                                cos(decl) * sin(lat) * cos(currentTime * 3.1415))
                                /
                                cos(solarElevation)
                              );

        if(currentTime < 0.0)
            azimuth = 6.28 - azimuth;

        sunDirection = glm::vec3(cos(azimuth) * cos(solarElevation),sin(solarElevation),sin(azimuth) * cos(solarElevation));
        sunDirection = glm::normalize(sunDirection);

        shadowsCalc(playerCamera,sunDirection);
    }

    void environment::passLightMatricies(uniformsHolder *uniforms)
    {
        glUniformMatrix4fv(uniforms->target->getUniformLocation("lightSpaceMatricies"),numShadowLayers,GL_FALSE,(GLfloat*)lightSpaceMatricies);
    }

    void environment::passUniforms(uniformsHolder *uniforms,bool forgoShadowMaps)
    {
        if(!forgoShadowMaps)
        {
            /*shadowNearTransparentBuffer->depthResult->bind(shadowNearTransMap);
            shadowNearTransparentBuffer->colorResult->bind(shadowColorMap);
            shadowNearBuffer->depthResult->bind(shadowNearMap);
            shadowFarBuffer->depthResult->bind(shadowFarMap);
            shadowNearCamera.render(uniforms);
            shadowFarCamera.render(uniforms);*/

            shadowBuffer->colorResult->bind(shadowColorMap);
            shadowBuffer->depthResult->bind(shadowNearMap);
            passLightMatricies(uniforms);
        }

        glUniform1i(uniforms->useIBL,useIBL);
        glUniform4vec(uniforms->sunColor,sunColor);
        glUniform3vec(uniforms->sunDirection,sunDirection);
        glUniform1f(uniforms->sunDistance,sunDistance);
        glUniform3vec(uniforms->skyColor,skyColor);
        glUniform3vec(uniforms->fogColor,fogColor);
        glUniform1f(uniforms->fogMaxDist,fogDistanceMax);
        glUniform1f(uniforms->fogMinDist,fogDistanceMin);
        glUniform1f(uniforms->godRayDensity,godRayDensity);
        glUniform1f(uniforms->godRayExposure,godRayExposure);
        glUniform1f(uniforms->godRayDecay,godRayDecay);
        glUniform1f(uniforms->godRayWeight,godRayWeight);
//        glUniform1i(uniforms->godRaySamples,numGodRaySamples);
        glUniform1f(uniforms->dncInterpolation,skyboxInterpolate);
        glUniform1i(uniforms->renderingSun,0);
        glUniform1i(uniforms->sunAboveHorizon,solarElevation > 0);
    }
}
