#ifndef ENVIRONMENT_H_INCLUDED
#define ENVIRONMENT_H_INCLUDED

#include "code/graphics/renderTarget.h"
#include "code/graphics/camera.h"
#include "code/graphics/material.h"
#include "code/graphics/model.h"
#include "code/utility/debugGraphics.h"

namespace syj
{
    struct dayNightCycle
    {
        //The following are times, -1 = midnight, 1 = also midnight:
        double dawnStart = -0.8;
        double dawnEnd = -0.4;
        double duskStart = 0.4;
        double duskEnd = 0.8;
        double secondsInDay = 1000;

        //There are 4 colors to interpolate between for night, dawn, day, and dusk
        //I'd use an enum, but really we only use them in their interpolated form so why bother
        glm::vec3 dncSkyColors[4];
        glm::vec3 dncFogColors[4];
        glm::vec4 dncSunColors[4];

        //time = 0.0;
        //+/-1 = midnight
        //-0.5 = dawn
        //0 = noon
        //0.5 = dusk
        //Actually it's like 0.6 is dusk and -0.6 is dawn for the given lat and decl

        dayNightCycle()
        {
            dncSunColors[0]        = glm::vec4(0.06,0.06,0.1,0);
            dncFogColors[0]        = glm::vec3(0.06,0.06,0.1);
            dncSkyColors[0]        = glm::vec3(1.0,1.0,1.0);

            dncSunColors[1]        = glm::vec4(235,116,26,255) / glm::vec4(256,256,256,255);
            dncFogColors[1]        = glm::vec3(21.0/255.0,26.0/255.0,29.0/255.0);
            dncSkyColors[1]        = glm::vec3(1.0  ,0.77 ,0.541);

            dncSunColors[2]        = glm::vec4(0.7,0.6,0.5,1.0/30.0) * glm::vec4(30.0);
            dncFogColors[2]        = glm::vec3(109.0/255.0,130.0/255.0,132.0/255.0);
            dncSkyColors[2]        = glm::vec3(1.0,1.0,1.0);

            dncSunColors[3]        = (glm::vec4(235,116,26,255) / glm::vec4(256,256,256,255)) * glm::vec4(1.2,1.2,1.2,1.0);
            dncFogColors[3]        = glm::vec3(74.0/255.0  ,71.0/255.0 ,56.0/255.0);
            dncSkyColors[3]        = glm::vec3(1.0  ,0.77 ,0.541);
        }
    };

    struct environment
    {
        //Image based lighting stuff:
        bool useIBL = false;

        GLuint IBL = 0;
        GLuint IBLRad = 0;
        GLuint IBLIrr = 0;
        //End IBL stuff


        GLuint skyBoxVertBuffer[6];
        GLuint skyBoxUVBuffer[6];

        unsigned int numShadowLayers = 3;

        //Cascading shadow maps:
        renderTarget *shadowBuffer;
        glm::mat4 *lightSpaceMatricies;
        /*renderTarget *shadowNearTransparentBuffer;
        renderTarget *shadowNearBuffer;
        renderTarget *shadowFarBuffer;*/

        renderTarget *godRayPass;

        //Shadow's point of view:
        /*orthographicCamera shadowNearCamera;
        orthographicCamera shadowFarCamera;*/

        dayNightCycle cycle;
        float currentTime = 0.1;
        float skyboxInterpolate = 0.0;
        double solarElevation = 0.0;

        //This just holds a sphere or something we use as the model for the sun in the sky:
        model *sun;

        //The *current* post-interpolation values for all this stuff:
        glm::vec3 skyColor;
        glm::vec3 fogColor;
        glm::vec4 sunColor;
        glm::vec3 sunDirection;

        //Configuration for crepuscular rays
        float godRayDecay = 0.995;
        float godRayDensity = 1.0;
        float godRayWeight = 6.0;
        float godRayExposure = 0.004;
        float sunDistance = 1.0;

        //Fog start and end distances, TODO: maybe let you set the fog function too? Like linear or logarithmic.
        float fogDistanceMin = 500;
        float fogDistanceMax = 1000;

        //Literally just holds a square and a cube for rendering post processing stuff
        GLuint godRaySquareVAO;
        GLuint skyBoxFacesVAO[6];

        //One texture for each side of the sky, excluding the bottom which is untextured currently:
        //We interpolate between the two skyboxes during dawn and dusk
        texture *skyTexturesSideDay[5] = {0,0,0,0,0};
        texture *skyTexturesSideNight[5] = {0,0,0,0,0};

        void shadowsCalc(perspectiveCamera *playerCamera,glm::vec3 shadowDir);
        void calc(float deltaMS,perspectiveCamera *playerCamera);
        void passUniforms(uniformsHolder *uniforms,bool forgoShadowMaps = false);
        void loadDaySkyBox(std::string path);
        void loadNightSkyBox(std::string path);
        void loadSunModel(std::string path);
        void drawSun(uniformsHolder *uniforms);
        void drawSky(uniformsHolder *uniforms);
        void renderGodRays(uniformsHolder *uniforms);

        void passLightMatricies(uniformsHolder *uniforms);

        environment(int resX,int resY);
        ~environment();
    };

}

#endif // ENVIRONMENT_H_INCLUDED
