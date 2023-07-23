#include "recv.h"

enum serverToClientPacketType
{
    packetType_connectionRepsonse = 0,
    //packetType_addDynamic = 1,
    packetType_addRemoveDynamic = 1,
    packetType_addDynamicType = 2,
    packetType_addSpecialBrickType = 3,
    packetType_cameraDetails = 4,
    packetType_addBricks = 5,
    packetType_skipBricksCompile = 6,
    packetType_updateDynamicTransforms = 7,
    packetType_addRemoveLight = 8,
    //packetType_removeDynamic = 8,
    packetType_removeBrick = 9,
    packetType_addBricksToBuiltCar = 10,
    packetType_luaPasswordResponse = 11,
    packetType_setColorPalette = 12,
    //packetType_clientLuaConsoleText = 13,
    packetType_clientPhysicsData = 13,
    //packetType_addGUIText = 14,
    packetType_ping = 14,
    packetType_removeBrickVehicle = 15,
    packetType_setShapeName = 16,
    packetType_addItemType = 17,
    //packetType_addItem = 18,
    packetType_addRemoveItem = 18,
    packetType_addSoundType = 19,
    packetType_playSound = 20,
    packetType_forceWrenchDialog = 21,
    packetType_setInventory = 22,
    packetType_addOrRemovePlayer = 23,
    packetType_updateBrick = 24,
    packetType_debugLocations = 25,
    //packetType_removeItem = 26,
    packetType_addRemoveRope = 26,
    packetType_addMessage = 27,
    packetType_setNodeColors = 28,
    packetType_otherAsset = 29,
    packetType_newEmitterParticleType = 30,
    packetType_emitterAddRemove = 31
};

#define dynamicObjectIDBits 12

namespace syj
{
    void serverStuff::fatalNotify(std::string windowName,std::string windowText,std::string buttonText)
    {
        notify(windowName,windowText,buttonText);
        fatalNotifyStarted = true;
    }

    void finishLoadingTypesCheck(serverStuff *ohWow)
    {
        if(!ohWow->waitingForServerResponse)
            return;

        std::cout<<ohWow->newDynamicTypes.size()<<"-"<<ohWow->dynamicTypesToLoad<<" , "<<ohWow->staticBricks.blocklandTypes->specialBrickTypes.size()<<"-"<<ohWow->brickTypesToLoad<<" , "<<ohWow->itemTypes.size()<<"-"<<ohWow->itemTypesToLoad<<"\n";

        if(ohWow->newDynamicTypes.size() >= ohWow->dynamicTypesToLoad)
        {
            if(ohWow->staticBricks.blocklandTypes->specialBrickTypes.size() >= ohWow->brickTypesToLoad)
            {
                if(ohWow->itemTypes.size() >= ohWow->itemTypesToLoad)
                {
                    info("Finished loading types from server!");
                    ohWow->waitingForServerResponse = false;

                    ohWow->staticBricks.addBlocklandCompatibility(ohWow->staticBricks.blocklandTypes);

                    packet data;
                    data.writeUInt(clientPacketType_startLoadingStageTwo,4);
                    ohWow->connection->send(&data,true);

                    if(ohWow->newDynamicTypes.size() > 0)
                    {
                        std::cout<<"Starting avatar customizer...\n";

                        //TODO: Update avatar picker code
                        if(!ohWow->picker->playerModel)
                            ohWow->picker->playerModel = (model*)ohWow->newDynamicTypes[0]->oldModelType;

                        ohWow->picker->loadFromPrefs(ohWow->prefs);
                        ohWow->picker->sendAvatarPrefs(ohWow->connection,0);
                    }
                }
            }
        }
    }

    void checkForCameraToBind(serverStuff *ohWow)
    {
        if(!ohWow->cameraTarget && ohWow->boundToObject)
        {
            //std::cout<<"Checking for camera bind...\n";
            if(ohWow->cameraTargetServerID == -1)
                error("Camera bound to object but there's no object!");
            else
            {
                for(int a = 0; a<ohWow->newDynamics.size(); a++)
                {
                    if(ohWow->newDynamics[a]->serverID == ohWow->cameraTargetServerID)
                    {
                        //std::cout<<"Camera bound to "<<ohWow->cameraTargetServerID<<"\n";
                        ohWow->cameraTarget = ohWow->newDynamics[a];
                        //std::cout<<"Found the object "<<ohWow->cameraTarget->serverID<<"\n";
                        return;
                    }
                }
                //error("Could not find target for camera!");
            }
        }
    }

    int lastCalledTime = 0;

    void serverStuff::tryApplyHeldPackets()
    {
        //static int lastCalledTime;
        if(lastCalledTime + 300 > SDL_GetTicks())
            return;
        lastCalledTime = SDL_GetTicks();

        auto iter = heldAppearancePackets.begin();
        while(iter != heldAppearancePackets.end())
        {
            heldDynamicPacket *tmp = &(*iter);

            for(int a = 0; a<newDynamics.size(); a++)
            {
                if(newDynamics[a]->serverID == tmp->dynamicID)
                {
                    for(int b = 0; b<tmp->nodeColors.size(); b++)
                        newDynamics[a]->setNodeColor(tmp->nodeNames[b],tmp->nodeColors[b]);
                        //newDynamics[a]->meshColors[tmp->nodePickingIDs[b]] = tmp->nodeColors[b];
                    newDynamics[a]->decal = tmp->decal;

                    iter = heldAppearancePackets.erase(iter);
                    continue;
                }
            }

            if(tmp->deletionTime < SDL_GetTicks())
            {
                iter = heldAppearancePackets.erase(iter);
                continue;
            }

            ++iter;
        }

        auto iter2 = heldLightPackets.begin();
        while(iter2 != heldLightPackets.end())
        {
            heldLightPacket tmp = *iter2;

            bool good = false;
            for(int a = 0; a<livingBricks.size(); a++)
            {
                if(livingBricks[a]->serverID == tmp.brickCarID)
                {
                    std::cout<<"Attaching light to car from packet: "<<tmp.brickCarID<<"\n";
                    tmp.l->attachToCar(livingBricks[a],tmp.l->attachOffset);
                    lights.push_back(tmp.l);
                    good = true;
                    iter2 = heldLightPackets.erase(iter2);
                    break;
                }
            }
            if(good)
                continue;

            if(tmp.deletionTime < SDL_GetTicks())
                iter2 = heldLightPackets.erase(iter2);
            else
                iter2++;
        }

        auto iter3 = clientPhysicsPackets.begin();
        while(iter3 != clientPhysicsPackets.end())
        {
            heldClientPhysicsDataPacket tmp = *iter3;

            bool used = false;
            for(int a = 0; a<newDynamics.size(); a++)
            {
                if(newDynamics[a]->serverID == tmp.dynamicID)
                {
                    newDynamics[a]->createBoxBody(world,tmp.finalHalfExtents,tmp.finalOffset);
                    currentPlayer = newDynamics[a];
                    giveUpControlOfCurrentPlayer = false;

                    used = true;
                    iter3 = clientPhysicsPackets.erase(iter3);
                    break;
                }
            }
            if(used)
                continue;

            if(tmp.deletionTime < SDL_GetTicks())
                iter3 = clientPhysicsPackets.erase(iter3);
            else
                iter3++;
        }
    }

