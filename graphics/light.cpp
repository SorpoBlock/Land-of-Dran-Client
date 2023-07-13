#include "light.h"

namespace syj
{
    glm::vec3 lightSortPos = glm::vec3(0,0,0);

    bool sortLight(light *a,light *b)
    {
        return glm::distance2(a->position,lightSortPos) < glm::distance2(b->position,lightSortPos);
    }

    void light::clearAttachments(glm::vec3 _worldPos)
    {
        mode = worldPos;
        position = _worldPos;
        attachedBrick = 0;
        attachedCar = 0;
        attachedDynamic = 0;
        attachedItem = 0;
        node = "";
        attachOffset = glm::vec3(0,0,0);
    }

    void light::attachToCar(livingBrick *car,glm::vec3 offset)
    {
        clearAttachments();
        mode = carBrick;
        attachedCar = car;
        attachOffset = offset;
    }

    void light::attachToBrick(brickRenderData *brick)
    {
        clearAttachments();
        mode = brickPos;
        attachedBrick = brick;
    }

    void light::attachToDynamic(newDynamic *dynamic,std::string target)
    {
        clearAttachments();
        mode = dynamicNode;
        attachedDynamic = dynamic;
        node = target;
    }

    void light::attachToItem(item *dynamic,std::string target)
    {
        clearAttachments();
        mode = itemNode;
        attachedItem = dynamic;
        node = target;
    }

    void light::calcPos()
    {
        finalDirection = direction;

        switch(mode)
        {
            case worldPos: return;
            case brickPos:
                if(attachedBrick)
                    position = attachedBrick->position;
                return;
            case carBrick:
                if(attachedCar)
                {
                    glm::mat4 rotMat = glm::toMat4(attachedCar->carTransform.getRotation());
                    glm::vec4 off = glm::vec4(attachOffset.x,attachOffset.y,attachOffset.z,1.0);
                    off = rotMat * off;
                    position = attachedCar->carTransform.getPosition() + glm::vec3(off.x,off.y,off.z);
                    glm::vec4 dir = direction;
                    dir.w = 0;
                    glm::vec4 dirRes =  rotMat * dir;
                    finalDirection.x = dirRes.x;
                    finalDirection.y = dirRes.y;
                    finalDirection.z = dirRes.z;
                }
                return;
            case dynamicNode:
                if(attachedDynamic)
                {
                    position = attachedDynamic->modelInterpolator.getPosition();
                    if(node != "")
                    {
                        for(unsigned int a = 0; a<attachedDynamic->type->allMeshes.size(); a++)
                        {
                            newMesh *tmp = attachedDynamic->type->allMeshes[a];
                            if(tmp->name == node)
                            {
                                glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                                center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                                center *= attachedDynamic->scale;
                                center = multVec3ByMat(attachedDynamic->modelInterpolator.getRotation(),center);
                                position += center;
                                break;
                            }
                        }
                    }
                }
                return;
            case itemNode:
                if(attachedItem)
                {
                    position = attachedItem->modelInterpolator.getPosition();
                    if(node != "")
                    {
                        for(unsigned int a = 0; a<attachedItem->type->allMeshes.size(); a++)
                        {
                            newMesh *tmp = attachedItem->type->allMeshes[a];
                            if(tmp->name == node)
                            {
                                glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                                center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                                center *= attachedItem->scale;
                                center = multVec3ByMat(attachedItem->modelInterpolator.getRotation(),center);
                                position += center;
                                break;
                            }
                        }
                    }
                }
                return;
        }
    }

    void sortLights(std::vector<light*> &lightVec,glm::vec3 cameraPos)
    {
        for(unsigned int a = 0; a<lightVec.size(); a++)
            lightVec[a]->calcPos();

        auto iter = lightVec.begin();
        while(iter != lightVec.end())
        {
            light *l = *iter;
            if(l->lifeTimeMS > 0)
            {
                if(l->lifeTimeMS + l->createdTime < SDL_GetTicks())
                {
                    delete l;
                    iter = lightVec.erase(iter);
                    continue;
                }
            }

            ++iter;
        }

        lightSortPos = cameraPos;
        std::sort(lightVec.begin(),lightVec.end(),sortLight);
    }

    void renderLights(uniformsHolder &unis,std::vector<light*> &lightVec)
    {
        for(int a = 0; a<std::min(8,(int)lightVec.size()); a++)
        {
            glUniform1i(unis.pointLightUsed[a],true);
            glUniform3vec(unis.pointLightColor[a],lightVec[a]->color);
            glUniform3vec(unis.pointLightPos[a],lightVec[a]->position);
            glUniform1i(unis.pointLightIsSpotlight[a],lightVec[a]->isSpotlight);
            if(lightVec[a]->isSpotlight)
            {
                //std::cout<<"Is spotlight, rendering with: "<<lightVec[a]->direction.w<<"\n";
                glm::vec4 pass = glm::vec4(lightVec[a]->finalDirection.x,lightVec[a]->finalDirection.y,lightVec[a]->finalDirection.z,lightVec[a]->direction.w);
                glUniform4vec(unis.pointLightDirection[a],pass);
            }
        }

        for(int a = lightVec.size(); a<8; a++)
            glUniform1i(unis.pointLightUsed[a],false);
    }
}
