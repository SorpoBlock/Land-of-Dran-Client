#include "location.h"

namespace syj
{
    std::vector<location*> location::locations;

    glm::vec3 location::getDirection() const
    {
        switch(type)
        {
            default: //brickPos, worldPos
                return direction;

            case itemNode:
            case dynamicNode:
            {
                if(!dynamic)
                    return direction;
                glm::mat4 rotMat = glm::toMat4(dynamic->modelInterpolator.getRotation());
                glm::vec4 dir = glm::vec4(direction.x,direction.y,direction.z,0);
                return rotMat * dir;
            }

            case carBrick:
            {
                if(!car)
                    return direction;
                glm::mat4 rotMat = glm::toMat4(car->carTransform.getRotation());
                glm::vec4 dir = glm::vec4(direction.x,direction.y,direction.z,0);
                return rotMat * dir;
            }
        }
    }

    location::~location()
    {
        for(int a = 0; a<locations.size(); a++)
        {
            if(locations[a] == this)
            {
                locations.erase(locations.begin() + a);
                return;
            }
        }
    }

    location::location(glm::vec3 position)
    {
        locations.push_back(this);
        fixedPos = position;
        type = worldPos;
    }

    location::location(brickRenderData *_brick)
    {
        locations.push_back(this);
        brick = _brick;
        type = brickPos;
        fixedPos = _brick->position;
    }

    location::location(livingBrick *_car,glm::vec3 offset)
    {
        locations.push_back(this);
        useOffset = glm::length(offset) > 0.01;
        car = _car;
        type = carBrick;
        calculatedOffset = offset;
    }

    location::location(newDynamic *_dynamic,std::string nodeName)
    {
        locations.push_back(this);
        scope("location::location(dynamic,node)");

        dynamic = _dynamic;
        type = dynamicNode;

        if(nodeName.length() > 0)
        {
            for(unsigned int a = 0; a<dynamic->type->instancedMeshes.size(); a++)
            {
                newMesh *tmp = dynamic->type->instancedMeshes[a];
                if(tmp->name == nodeName)
                {
                    glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                    center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                    center *= dynamic->scale;
                    calculatedOffset = center;
                    useOffset = true;
                    return;
                }
            }

            error("Could not find node " + nodeName);
        }

        useOffset = false;
        calculatedOffset = glm::vec3(0,0,0);
    }

    location::location(item *_item,std::string nodeName)
    {
        locations.push_back(this);
        scope("location::location(item,node)");

        dynamic = _item;
        type = itemNode;

        if(nodeName.length() > 0)
        {
            for(unsigned int a = 0; a<dynamic->type->instancedMeshes.size(); a++)
            {
                newMesh *tmp = dynamic->type->instancedMeshes[a];
                if(tmp->name == nodeName)
                {
                    glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                    center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                    center *= dynamic->scale;
                    calculatedOffset = center;
                    useOffset = true;
                    return;
                }
            }

            error("Could not find node " + nodeName);
        }

        useOffset = false;
        calculatedOffset = glm::vec3(0,0,0);
    }

    float location::getSpeed() const
    {
        if(type == carBrick && car)
            return car->lastSpeed;
        else
            return 0;
    }

    glm::vec3 location::getPosition() const
    {
        switch(type)
        {
            default: //worldPos, brickPos
                return fixedPos;
            /*case brickPos:
                if(!brick)
                    return fixedPos;
                return brick->position;*/
            case carBrick:
            {
                if(!car)
                    return fixedPos;
                glm::mat4 rotMat = glm::toMat4(car->carTransform.getRotation());
                glm::vec4 off = glm::vec4(calculatedOffset.x,calculatedOffset.y,calculatedOffset.z,1.0);
                off = rotMat * off;
                return car->carTransform.getPosition() + glm::vec3(off.x,off.y,off.z);
            }
            case itemNode:
            case dynamicNode:
                if(!dynamic)
                    return fixedPos;
                if(useOffset)
                    return dynamic->modelInterpolator.getPosition() + multVec3ByMat(dynamic->modelInterpolator.getRotation(),calculatedOffset);
                else
                    return dynamic->modelInterpolator.getPosition();
        }
    }

    location::location(const location &src)
    {
        locations.push_back(this);
        type = src.type;
        fixedPos = src.fixedPos;
        brick = src.brick;
        car = src.car;
        dynamic = src.dynamic;
        calculatedOffset = src.calculatedOffset;
        useOffset = src.useOffset;
        direction = src.direction;
    }

    location::location()
    {
        locations.push_back(this);
    }
}