    void recvHandle(client *theClient,packet *data,void *userData)
    {
        serverToClientPacketType packetType = (serverToClientPacketType)data->readUInt(5);
        serverStuff *ohWow = (serverStuff*)userData;

        //if(packetType != 7)
            //std::cout<<"Got "<<(data->critical?"critical":"normal")<<" packet type: "<<packetType<<" streampos: "<<data->getStreamPos()<<" allocated bytes: "<<data->allocatedChunks<<"\n";
        switch(packetType)
        {
            case packetType_clientPhysicsData:
            {
                heldClientPhysicsDataPacket tmp;

                int subtype = data->readUInt(2);

                if(subtype == 0) //create client physics for dynamic or start using client physics for dynamic again
                {
                    tmp.dynamicID = data->readUInt(dynamicObjectIDBits);
                    tmp.deletionTime = SDL_GetTicks() + 30000;
                    tmp.finalHalfExtents.setX(data->readFloat());
                    tmp.finalHalfExtents.setY(data->readFloat());
                    tmp.finalHalfExtents.setZ(data->readFloat());
                    tmp.finalOffset.setX(data->readFloat());
                    tmp.finalOffset.setY(data->readFloat());
                    tmp.finalOffset.setZ(data->readFloat());

                    for(int a = 0; a<ohWow->clientPhysicsPackets.size(); a++)
                    {
                        if(ohWow->clientPhysicsPackets[a].dynamicID == tmp.dynamicID)
                        {
                            ohWow->clientPhysicsPackets.erase(ohWow->clientPhysicsPackets.begin() + a);
                            break;
                        }
                    }

                    ohWow->clientPhysicsPackets.push_back(tmp);
                }
                else if(subtype == 1) //pause using client physics for dynamic
                {
                    ohWow->giveUpControlOfCurrentPlayer = true;
                }
                else if(subtype == 2) //delete client physics for dynamic
                {
                    if(ohWow->currentPlayer && ohWow->currentPlayer->body)
                    {
                        ohWow->currentPlayer->world->removeRigidBody(ohWow->currentPlayer->body);
                        delete ohWow->currentPlayer->defaultMotionState;
                        delete ohWow->currentPlayer->shape;
                        ohWow->currentPlayer->body = 0;
                        ohWow->currentPlayer = 0;
                    }
                }
                else if(subtype == 3) //force update transform for dynamic with client physics
                {
                    float posX = data->readFloat();
                    float posY = data->readFloat();
                    float posZ = data->readFloat();
                    float rotW = data->readFloat();
                    float rotX = data->readFloat();
                    float rotY = data->readFloat();
                    float rotZ = data->readFloat();
                    float velX = data->readFloat();
                    float velY = data->readFloat();
                    float velZ = data->readFloat();

                    if(ohWow->currentPlayer && ohWow->currentPlayer->body)
                    {
                        btTransform t;
                        t.setIdentity();
                        t.setOrigin(btVector3(posX,posY,posZ));
                        t.setRotation(btQuaternion(rotX,rotY,rotZ,rotW));
                        ohWow->currentPlayer->body->setWorldTransform(t);
                        ohWow->currentPlayer->body->setLinearVelocity(btVector3(velX,velY,velZ));
                    }
                }

                return;
            }

            case packetType_addRemoveLight:
            {
                int id = data->readUInt(20);
                bool addOrRemove = data->readBit();

                if(addOrRemove)
                {
                    light *tmp = 0;
                    bool needNewLight = true;
                    for(int a = 0; a<ohWow->lights.size(); a++)
                    {
                        if(ohWow->lights[a]->serverID == id)
                        {
                            needNewLight = false;
                            tmp = ohWow->lights[a];
                            break;
                        }
                    }
                    if(needNewLight)
                        tmp = new light;

                    tmp->lifeTimeMS = data->readUInt(16);
                    tmp->mode = (syj::attachMode)data->readUInt(3);

                    if(tmp->mode == brickPos)
                    {
                        int brickID = data->readUInt(20);
                        bool isSpecial;
                        tmp->attachedBrick = ohWow->staticBricks.getBrick(brickID,isSpecial);
                        if(!tmp->attachedBrick)
                        {
                            delete tmp;
                            return;
                        }
                    }
                    else if(tmp->mode == worldPos)
                    {
                        float x = data->readFloat();
                        float y = data->readFloat();
                        float z = data->readFloat();
                        tmp->position = glm::vec3(x,y,z);
                    }
                    else if(tmp->mode == dynamicNode)
                    {
                        int dynamicID = data->readUInt(dynamicObjectIDBits);
                        for(int a = 0; a<ohWow->newDynamics.size(); a++)
                        {
                            if(ohWow->newDynamics[a]->serverID == dynamicID)
                            {
                                tmp->attachedDynamic = ohWow->newDynamics[a];
                                break;
                            }
                        }

                        if(!tmp->attachedDynamic)
                        {
                            delete tmp;
                            return;
                        }

                        tmp->node = data->readString();
                    }
                    else if(tmp->mode == itemNode)
                    {
                        int dynamicID = data->readUInt(dynamicObjectIDBits);
                        for(int a = 0; a<ohWow->items.size(); a++)
                        {
                            if(ohWow->items[a]->serverID == dynamicID)
                            {
                                tmp->attachedItem = ohWow->items[a];
                                break;
                            }
                        }

                        if(!tmp->attachedItem)
                        {
                            delete tmp;
                            return;
                        }

                        tmp->node = data->readString();
                    }
                    else if(tmp->mode == carBrick)
                    {
                        int carID = data->readUInt(10);
                        std::cout<<"Attaching light to car id: "<<carID<<"\n";
                        for(int a = 0; a<ohWow->livingBricks.size(); a++)
                        {
                            if(ohWow->livingBricks[a]->serverID == carID)
                            {
                                tmp->attachedCar = ohWow->livingBricks[a];
                                break;
                            }
                        }

                        if(!tmp->attachedCar)
                        {
                            std::cout<<"No attached car!\n";
                            heldLightPacket heldTmp;
                            heldTmp.deletionTime = SDL_GetTicks() + 30000;
                            heldTmp.brickCarID = carID;
                            heldTmp.l = tmp;
                            float x = data->readFloat();
                            float y = data->readFloat();
                            float z = data->readFloat();
                            tmp->attachOffset = glm::vec3(x,y,z);
                            float r = data->readFloat();
                            float g = data->readFloat();
                            float b = data->readFloat();
                            tmp->serverID = id;
                            tmp->createdTime = SDL_GetTicks();
                            tmp->color = glm::vec3(r,g,b);

                            tmp->isSpotlight = data->readBit();
                            if(tmp->isSpotlight)
                            {
                                tmp->direction.x = data->readFloat();
                                tmp->direction.y = data->readFloat();
                                tmp->direction.z = data->readFloat();
                                tmp->direction.w = cos(data->readFloat());
                                std::cout<<"Is spotlight! "<<tmp->direction.x<<","<<tmp->direction.y<<","<<tmp->direction.z<<","<<tmp->direction.w<<"\n";
                            }

                            ohWow->heldLightPackets.push_back(heldTmp);

                            return;
                        }

                        float x = data->readFloat();
                        float y = data->readFloat();
                        float z = data->readFloat();
                        tmp->attachOffset = glm::vec3(x,y,z);
                    }

                    float r = data->readFloat();
                    float g = data->readFloat();
                    float b = data->readFloat();
                    tmp->serverID = id;
                    tmp->createdTime = SDL_GetTicks();
                    tmp->color = glm::vec3(r,g,b);

                    tmp->isSpotlight = data->readBit();
                    if(tmp->isSpotlight)
                    {
                        tmp->direction.x = data->readFloat();
                        tmp->direction.y = data->readFloat();
                        tmp->direction.z = data->readFloat();
                        tmp->direction.w = cos(data->readFloat());
                    }

                    if(needNewLight)
                        ohWow->lights.push_back(tmp);
                }
                else
                {
                    for(int a = 0; a<ohWow->lights.size(); a++)
                    {
                        if(ohWow->lights[a]->serverID == id)
                        {
                            delete ohWow->lights[a];
                            ohWow->lights[a] = 0;
                            ohWow->lights.erase(ohWow->lights.begin() + a);
                            break;
                        }
                    }
                }

                return;
            }
            case packetType_addRemoveRope:
            {
                bool add = data->readBit();
                if(add)
                {
                    int id = data->readUInt(12);
                    int nodes = data->readUInt(8);

                    rope *tmp = new rope(id,nodes);
                    ohWow->ropes.push_back(tmp);
                }
                else
                {
                    int id = data->readUInt(12);

                    for(int a = 0; a<ohWow->ropes.size(); a++)
                    {
                        if(ohWow->ropes[a]->serverID == id)
                        {
                            delete ohWow->ropes[a];
                            ohWow->ropes[a] = 0;
                            ohWow->ropes.erase(ohWow->ropes.begin() + a);
                            return;
                        }
                    }
                }
                return;
            }
            case packetType_ping:
            {
                int id = data->readUInt(32);
                packet response;
                response.writeUInt(15,4);
                response.writeUInt(id,32);
                theClient->send(&response,true);
                return;
            }
            case packetType_emitterAddRemove:
            {
                if(data->readBit()) //adding, not removing
                {
                    int serverID = data->readUInt(20);
                    int vecPos = -1;

                    emitter *tmp = 0;
                    for(unsigned int a = 0; a<ohWow->emitters.size(); a++)
                    {
                        if(ohWow->emitters[a]->serverID == serverID)
                        {
                            vecPos = a;
                            tmp = ohWow->emitters[a];
                            break;
                        }
                    }

                    bool newEmitter = false;
                    if(!tmp)
                    {
                        vecPos = -1;
                        newEmitter = true;
                        tmp = new emitter;
                        tmp->serverID = serverID;
                    }

                    tmp->creationTime = SDL_GetTicks();

                    int typeID = data->readUInt(10);

                    bool found = false;
                    for(unsigned int a = 0; a<ohWow->emitterTypes.size(); a++)
                    {
                        if(ohWow->emitterTypes[a]->serverID == typeID)
                        {
                            tmp->type = ohWow->emitterTypes[a];
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                    {
                        if(vecPos != -1)
                            ohWow->emitters.erase(ohWow->emitters.begin() + vecPos);
                        delete tmp;
                        error("Could not find emitter type!");
                        return;
                    }

                    tmp->color.x = data->readFloat();
                    tmp->color.y = data->readFloat();
                    tmp->color.z = data->readFloat();
                    tmp->shootFromHand = data->readBit();

                    int attachedType = data->readUInt(2);
                    if(attachedType == 0)
                    {
                        int brickID = data->readUInt(20);
                        bool isSpecial;
                        brickRenderData *brick = ohWow->staticBricks.getBrick(brickID,isSpecial);
                        if(!brick)
                        {
                            if(vecPos != -1)
                                ohWow->emitters.erase(ohWow->emitters.begin() + vecPos);
                            delete tmp;
                            error("Could not find brick to attach emitter to!");
                            return;
                        }
                        if(isSpecial)
                            tmp->attachedToSpecialBrick = (specialBrickRenderData*)brick;
                        else
                            tmp->attachedToBasicBrick = (basicBrickRenderData*)brick;
                        tmp->justAttached = true;
                    }
                    else if(attachedType == 1)
                    {
                        int dynamicID = data->readUInt(dynamicObjectIDBits);
                        newDynamic *attached = 0;
                        for(unsigned int a = 0; a<ohWow->newDynamics.size(); a++)
                        {
                            if(dynamicID == ohWow->newDynamics[a]->serverID)
                            {
                                attached = ohWow->newDynamics[a];
                                break;
                            }
                        }
                        if(!attached)
                        {
                            if(vecPos != -1)
                                ohWow->emitters.erase(ohWow->emitters.begin() + vecPos);
                            delete tmp;
                            error("Could not find dynamic to attach emitter to!");
                            return;
                        }
                        tmp->attachedToModel = attached;
                        tmp->meshName = data->readString();
                        tmp->justAttached = true;
                    }
                    else if(attachedType == 2)
                    {
                        tmp->position.x = data->readFloat();
                        tmp->position.y = data->readFloat();
                        tmp->position.z = data->readFloat();
                        tmp->justAttached = false;
                    }
                    else
                    {
                        if(vecPos != -1)
                            ohWow->emitters.erase(ohWow->emitters.begin() + vecPos);
                        delete tmp;
                        error("Bad attachment type for emitter.");
                        return;
                    }
                    if(vecPos == -1)
                        ohWow->emitters.push_back(tmp);
                }
                else
                {
                    int serverID = data->readUInt(20);
                    for(unsigned int a = 0; a<ohWow->emitters.size(); a++)
                    {
                        if(ohWow->emitters[a]->serverID == serverID)
                        {
                            delete ohWow->emitters[a];
                            ohWow->emitters[a] = 0;
                            ohWow->emitters.erase(ohWow->emitters.begin() + a);
                            return;
                        }
                    }
                }
                return;
            }
            case packetType_newEmitterParticleType:
            {
                if(data->readBit()) //Emitter:
                {
                    int serverID = data->readUInt(10);

                    bool foundEmitterType = false;
                    emitterType *tmp = 0;

                    for(unsigned int a = 0; a<ohWow->emitterTypes.size(); a++)
                    {
                        if(ohWow->emitterTypes[a]->serverID == serverID)
                        {
                            tmp = ohWow->emitterTypes[a];
                            foundEmitterType = true;
                            break;
                        }
                    }

                    if(!tmp)
                    {
                        tmp = new emitterType;
                        tmp->serverID = serverID;
                    }

                    int howMany = data->readUInt(8);
                    for(int a = 0; a<howMany; a++)
                    {
                        particleType *found = 0;
                        int particleID = data->readUInt(10);
                        for(unsigned int b = 0; b<ohWow->particleTypes.size(); b++)
                        {
                            if(particleID == ohWow->particleTypes[b]->serverID)
                            {
                                found = ohWow->particleTypes[b];
                                break;
                            }
                        }
                        if(!found)
                        {
                            error("Could not find particle type for emitter!");
                            continue;
                        }
                        tmp->particleTypes.push_back(found);
                    }
                    tmp->ejectionOffset = data->readFloat();
                    tmp->ejectionPeriodMS = data->readFloat();
                    tmp->ejectionVelocity = data->readFloat();
                    tmp->periodVarianceMS = data->readFloat();
                    tmp->phiReferenceVel = data->readFloat();
                    tmp->phiVariance = data->readFloat();
                    tmp->thetaMin = data->readFloat();
                    tmp->thetaMax = data->readFloat();
                    tmp->velocityVariance = data->readFloat();
                    tmp->lifetimeMS = data->readFloat();
                    if(data->readBit())
                    {
                        tmp->uiName = data->readString();
                        if(tmp->uiName == "Wrench Oil")
                            ohWow->wheelDirtEmitter = tmp;
                    }

                    if(!foundEmitterType)
                        ohWow->emitterTypes.push_back(tmp);
                }
                else //Particle:
                {
                    int serverID = data->readUInt(10);

                    particleType *tmp = 0;
                    bool foundParticleType = false;

                    for(unsigned int a = 0; a<ohWow->particleTypes.size(); a++)
                    {
                        if(ohWow->particleTypes[a]->serverID == serverID)
                        {
                            tmp = ohWow->particleTypes[a];
                            foundParticleType = true;
                            break;
                        }
                    }

                    if(!tmp)
                    {
                        tmp = new particleType;
                        tmp->serverID = serverID;
                    }

                    tmp->baseTexture = new texture;
                    tmp->baseTexture->createFromFile(data->readString());
                    for(int a = 0; a<4; a++)
                    {
                        tmp->colors[a].r = data->readFloat();
                        tmp->colors[a].g = data->readFloat();
                        tmp->colors[a].b = data->readFloat();
                        tmp->colors[a].a = data->readFloat();
                        tmp->sizes[a] = data->readFloat();
                        tmp->times[a] = data->readFloat();
                    }
                    tmp->dragCoefficient.x = data->readFloat();
                    tmp->dragCoefficient.y = data->readFloat();
                    tmp->dragCoefficient.z = data->readFloat();
                    tmp->gravityCoefficient.x = data->readFloat();
                    tmp->gravityCoefficient.y = data->readFloat();
                    tmp->gravityCoefficient.z = data->readFloat();
                    float inheritedVelFactor = data->readFloat();
                    tmp->inheritedVelFactor = glm::vec3(inheritedVelFactor,inheritedVelFactor,inheritedVelFactor);
                    tmp->lifetimeMS = data->readFloat();
                    tmp->lifetimeVarianceMS = data->readFloat();
                    tmp->spinSpeed = data->readFloat();
                    tmp->useInvAlpha = data->readBit();
                    tmp->needSorting = data->readBit();

                    if(!foundParticleType)
                        ohWow->particleTypes.push_back(tmp);
                }
                return;
            }
            case packetType_otherAsset:
            {
                int howMany = data->readUInt(7);
                //std::cout<<"How many other assets: "<<howMany<<"\n";
                for(int a = 0; a<howMany; a++)
                {
                    std::string fileName = data->readString();
                    ohWow->picker->addDecalToPicker(fileName);
                    //std::cout<<a<<" file name: "<<fileName<<"\n";
                }
                return;
            }
            case packetType_setNodeColors:
            {
                int dynamicID = data->readUInt(dynamicObjectIDBits);

                int decal = data->readUInt(7);
                //std::cout<<"Decal: "<<decal<<"\n";

                newDynamic *tmp = 0;

                for(unsigned int a = 0; a<ohWow->newDynamics.size(); a++)
                {
                    if(ohWow->newDynamics[a]->serverID == dynamicID)
                    {
                        tmp = ohWow->newDynamics[a];
                        break;
                    }
                }

                heldDynamicPacket *placeHolder = 0;
                if(!tmp)
                {
                    for(int a = 0; a<ohWow->heldAppearancePackets.size(); a++)
                    {
                        if(ohWow->heldAppearancePackets[a].dynamicID == dynamicID)
                        {
                            placeHolder = &ohWow->heldAppearancePackets[a];
                            break;
                        }
                    }

                    if(!placeHolder)
                    {
                        ohWow->heldAppearancePackets.push_back(heldDynamicPacket());
                        placeHolder = &ohWow->heldAppearancePackets[ohWow->heldAppearancePackets.size()-1];
                    }
                }

                if(placeHolder)
                    placeHolder->deletionTime = SDL_GetTicks() + 1000;

                if(decal >= 0 && decal < ohWow->picker->faceDecals.size())
                {
                    if(tmp)
                        tmp->decal = ohWow->picker->faceDecals[decal];
                    else if(placeHolder)
                        placeHolder->decal = ohWow->picker->faceDecals[decal];
                }

                unsigned int howManyUpdates = data->readUInt(7);

                //std::cout<<"Node colors packet: "<<dynamicID<<" "<<decal<<" how many: "<<howManyUpdates<<"\n";

                for(unsigned int a = 0; a<howManyUpdates; a++)
                {
                    std::string nodeName = data->readString();
                    float r = data->readUInt(8);
                    r /= 255.0;
                    float g = data->readUInt(8);
                    g /= 255.0;
                    float b = data->readUInt(8);
                    b /= 255.0;

                    if(tmp)
                        tmp->setNodeColor(nodeName,glm::vec3(r,g,b));
                    else if(placeHolder)
                    {
                        placeHolder->nodeColors.push_back(glm::vec3(r,g,b));
                        placeHolder->nodeNames.push_back(nodeName);
                    }
                }

                return;
            }
            case packetType_addMessage:
            {
                //0 = chat message
                //1 = bottom print
                //2 = eval window output
                int messageLocation = data->readUInt(2);

                if(messageLocation == 1)
                {
                    std::string text = data->readString();
                    int timeoutMS = data->readUInt(16);
                    ohWow->bottomPrint.setText(text,timeoutMS);
                    return;
                }

                if(messageLocation == 2)
                {
                    std::string text = data->readString();
                    textBoxAdd(ohWow->evalWindow->getChild("Code/Listbox"),text);
                    return;
                }

                std::string message = data->readString();
                std::string category = data->readString();

                float chatScroll = ohWow->chat->getVertScrollbar()->getScrollPosition();
                float maxScroll = ohWow->chat->getVertScrollbar()->getDocumentSize() - ohWow->chat->getVertScrollbar()->getPageSize();

                textBoxAdd(ohWow->chat,message,0,false);

                if(ohWow->chat->getItemCount() > 100)
                {
                    CEGUI::ListboxItem *line = ohWow->chat->getListboxItemFromIndex(0);
                    if(line)
                        ohWow->chat->removeItem(line);
                }

                if(chatScroll >= maxScroll)
                    ohWow->chat->getVertScrollbar()->setScrollPosition(ohWow->chat->getVertScrollbar()->getDocumentSize() - ohWow->chat->getVertScrollbar()->getPageSize());

                //std::cout<<"Chat message added: "<<message<<"| |"<<category<<"\n";

                return;
            }
            case packetType_debugLocations:
            {
                std::cout<<"Debug locations...\n";

                ohWow->debugColors.clear();
                ohWow->debugLocations.clear();

                int howMany = data->readUInt(6);
                for(int a = 0; a<howMany; a++)
                {
                    float r = data->readFloat();
                    float g = data->readFloat();
                    float b = data->readFloat();
                    float x = data->readFloat();
                    float y = data->readFloat();
                    float z = data->readFloat();

                    ohWow->debugColors.push_back(glm::vec3(r,g,b));
                    ohWow->debugLocations.push_back(glm::vec3(x,y,z));
                }

                return;
            }
            case packetType_updateBrick:
            {
                int id = data->readUInt(20);

                float r = data->readUInt(8);
                r /= 255.0;
                float g = data->readUInt(8);
                g /= 255.0;
                float blu = data->readUInt(8);
                blu /= 255.0;
                float al = data->readUInt(8);
                al /= 255.0;

                float x = data->readUInt(14);
                x -= 8192;
                x += data->readBit() ? 0.5 : 0;
                float y = data->readUInt(16);
                y += data->readBit() ? 0.5 : 0;
                y *= 0.4;
                float z = data->readUInt(14);
                z -= 8192;
                z += data->readBit() ? 0.5 : 0;

                int angleID = data->readUInt(2);
                int shapeFx = data->readUInt(4);
                int material = data->readUInt(4);
                if(shapeFx == 1)
                    material += 1000;
                else if(shapeFx == 2)
                    material += 2000;

                for(int b = 0; b<ohWow->staticBricks.opaqueBasicBricks.size(); b++)
                {
                    if(!ohWow->staticBricks.opaqueBasicBricks[b])
                        continue;
                    if(ohWow->staticBricks.opaqueBasicBricks[b]->serverID == id)
                    {
                        float oldTrans = ohWow->staticBricks.opaqueBasicBricks[b]->color.a;
                        bool changedTrans = (oldTrans < 0.99 && al > 0.99) || (oldTrans > 0.99 && al < 0.99);
                        ohWow->staticBricks.opaqueBasicBricks[b]->color = glm::vec4(r,g,blu,al);
                        ohWow->staticBricks.opaqueBasicBricks[b]->position = glm::vec3(x,y,z);
                        ohWow->staticBricks.opaqueBasicBricks[b]->rotation = ohWow->staticBricks.rotations[angleID];
                        ohWow->staticBricks.opaqueBasicBricks[b]->material = material;
                        ohWow->staticBricks.updateBasicBrick(ohWow->staticBricks.opaqueBasicBricks[b],changedTrans);
                        return;
                    }
                }

                for(int b = 0; b<ohWow->staticBricks.transparentBasicBricks.size(); b++)
                {
                    if(!ohWow->staticBricks.transparentBasicBricks[b])
                        continue;
                    if(ohWow->staticBricks.transparentBasicBricks[b]->serverID == id)
                    {
                        float oldTrans = ohWow->staticBricks.opaqueBasicBricks[b]->color.a;
                        bool changedTrans = (oldTrans < 0.99 && al > 0.99) || (oldTrans > 0.99 && al < 0.99);
                        ohWow->staticBricks.transparentBasicBricks[b]->color = glm::vec4(r,g,blu,al);
                        ohWow->staticBricks.transparentBasicBricks[b]->position = glm::vec3(x,y,z);
                        ohWow->staticBricks.transparentBasicBricks[b]->rotation = ohWow->staticBricks.rotations[angleID];
                        ohWow->staticBricks.transparentBasicBricks[b]->material = material;
                        ohWow->staticBricks.updateBasicBrick(ohWow->staticBricks.transparentBasicBricks[b],changedTrans);
                        return;
                    }
                }

                for(int b = 0; b<ohWow->staticBricks.opaqueSpecialBricks.size(); b++)
                {
                    if(!ohWow->staticBricks.opaqueSpecialBricks[b])
                        continue;
                    if(ohWow->staticBricks.opaqueSpecialBricks[b]->serverID == id)
                    {
                        ohWow->staticBricks.opaqueSpecialBricks[b]->color = glm::vec4(r,g,blu,al);
                        ohWow->staticBricks.opaqueSpecialBricks[b]->position = glm::vec3(x,y,z);
                        ohWow->staticBricks.opaqueSpecialBricks[b]->rotation = ohWow->staticBricks.rotations[angleID];
                        ohWow->staticBricks.opaqueSpecialBricks[b]->material = material;
                        ohWow->staticBricks.updateSpecialBrick(ohWow->staticBricks.opaqueSpecialBricks[b],ohWow->world,angleID);
                        return;
                    }
                }

                for(int b = 0; b<ohWow->staticBricks.transparentSpecialBricks.size(); b++)
                {
                    if(!ohWow->staticBricks.transparentSpecialBricks[b])
                        continue;
                    if(ohWow->staticBricks.transparentSpecialBricks[b]->serverID == id)
                    {
                        ohWow->staticBricks.transparentSpecialBricks[b]->color = glm::vec4(r,g,blu,al);
                        ohWow->staticBricks.transparentSpecialBricks[b]->position = glm::vec3(x,y,z);
                        ohWow->staticBricks.transparentSpecialBricks[b]->rotation = ohWow->staticBricks.rotations[angleID];
                        ohWow->staticBricks.transparentSpecialBricks[b]->material = material;
                        ohWow->staticBricks.updateSpecialBrick(ohWow->staticBricks.transparentSpecialBricks[b],ohWow->world,angleID);
                        return;
                    }
                }

                return;
            }
            case packetType_addOrRemovePlayer:
            {
                bool forPlayersList = data->readBit();
                bool addOrRemove = data->readBit();
                std::string name = data->readString();
                int id = data->readUInt(16);

                if(name.length() < 1)
                {
                    error("Add or remove player, name was 0 characters.");
                    return;
                }

                if(forPlayersList)
                {
                    CEGUI::MultiColumnList *playerList = (CEGUI::MultiColumnList*)ohWow->playerList->getChild("List");

                    if(!playerList)
                    {
                        error("Could not find player list!");
                        return;
                    }

                    if(addOrRemove)
                    {
                        CEGUI::ListboxItem *entry = playerList->findListItemWithText(name,NULL);
                        if(entry)
                            return;

                        int idx = playerList->getRowCount();
                        playerList->addRow(idx);

                        CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem(name,idx);
                        cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
                        cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
                        cell->setID(idx);
                        playerList->setItem(cell,0,idx);
                    }
                    else
                    {
                        CEGUI::ListboxItem *entry = playerList->findListItemWithText(name,NULL);
                        std::cout<<name<<" "<<entry<<" "<<entry->getID()<<"\n";
                        if(entry)
                        {
                            //playerList->removeRow(entry->getID());
                            int rowToRemove = playerList->getRowWithID(entry->getID());
                            if(rowToRemove < 0 || rowToRemove >= playerList->getRowCount())
                                error("Player list row to remove is " + std::to_string(rowToRemove) + " num rows " + std::to_string(playerList->getRowCount()));
                            else
                                playerList->removeRow(rowToRemove);
                        }
                    }
                }
                else
                {
                    for(unsigned int a = 0; a<ohWow->typingPlayersNames.size(); a++)
                    {
                        if(ohWow->typingPlayersNames[a] == name)
                        {
                            if(!addOrRemove)
                            {
                                ohWow->typingPlayersNames.erase(ohWow->typingPlayersNames.begin() + a);
                                ohWow->typingPlayerNameString = "";
                                for(unsigned int b = 0; b<ohWow->typingPlayersNames.size(); b++)
                                    ohWow->typingPlayerNameString = ohWow->typingPlayerNameString + ohWow->typingPlayersNames[b] + " ";
                                ohWow->whosTyping->setText(ohWow->typingPlayerNameString);
                            }
                            return;
                        }
                    }

                    ohWow->typingPlayersNames.push_back(name);
                    ohWow->typingPlayerNameString = "";
                    for(unsigned int b = 0; b<ohWow->typingPlayersNames.size(); b++)
                        ohWow->typingPlayerNameString = ohWow->typingPlayerNameString + ohWow->typingPlayersNames[b] + " ";
                    ohWow->whosTyping->setText(ohWow->typingPlayerNameString);
                }

                return;
            }
            case packetType_setInventory:
            {
                int slot = data->readUInt(3);

                if(slot >= inventorySize)
                {
                    error("Slot greater than inventory size! " + std::to_string(slot));
                    return;
                }

                bool hasItem = data->readBit();
                if(!hasItem)
                {
                    ohWow->inventoryBox->getChild("ItemName" + std::to_string(slot+1))->setText("");
                    ohWow->inventory[slot] = 0;
                    return;
                }
                else
                {
                    int itemType = data->readUInt(10);

                    for(unsigned int a = 0; a<ohWow->itemTypes.size(); a++)
                    {
                        if(ohWow->itemTypes[a]->serverID == itemType)
                        {
                            ohWow->inventoryBox->getChild("ItemName" + std::to_string(slot+1))->setText(ohWow->itemTypes[a]->uiName);
                            ohWow->inventory[slot] = ohWow->itemTypes[a];
                            return;
                        }
                    }

                    error("Could not find item for inventory with type " + std::to_string(itemType));
                }


                return;
            }
            case packetType_forceWrenchDialog:
            {
                bool isWheel = data->readBit();
                if(isWheel)
                {
                    ohWow->context->setMouseLock(false);
                    ohWow->wheelWrench->moveToFront();
                    ohWow->wheelWrench->setVisible(true);

                    if(!((CEGUI::ToggleButton*)ohWow->wheelWrench->getChild("Copy"))->isSelected())
                    {
                        ohWow->wheelWrench->getChild("BreakForce")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("SteeringForce")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("EngineForce")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("SuspensionLength")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("SuspensionStiffness")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("Friction")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("RollInfluence")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("DampingCompression")->setText(std::to_string(data->readFloat()));
                        ohWow->wheelWrench->getChild("DampingRelaxation")->setText(std::to_string(data->readFloat()));
                    }

                    return;
                }

                bool isSteering = data->readBit();
                if(isSteering)
                {
                    ohWow->context->setMouseLock(false);
                    ohWow->steeringWrench->moveToFront();
                    ohWow->steeringWrench->setVisible(true);

                    ohWow->steeringWrench->getChild("Mass")->setText(std::to_string(data->readFloat()));
                    ohWow->steeringWrench->getChild("Damping")->setText(std::to_string(data->readFloat()));
                    ((CEGUI::ToggleButton*)ohWow->steeringWrench->getChild("CenterMass"))->setSelected(data->readBit());

                    return;
                }

                bool colliding = data->readBit();

                bool renderUp = !data->readBit();
                bool renderDown = !data->readBit();
                bool renderNorth = !data->readBit();
                bool renderSouth = !data->readBit();
                bool renderEast = !data->readBit();
                bool renderWest = !data->readBit();

                int musicId = data->readUInt(10);
                float pitch = data->readFloat();
                std::string name = data->readString();


                ohWow->context->setMouseLock(false);
                ohWow->wrench->setVisible(true);
                ohWow->wrench->moveToFront();
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("Colliding"))->setSelected(colliding);
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("RenderDown"))->setSelected(renderDown);
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("RenderUp"))->setSelected(renderUp);
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("RenderNorth"))->setSelected(renderNorth);
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("RenderSouth"))->setSelected(renderSouth);
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("RenderEast"))->setSelected(renderEast);
                ((CEGUI::ToggleButton*)ohWow->wrench->getChild("RenderWest"))->setSelected(renderWest);
                //((CEGUI::Combobox*)ohWow->wrench->getChild("MusicDropdown"))->setItemSelectState(musicId,true);
                CEGUI::Combobox *musicDropdown = ((CEGUI::Combobox*)ohWow->wrench->getChild("MusicDropdown"));
                CEGUI::Slider *pitchSlider = ((CEGUI::Slider*)ohWow->wrench->getChild("PitchSlider"));
                pitchSlider->setCurrentValue(pitch*100);
                ohWow->wrench->getChild("BrickName")->setText(name);

                bool hasLight = data->readBit();
                if(hasLight)
                {
                    ((CEGUI::ToggleButton*)ohWow->wrench->getChild("UseLight"))->setSelected(true);

                    float red = data->readFloat();
                    float green = data->readFloat();
                    float blue = data->readFloat();

                    ohWow->wrench->getChild("Red")->setText(std::to_string(red));
                    ohWow->wrench->getChild("Green")->setText(std::to_string(green));
                    ohWow->wrench->getChild("Blue")->setText(std::to_string(blue));

                    bool isSpotlight = data->readBit();
                    if(isSpotlight)
                    {
                        ((CEGUI::ToggleButton*)ohWow->wrench->getChild("IsSpotlight"))->setSelected(true);
                        float phi = data->readFloat();
                        float yaw = data->readFloat();
                        float lightPitch = data->readFloat();

                        CEGUI::Slider *phiSlider = ((CEGUI::Slider*)ohWow->wrench->getChild("PhiSlider"));
                        phiSlider->setCurrentValue(phi);
                        CEGUI::Slider *yawSlider = ((CEGUI::Slider*)ohWow->wrench->getChild("YawSlider"));
                        yawSlider->setCurrentValue(yaw);
                        CEGUI::Slider *lightPitchSlider = ((CEGUI::Slider*)ohWow->wrench->getChild("LightPitchSlider"));
                        lightPitchSlider->setCurrentValue(lightPitch);
                    }
                    else
                        ((CEGUI::ToggleButton*)ohWow->wrench->getChild("IsSpotlight"))->setSelected(false);
                }
                else
                    ((CEGUI::ToggleButton*)ohWow->wrench->getChild("UseLight"))->setSelected(false);

                musicDropdown->clearAllSelections();

                std::string text = "";
                for(unsigned int a = 0; a<ohWow->speaker->sounds.size(); a++)
                {
                    if(ohWow->speaker->sounds[a]->serverID == musicId)
                    {
                        CEGUI::ListboxItem *item = musicDropdown->findItemWithText(ohWow->speaker->sounds[a]->scriptName,NULL);
                        if(!item)
                            error("Error selecting music " + ohWow->speaker->sounds[a]->scriptName);
                        else
                            musicDropdown->setItemSelectState(item,true);
                        return;
                    }
                }

                return;
            }
            case packetType_addSoundType:
            {
                std::string file = data->readString();
                std::string name = data->readString();

                int id = data->readUInt(10);
                bool isMusic = data->readBit();

                if(name.length() == 0)
                    return;

                ohWow->speaker->loadSound(id,file,name,isMusic);
                return;
            }
            case packetType_playSound:
            {
                int id = data->readUInt(10);
                bool loop = data->readBit();

                if(loop)
                {
                    int loopId = data->readUInt(5);
                    float pitch = data->readFloat();
                    bool onCar = data->readBit();
                    if(onCar)
                    {
                        std::cout<<"Received on car music loop packet!\n";
                        int carId = data->readUInt(10);
                        livingBrick *car = 0;
                        for(unsigned int a = 0; a<ohWow->livingBricks.size(); a++)
                        {
                            if(ohWow->livingBricks[a]->serverID == carId)
                            {
                                car = ohWow->livingBricks[a];
                                break;
                            }
                        }
                        if(!car)
                        {
                            error("Could not find car " + std::to_string(carId) + " to attach music!");
                            return;
                        }
                        ohWow->speaker->loopSound(id,loopId,car,pitch);
                    }
                    else
                    {
                        float x = data->readFloat();
                        float y = data->readFloat();
                        float z = data->readFloat();
                        ohWow->speaker->loopSound(id,loopId,x,y,z,pitch);
                    }
                    return;
                }

                bool flatSound = data->readBit();
                if(flatSound)
                {
                    ohWow->speaker->playSound(id,loop);
                    return;
                }

                bool attached = data->readBit();
                if(attached)
                {
                    int objectId = data->readUInt(10); //no clue if this will end up being dynamicObjectIDBits or just a car ID
                }
                else
                {
                    float x = data->readFloat();
                    float y = data->readFloat();
                    float z = data->readFloat();
                    ohWow->speaker->playSound(id,loop,x,y,z);
                }
                return;
            }
            case packetType_addRemoveItem:
            {
                bool add = data->readBit();

                if(add)
                {
                    int typeId = data->readUInt(10);
                    int itemId = data->readUInt(dynamicObjectIDBits);

                    heldItemType *type = 0;
                    for(unsigned int a = 0; a<ohWow->itemTypes.size(); a++)
                    {
                        if(ohWow->itemTypes[a]->serverID == typeId)
                        {
                            type = ohWow->itemTypes[a];
                            break;
                        }
                    }

                    if(!type)
                    {
                        error("Could not find item type " + std::to_string(typeId));
                        return;
                    }

                    item *tmp = new item(ohWow->world,type->type,glm::vec3(0.03));
                    tmp->serverID = itemId;
                    tmp->itemType = type;
                    ohWow->items.push_back(tmp);
                }
                else
                {
                    int itemId = data->readUInt(dynamicObjectIDBits);

                    for(unsigned int a = 0; a<ohWow->items.size(); a++)
                    {
                        if(ohWow->items[a]->serverID == itemId)
                        {
                            delete ohWow->items[a];
                            ohWow->items[a] = 0;
                            ohWow->items.erase(ohWow->items.begin() + a);
                            return;
                        }
                    }

                    error("Could not find item to remove: " + std::to_string(itemId));
                }

                return;
            }
            case packetType_addItemType:
            {
                int itemId = data->readUInt(10);
                int modelId = data->readUInt(10);
                bool defaultSwing = data->readBit();
                int swingAnim = -1;
                if(!defaultSwing)
                {
                    if(data->readBit())
                        swingAnim = data->readUInt(8);
                    else
                        swingAnim = -1;
                }
                int switchAnim = -1;
                if(data->readBit())
                    switchAnim = data->readUInt(8);
                else
                    switchAnim = -1;

                std::string uiName = data->readString();

                float x = data->readFloat();
                float y = data->readFloat();
                float z = data->readFloat();

                std::cout<<"Added item type: "<<uiName<<"\n";

                for(unsigned int a = 0; a<ohWow->newDynamicTypes.size(); a++)
                {
                    if(ohWow->newDynamicTypes[a]->serverID == modelId)
                    {
                        heldItemType *tmp = new heldItemType;
                        tmp->useDefaultSwing = defaultSwing;
                        tmp->serverID = itemId;
                        tmp->type = ohWow->newDynamicTypes[a];
                        tmp->handOffset = glm::vec3(x,y,z);
                        tmp->uiName = uiName;
                        tmp->waitingForModel = false;
                        for(unsigned int b = 0; b<ohWow->newDynamicTypes[a]->animations.size(); b++)
                        {
                            if(ohWow->newDynamicTypes[a]->animations[b].serverID == swingAnim)
                            {
                                tmp->fireAnim = &ohWow->newDynamicTypes[a]->animations[b];
                            }
                            if(ohWow->newDynamicTypes[a]->animations[b].serverID == switchAnim)
                            {
                                tmp->switchAnim = &ohWow->newDynamicTypes[a]->animations[b];
                            }
                        }
                        ohWow->itemTypes.push_back(tmp);

                        if(uiName == std::string("Paint Can"))
                        {
                            ohWow->paintCan = tmp;
                            ohWow->fixedPaintCanItem = new item(ohWow->world,tmp->type,glm::vec3(0.02,0.02,0.02));
                            ohWow->fixedPaintCanItem->itemType = tmp;
                            ohWow->items.push_back(ohWow->fixedPaintCanItem);
                        }

                        finishLoadingTypesCheck(ohWow);
                        return;
                    }
                }

                heldItemType *tmp = new heldItemType;
                tmp->useDefaultSwing = defaultSwing;
                tmp->serverID = itemId;
                tmp->type = 0;
                tmp->handOffset = glm::vec3(x,y,z);
                tmp->uiName = uiName;
                tmp->waitingForModel = true;
                tmp->waitingForSwingAnim = swingAnim;
                tmp->waitingForSwitchAnim = switchAnim;
                tmp->waitingModel = modelId;
                ohWow->itemTypes.push_back(tmp);
                if(uiName == "Paint Can")
                {
                    ohWow->paintCan = tmp;
                    ohWow->fixedPaintCanItem = new item(ohWow->world,tmp->type,glm::vec3(0.02,0.02,0.02));
                    ohWow->fixedPaintCanItem->itemType = tmp;
                    ohWow->items.push_back(ohWow->fixedPaintCanItem);
                }

                error("Could not add item with model ID: " + std::to_string(modelId));

                finishLoadingTypesCheck(ohWow);

                return;
            }
            case packetType_setShapeName:
            {
                int id = data->readUInt(dynamicObjectIDBits);
                std::string shapeName = data->readString();
                float r = data->readFloat();
                float g = data->readFloat();
                float b = data->readFloat();

                for(int a = 0; a<ohWow->newDynamics.size(); a++)
                {
                    if(ohWow->newDynamics[a]->serverID == id)
                    {
                        ohWow->newDynamics[a]->shapeName = shapeName;
                        ohWow->newDynamics[a]->shapeNameColor = glm::vec3(r,g,b);
                        //ohWow->newDynamics[a]->colorTint = glm::vec3(r,g,b);
                        return;
                    }
                }

                error("Could not find id " + std::to_string(id) + " to add shape name!");
                return;
            }
            case packetType_removeBrickVehicle:
            {
                int id = data->readUInt(10);
                for(int a = 0; a<ohWow->livingBricks.size(); a++)
                {
                    if(ohWow->livingBricks[a]->serverID == id)
                    {
                        for(unsigned int b = 0; b<32; b++)
                        {
                            if(ohWow->speaker->carToTrack[b] == ohWow->livingBricks[a])
                            {
                                alSourceStop(ohWow->speaker->sources[b]);
                                ohWow->speaker->carToTrack[b] = 0;
                            }
                        }

                        auto emitterIter = ohWow->emitters.begin();
                        while(emitterIter != ohWow->emitters.end())
                        {
                            emitter *e = *emitterIter;
                            if(e->attachedToCar == ohWow->livingBricks[a])
                            {
                                delete e;
                                e = 0;
                                emitterIter = ohWow->emitters.erase(emitterIter);
                                continue;
                            }
                            ++emitterIter;
                        }

                        delete ohWow->livingBricks[a];
                        ohWow->livingBricks.erase(ohWow->livingBricks.begin() + a);
                        return;
                    }
                }
                return;
            }
            case packetType_setColorPalette:
            {
                int howMany = data->readUInt(8);
                //std::cout<<"Colors: "<<howMany<<"\n";
                for(int a = 0; a<howMany; a++)
                {
                    int paletteIdx = data->readUInt(6);
                    float r = data->readUInt(8);
                    float g = data->readUInt(8);
                    float b = data->readUInt(8);
                    float al = data->readUInt(8);
                    r /= 255;
                    b /= 255;
                    g /= 255;
                    al /= 255;
                    //std::cout<<paletteIdx<<" is: "<<r<<","<<g<<","<<b<<","<<al<<"\n";
                    if(paletteIdx >= 40)
                        continue;
                    ohWow->palette->setColor(paletteIdx,glm::vec4(r,g,b,al));
                }
                return;
            }
            case packetType_luaPasswordResponse:
            {
                bool correct = data->readBit();
                if(correct)
                {
                    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
                    CEGUI::Window *evalWindow = root->getChild("HUD/EvalWindow");
                    evalWindow->getChild("Login")->setVisible(false);
                    evalWindow->getChild("Code")->setVisible(true);
                }
                else
                    error("You guessed the wrong password!");
                return;
            }
            case packetType_addBricksToBuiltCar:
            {
                int id = data->readUInt(10);
                int wheels = data->readUInt(5);
                extraWheelData *wheelBrickData = 0;
                float *radii = 0;
                if(wheels > 0)
                {
                    wheelBrickData = new extraWheelData[wheels];
                    radii = new float[wheels];
                    for(int a = 0; a<wheels; a++)
                    {
                        radii[a] = data->readFloat();

                        wheelBrickData[a].wheelScale = radii[a];

                        wheelBrickData[a].carX = data->readFloat();
                        wheelBrickData[a].carY = data->readFloat();
                        wheelBrickData[a].carZ = data->readFloat();

                        wheelBrickData[a].breakForce = data->readFloat();
                        wheelBrickData[a].steerAngle = data->readFloat();
                        wheelBrickData[a].engineForce = data->readFloat();
                        wheelBrickData[a].suspensionLength = data->readFloat();
                        wheelBrickData[a].suspensionStiffness = data->readFloat();
                        wheelBrickData[a].dampingCompression = data->readFloat();
                        wheelBrickData[a].dampingRelaxation = data->readFloat();
                        wheelBrickData[a].frictionSlip = data->readFloat();
                        wheelBrickData[a].rollInfluence = data->readFloat();
                        wheelBrickData[a].typeID = data->readUInt(20);
                    }
                }

                livingBrick *found = 0;
                for(int a = 0; a<ohWow->livingBricks.size(); a++)
                {
                    if(ohWow->livingBricks[a])
                    {
                        if(ohWow->livingBricks[a]->serverID == id)
                        {
                            found = ohWow->livingBricks[a];
                            break;
                        }
                    }
                }

                //std::cout<<"Received packet, id "<<id<<" wheels: "<<wheels<<"\n";

                if(!found)
                {
                    //std::cout<<"Making new car...\n";
                    found = new livingBrick;

                    while(found->wheels.size() < wheels)
                        found->wheels.push_back(new interpolator);
                    for(unsigned int a = 0; a<wheels; a++)
                    {
                        found->wheels[a]->scale = glm::vec3(radii[a]/1.6,radii[a]/1.6,radii[a]/1.6);

                        if(ohWow->wheelDirtEmitter)
                        {
                            emitter *dirtEmitter = new emitter;
                            dirtEmitter->creationTime = SDL_GetTicks();
                            dirtEmitter->type = ohWow->wheelDirtEmitter;
                            dirtEmitter->whichWheel = a;
                            dirtEmitter->attachedToCar = found;
                            dirtEmitter->serverID = -1;
                            wheelBrickData[a].dirtEmitter = dirtEmitter;
                            ohWow->emitters.push_back(dirtEmitter);
                        }

                        found->wheelBrickData.push_back(wheelBrickData[a]);
                    }

                    found->serverID = id;
                    ohWow->livingBricks.push_back(found);
                    found->allocateVertBuffer();
                    found->allocatePerTexture(ohWow->brickMat);
                    found->allocatePerTexture(ohWow->brickMatSide,true,true);
                    found->allocatePerTexture(ohWow->brickMatBottom,true);
                    found->allocatePerTexture(ohWow->brickMatRamp);
                    found->addBlocklandCompatibility(ohWow->staticBricks.blocklandTypes);
                    //for(int a = 0; a<prints.names.size(); a++)
                        //found->allocatePerTexture(prints.textures[a],false,false,true);
                }

                if(radii)
                {
                    delete radii;
                    radii = 0;
                    //std::cout<<"Deleted radii\n";
                }

                int howManyTotal = data->readUInt(16);
                int howMany = data->readUInt(8);
                //std::cout<<"Total: "<<howManyTotal<<" this time: "<<howMany<<"\n";
                //std::cout<<"Adding "<<howMany<<" living bricks to "<<found->serverID<<"\n";
                for(int a = 0; a<howMany; a++)
                {
                    int serverID = data->readUInt(20);

                    float r = data->readUInt(8);
                    r /= 255.0;
                    float g = data->readUInt(8);
                    g /= 255.0;
                    float b = data->readUInt(8);
                    b /= 255.0;
                    float al = data->readUInt(8);
                    al /= 255.0;

                    //std::cout<<r<<" "<<g<<" "<<b<<" "<<al<<" color\n";

                    float x = data->readFloat();
                    float y = data->readFloat();
                    unsigned int platesUp = data->readUInt(10);
                    bool yHalfPos = data->readBit();
                    float z = data->readFloat();

                    //std::cout<<"Pos: "<<x<<","<<y<<","<<z<<"\n";

                    int angleID = data->readUInt(2);
                    int shapeFx = data->readUInt(4);
                    int material = data->readUInt(4);
                    if(shapeFx == 1)
                        material += 1000;
                    else if(shapeFx == 2)
                        material += 2000;
                    bool isSpecial = data->readBit();
                    if(!isSpecial)
                    {
                        basicBrickRenderData *tmp = new basicBrickRenderData;
                        tmp->yHalfPos = yHalfPos;
                        tmp->carPlatesUp = platesUp;
                        tmp->serverID = serverID;
                        tmp->color = glm::vec4(r,g,b,al);
                        tmp->position = glm::vec3(x,y,z);
                        tmp->rotation = ohWow->staticBricks.rotations[angleID];
                        tmp->material = material;
                        int width = data->readUInt(8);
                        int height = data->readUInt(8);
                        int length = data->readUInt(8);
                        //std::cout<<"Dims: "<<width<<","<<height<<","<<length<<"\n";
                        tmp->dimensions = glm::ivec4(width,height,length,0);
                        bool doNotCompile = false;
                        found->addBasicBrick(tmp,angleID,0,ohWow->world,doNotCompile);
                    }
                    else
                    {
                        int typeID = data->readUInt(10);
                        bool foundSpecial = false;
                        for(int i = 0; i<ohWow->staticBricks.blocklandTypes->specialBrickTypes.size(); i++)
                        {
                            if(ohWow->staticBricks.blocklandTypes->specialBrickTypes[i]->serverID == typeID)
                            {
                                foundSpecial = true;
                                specialBrickRenderData *tmp = new specialBrickRenderData;
                                tmp->yHalfPos = yHalfPos;
                                tmp->carPlatesUp = platesUp;
                                tmp->serverID = serverID;
                                tmp->color = glm::vec4(r,g,b,al);
                                tmp->position = glm::vec3(x,y,z);
                                tmp->rotation = ohWow->staticBricks.rotations[angleID];
                                tmp->material = material;
                                bool doNotCompile = false;
                                found->addSpecialBrick(tmp,ohWow->world,i,angleID,doNotCompile);
                                //std::cout<<tmp->type->type->uiName<<" special type\n";
                                break;
                            }
                        }
                        if(!foundSpecial)
                            error("Could not find special brick type " + std::to_string(typeID) + " server asked us to make!");
                    }
                }

                //std::cout<<found->getBrickCount()<<" >= "<<howManyTotal<<"\n";
                if(found->getBrickCount() >= howManyTotal)
                {
                    found->recompileEverything();
                    found->compiled = true;
                }

                return;
            }
            case packetType_removeBrick:
            {
                int howMany = data->readUInt(8);
                for(int a = 0; a<howMany; a++)
                {
                    brickFakeKills tmp;
                    float vel = 0.06;
                    tmp.angVel = glm::vec3(drand(-vel,vel),drand(-vel,vel),drand(-vel,vel));
                    tmp.linVel = glm::vec3(drand(-vel,vel),drand(0,vel*3.0),drand(-vel,vel));
                    tmp.startTime = SDL_GetTicks();
                    tmp.endTime = SDL_GetTicks() + 1500;

                    int id = data->readUInt(20);

                    for(int b = 0; b<ohWow->staticBricks.opaqueBasicBricks.size(); b++)
                    {
                        if(!ohWow->staticBricks.opaqueBasicBricks[b])
                            continue;
                        if(ohWow->staticBricks.opaqueBasicBricks[b]->serverID == id)
                        {
                            if(ohWow->staticBricks.opaqueBasicBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                tmp.basic = ohWow->staticBricks.opaqueBasicBricks[b];
                                ohWow->staticBricks.opaqueBasicBricks[b]->markedForDeath = true;
                                ohWow->fakeKills.push_back(tmp);
                            }
                            else
                                ohWow->staticBricks.removeBasicBrick(ohWow->staticBricks.opaqueBasicBricks[b],ohWow->world);
                            continue;
                        }
                    }
                    for(int b = 0; b<ohWow->staticBricks.transparentBasicBricks.size(); b++)
                    {
                        if(!ohWow->staticBricks.transparentBasicBricks[b])
                            continue;
                        if(ohWow->staticBricks.transparentBasicBricks[b]->serverID == id)
                        {
                            if(ohWow->staticBricks.transparentBasicBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                tmp.basic = ohWow->staticBricks.transparentBasicBricks[b];
                                ohWow->staticBricks.transparentBasicBricks[b]->markedForDeath = true;
                                ohWow->fakeKills.push_back(tmp);
                            }
                            else
                                ohWow->staticBricks.removeBasicBrick(ohWow->staticBricks.transparentBasicBricks[b],ohWow->world);
                            continue;
                        }
                    }
                    for(int b = 0; b<ohWow->staticBricks.opaqueSpecialBricks.size(); b++)
                    {
                        if(!ohWow->staticBricks.opaqueSpecialBricks[b])
                            continue;
                        if(ohWow->staticBricks.opaqueSpecialBricks[b]->serverID == id)
                        {
                            if(ohWow->staticBricks.opaqueSpecialBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                tmp.special = ohWow->staticBricks.opaqueSpecialBricks[b];
                                ohWow->staticBricks.opaqueSpecialBricks[b]->markedForDeath = true;
                                ohWow->fakeKills.push_back(tmp);
                            }
                            else
                                ohWow->staticBricks.removeSpecialBrick(ohWow->staticBricks.opaqueSpecialBricks[b],ohWow->world);
                            continue;
                        }
                    }
                    for(int b = 0; b<ohWow->staticBricks.transparentSpecialBricks.size(); b++)
                    {
                        if(!ohWow->staticBricks.transparentSpecialBricks[b])
                            continue;
                        if(ohWow->staticBricks.transparentSpecialBricks[b]->serverID == id)
                        {
                            if(ohWow->staticBricks.transparentSpecialBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                tmp.special = ohWow->staticBricks.transparentSpecialBricks[b];
                                ohWow->staticBricks.transparentSpecialBricks[b]->markedForDeath = true;
                                ohWow->fakeKills.push_back(tmp);
                            }
                            else
                                ohWow->staticBricks.removeSpecialBrick(ohWow->staticBricks.transparentSpecialBricks[b],ohWow->world);
                            continue;
                        }
                    }
                }
                return;
            }
            case packetType_updateDynamicTransforms:
            {
                unsigned int packetTime = data->readUInt(32);
                unsigned int currentServerTime = (SDL_GetTicks() - interpolator::clientTimePoint) + interpolator::serverTimePoint;
                //std::cout<<"Server time: "<<packetTime<<" predicted server time: "<<currentServerTime<<"\n";
                //int msSinceLastUpdate = data->readUInt(8);
                int howMany = data->readUInt(8);
                //int snapshotID = data->readUInt(20);
                for(int a = 0; a<howMany; a++)
                {
                    int id = data->readUInt(dynamicObjectIDBits);

                    bool isItem = data->readBit();
                    if(!isItem)
                    {
                        float x = data->readFloat();
                        float y = data->readFloat();
                        float z = data->readFloat();
                        btQuaternion quat = readQuat(data);
                        float rotX = quat.x();
                        float rotY = quat.y();
                        float rotZ = quat.z();
                        float rotW = quat.w();
                        /*float velX = data->readFloat();
                        float velY = data->readFloat();
                        float velZ = data->readFloat();*/
                        bool isPlayer = data->readBit();

                        bool walking = false;
                        float pitch = 0;
                        if(isPlayer)
                        {
                            walking = data->readBit();
                            pitch = data->readFloat();
                        }

                        bool skip = isnanf(x) || isinf(x);
                        if(isnanf(y) || isinf(y))
                            skip = true;
                        if(isnanf(z) || isinf(z))
                            skip = true;
                        if(isnanf(rotX) || isinf(rotX))
                            skip = true;
                        if(isnanf(rotY) || isinf(rotY))
                            skip = true;
                        if(isnanf(rotZ) || isinf(rotZ))
                            skip = true;
                        if(isnanf(rotW) || isinf(rotW))
                            skip = true;

                        if(!skip)
                        {
                            for(int i = 0; i<ohWow->newDynamics.size(); i++)
                            {
                                if(ohWow->newDynamics[i]->serverID == id)
                                {
                                    //std::cout<<id<<" has pos: "<<x<<","<<y<<","<<z<<"\n";
                                    ohWow->newDynamics[i]->modelInterpolator.addTransform(packetTime,glm::vec3(x,y,z),glm::quat(rotW,rotX,rotY,rotZ));//,glm::vec3(velX,velY,velZ));
                                    if(isPlayer)
                                    {
                                        glm::mat4 final = glm::translate(glm::vec3(0,176.897,0)) * glm::rotate(glm::mat4(1.0),-pitch,glm::vec3(1.0,0.0,0.0)) * glm::translate(glm::vec3(0,-176.897,0));
                                        /*ohWow->newDynamics[i]->setExtraTransform("Head",final);
                                        ohWow->newDynamics[i]->setExtraTransform("Face1",final);*/
                                        ohWow->newDynamics[i]->setFixedRotation("Head",final);
                                        ohWow->newDynamics[i]->setFixedRotation("Face1",final);
                                        if(walking)
                                            ohWow->newDynamics[i]->play("walk");
                                        else
                                            ohWow->newDynamics[i]->stop();
                                    }

                                    break;
                                }
                            }
                        }
                        else
                            error("Received nan or inf transform for non-item dynamic " + std::to_string(id));
                    }
                    else
                    {
                        item *tmp = 0;
                        for(unsigned int a = 0; a<ohWow->items.size(); a++)
                        {
                            if(ohWow->items[a]->serverID == id)
                            {
                                tmp = ohWow->items[a];
                                break;
                            }
                        }

                        if(!tmp)
                        {
                            //error("Could not find item " + std::to_string(id) + " to update held by.");
                            //Still need to read data to keep going through rest of packet correctly
                            if(data->readBit())
                            {
                                data->readUInt(dynamicObjectIDBits);
                                data->readBit();
                                data->readBit();
                            }
                            else
                            {
                                data->readFloat();
                                data->readFloat();
                                data->readFloat();
                                readQuat(data);
                            }
                            continue;
                        }

                        bool held = data->readBit();
                        if(held)
                        {
                            int heldBy = data->readUInt(dynamicObjectIDBits);
                            tmp->hidden = data->readBit();
                            tmp->swinging = data->readBit();
                            newDynamic *holder = 0;
                            for(unsigned int a = 0; a<ohWow->newDynamics.size(); a++)
                            {
                                if(ohWow->newDynamics[a]->serverID == heldBy)
                                {
                                    holder = ohWow->newDynamics[a];
                                    break;
                                }
                            }
                            tmp->heldBy = holder;
                            if(tmp->heldBy)
                                tmp->modelInterpolator.keyFrames.clear();
                        }
                        else
                        {
                            tmp->heldBy = 0;

                            float x = data->readFloat();
                            float y = data->readFloat();
                            float z = data->readFloat();
                            btQuaternion quat = readQuat(data);
                            float rotX = quat.x();
                            float rotY = quat.y();
                            float rotZ = quat.z();
                            float rotW = quat.w();

                            tmp->modelInterpolator.addTransform(packetTime,glm::vec3(x,y,z),glm::quat(rotW,rotX,rotY,rotZ));
                        }
                    }
                }

                //now we do brick cars
                howMany = data->readUInt(8);

                for(int a = 0; a<howMany; a++)
                {
                    int id = data->readUInt(10);
                    float x = data->readFloat();
                    float y = data->readFloat();
                    float z = data->readFloat();
                    btQuaternion quat = readQuat(data);
                    float rotX = quat.x();
                    float rotY = quat.y();
                    float rotZ = quat.z();
                    float rotW = quat.w();

                    int numWheels = data->readUInt(5);

                    float wheelX[numWheels];
                    float wheelY[numWheels];
                    float wheelZ[numWheels];
                    float wheelRotX[numWheels];
                    float wheelRotY[numWheels];
                    float wheelRotZ[numWheels];
                    float wheelRotW[numWheels];
                    bool emitterOn[numWheels];

                    for(int i = 0; i<numWheels; i++)
                    {
                        wheelX[i] = data->readFloat();
                        wheelY[i] = data->readFloat();
                        wheelZ[i] = data->readFloat();
                        btQuaternion quat = readQuat(data);
                        wheelRotX[i] = quat.x();
                        wheelRotY[i] = quat.y();
                        wheelRotZ[i] = quat.z();
                        wheelRotW[i] = quat.w();
                        emitterOn[i] = data->readBit();
                    }

                    for(int i = 0; i<ohWow->livingBricks.size(); i++)
                    {
                        if(ohWow->livingBricks[i]->serverID == id)
                        {
                            ohWow->livingBricks[i]->carTransform.addTransform(packetTime,glm::vec3(x,y,z),glm::quat(rotW,rotX,rotY,rotZ));
                            for(int wheel = 0; wheel<numWheels; wheel++)
                            {
                                ohWow->livingBricks[i]->wheels[wheel]->addTransform(packetTime,glm::vec3(wheelX[wheel],wheelY[wheel],wheelZ[wheel]),glm::quat(wheelRotW[wheel],wheelRotX[wheel],wheelRotY[wheel],wheelRotZ[wheel]));
                                //std::cout<<"emitterOn["<<wheel<<"] is "<<(emitterOn[i]?"true":"false")<<" for car ID "<<ohWow->livingBricks[i]->serverID<<"\n";
                                if(ohWow->livingBricks[i]->wheelBrickData[wheel].dirtEmitter)
                                    ((emitter*)ohWow->livingBricks[i]->wheelBrickData[wheel].dirtEmitter)->enabled = emitterOn[i];
                                else
                                    std::cout<<"Could not find emitter!\n";
                            }
                            break;
                        }
                    }
                }

                howMany = data->readUInt(8);

                for(int a = 0; a<howMany; a++)
                {
                    int id = data->readUInt(12);
                    int nodes = data->readUInt(8);

                    rope *toUpdate = 0;
                    for(int b = 0; b<ohWow->ropes.size(); b++)
                    {
                        if(ohWow->ropes[b]->serverID == id)
                        {
                            toUpdate = ohWow->ropes[b];
                            break;
                        }
                    }

                    if(toUpdate)
                    {
                        for(int b = 0; b<nodes; b++)
                        {
                            toUpdate->nodePositions[b].x = data->readFloat();
                            toUpdate->nodePositions[b].y = data->readFloat();
                            toUpdate->nodePositions[b].z = data->readFloat();
                        }

                        glBindVertexArray(toUpdate->vao);
                        glBindBuffer(GL_ARRAY_BUFFER,toUpdate->positionsBuffer);
                        glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(glm::vec3)*toUpdate->nodePositions.size(),&toUpdate->nodePositions[0][0]);
                        glBindVertexArray(0);
                    }
                    else
                    {
                        for(int b = 0; b<nodes; b++)
                        {
                            data->readFloat();
                            data->readFloat();
                            data->readFloat();
                        }
                    }
                }

                return;
            }

            case packetType_skipBricksCompile:
            {
                ohWow->skippingCompileNextBricks = data->readUInt(24);
                //std::cout<<"Skipping compile for next: "<<ohWow->skippingCompileNextBricks<<" bricks.\n";
                return;
            }

            case packetType_addBricks:
            {
                int howMany = data->readUInt(8);
                for(int a = 0; a<howMany; a++)
                {
                    int serverID = data->readUInt(20);

                    float r = data->readUInt(8);
                    r /= 255.0;
                    float g = data->readUInt(8);
                    g /= 255.0;
                    float b = data->readUInt(8);
                    b /= 255.0;
                    float al = data->readUInt(8);
                    al /= 255.0;

                    /*float x = data->readUInt(13);
                    x -= 2048;
                    x /= 2;
                    float y = data->readUInt(16);
                    y -= 2048;
                    y /= 10;
                    float z = data->readUInt(13);
                    z -= 2048;
                    z /= 2;*/

                    float x = data->readUInt(14);
                    x -= 8192;
                    x += data->readBit() ? 0.5 : 0;
                    float y = data->readUInt(16);
                    y += data->readBit() ? 0.5 : 0;
                    y *= 0.4;
                    float z = data->readUInt(14);
                    z -= 8192;
                    z += data->readBit() ? 0.5 : 0;

                    int angleID = data->readUInt(2);
                    int shapeFx = data->readUInt(4);
                    int material = data->readUInt(4);
                    if(shapeFx == 1)
                        material += 1000;
                    else if(shapeFx == 2)
                        material += 2000;
                    bool isSpecial = data->readBit();
                    if(!isSpecial)
                    {
                        basicBrickRenderData *tmp = new basicBrickRenderData;
                        tmp->serverID = serverID;
                        tmp->color = glm::vec4(r,g,b,al);
                        tmp->position = glm::vec3(x,y,z);
                        tmp->rotation = ohWow->staticBricks.rotations[angleID];
                        tmp->material = material;
                        int width = data->readUInt(8);
                        int height = data->readUInt(8);
                        int length = data->readUInt(8);

                        bool isPrint = data->readBit();
                        if(isPrint)
                        {
                            int mask = data->readUInt(6);
                            std::string printName = data->readString();

                            //TODO: Anything but this:
                            printAlias *printType = 0;
                            for(unsigned int a = 0; a<ohWow->staticBricks.blocklandTypes->printTypes.size(); a++)
                            {
                                printAlias *comp = ohWow->staticBricks.blocklandTypes->printTypes[a];
                                if(comp->faceMask == mask)
                                {
                                    if(comp->width == width)
                                    {
                                        if(comp->length == length)
                                        {
                                            if(comp->height == height)
                                            {
                                                printType = comp;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            if(!printType)
                                error("Failed to find compatible print brick type!");
                            else
                            {
                                //TODO: Replace with server sending printID
                                for(unsigned int a = 0; a<ohWow->prints->names.size(); a++)
                                {
                                    if(ohWow->prints->names[a] == printName)
                                    {
                                        tmp->printFaces = printType;
                                        tmp->printID = a;
                                        break;
                                    }
                                }
                            }
                        }

                        tmp->dimensions = glm::ivec4(width,height,length,0);
                        bool doNotCompile = false;
                        if(ohWow->skippingCompileNextBricks > 0)
                        {
                            if(ohWow->staticBricks.getBrickCount() < ohWow->skippingCompileNextBricks)
                                doNotCompile = true;
                            else
                            {
                                ohWow->skippingCompileNextBricks = 0;
                                ohWow->staticBricks.recompileEverything();
                            }

                        }
                        ohWow->staticBricks.addBasicBrick(tmp,angleID,0,ohWow->world,doNotCompile);
                    }
                    else
                    {
                        int typeID = data->readUInt(10);

                        int mask = 0;
                        std::string printName = "";
                        bool isPrint = data->readBit();
                        if(isPrint)
                        {
                            mask = data->readUInt(6);
                            printName = data->readString();
                        }

                        bool foundSpecial = false;
                        for(int i = 0; i<ohWow->staticBricks.blocklandTypes->specialBrickTypes.size(); i++)
                        {
                            if(ohWow->staticBricks.blocklandTypes->specialBrickTypes[i]->serverID == typeID)
                            {
                                foundSpecial = true;
                                specialBrickRenderData *tmp = new specialBrickRenderData;
                                tmp->serverID = serverID;
                                tmp->color = glm::vec4(r,g,b,al);
                                tmp->position = glm::vec3(x,y,z);
                                tmp->rotation = ohWow->staticBricks.rotations[angleID];
                                tmp->material = material;

                                if(isPrint)
                                {
                                    //TODO: Replace with server sending printID
                                    for(unsigned int a = 0; a<ohWow->prints->names.size(); a++)
                                    {
                                        if(ohWow->prints->names[a] == printName)
                                        {
                                            tmp->printID = a;
                                            break;
                                        }
                                    }
                                }
                                else
                                    tmp->printID = -1;

                                bool doNotCompile = false;
                                if(ohWow->skippingCompileNextBricks > 0)
                                {
                                    if(ohWow->staticBricks.getBrickCount() < ohWow->skippingCompileNextBricks)
                                        doNotCompile = true;
                                    else
                                    {
                                        ohWow->skippingCompileNextBricks = 0;
                                        ohWow->staticBricks.recompileEverything();
                                    }
                                }
                                ohWow->staticBricks.addSpecialBrick(tmp,ohWow->world,i,angleID,doNotCompile);
                                break;
                            }
                        }
                        if(!foundSpecial)
                            error("Could not find special brick type " + std::to_string(typeID) + " server asked us to make!");
                    }
                }
                return;
            }
            case packetType_addRemoveDynamic:
            {
                int objectID = data->readUInt(dynamicObjectIDBits);

                bool addOrRemove = data->readBit();

                if(addOrRemove) //add
                {
                    int typeID = data->readUInt(10);
                    float red = data->readFloat();
                    float green = data->readFloat();
                    float blue = data->readFloat();

                    for(int a = 0; a<ohWow->newDynamics.size(); a++)
                    {
                        if(ohWow->newDynamics[a]->serverID == objectID)
                        {
                            error("Dynamic with serverID " + std::to_string(objectID) + " already exists!");
                            return;
                        }
                    }

                    newModel *type = 0;
                    for(int a = 0; a<ohWow->newDynamicTypes.size(); a++)
                    {
                        if(ohWow->newDynamicTypes[a]->serverID == typeID)
                        {
                            type = ohWow->newDynamicTypes[a];
                            break;
                        }
                    }

                    if(type)
                    {
                        //TODO: Let server set scale
                        newDynamic *tmp = new newDynamic(type,glm::vec3(0.02,0.02,0.02));
                        /*tmp->addExtraTransform("Head");
                        tmp->addExtraTransform("Face1");

                        glm::mat4 final = glm::translate(glm::vec3(0,176.897,0)) * glm::rotate(glm::mat4(1.0),glm::radians(0.f),glm::vec3(1.0,0.0,0.0)) * glm::translate(glm::vec3(0,-176.897,0));
                        tmp->setExtraTransform("Head",final);
                        tmp->setExtraTransform("Face1",final);
                        tmp->colorTint = glm::vec3(red,green,blue);*/
                        tmp->serverID = objectID;
                        tmp->shapeNameColor = glm::vec3(red,green,blue);
                        tmp->shapeName = "";
                        ohWow->newDynamics.push_back(tmp);
                    }
                    else
                        error("Could not find dynamic type: " + std::to_string(typeID));
                }
                else //remove
                {
                    for(int a = 0; a<ohWow->newDynamics.size(); a++)
                    {
                        if(ohWow->newDynamics[a]->serverID == objectID)
                        {
                            if(ohWow->cameraTarget == ohWow->newDynamics[a])
                                ohWow->cameraTarget = 0;

                            if(ohWow->currentPlayer == ohWow->newDynamics[a])
                                ohWow->currentPlayer = 0;
                            delete ohWow->newDynamics[a];
                            ohWow->newDynamics.erase(ohWow->newDynamics.begin() + a);
                            break;
                        }
                    }
                }

                return;
            }
            case packetType_cameraDetails:
            {
                ohWow->adminCam = data->readBit();
                ohWow->boundToObject = data->readBit();
                if(ohWow->boundToObject)
                {
                    ohWow->cameraTargetServerID = data->readUInt(dynamicObjectIDBits);
                    ohWow->cameraLean = data->readBit();
                    ohWow->cameraTarget = 0;
                    checkForCameraToBind(ohWow);
                }
                else
                {
                    float x = data->readFloat();
                    float y = data->readFloat();
                    float z = data->readFloat();
                    ohWow->cameraLean = false;
                    ohWow->cameraPos = glm::vec3(x,y,z);
                    ohWow->freeLook = data->readBit();
                    if(!ohWow->freeLook)
                    {
                        x = data->readFloat();
                        y = data->readFloat();
                        z = data->readFloat();
                        ohWow->cameraDir = glm::vec3(x,y,z);
                    }
                }

                CEGUI::Window *saveLoadWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("SaveLoad");

                if(data->readBit())
                {
                    saveLoadWindow->getChild("SaveCar")->setDisabled(false);
                    ohWow->drivenCarId = data->readUInt(10);
                }
                else
                {
                    saveLoadWindow->getChild("SaveCar")->setDisabled(true);
                    ohWow->drivenCarId = -1;
                }
                return;
            }
            case packetType_connectionRepsonse:
            {
                bool nameOkay = data->readBit();
                if(!nameOkay)
                {
                    ohWow->kicked = true;
                    ohWow->waitingForServerResponse = false;
                    error("Your name was not acceptable or the server failed to respond!");
                    return;
                }
                interpolator::serverTimePoint = data->readUInt(32) - 50;
                interpolator::clientTimePoint = getServerTime();
                std::cout<<"Difference between server and client time: "<<(int)interpolator::serverTimePoint - (int) getServerTime()<<"\n";
                ohWow->brickTypesToLoad = data->readUInt(10);
                ohWow->dynamicTypesToLoad = data->readUInt(10);
                ohWow->itemTypesToLoad = data->readUInt(10);

                CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
                joinServerWindow->getChild("StatusText")->setText("Connected!");

                info("Loading " + std::to_string(ohWow->brickTypesToLoad) + " brick types and " + std::to_string(ohWow->dynamicTypesToLoad) + " dynamic types from server!");

                finishLoadingTypesCheck(ohWow);

                return;
            }
            case packetType_addDynamicType:
            {
                int serverID = data->readUInt(10);
                std::string filePath = data->readString();
                int standingFrame = data->readUInt(8);
                float eyeOffsetX = data->readFloat();
                float eyeOffsetY = data->readFloat();
                float eyeOffsetZ = data->readFloat();

                info("Loading dynamic type: " + filePath);

                animatedModel *removeMeSomeday = new animatedModel(filePath);
                ohWow->oldDynamicTypes.push_back(removeMeSomeday);

                newModel *tmp = new newModel(filePath);
                tmp->oldModelType = (void*)removeMeSomeday;
                tmp->defaultFrame = standingFrame;
                tmp->serverID = serverID;
                tmp->eyeOffset = glm::vec3(eyeOffsetX,eyeOffsetY,eyeOffsetZ);

                int numAnims = data->readUInt(8);
                for(int a = 0; a<numAnims; a++)
                {
                    int startFrame = data->readUInt(8);
                    int endFrame = data->readUInt(8);
                    float speed = data->readFloat();
                    newAnimation anim;
                    anim.serverID = a;
                    anim.startFrame = startFrame;
                    anim.endFrame = endFrame;
                    anim.speedDefault = speed;
                    tmp->animations.push_back(anim);
                }

                //TODO: Remove this!
                if(ohWow->newDynamicTypes.size() == 0)
                {
                    newAnimation walk;
                    walk.endFrame = 30;
                    walk.startFrame = 0;
                    walk.speedDefault = 0.045;
                    walk.name = "walk";
                    tmp->animations.push_back(walk);
                }

                ohWow->newDynamicTypes.push_back(tmp);

                for(unsigned int a = 0; a<ohWow->itemTypes.size(); a++)
                {
                    if(ohWow->itemTypes[a]->waitingForModel)
                    {
                        if(ohWow->itemTypes[a]->waitingModel == serverID)
                        {
                            info("Added model to " + ohWow->itemTypes[a]->uiName + " after the fact.");
                            ohWow->itemTypes[a]->waitingForModel = false;
                            ohWow->itemTypes[a]->type = tmp;
                            for(unsigned int b = 0; b<tmp->animations.size(); b++)
                            {
                                if(tmp->animations[b].serverID == ohWow->itemTypes[a]->waitingForSwingAnim)
                                {
                                    ohWow->itemTypes[a]->fireAnim = &tmp->animations[b];
                                }
                                if(tmp->animations[b].serverID == ohWow->itemTypes[a]->waitingForSwitchAnim)
                                {
                                    ohWow->itemTypes[a]->switchAnim = &tmp->animations[b];
                                }
                            }
                        }
                    }
                }

                finishLoadingTypesCheck(ohWow);

                return;
            }
            case packetType_addSpecialBrickType:
            {
                int howMany = data->readUInt(6);
                debug("Loading another: " + std::to_string(howMany) + " brick types");

                for(int a = 0; a<howMany; a++)
                {
                    int typeID = data->readUInt(10);
                    std::string filePath = "assets\\brick\\types" + data->readString();

                    //std::cout<<"Adding type: "<<typeID<<" "<<filePath<<"\n";
                    ohWow->staticBricks.blocklandTypes->addSpecialBrickType(filePath,ohWow->brickSelector,typeID);
                }

                finishLoadingTypesCheck(ohWow);

                return;
            }
        }
    }
}



















