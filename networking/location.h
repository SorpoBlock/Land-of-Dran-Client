#ifndef LOCATION_H_INCLUDED
#define LOCATION_H_INCLUDED

#include "code/physics/brickPhysics.h"
#include "code/physics/player.h"

namespace syj
{
    enum attachMode
    {
        worldPos = 0,
        brickPos = 1,
        carBrick = 2,
        dynamicNode = 3,
        itemNode = 4
    };

    struct location
    {
        static std::vector<location*> locations;

        attachMode type = worldPos;
        glm::vec3 fixedPos = glm::vec3(0,0,0);
        brickRenderData *brick = 0;
        livingBrick *car = 0;
        newDynamic *dynamic = 0;
        glm::vec3 calculatedOffset = glm::vec3(0,0,0);
        bool useOffset = false;

        //Only used by lights, maybe later emitters:
        glm::vec3 direction = glm::vec3(0,0,0);
        float getSpeed() const;
        glm::vec3 getDirection() const;
        ~location();
        location(glm::vec3 position);
        location(brickRenderData *_brick);
        location(livingBrick *_car,glm::vec3 offset = glm::vec3(0,0,0));
        location(newDynamic *_dynamic,std::string nodeName = "");
        location(item *_item,std::string nodeName = "");
        glm::vec3 getPosition() const;
        location(const location &src);
        location();
    };

}

#endif // LOCATION_H_INCLUDED
