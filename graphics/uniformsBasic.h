#ifndef UNIFORMSBASIC_H_INCLUDED
#define UNIFORMSBASIC_H_INCLUDED

#include "code/graphics/program.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace syj
{
    struct uniformsHolder
    {
        std::string name = "";

         //Basic:

         /*GLint angleMatrix = -1;
         GLint viewMatrix = -1;
         GLint projectionMatrix = -1;*/
         GLint translateMatrix = -1;
         GLint rotateMatrix = -1;
         GLint scaleMatrix = -1;
         GLint modelMatrix = -1;

         GLint useAlbedo = -1;
         GLint useNormal = -1;
         GLint useMetalness = -1;
         GLint useRoughness = -1;
         GLint useHeight = -1;
         GLint useAO = -1;

         /*GLint cameraDirection = -1;
         GLint cameraPosition = -1;*/

         GLint isAnimated = -1;
         GLint deltaT = -1;

         GLint albedoTexture = -1;
         GLint normalTexture = -1;
         GLint mohrTexture = -1;
         GLint brdfTexture = -1;
         GLint shadowNearMapTexture = -1;
         GLint shadowFarMapTexture = -1;
         GLint refractionTexture = -1;
         GLint reflectionTexture = -1;
         GLint shadowColorTexture = -1;
         GLint shadowNearTransMapTexture = -1;
         GLint cubeMapEnvironmentUni = -1;
         GLint cubeMapRadianceUni = -1;
         GLint cubeMapIrradianceUni = -1;

         GLint sunAboveHorizon = -1;
         GLint doingGodRayPass = -1;

         GLint calcTBN = -1;

         //Brick textures:

         GLint brickTopNormalTexture = -1;
         GLint brickTopMohrTexture = -1;
         GLint brickSideNormalTexture = -1;
         GLint brickSideMohrTexture = -1;
         GLint brickBottomNormalTexture = -1;
         GLint brickBottomMohrTexture = -1;
         GLint brickRampMohrTexture   = -1;
         GLint brickRampNormalTexture   = -1;
         GLint brickPrintTexture   = -1;
         GLint printTextureSpecialBrickNormalTexture = -1;

         //Environment:

         GLint sunDirection = -1;
         GLint sunColor = -1;
         GLint skyColor = -1;
         GLint fogMaxDist = -1;
         GLint fogMinDist = -1;
         GLint fogColor = -1;
         GLint godRayDensity = -1;
         GLint godRayExposure = -1;
         GLint godRayDecay = -1;
         GLint godRayWeight = -1;
         GLint godRaySamples = -1;
         GLint dncInterpolation = -1;
         GLint renderingSun = -1;
         GLint renderingSky = -1;
         GLint renderingRays = -1;

         //Emitter:

         GLint lifetimeMS = -1;
         GLint drag = -1;
         GLint gravity = -1;
         GLint spinSpeed = -1;
         GLint sizes[4] = {-1,-1,-1,-1};
         GLint colors[4] = {-1,-1,-1,-1};
         GLint times[4] = {-1,-1,-1,-1};
         GLint useInvAlpha = -1;
         GLint currentTimeMS = -1;
         GLint calculateMovement = -1;

         //Tessellation:

         GLint tessellationScale = -1;
         GLint heightMapTexture = -1;

         //Other:

         GLint clipHeight = -1;
         GLint previewTexture = -1;
         GLint waterDelta = -1;

         //Lights

         GLint pointLightUsed[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
         GLint pointLightPos[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
         GLint pointLightColor[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
         GLint pointLightIsSpotlight[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
         GLint pointLightDirection[8] = {-1,-1,-1,-1,-1,-1,-1,-1};

         //Bullet trails:

         GLint bulletTrailStart = -1;
         GLint bulletTrailEnd = -1;
         GLint bulletTrailColor = -1;
         GLint bulletTrailProgress = -1;

         //New camera:

         GLint cameraUp(std::string name);
         GLint cameraRight(std::string name);
         GLint cameraPosition(std::string uname);
         GLint cameraDirection(std::string uname);
         GLint angleMatrix(std::string uname);
         GLint viewMatrix(std::string uname);
         GLint projectionMatrix(std::string uname);

         void setModelMatrix(glm::mat4 matrix);
         void setModelMatrix(glm::mat4 scale,glm::mat4 trans,glm::mat4 rot);

         //End

         program *target = nullptr;
         uniformsHolder(program &toAdd);
         void use(bool reset = true){target->use(true);}
    };

    std::vector<uniformsHolder*> loadAllShaders(std::string &errorString,std::string shadersListFilePath = "shaders/shadersList.txt");
}

#endif // UNIFORMSBASIC_H_INCLUDED
