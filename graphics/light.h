#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include <vector>
#include <algorithm>
#include <glm/gtx/norm.hpp>
#include "code/graphics/uniformsBasic.h"
#include "code/physics/player.h"
#include "code/physics/brickPhysics.h"
#include "code/networking/location.h"

namespace syj
{
    struct light
    {
        attachMode mode = worldPos;
        brickRenderData *attachedBrick = 0;
        livingBrick *attachedCar = 0;
        item *attachedItem = 0;
        newDynamic *attachedDynamic = 0;
        std::string node = "";
        glm::vec3 attachOffset = glm::vec3(0,0,0);
        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec4 direction = glm::vec4(0,0,0,0); //w = cosine angle phi
        glm::vec4 finalDirection = glm::vec4(0,0,0,0);
        bool isSpotlight = false;
        glm::vec3 blinkVel = glm::vec3(0,0,0);
        glm::vec3 finalLightColor = glm::vec3(0,0,0);
        float yawVel = 0;

        unsigned int createdTime = 0;
        int lifeTimeMS = -1;

        int serverID = -1;
        glm::vec3 color = glm::vec3(1,1,1);
        void calcPos(float deltaT);
        void clearAttachments(glm::vec3 _worldPos = glm::vec3(0,0,0));
        void attachToCar(livingBrick *car,glm::vec3 offset);
        void attachToBrick(brickRenderData *brick);
        void attachToDynamic(newDynamic *dynamic,std::string target = "");
        void attachToItem(item *dynamic,std::string target = "");
    };

    void sortLights(std::vector<light*> &lightVec,glm::vec3 cameraPos,float deltaT);
    void renderLights(uniformsHolder &unis,std::vector<light*> &lightVec);
}

#endif // LIGHT_H_INCLUDED
