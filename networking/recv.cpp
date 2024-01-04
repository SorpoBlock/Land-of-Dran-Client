#include "recv.h"

enum serverToClientPacketType
{
    packetType_connectionRepsonse = 0,
    packetType_addRemoveDynamic = 1,
    packetType_addDynamicType = 2,
    packetType_addSpecialBrickType = 3,
    packetType_cameraDetails = 4,
    packetType_addBricks = 5,
    packetType_serverCommand = 6,
    packetType_updateDynamicTransforms = 7,
    packetType_addRemoveLight = 8,
    packetType_removeBrick = 9,
    packetType_addBricksToBuiltCar = 10,
    packetType_luaPasswordResponse = 11,
    packetType_setColorPalette = 12,
    packetType_clientPhysicsData = 13,
    packetType_ping = 14,
    packetType_removeBrickVehicle = 15,
    packetType_setShapeName = 16,
    packetType_addItemType = 17,
    packetType_addRemoveItem = 18,
    packetType_addSoundType = 19,
    packetType_playSound = 20,
    packetType_forceWrenchDialog = 21,
    packetType_setInventory = 22,
    packetType_addOrRemovePlayer = 23,
    packetType_updateBrick = 24,
    packetType_debugLocations = 25,
    packetType_addRemoveRope = 26,
    packetType_addMessage = 27,
    packetType_setNodeColors = 28,
    packetType_undefined = 29,
    packetType_newEmitterParticleType = 30,
    packetType_emitterAddRemove = 31
};

#define dynamicObjectIDBits 12

std::string GetHexRepresentation(const unsigned char *Bytes, size_t Length) {
    std::ostringstream os;
    os.fill('0');
    os<<std::hex;
    for(const unsigned char *ptr = Bytes; ptr < Bytes+Length; ++ptr) {
        os<<std::setw(2)<<(unsigned int)*ptr;
    }
    return os.str();
}

void processCommand(clientStuff *clientEnvironment,std::string commandType,packet *data)
{
    serverStuff *serverData = clientEnvironment->serverData;

    if(clientEnvironment->ignoreGamePackets)
    {
        if(!(commandType == "numCustomFiles" || commandType == "customFileDescription"))
            return;
    }

    if(commandType == "clearAllBricks")
    {
        //TODO: CHECKS FOR BOUND LIGHTS AND MUSIC???

        basicBrickRenderData *basicTempBrick = 0;
        specialBrickRenderData *specialTempBrick = 0;

        for(unsigned int a = 0; a<serverData->staticBricks.opaqueBasicBricks.size(); a++)
        {
            basicBrickRenderData *theBrick = serverData->staticBricks.opaqueBasicBricks[a];
            if(!theBrick)
                continue;

            if(theBrick->isTempBrick)
            {
                basicTempBrick = theBrick;
                continue;
            }

            if(theBrick->body && serverData->world)
            {
                serverData->world->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }

            delete theBrick;
        }
        for(unsigned int a = 0; a<serverData->staticBricks.transparentBasicBricks.size(); a++)
        {
            basicBrickRenderData *theBrick = serverData->staticBricks.transparentBasicBricks[a];
            if(!theBrick)
                continue;

            if(theBrick->isTempBrick)
            {
                basicTempBrick = theBrick;
                continue;
            }

            if(theBrick->body && serverData->world)
            {
                serverData->world->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }

            delete theBrick;
        }
        for(unsigned int a = 0; a<serverData->staticBricks.opaqueSpecialBricks.size(); a++)
        {
            specialBrickRenderData *theBrick = serverData->staticBricks.opaqueSpecialBricks[a];
            if(!theBrick)
                continue;

            if(theBrick->isTempBrick)
            {
                specialTempBrick = theBrick;
                continue;
            }

            if(theBrick->body && serverData->world)
            {
                serverData->world->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }

            delete theBrick;
        }
        for(unsigned int a = 0; a<serverData->staticBricks.transparentSpecialBricks.size(); a++)
        {
            specialBrickRenderData *theBrick = serverData->staticBricks.transparentSpecialBricks[a];
            if(!theBrick)
                continue;

            if(theBrick->isTempBrick)
            {
                specialTempBrick = theBrick;
                continue;
            }

            if(theBrick->body && serverData->world)
            {
                serverData->world->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }

            delete theBrick;
        }

        serverData->staticBricks.opaqueBasicBricks.clear();
        serverData->staticBricks.transparentBasicBricks.clear();
        serverData->staticBricks.opaqueSpecialBricks.clear();
        serverData->staticBricks.transparentSpecialBricks.clear();

        if(basicTempBrick)
            serverData->staticBricks.transparentBasicBricks.push_back(basicTempBrick);
        if(specialTempBrick)
            serverData->staticBricks.transparentSpecialBricks.push_back(specialTempBrick);

        serverData->staticBricks.recompileEverything();

        return;
    }
    else if(commandType == "numCustomFiles")
    {
        clientEnvironment->expectedCustomFiles = data->readUInt(16);

        return;
    }
    else if(commandType == "customFileDescription")
    {
        std::string name = data->readString();
        std::string path = data->readString();
        int checksum = data->readUInt(32);
        int sizebytes = data->readUInt(32);
        int type = data->readUInt(4);
        int id = data->readUInt(16);
        clientEnvironment->cancelCustomContentTimeoutTime = SDL_GetTicks() + 15000;
        addCustomFileToGUI(name,path,checksum,sizebytes,type,id);

        return;
    }
    else if(commandType == "dncTime")
    {
        serverData->env->currentTime = data->readFloat();
    }
    else if(commandType == "dncSpeed")
    {
        serverData->env->cycle.secondsInDay = data->readFloat();
    }
    else if(commandType == "environment")
    {
        serverData->env->useIBL = data->readBit();

        if(serverData->env->useIBL)
        {
            std::string skyBox = data->readString();
            std::string radianceMap = data->readString();
            std::string irradianceMap = data->readString();

            if(!okayFilePath(skyBox))
            {
                error("Invalid file path for skybox: " + skyBox);
                return;
            }

            if(!okayFilePath(radianceMap))
            {
                error("Invalid file path for radiance map: " + radianceMap);
                return;
            }

            if(!okayFilePath(irradianceMap))
            {
                error("Invalid file path for irradiance map: " + irradianceMap);
                return;
            }

            serverData->env->IBL = processEquirectangularMap(clientEnvironment->rectToCubeUnis->target,clientEnvironment->cubeVAO,"assets/"+skyBox,true);
            serverData->env->IBLRad = processEquirectangularMap(clientEnvironment->rectToCubeUnis->target,clientEnvironment->cubeVAO,"assets/"+radianceMap,true);
            serverData->env->IBLIrr = processEquirectangularMap(clientEnvironment->rectToCubeUnis->target,clientEnvironment->cubeVAO,"assets/"+irradianceMap,true);

            serverData->env->sunDirection.x = data->readFloat();
            serverData->env->sunDirection.y = data->readFloat();
            serverData->env->sunDirection.z = data->readFloat();
        }
        else
        {
            serverData->env->cycle.dawnStart = data->readFloat();
            serverData->env->cycle.dawnEnd = data->readFloat();
            serverData->env->cycle.duskStart = data->readFloat();
            serverData->env->cycle.duskEnd = data->readFloat();
            serverData->env->cycle.secondsInDay = data->readFloat();

            for(int a = 0; a<4; a++)
            {
                serverData->env->cycle.dncSkyColors[a].r = data->readFloat();
                serverData->env->cycle.dncSkyColors[a].g = data->readFloat();
                serverData->env->cycle.dncSkyColors[a].b = data->readFloat();

                serverData->env->cycle.dncFogColors[a].r = data->readFloat();
                serverData->env->cycle.dncFogColors[a].g = data->readFloat();
                serverData->env->cycle.dncFogColors[a].b = data->readFloat();

                serverData->env->cycle.dncSunColors[a].r = data->readFloat();
                serverData->env->cycle.dncSunColors[a].g = data->readFloat();
                serverData->env->cycle.dncSunColors[a].b = data->readFloat();
                serverData->env->cycle.dncSunColors[a].a = data->readFloat();
            }

            /*lengthPrefixedString cycleBinaryData = data->readString();
            for(int a = 0; a<sizeof(dayNightCycle); a++)
                std::cout<<(int)cycleBinaryData.data[a]<<"\n";
            memcpy(&serverData->env->cycle,cycleBinaryData.data,sizeof(dayNightCycle));*/

        }
    }
    else if(commandType == "skipBricksCompile")
    {
        serverData->skippingCompileNextBricks = data->readUInt(24);
    }
    else if(commandType == "setWaterLevel")
    {
        serverData->waterLevel = data->readFloat();
        if(serverData->waterLevel < -10000)
            serverData->waterLevel = 10000;
        if(serverData->waterLevel > 10000)
            serverData->waterLevel = 10000;
        return;
    }
    else if(commandType == "sendDecals")
    {
        int howMany = data->readUInt(7);
        for(int a = 0; a<howMany; a++)
        {
            std::string fileName = data->readString();
            clientEnvironment->picker->addDecalToPicker(fileName);
        }
    }
    else if(commandType == "stopLoop")
    {
        int loopId = data->readUInt(24);
        clientEnvironment->speaker->removeLoop(loopId);
    }
    else if(commandType == "audioEffect")
    {
        std::string effect = data->readString();
        clientEnvironment->speaker->setEffect(effect);
    }
    else if(commandType == "setItemFireAnim")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
        {
            heldSetItemPropertiesPacket tmp;
            tmp.itemID = itemId;

            tmp.setAnim = true;
            tmp.hasAnim = data->readBit();
            if(tmp.hasAnim)
            {
                tmp.animID = data->readUInt(10);
                tmp.animSpeed = data->readFloat();
            }

            tmp.deletionTime = SDL_GetTicks() + 10000;
            serverData->heldItemPackets.push_back(tmp);
            return;
        }

        if(data->readBit())
        {
            foundItem->nextFireAnim = data->readUInt(10);
            foundItem->nextFireAnimSpeed = data->readFloat();
        }
        else
            foundItem->nextFireAnim = -1;
    }
    else if(commandType == "setItemFireSound")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
        {
            heldSetItemPropertiesPacket tmp;
            tmp.itemID = itemId;

            tmp.setSound = true;
            tmp.hasSound = data->readBit();
            if(tmp.hasSound)
            {
                tmp.soundID = data->readUInt(10);
                tmp.soundPitch = data->readFloat();
                tmp.soundGain = data->readFloat();
            }

            tmp.deletionTime = SDL_GetTicks() + 10000;
            serverData->heldItemPackets.push_back(tmp);
            return;
        }

        if(data->readBit())
        {
            foundItem->nextFireSound = data->readUInt(10);
            foundItem->nextFireSoundPitch = data->readFloat();
            foundItem->nextFireSoundGain = data->readFloat();
        }
        else
            foundItem->nextFireSound = -1;
    }
    else if(commandType == "setItemFireEmitter")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
        {
            heldSetItemPropertiesPacket tmp;
            tmp.itemID = itemId;

            tmp.setEmitter = true;
            tmp.hasEmitter = data->readBit();
            if(tmp.hasEmitter)
            {
                tmp.emitterID = data->readUInt(10);
                tmp.emitterMesh = data->readString();
            }

            tmp.deletionTime = SDL_GetTicks() + 10000;
            serverData->heldItemPackets.push_back(tmp);
            return;
        }

        if(data->readBit())
        {
            foundItem->nextFireEmitter = data->readUInt(10);
            foundItem->nextFireEmitterMesh = data->readString();
        }
        else
            foundItem->nextFireEmitter = -1;
    }
    else if(commandType == "setItemCooldown")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
        {
            heldSetItemPropertiesPacket tmp;
            tmp.itemID = itemId;

            tmp.setCooldown = true;
            tmp.cooldown = data->readUInt(16);

            tmp.deletionTime = SDL_GetTicks() + 10000;
            serverData->heldItemPackets.push_back(tmp);
            return;
        }

        foundItem->fireCooldownMS = data->readUInt(16);
    }
    else if(commandType == "playItemAnimation")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
            return;

        int animId = data->readUInt(10);
        float speed = data->readFloat();

        foundItem->play(animId,speed);
    }
    else if(commandType == "itemFired")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
            return;

        if(foundItem->nextFireAnim != -1)
            foundItem->play(foundItem->nextFireAnim,true,foundItem->nextFireAnimSpeed,false);
        if(foundItem->nextFireSound != -1)
        {
            location *soundLoc = new location(foundItem);
            clientEnvironment->speaker->playSound(foundItem->nextFireSound,soundLoc,foundItem->nextFireSoundPitch,foundItem->nextFireSoundGain);
        }

        if(foundItem->nextFireEmitter != -1)
        {
            for(int a = 0; a<serverData->emitterTypes.size(); a++)
            {
                if(serverData->emitterTypes[a]->serverID == foundItem->nextFireEmitter)
                {
                    emitter *e = new emitter;
                    e->creationTime = SDL_GetTicks();
                    e->type = serverData->emitterTypes[a];
                    e->attachedToItem = foundItem;
                    e->justAttached = true;
                    e->meshName = foundItem->nextFireEmitterMesh;
                    serverData->emitters.push_back(e);
                    break;
                }
            }
        }

        if(foundItem->useBulletTrail)
        {
            bulletTrail trail;
            trail.color = foundItem->bulletTrailColor;
            trail.end.x = data->readFloat();
            trail.end.y = data->readFloat();
            trail.end.z = data->readFloat();
            trail.start.x = data->readFloat();
            trail.start.y = data->readFloat();
            trail.start.z = data->readFloat();

            trail.creationTime = SDL_GetTicks();
            trail.deletionTime = SDL_GetTicks() + glm::length(trail.end-trail.start) * 15 * foundItem->bulletTrailSpeed;

            serverData->bulletTrails->bulletTrails.push_back(trail);
        }
    }
    else if(commandType == "vignette")
    {
        clientEnvironment->vignetteStrength = data->readFloat();
        if(data->readBit())
        {
            clientEnvironment->vignetteColor.r = data->readFloat();
            clientEnvironment->vignetteColor.g = data->readFloat();
            clientEnvironment->vignetteColor.b = data->readFloat();
        }
    }
    else if(commandType == "bulletTrail")
    {
        bulletTrail trail;
        trail.color.r = data->readFloat();
        trail.color.g = data->readFloat();
        trail.color.b = data->readFloat();
        trail.start.x = data->readFloat();
        trail.start.y = data->readFloat();
        trail.start.z = data->readFloat();
        trail.end.x = data->readFloat();
        trail.end.y = data->readFloat();
        trail.end.z = data->readFloat();
        trail.creationTime = SDL_GetTicks();
        trail.deletionTime = glm::length(trail.end-trail.start) * 10 * data->readFloat();
        serverData->bulletTrails->bulletTrails.push_back(trail);
    }
    else if(commandType == "setItemBulletTrail")
    {
        int itemId = data->readUInt(dynamicObjectIDBits);

        item *foundItem = 0;
        for(int a = 0; a<serverData->items.size(); a++)
        {
            if(serverData->items[a]->serverID == itemId)
            {
                foundItem = serverData->items[a];
                break;
            }
        }

        if(!foundItem)
            return;

        foundItem->useBulletTrail = data->readBit();
        foundItem->bulletTrailColor.r = data->readFloat();
        foundItem->bulletTrailColor.g = data->readFloat();
        foundItem->bulletTrailColor.b = data->readFloat();
        foundItem->bulletTrailSpeed = data->readFloat();

    }
    else
        error("Unrecognized command: " + commandType);
}

namespace syj
{
    void clientStuff::fatalNotify(std::string windowName,std::string windowText,std::string buttonText)
    {
        notify(windowName,windowText,buttonText);
        fatalNotifyStarted = true;
    }

    void finishLoadingTypesCheck(clientStuff *clientEnvironment)
    {
        serverStuff *serverData = clientEnvironment->serverData;

        if(!serverData->waitingForServerResponse)
            return;

        std::cout<<serverData->newDynamicTypes.size()<<"-"<<serverData->dynamicTypesToLoad<<" , "<<serverData->staticBricks.blocklandTypes->specialBrickTypes.size()<<"-"<<serverData->brickTypesToLoad<<" , "<<serverData->itemTypes.size()<<"-"<<serverData->itemTypesToLoad<<"\n";

        if(serverData->newDynamicTypes.size() >= serverData->dynamicTypesToLoad)
        {
            if(serverData->staticBricks.blocklandTypes->specialBrickTypes.size() >= serverData->brickTypesToLoad)
            {
                if(serverData->itemTypes.size() >= serverData->itemTypesToLoad)
                {
                    info("Finished loading types from server!");
                    serverData->waitingForServerResponse = false;

                    serverData->staticBricks.addBlocklandCompatibility(serverData->staticBricks.blocklandTypes);

                    packet data;
                    data.writeUInt(clientPacketType_startLoadingStageTwo,4);
                    serverData->connection->send(&data,true);

                    if(serverData->newDynamicTypes.size() > 0)
                    {
                        //TODO: Update avatar picker code
                        //if(!serverData->picker->playerModel)
                            //serverData->picker->playerModel = (model*)serverData->newDynamicTypes[0]->oldModelType;
                        clientEnvironment->picker->pickingPlayer = new newDynamic(serverData->newDynamicTypes[0]);
                        clientEnvironment->picker->pickingPlayer->useGlobalTransform = true;
                        clientEnvironment->picker->pickingPlayer->hidden = true;
                        clientEnvironment->picker->pickingPlayer->calculateMeshTransforms(0);
                        clientEnvironment->picker->pickingPlayer->bufferSubData();

                        clientEnvironment->picker->loadFromPrefs(clientEnvironment->prefs);
                        clientEnvironment->picker->sendAvatarPrefs(serverData->connection,0);
                    }
                }
            }
        }
    }

    void checkForCameraToBind(serverStuff *serverData)
    {
        if(!serverData->cameraTarget && serverData->boundToObject)
        {
            //std::cout<<"Checking for camera bind...\n";
            if(serverData->cameraTargetServerID == -1)
                error("Camera bound to object but there's no object!");
            else
            {
                for(int a = 0; a<serverData->newDynamics.size(); a++)
                {
                    if(serverData->newDynamics[a]->serverID == serverData->cameraTargetServerID)
                    {
                        //std::cout<<"Camera bound to "<<serverData->cameraTargetServerID<<"\n";
                        serverData->cameraTarget = serverData->newDynamics[a];
                        //std::cout<<"Found the object "<<serverData->cameraTarget->serverID<<"\n";
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

        auto iterItem = heldItemPackets.begin();
        while(iterItem != heldItemPackets.end())
        {
            heldSetItemPropertiesPacket *tmp = &(*iterItem);

            for(int a = 0; a<items.size(); a++)
            {
                item *i = items[a];
                if(i->serverID == tmp->itemID)
                {
                    if(tmp->setSound)
                    {
                        if(tmp->hasSound)
                        {
                            i->nextFireSound = tmp->soundID;
                            i->nextFireSoundPitch = tmp->soundPitch;
                            i->nextFireSoundGain = tmp->soundGain;
                        }
                        else
                            i->nextFireSound = -1;
                    }

                    if(tmp->setAnim)
                    {
                        if(tmp->hasAnim)
                        {
                            i->nextFireAnim = tmp->animID;
                            i->nextFireAnimSpeed = tmp->animSpeed;
                        }
                        else
                            i->nextFireAnim = -1;
                    }

                    if(tmp->setCooldown)
                        i->fireCooldownMS = tmp->cooldown;

                    if(tmp->setEmitter)
                    {
                        if(tmp->hasEmitter)
                        {
                            i->nextFireEmitter = tmp->emitterID;
                            i->nextFireEmitterMesh = tmp->emitterMesh;
                        }
                        else
                            i->nextFireEmitter = -1;
                    }

                    iterItem = heldItemPackets.erase(iterItem);
                    continue;
                }
            }

            if(tmp->deletionTime < SDL_GetTicks())
            {
                iterItem = heldItemPackets.erase(iterItem);
                continue;
            }

            ++iterItem;
        }

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
                    //Fix glitch getting out of vehicle:
                    if(newDynamics[a]->body)
                    {
                        if(currentPlayer)
                        {
                            currentPlayer->flingPreventionStartTime = SDL_GetTicks();
                            currentPlayer->lastPlayerControl = SDL_GetTicks();
                        }
                        btTransform t = newDynamics[a]->body->getWorldTransform();
                        glm::vec3 o = newDynamics[a]->modelInterpolator.getPosition();
                        t.setOrigin(btVector3(o.x,o.y,o.z));
                        newDynamics[a]->body->setLinearVelocity(btVector3(0,0,0));
                        newDynamics[a]->body->setAngularVelocity(btVector3(0,0,0));
                        newDynamics[a]->body->setWorldTransform(t);
                        //std::cout<<"\nPhysics subpacket 0 "<<o.x<<","<<o.y<<","<<o.z<<"\n";
                    }

                    newDynamics[a]->createBoxBody(world,tmp.finalHalfExtents,tmp.finalOffset,tmp.initPos); //will return it it already has one
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
        clientStuff *clientEnvironment = (clientStuff*)userData;
        serverStuff *serverData = clientEnvironment->serverData;

        //if(packetType != 7)
            //std::cout<<"Got "<<(data->critical?"critical":"normal")<<" packet type: "<<packetType<<" streampos: "<<data->getStreamPos()<<" allocated bytes: "<<data->allocatedChunks<<"\n";
        //if(packetType >= 0 && packetType <= 31)
            //++serverData->numGottenPackets[packetType];

        if(packetType != packetType_serverCommand && clientEnvironment->ignoreGamePackets)
            return;

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
                    tmp.initPos.setX(data->readFloat());
                    tmp.initPos.setY(data->readFloat());
                    tmp.initPos.setZ(data->readFloat());
                    serverData->canJet = data->readBit();

                    for(int a = 0; a<serverData->clientPhysicsPackets.size(); a++)
                    {
                        if(serverData->clientPhysicsPackets[a].dynamicID == tmp.dynamicID)
                        {
                            serverData->clientPhysicsPackets.erase(serverData->clientPhysicsPackets.begin() + a);
                            break;
                        }
                    }

                    serverData->clientPhysicsPackets.push_back(tmp);
                }
                else if(subtype == 1) //pause using client physics for dynamic
                {
                    serverData->giveUpControlOfCurrentPlayer = true;
                }
                else if(subtype == 2) //delete client physics for dynamic
                {
                    unsigned int netID = data->readUInt(dynamicObjectIDBits);
                    if(serverData->currentPlayer && serverData->currentPlayer->body && serverData->currentPlayer->serverID == netID)
                    {
                        serverData->currentPlayer->world->removeRigidBody(serverData->currentPlayer->body);
                        delete serverData->currentPlayer->defaultMotionState;
                        delete serverData->currentPlayer->shape;
                        serverData->currentPlayer->body = 0;
                        serverData->currentPlayer = 0;
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
                    serverData->canJet = data->readBit();

                    //std::cout<<"\nPhysics subtype 3 "<<posX<<","<<posY<<","<<posZ<<"\n";

                    if(serverData->currentPlayer && serverData->currentPlayer->body)
                    {
                        btTransform t;
                        t.setIdentity();
                        t.setOrigin(btVector3(posX,posY,posZ));
                        t.setRotation(btQuaternion(rotX,rotY,rotZ,rotW));
                        serverData->currentPlayer->body->setWorldTransform(t);
                        serverData->currentPlayer->body->setLinearVelocity(btVector3(velX,velY,velZ));
                        serverData->currentPlayer->flingPreventionStartTime = SDL_GetTicks();
                    }
                }

                return;
            }

            case packetType_addRemoveLight:
            {
                int id = data->readUInt(20);
                bool addOrRemove = data->readBit();

                if(addOrRemove) //adding or updating light
                {
                    light *tmp = 0;
                    bool needNewLight = true;
                    for(int a = 0; a<serverData->lights.size(); a++)
                    {
                        if(serverData->lights[a]->serverID == id)
                        {
                            needNewLight = false;
                            tmp = serverData->lights[a];
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
                        tmp->attachedBrick = serverData->staticBricks.getBrick(brickID,isSpecial);
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
                        for(int a = 0; a<serverData->newDynamics.size(); a++)
                        {
                            if(serverData->newDynamics[a]->serverID == dynamicID)
                            {
                                tmp->attachedDynamic = serverData->newDynamics[a];
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
                        for(int a = 0; a<serverData->items.size(); a++)
                        {
                            if(serverData->items[a]->serverID == dynamicID)
                            {
                                tmp->attachedItem = serverData->items[a];
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
                        for(int a = 0; a<serverData->livingBricks.size(); a++)
                        {
                            if(serverData->livingBricks[a]->serverID == carID)
                            {
                                tmp->attachedCar = serverData->livingBricks[a];
                                break;
                            }
                        }

                        if(!tmp->attachedCar)
                        {
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
                            float rVel = data->readFloat();
                            float gVel = data->readFloat();
                            float bVel = data->readFloat();
                            float yawVel = data->readFloat();
                            tmp->yawVel = yawVel;
                            tmp->blinkVel = glm::vec3(rVel,gVel,bVel);
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

                            serverData->heldLightPackets.push_back(heldTmp);

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
                    float rVel = data->readFloat();
                    float gVel = data->readFloat();
                    float bVel = data->readFloat();
                    float yawVel = data->readFloat();
                    tmp->yawVel = yawVel;
                    tmp->blinkVel = glm::vec3(rVel,gVel,bVel);
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
                        serverData->lights.push_back(tmp);
                }
                else
                {
                    for(int a = 0; a<serverData->lights.size(); a++)
                    {
                        if(serverData->lights[a]->serverID == id)
                        {
                            delete serverData->lights[a];
                            serverData->lights[a] = 0;
                            serverData->lights.erase(serverData->lights.begin() + a);
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
                    serverData->ropes.push_back(tmp);
                }
                else
                {
                    int id = data->readUInt(12);

                    for(int a = 0; a<serverData->ropes.size(); a++)
                    {
                        if(serverData->ropes[a]->serverID == id)
                        {
                            delete serverData->ropes[a];
                            serverData->ropes[a] = 0;
                            serverData->ropes.erase(serverData->ropes.begin() + a);
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
                    for(unsigned int a = 0; a<serverData->emitters.size(); a++)
                    {
                        if(serverData->emitters[a]->serverID == serverID)
                        {
                            vecPos = a;
                            tmp = serverData->emitters[a];
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
                    for(unsigned int a = 0; a<serverData->emitterTypes.size(); a++)
                    {
                        if(serverData->emitterTypes[a]->serverID == typeID)
                        {
                            tmp->type = serverData->emitterTypes[a];
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                    {
                        std::cout<<"Got request to create emitter with serverID "<<serverID<<" and type " <<typeID<<" "<<(newEmitter?"new":"found")<<"\n";
                        if(vecPos != -1)
                            serverData->emitters.erase(serverData->emitters.begin() + vecPos);
                        delete tmp;
                        error("Could not find emitter type " + std::to_string(typeID));
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
                        brickRenderData *brick = serverData->staticBricks.getBrick(brickID,isSpecial);
                        if(!brick)
                        {
                            if(vecPos != -1)
                                serverData->emitters.erase(serverData->emitters.begin() + vecPos);
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
                        for(unsigned int a = 0; a<serverData->newDynamics.size(); a++)
                        {
                            if(dynamicID == serverData->newDynamics[a]->serverID)
                            {
                                attached = serverData->newDynamics[a];
                                break;
                            }
                        }
                        if(!attached)
                        {
                            if(vecPos != -1)
                                serverData->emitters.erase(serverData->emitters.begin() + vecPos);
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
                            serverData->emitters.erase(serverData->emitters.begin() + vecPos);
                        delete tmp;
                        error("Bad attachment type for emitter.");
                        return;
                    }
                    if(vecPos == -1)
                        serverData->emitters.push_back(tmp);
                }
                else
                {
                    int serverID = data->readUInt(20);
                    for(unsigned int a = 0; a<serverData->emitters.size(); a++)
                    {
                        if(serverData->emitters[a]->serverID == serverID)
                        {
                            delete serverData->emitters[a];
                            serverData->emitters[a] = 0;
                            serverData->emitters.erase(serverData->emitters.begin() + a);
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

                    //std::cout<<"Adding emitter type "<<serverID<<"\n";

                    bool foundEmitterType = false;
                    emitterType *tmp = 0;

                    for(unsigned int a = 0; a<serverData->emitterTypes.size(); a++)
                    {
                        if(serverData->emitterTypes[a]->serverID == serverID)
                        {
                            tmp = serverData->emitterTypes[a];
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
                        for(unsigned int b = 0; b<serverData->particleTypes.size(); b++)
                        {
                            if(particleID == serverData->particleTypes[b]->serverID)
                            {
                                found = serverData->particleTypes[b];
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
                        {
                            serverData->wheelDirtEmitter = tmp;

                            for(unsigned int b = 0; b<serverData->livingBricks.size(); b++)
                            {
                                livingBrick *found = serverData->livingBricks[b];

                                for(unsigned int a = 0; a<found->wheelBrickData.size(); a++)
                                {
                                    if(found->wheelBrickData[a].dirtEmitter)
                                        continue;

                                    emitter *dirtEmitter = new emitter;
                                    dirtEmitter->creationTime = SDL_GetTicks();
                                    dirtEmitter->type = serverData->wheelDirtEmitter;
                                    dirtEmitter->whichWheel = a;
                                    dirtEmitter->attachedToCar = found;
                                    dirtEmitter->serverID = -1;
                                    found->wheelBrickData[a].dirtEmitter = dirtEmitter;
                                    serverData->emitters.push_back(dirtEmitter);
                                }
                            }
                        }
                    }

                    if(!foundEmitterType)
                        serverData->emitterTypes.push_back(tmp);
                }
                else //Particle:
                {
                    int serverID = data->readUInt(10);

                    particleType *tmp = 0;
                    bool foundParticleType = false;

                    for(unsigned int a = 0; a<serverData->particleTypes.size(); a++)
                    {
                        if(serverData->particleTypes[a]->serverID == serverID)
                        {
                            tmp = serverData->particleTypes[a];
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
                        serverData->particleTypes.push_back(tmp);
                }
                return;
            }
            case packetType_setNodeColors:
            {
                int dynamicID = data->readUInt(dynamicObjectIDBits);

                int decal = data->readUInt(7);
                //std::cout<<"Decal: "<<decal<<"\n";

                newDynamic *tmp = 0;

                for(unsigned int a = 0; a<serverData->newDynamics.size(); a++)
                {
                    if(serverData->newDynamics[a]->serverID == dynamicID)
                    {
                        tmp = serverData->newDynamics[a];
                        break;
                    }
                }

                heldDynamicPacket *placeHolder = 0;
                if(!tmp)
                {
                    for(int a = 0; a<serverData->heldAppearancePackets.size(); a++)
                    {
                        if(serverData->heldAppearancePackets[a].dynamicID == dynamicID)
                        {
                            placeHolder = &serverData->heldAppearancePackets[a];
                            break;
                        }
                    }

                    if(!placeHolder)
                    {
                        serverData->heldAppearancePackets.push_back(heldDynamicPacket());
                        placeHolder = &serverData->heldAppearancePackets[serverData->heldAppearancePackets.size()-1];
                    }
                }

                if(placeHolder)
                    placeHolder->deletionTime = SDL_GetTicks() + 1000;

                if(decal >= 0 && decal < clientEnvironment->picker->faceDecals.size())
                {
                    if(tmp)
                        tmp->decal = clientEnvironment->picker->faceDecals[decal];
                    else if(placeHolder)
                        placeHolder->decal = clientEnvironment->picker->faceDecals[decal];
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
                    clientEnvironment->bottomPrint.setText(text,timeoutMS);
                    return;
                }

                if(messageLocation == 2)
                {
                    std::string text = data->readString();
                    textBoxAdd(clientEnvironment->evalWindow->getChild("Code/Listbox"),text);
                    return;
                }

                std::string message = data->readString();
                std::string category = data->readString();

                float chatScroll = clientEnvironment->chat->getVertScrollbar()->getScrollPosition();
                float maxScroll = clientEnvironment->chat->getVertScrollbar()->getDocumentSize() - clientEnvironment->chat->getVertScrollbar()->getPageSize();

                textBoxAdd(clientEnvironment->chat,message,0,false);

                if(clientEnvironment->chat->getItemCount() > 100)
                {
                    CEGUI::ListboxItem *line = clientEnvironment->chat->getListboxItemFromIndex(0);
                    if(line)
                        clientEnvironment->chat->removeItem(line);
                }

                if(chatScroll >= maxScroll)
                    clientEnvironment->chat->getVertScrollbar()->setScrollPosition(clientEnvironment->chat->getVertScrollbar()->getDocumentSize() - clientEnvironment->chat->getVertScrollbar()->getPageSize());

                //std::cout<<"Chat message added: "<<message<<"| |"<<category<<"\n";

                return;
            }
            case packetType_debugLocations:
            {
                std::cout<<"Debug locations...\n";

                serverData->debugColors.clear();
                serverData->debugLocations.clear();

                int howMany = data->readUInt(6);
                for(int a = 0; a<howMany; a++)
                {
                    float r = data->readFloat();
                    float g = data->readFloat();
                    float b = data->readFloat();
                    float x = data->readFloat();
                    float y = data->readFloat();
                    float z = data->readFloat();

                    serverData->debugColors.push_back(glm::vec3(r,g,b));
                    serverData->debugLocations.push_back(glm::vec3(x,y,z));
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

                bool colliding = data->readBit();

                int printMask = 0;
                std::string printName = "";
                bool hasPrint = data->readBit();
                if(hasPrint)
                {
                    printMask = data->readUInt(6);
                    printName = data->readString();
                }

                for(int b = 0; b<serverData->staticBricks.opaqueBasicBricks.size(); b++)
                {
                    if(!serverData->staticBricks.opaqueBasicBricks[b])
                        continue;
                    if(serverData->staticBricks.opaqueBasicBricks[b]->serverID == id)
                    {
                        float oldTrans = serverData->staticBricks.opaqueBasicBricks[b]->color.a;
                        bool changedTrans = (oldTrans < 0.99 && al > 0.99) || (oldTrans > 0.99 && al < 0.99);
                        serverData->staticBricks.opaqueBasicBricks[b]->color = glm::vec4(r,g,blu,al);
                        serverData->staticBricks.opaqueBasicBricks[b]->position = glm::vec3(x,y,z);
                        serverData->staticBricks.opaqueBasicBricks[b]->rotation = serverData->staticBricks.rotations[angleID];
                        serverData->staticBricks.opaqueBasicBricks[b]->material = material;
                        serverData->staticBricks.opaqueBasicBricks[b]->shouldCollide = colliding;

                        if(hasPrint)
                        {
                            serverData->staticBricks.opaqueBasicBricks[b]->printID = clientEnvironment->prints->getPrintID(printName);
                            if(serverData->staticBricks.opaqueBasicBricks[b]->printID != 0)
                            {
                                serverData->staticBricks.opaqueBasicBricks[b]->hasPrint = true;
                                serverData->staticBricks.opaqueBasicBricks[b]->printMask = printMask;
                                serverData->staticBricks.opaqueBasicBricks[b]->dimensions.w = printMask;
                            }
                        }
                        else
                            serverData->staticBricks.opaqueBasicBricks[b]->dimensions.w = 0;

                        serverData->staticBricks.updateBasicBrick(serverData->staticBricks.opaqueBasicBricks[b],serverData->world,changedTrans);
                        return;
                    }
                }

                for(int b = 0; b<serverData->staticBricks.transparentBasicBricks.size(); b++)
                {
                    if(!serverData->staticBricks.transparentBasicBricks[b])
                        continue;
                    if(serverData->staticBricks.transparentBasicBricks[b]->serverID == id)
                    {
                        float oldTrans = serverData->staticBricks.opaqueBasicBricks[b]->color.a;
                        bool changedTrans = (oldTrans < 0.99 && al > 0.99) || (oldTrans > 0.99 && al < 0.99);
                        serverData->staticBricks.transparentBasicBricks[b]->color = glm::vec4(r,g,blu,al);
                        serverData->staticBricks.transparentBasicBricks[b]->position = glm::vec3(x,y,z);
                        serverData->staticBricks.transparentBasicBricks[b]->rotation = serverData->staticBricks.rotations[angleID];
                        serverData->staticBricks.transparentBasicBricks[b]->material = material;
                        serverData->staticBricks.transparentBasicBricks[b]->shouldCollide = colliding;

                        if(hasPrint)
                        {
                            serverData->staticBricks.transparentBasicBricks[b]->printID = clientEnvironment->prints->getPrintID(printName);
                            if(serverData->staticBricks.transparentBasicBricks[b]->printID != 0)
                            {
                                serverData->staticBricks.transparentBasicBricks[b]->hasPrint = true;
                                serverData->staticBricks.transparentBasicBricks[b]->printMask = printMask;
                            }
                        }

                        serverData->staticBricks.updateBasicBrick(serverData->staticBricks.transparentBasicBricks[b],serverData->world,changedTrans);
                        return;
                    }
                }

                for(int b = 0; b<serverData->staticBricks.opaqueSpecialBricks.size(); b++)
                {
                    if(!serverData->staticBricks.opaqueSpecialBricks[b])
                        continue;
                    if(serverData->staticBricks.opaqueSpecialBricks[b]->serverID == id)
                    {
                        serverData->staticBricks.opaqueSpecialBricks[b]->color = glm::vec4(r,g,blu,al);
                        serverData->staticBricks.opaqueSpecialBricks[b]->position = glm::vec3(x,y,z);
                        serverData->staticBricks.opaqueSpecialBricks[b]->rotation = serverData->staticBricks.rotations[angleID];
                        serverData->staticBricks.opaqueSpecialBricks[b]->material = material;
                        serverData->staticBricks.opaqueSpecialBricks[b]->shouldCollide = colliding;
                        serverData->staticBricks.updateSpecialBrick(serverData->staticBricks.opaqueSpecialBricks[b],serverData->world,angleID);
                        return;
                    }
                }

                for(int b = 0; b<serverData->staticBricks.transparentSpecialBricks.size(); b++)
                {
                    if(!serverData->staticBricks.transparentSpecialBricks[b])
                        continue;
                    if(serverData->staticBricks.transparentSpecialBricks[b]->serverID == id)
                    {
                        serverData->staticBricks.transparentSpecialBricks[b]->color = glm::vec4(r,g,blu,al);
                        serverData->staticBricks.transparentSpecialBricks[b]->position = glm::vec3(x,y,z);
                        serverData->staticBricks.transparentSpecialBricks[b]->rotation = serverData->staticBricks.rotations[angleID];
                        serverData->staticBricks.transparentSpecialBricks[b]->material = material;
                        serverData->staticBricks.transparentSpecialBricks[b]->shouldCollide = colliding;
                        serverData->staticBricks.updateSpecialBrick(serverData->staticBricks.transparentSpecialBricks[b],serverData->world,angleID);
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
                    CEGUI::MultiColumnList *playerList = (CEGUI::MultiColumnList*)clientEnvironment->playerList->getChild("List");

                    if(!playerList)
                    {
                        error("Could not find player list!");
                        return;
                    }

                    if(addOrRemove) //adding
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
                    else //removing
                    {
                        CEGUI::ListboxItem *entry = playerList->findListItemWithText(name,NULL);
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
                    for(unsigned int a = 0; a<serverData->typingPlayersNames.size(); a++)
                    {
                        if(serverData->typingPlayersNames[a] == name)
                        {
                            if(!addOrRemove)
                            {
                                serverData->typingPlayersNames.erase(serverData->typingPlayersNames.begin() + a);
                                serverData->typingPlayerNameString = "";
                                for(unsigned int b = 0; b<serverData->typingPlayersNames.size(); b++)
                                    serverData->typingPlayerNameString = serverData->typingPlayerNameString + serverData->typingPlayersNames[b] + " ";
                                clientEnvironment->whosTyping->setText(serverData->typingPlayerNameString);
                            }
                            return;
                        }
                    }

                    serverData->typingPlayersNames.push_back(name);
                    serverData->typingPlayerNameString = "";
                    for(unsigned int b = 0; b<serverData->typingPlayersNames.size(); b++)
                        serverData->typingPlayerNameString = serverData->typingPlayerNameString + serverData->typingPlayersNames[b] + " ";
                    clientEnvironment->whosTyping->setText(serverData->typingPlayerNameString);
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
                    clientEnvironment->inventoryBox->getChild("ItemName" + std::to_string(slot+1))->setText("");
                    serverData->inventory[slot] = 0;
                    return;
                }
                else
                {
                    int itemType = data->readUInt(10);

                    for(unsigned int a = 0; a<serverData->itemTypes.size(); a++)
                    {
                        if(serverData->itemTypes[a]->serverID == itemType)
                        {
                            clientEnvironment->inventoryBox->getChild("ItemName" + std::to_string(slot+1))->setText(serverData->itemTypes[a]->uiName);
                            serverData->inventory[slot] = serverData->itemTypes[a];
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
                    clientEnvironment->context->setMouseLock(false);
                    clientEnvironment->wheelWrench->moveToFront();
                    clientEnvironment->wheelWrench->setVisible(true);

                    if(!((CEGUI::ToggleButton*)clientEnvironment->wheelWrench->getChild("Copy"))->isSelected())
                    {
                        clientEnvironment->wheelWrench->getChild("BreakForce")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("SteeringForce")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("EngineForce")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("SuspensionLength")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("SuspensionStiffness")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("Friction")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("RollInfluence")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("DampingCompression")->setText(std::to_string(data->readFloat()));
                        clientEnvironment->wheelWrench->getChild("DampingRelaxation")->setText(std::to_string(data->readFloat()));
                    }

                    return;
                }

                bool isSteering = data->readBit();
                if(isSteering)
                {
                    clientEnvironment->context->setMouseLock(false);
                    clientEnvironment->steeringWrench->moveToFront();
                    clientEnvironment->steeringWrench->setVisible(true);

                    clientEnvironment->steeringWrench->getChild("Mass")->setText(std::to_string(data->readFloat()));
                    clientEnvironment->steeringWrench->getChild("Damping")->setText(std::to_string(data->readFloat()));
                    ((CEGUI::ToggleButton*)clientEnvironment->steeringWrench->getChild("CenterMass"))->setSelected(data->readBit());

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


                clientEnvironment->context->setMouseLock(false);
                clientEnvironment->wrench->setVisible(true);
                clientEnvironment->wrench->moveToFront();
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("Colliding"))->setSelected(colliding);
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("RenderDown"))->setSelected(renderDown);
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("RenderUp"))->setSelected(renderUp);
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("RenderNorth"))->setSelected(renderNorth);
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("RenderSouth"))->setSelected(renderSouth);
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("RenderEast"))->setSelected(renderEast);
                ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("RenderWest"))->setSelected(renderWest);
                //((CEGUI::Combobox*)clientEnvironment->wrench->getChild("MusicDropdown"))->setItemSelectState(musicId,true);
                CEGUI::Combobox *musicDropdown = ((CEGUI::Combobox*)clientEnvironment->wrench->getChild("MusicDropdown"));
                CEGUI::Slider *pitchSlider = ((CEGUI::Slider*)clientEnvironment->wrench->getChild("PitchSlider"));
                pitchSlider->setCurrentValue(pitch*100);
                clientEnvironment->wrench->getChild("BrickName")->setText(name);

                bool hasLight = data->readBit();
                if(hasLight)
                {
                    ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("UseLight"))->setSelected(true);

                    float red = data->readFloat();
                    float green = data->readFloat();
                    float blue = data->readFloat();

                    clientEnvironment->wrench->getChild("Red")->setText(std::to_string(red));
                    clientEnvironment->wrench->getChild("Green")->setText(std::to_string(green));
                    clientEnvironment->wrench->getChild("Blue")->setText(std::to_string(blue));

                    bool isSpotlight = data->readBit();
                    if(isSpotlight)
                    {
                        ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("IsSpotlight"))->setSelected(true);
                        float phi = data->readFloat();
                        float yaw = data->readFloat();
                        float lightPitch = data->readFloat();

                        CEGUI::Slider *phiSlider = ((CEGUI::Slider*)clientEnvironment->wrench->getChild("PhiSlider"));
                        phiSlider->setCurrentValue(phi);
                        CEGUI::Slider *yawSlider = ((CEGUI::Slider*)clientEnvironment->wrench->getChild("YawSlider"));
                        yawSlider->setCurrentValue(yaw);
                        CEGUI::Slider *lightPitchSlider = ((CEGUI::Slider*)clientEnvironment->wrench->getChild("LightPitchSlider"));
                        lightPitchSlider->setCurrentValue(lightPitch);
                    }
                    else
                        ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("IsSpotlight"))->setSelected(false);
                }
                else
                    ((CEGUI::ToggleButton*)clientEnvironment->wrench->getChild("UseLight"))->setSelected(false);

                musicDropdown->clearAllSelections();

                std::string text = "";
                for(unsigned int a = 0; a<clientEnvironment->speaker->sounds.size(); a++)
                {
                    if(clientEnvironment->speaker->sounds[a]->serverID == musicId)
                    {
                        CEGUI::ListboxItem *item = musicDropdown->findItemWithText(clientEnvironment->speaker->sounds[a]->scriptName,NULL);
                        if(!item)
                            error("Error selecting music " + clientEnvironment->speaker->sounds[a]->scriptName);
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

                replaceAll(file,"add-ons/","cache/");

                int id = data->readUInt(10);
                bool isMusic = data->readBit();

                if(name.length() == 0)
                    return;

                clientEnvironment->speaker->loadSound(id,file,name,isMusic);
                return;
            }
            case packetType_playSound:
            {
                int id = data->readUInt(10);
                bool loop = data->readBit();

                if(loop)
                {
                    int loopId = data->readUInt(24);
                    float pitch = data->readFloat();
                    bool onCar = data->readBit();
                    if(onCar)
                    {
                        int carId = data->readUInt(10);
                        livingBrick *car = 0;
                        for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                        {
                            if(serverData->livingBricks[a]->serverID == carId)
                            {
                                car = serverData->livingBricks[a];
                                break;
                            }
                        }
                        if(!car)
                        {
                            error("Could not find car " + std::to_string(carId) + " to attach music!");
                            return;
                        }
                        location *soundLocation = new location(car);
                        clientEnvironment->speaker->playSound(id,soundLocation,pitch,1.0,loopId);
                    }
                    else
                    {
                        float x = data->readFloat();
                        float y = data->readFloat();
                        float z = data->readFloat();
                        location *soundLocation = new location(glm::vec3(x,y,z));
                        clientEnvironment->speaker->playSound(id,soundLocation,pitch,1.0,loopId);
                    }
                    return;
                }

                bool flatSound = data->readBit();
                if(flatSound)
                {
                    float pitch = data->readFloat();
                    float vol = data->readFloat();
                    clientEnvironment->speaker->playSound(id,NULL,pitch,vol);
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
                    float pitch = data->readFloat();
                    float vol = data->readFloat();
                    location *soundLocation = new location(glm::vec3(x,y,z));
                    clientEnvironment->speaker->playSound(id,soundLocation,pitch,vol);
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
                    for(unsigned int a = 0; a<serverData->itemTypes.size(); a++)
                    {
                        if(serverData->itemTypes[a]->serverID == typeId)
                        {
                            type = serverData->itemTypes[a];
                            break;
                        }
                    }

                    if(!type)
                    {
                        error("Could not find item type " + std::to_string(typeId));
                        return;
                    }

                    item *tmp = new item(serverData->world,type->type,type->type->networkScale);
                    tmp->serverID = itemId;
                    tmp->itemType = type;
                    serverData->items.push_back(tmp);
                }
                else
                {
                    int itemId = data->readUInt(dynamicObjectIDBits);

                    for(unsigned int a = 0; a<serverData->items.size(); a++)
                    {
                        if(serverData->items[a]->serverID == itemId)
                        {
                            for(int l = 0; l<location::locations.size(); l++)
                            {
                                if(location::locations[l]->dynamic == serverData->items[a])
                                    location::locations[l]->dynamic = 0;
                            }

                            std::vector<emitter*> justToDebugPrint;
                            int removedEmitters = 0;
                            auto iterEmitter = serverData->emitters.begin();
                            while(iterEmitter != serverData->emitters.end())
                            {
                                if((*iterEmitter)->attachedToModel == serverData->items[a])
                                {
                                    justToDebugPrint.push_back((*iterEmitter));
                                    delete (*iterEmitter);
                                    iterEmitter = serverData->emitters.erase(iterEmitter);
                                    removedEmitters++;
                                    continue;
                                }
                                if((*iterEmitter)->attachedToItem == serverData->items[a])
                                {
                                    justToDebugPrint.push_back((*iterEmitter));
                                    delete (*iterEmitter);
                                    iterEmitter = serverData->emitters.erase(iterEmitter);
                                    removedEmitters++;
                                    continue;
                                }
                                ++iterEmitter;
                            }
                            if(removedEmitters > 1)
                            {
                                std::cout<<"More than one removed emitter, removing dynamic!\n";
                                for(int z = 0; z<justToDebugPrint.size(); z++)
                                    std::cout<<justToDebugPrint[z]<<"\n";
                            }

                            delete serverData->items[a];
                            serverData->items[a] = 0;
                            serverData->items.erase(serverData->items.begin() + a);
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

                std::string iconPath = data->readString();

                float rotW = data->readFloat();
                float rotX = data->readFloat();
                float rotY = data->readFloat();
                float rotZ = data->readFloat();

                if(!CEGUI::System::getSingleton().getRenderer()->isTextureDefined(uiName) && !CEGUI::ImageManager::getSingleton().isImageTypeAvailable(uiName) && !CEGUI::ImageManager::getSingleton().isDefined(uiName))
                    CEGUI::ImageManager::getSingleton().addFromImageFile(uiName, iconPath,"/");

                for(unsigned int a = 0; a<serverData->newDynamicTypes.size(); a++)
                {
                    if(serverData->newDynamicTypes[a]->serverID == modelId)
                    {
                        heldItemType *tmp = new heldItemType;
                        tmp->useDefaultSwing = defaultSwing;
                        tmp->serverID = itemId;
                        tmp->type = serverData->newDynamicTypes[a];
                        tmp->handOffset = glm::vec3(x,y,z);
                        tmp->uiName = uiName;
                        tmp->handRot = glm::quat(rotW,rotX,rotY,rotZ);
                        tmp->waitingForModel = false;
                        for(unsigned int b = 0; b<serverData->newDynamicTypes[a]->animations.size(); b++)
                        {
                            if(serverData->newDynamicTypes[a]->animations[b].serverID == swingAnim)
                            {
                                tmp->fireAnim = &serverData->newDynamicTypes[a]->animations[b];
                            }
                            if(serverData->newDynamicTypes[a]->animations[b].serverID == switchAnim)
                            {
                                tmp->switchAnim = &serverData->newDynamicTypes[a]->animations[b];
                            }
                        }
                        serverData->itemTypes.push_back(tmp);

                        if(uiName == std::string("Paint Can"))
                        {
                            serverData->paintCan = tmp;
                            serverData->fixedPaintCanItem = new item(serverData->world,tmp->type,glm::vec3(0.02,0.02,0.02));
                            serverData->fixedPaintCanItem->itemType = tmp;
                            serverData->items.push_back(serverData->fixedPaintCanItem);

                            /*newDynamic *itemIcon = new newDynamic(serverData->newDynamicTypes[a]);
                            itemIcon->hidden = true;
                            tmp->icon = itemIcon;
                            itemIcon->setAllFlag(meshFlag_skipCameraMatrix);
                            serverData->itemIcons.push_back(itemIcon);
                            itemIcon->useGlobalTransform = true;
                            itemIcon->calculateMeshTransforms(0);
                            itemIcon->bufferSubData();*/
                        }
                        /*else
                        {
                            serverData->itemIcons.push_back(0);
                            tmp->icon = 0;
                        }*/

                        finishLoadingTypesCheck(clientEnvironment);
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
                serverData->itemTypes.push_back(tmp);
                //serverData->itemIcons.push_back(0);

                if(uiName == "Paint Can")
                {
                    serverData->paintCan = tmp;
                    serverData->fixedPaintCanItem = new item(serverData->world,tmp->type,tmp->type->networkScale);
                    serverData->fixedPaintCanItem->itemType = tmp;
                    serverData->items.push_back(serverData->fixedPaintCanItem);
                }

                error("Could not add item with model ID: " + std::to_string(modelId));

                finishLoadingTypesCheck(clientEnvironment);

                return;
            }
            case packetType_setShapeName:
            {
                int id = data->readUInt(dynamicObjectIDBits);
                std::string shapeName = data->readString();
                float r = data->readFloat();
                float g = data->readFloat();
                float b = data->readFloat();

                for(int a = 0; a<serverData->newDynamics.size(); a++)
                {
                    if(serverData->newDynamics[a]->serverID == id)
                    {
                        serverData->newDynamics[a]->shapeName = shapeName;
                        serverData->newDynamics[a]->shapeNameColor = glm::vec3(r,g,b);
                        //serverData->newDynamics[a]->colorTint = glm::vec3(r,g,b);
                        return;
                    }
                }

                error("Could not find id " + std::to_string(id) + " to add shape name!");
                return;
            }
            case packetType_removeBrickVehicle:
            {
                int id = data->readUInt(10);
                for(int a = 0; a<serverData->livingBricks.size(); a++)
                {
                    if(serverData->livingBricks[a]->serverID == id)
                    {
                        /*for(unsigned int b = 0; b<32; b++)
                        {
                            if(clientEnvironment->speaker->carToTrack[b] == serverData->livingBricks[a])
                            {
                                alSourceStop(clientEnvironment->speaker->sources[b]);
                                clientEnvironment->speaker->carToTrack[b] = 0;
                            }
                        }*/

                        auto lightIter = serverData->lights.begin();
                        while(lightIter != serverData->lights.end())
                        {
                            light *l = *lightIter;
                            if(l->attachedCar == serverData->livingBricks[a])
                            {
                                delete l;
                                l = 0;
                                lightIter = serverData->lights.erase(lightIter);
                                continue;
                            }
                            ++lightIter;
                        }

                        auto emitterIter = serverData->emitters.begin();
                        while(emitterIter != serverData->emitters.end())
                        {
                            emitter *e = *emitterIter;
                            if(e->attachedToCar == serverData->livingBricks[a])
                            {
                                delete e;
                                e = 0;
                                emitterIter = serverData->emitters.erase(emitterIter);
                                continue;
                            }
                            ++emitterIter;
                        }

                        for(int l = 0; l<location::locations.size(); l++)
                        {
                            if(location::locations[l]->car == serverData->livingBricks[a])
                                location::locations[l]->car = 0;
                        }

                        for(int b = 0; b<serverData->livingBricks[a]->newWheels.size(); b++)
                            delete serverData->livingBricks[a]->newWheels[b];
                        serverData->livingBricks[a]->newWheels.clear();

                        delete serverData->livingBricks[a];
                        serverData->livingBricks.erase(serverData->livingBricks.begin() + a);
                        return;
                    }
                }
                return;
            }
            case packetType_setColorPalette:
            {
                int howMany = data->readUInt(8);
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
                    if(paletteIdx >= 40)
                        continue;
                    clientEnvironment->palette->setColor(paletteIdx,glm::vec4(r,g,b,al));
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
                for(int a = 0; a<serverData->livingBricks.size(); a++)
                {
                    if(serverData->livingBricks[a])
                    {
                        if(serverData->livingBricks[a]->serverID == id)
                        {
                            found = serverData->livingBricks[a];
                            break;
                        }
                    }
                }

                //std::cout<<"Received packet, id "<<id<<" wheels: "<<wheels<<"\n";

                if(!found)
                {
                    //std::cout<<"Making new car...\n";
                    found = new livingBrick;

                    found->serverID = id;
                    serverData->livingBricks.push_back(found);
                    found->allocateVertBuffer();
                    found->allocatePerTexture(clientEnvironment->brickMat);
                    found->allocatePerTexture(clientEnvironment->brickMatSide,true,true);
                    found->allocatePerTexture(clientEnvironment->brickMatBottom,true);
                    found->allocatePerTexture(clientEnvironment->brickMatRamp);
                    found->addBlocklandCompatibility(serverData->staticBricks.blocklandTypes);
                    //for(int a = 0; a<prints.names.size(); a++)
                        //found->allocatePerTexture(prints.textures[a],false,false,true);
                }


                if(radii)
                {
                    //while(found->wheels.size() < wheels)
                        //found->wheels.push_back(new interpolator);
                    while(found->newWheels.size() < wheels)
                        found->newWheels.push_back(new newDynamic(clientEnvironment->newWheelModel));
                    for(unsigned int a = 0; a<wheels; a++)
                    {
                        found->newWheels[a]->scale = glm::vec3(radii[a]/1.6,radii[a]/1.6,radii[a]/1.6);
                        found->newWheels[a]->scale.x *= 0.06;
                        found->newWheels[a]->scale.y *= 0.06;
                        found->newWheels[a]->scale.z *= 0.06;

                        if(serverData->wheelDirtEmitter)
                        {
                            emitter *dirtEmitter = new emitter;
                            dirtEmitter->creationTime = SDL_GetTicks();
                            dirtEmitter->type = serverData->wheelDirtEmitter;
                            dirtEmitter->whichWheel = a;
                            dirtEmitter->attachedToCar = found;
                            dirtEmitter->serverID = -1;
                            wheelBrickData[a].dirtEmitter = dirtEmitter;
                            serverData->emitters.push_back(dirtEmitter);
                        }

                        found->wheelBrickData.push_back(wheelBrickData[a]);
                    }

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
                        tmp->rotation = serverData->staticBricks.rotations[angleID];
                        tmp->material = material;
                        int width = data->readUInt(8);
                        int height = data->readUInt(8);
                        int length = data->readUInt(8);
                        //std::cout<<"Dims: "<<width<<","<<height<<","<<length<<"\n";
                        tmp->dimensions = glm::ivec4(width,height,length,0);
                        bool doNotCompile = false;
                        found->addBasicBrick(tmp,angleID,0,(btDynamicsWorld*)0,doNotCompile);
                    }
                    else
                    {
                        int typeID = data->readUInt(10);
                        bool foundSpecial = false;
                        for(int i = 0; i<serverData->staticBricks.blocklandTypes->specialBrickTypes.size(); i++)
                        {
                            if(serverData->staticBricks.blocklandTypes->specialBrickTypes[i]->serverID == typeID)
                            {
                                foundSpecial = true;
                                specialBrickRenderData *tmp = new specialBrickRenderData;
                                tmp->yHalfPos = yHalfPos;
                                tmp->carPlatesUp = platesUp;
                                tmp->serverID = serverID;
                                tmp->color = glm::vec4(r,g,b,al);
                                tmp->position = glm::vec3(x,y,z);
                                tmp->rotation = serverData->staticBricks.rotations[angleID];
                                tmp->material = material;
                                bool doNotCompile = false;
                                found->addSpecialBrick(tmp,(btDynamicsWorld*)0,i,angleID,doNotCompile);

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

                    for(int b = 0; b<serverData->staticBricks.opaqueBasicBricks.size(); b++)
                    {
                        if(!serverData->staticBricks.opaqueBasicBricks[b])
                            continue;
                        if(serverData->staticBricks.opaqueBasicBricks[b]->serverID == id)
                        {
                            if(serverData->staticBricks.opaqueBasicBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                if(serverData->staticBricks.opaqueBasicBricks[b]->body)
                                {
                                    serverData->world->removeRigidBody(serverData->staticBricks.opaqueBasicBricks[b]->body);
                                    delete serverData->staticBricks.opaqueBasicBricks[b]->body;
                                    serverData->staticBricks.opaqueBasicBricks[b]->body = 0;
                                }

                                tmp.basic = serverData->staticBricks.opaqueBasicBricks[b];
                                serverData->staticBricks.opaqueBasicBricks[b]->markedForDeath = true;
                                serverData->fakeKills.push_back(tmp);
                            }
                            else
                                serverData->staticBricks.removeBasicBrick(serverData->staticBricks.opaqueBasicBricks[b],serverData->world);
                            continue;
                        }
                    }
                    for(int b = 0; b<serverData->staticBricks.transparentBasicBricks.size(); b++)
                    {
                        if(!serverData->staticBricks.transparentBasicBricks[b])
                            continue;
                        if(serverData->staticBricks.transparentBasicBricks[b]->serverID == id)
                        {
                            if(serverData->staticBricks.transparentBasicBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                if(serverData->staticBricks.transparentBasicBricks[b]->body)
                                {
                                    serverData->world->removeRigidBody(serverData->staticBricks.transparentBasicBricks[b]->body);
                                    delete serverData->staticBricks.transparentBasicBricks[b]->body;
                                    serverData->staticBricks.transparentBasicBricks[b]->body = 0;
                                }

                                tmp.basic = serverData->staticBricks.transparentBasicBricks[b];
                                serverData->staticBricks.transparentBasicBricks[b]->markedForDeath = true;
                                serverData->fakeKills.push_back(tmp);
                            }
                            else
                                serverData->staticBricks.removeBasicBrick(serverData->staticBricks.transparentBasicBricks[b],serverData->world);
                            continue;
                        }
                    }
                    for(int b = 0; b<serverData->staticBricks.opaqueSpecialBricks.size(); b++)
                    {
                        if(!serverData->staticBricks.opaqueSpecialBricks[b])
                            continue;
                        if(serverData->staticBricks.opaqueSpecialBricks[b]->serverID == id)
                        {
                            if(serverData->staticBricks.opaqueSpecialBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                if(serverData->staticBricks.opaqueSpecialBricks[b]->body)
                                {
                                    serverData->world->removeRigidBody(serverData->staticBricks.opaqueSpecialBricks[b]->body);
                                    delete serverData->staticBricks.opaqueSpecialBricks[b]->body;
                                    serverData->staticBricks.opaqueSpecialBricks[b]->body = 0;
                                }

                                tmp.special = serverData->staticBricks.opaqueSpecialBricks[b];
                                serverData->staticBricks.opaqueSpecialBricks[b]->markedForDeath = true;
                                serverData->fakeKills.push_back(tmp);
                            }
                            else
                                serverData->staticBricks.removeSpecialBrick(serverData->staticBricks.opaqueSpecialBricks[b],serverData->world);
                            continue;
                        }
                    }
                    for(int b = 0; b<serverData->staticBricks.transparentSpecialBricks.size(); b++)
                    {
                        if(!serverData->staticBricks.transparentSpecialBricks[b])
                            continue;
                        if(serverData->staticBricks.transparentSpecialBricks[b]->serverID == id)
                        {
                            if(serverData->staticBricks.transparentSpecialBricks[b]->markedForDeath)
                                continue;
                            if(howMany < 12)
                            {
                                if(serverData->staticBricks.transparentSpecialBricks[b]->body)
                                {
                                    serverData->world->removeRigidBody(serverData->staticBricks.transparentSpecialBricks[b]->body);
                                    delete serverData->staticBricks.transparentSpecialBricks[b]->body;
                                    serverData->staticBricks.transparentSpecialBricks[b]->body = 0;
                                }

                                tmp.special = serverData->staticBricks.transparentSpecialBricks[b];
                                serverData->staticBricks.transparentSpecialBricks[b]->markedForDeath = true;
                                serverData->fakeKills.push_back(tmp);
                            }
                            else
                                serverData->staticBricks.removeSpecialBrick(serverData->staticBricks.transparentSpecialBricks[b],serverData->world);
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
                            for(int i = 0; i<serverData->newDynamics.size(); i++)
                            {
                                if(serverData->newDynamics[i]->serverID == id)
                                {
                                    //std::cout<<id<<" has pos: "<<x<<","<<y<<","<<z<<"\n";
                                    serverData->newDynamics[i]->modelInterpolator.addTransform(packetTime,glm::vec3(x,y,z),glm::quat(rotW,rotX,rotY,rotZ));//,glm::vec3(velX,velY,velZ));
                                    if(isPlayer)
                                    {
                                        glm::mat4 final = glm::translate(glm::vec3(0,176.897,0)) * glm::rotate(glm::mat4(1.0),-pitch,glm::vec3(1.0,0.0,0.0)) * glm::translate(glm::vec3(0,-176.897,0));
                                        /*serverData->newDynamics[i]->setExtraTransform("Head",final);
                                        serverData->newDynamics[i]->setExtraTransform("Face1",final);*/
                                        serverData->newDynamics[i]->setFixedRotation("Head",final);
                                        serverData->newDynamics[i]->setFixedRotation("Face1",final);

                                        if(walking)
                                            serverData->newDynamics[i]->play("walk",false,1.0,true);
                                        else
                                            serverData->newDynamics[i]->stop("walk");
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
                        for(unsigned int a = 0; a<serverData->items.size(); a++)
                        {
                            if(serverData->items[a]->serverID == id)
                            {
                                tmp = serverData->items[a];
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
                            for(unsigned int a = 0; a<serverData->newDynamics.size(); a++)
                            {
                                if(serverData->newDynamics[a]->serverID == heldBy)
                                {
                                    holder = serverData->newDynamics[a];
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

                    for(int i = 0; i<serverData->livingBricks.size(); i++)
                    {
                        if(serverData->livingBricks[i]->serverID == id)
                        {
                            serverData->livingBricks[i]->carTransform.addTransform(packetTime,glm::vec3(x,y,z),glm::quat(rotW,rotX,rotY,rotZ));
                            for(int wheel = 0; wheel<numWheels; wheel++)
                            {
                                serverData->livingBricks[i]->newWheels[wheel]->modelInterpolator.addTransform(packetTime,glm::vec3(wheelX[wheel],wheelY[wheel],wheelZ[wheel]),glm::quat(wheelRotW[wheel],wheelRotX[wheel],wheelRotY[wheel],wheelRotZ[wheel]));
                                //std::cout<<"emitterOn["<<wheel<<"] is "<<(emitterOn[i]?"true":"false")<<" for car ID "<<serverData->livingBricks[i]->serverID<<"\n";
                                if(serverData->livingBricks[i]->wheelBrickData[wheel].dirtEmitter)
                                    ((emitter*)serverData->livingBricks[i]->wheelBrickData[wheel].dirtEmitter)->enabled = emitterOn[i];
                                //else
                                    //std::cout<<"Could not find emitter for car "<<id<<" wheel "<<wheel<<"...\n";
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
                    for(int b = 0; b<serverData->ropes.size(); b++)
                    {
                        if(serverData->ropes[b]->serverID == id)
                        {
                            toUpdate = serverData->ropes[b];
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

            case packetType_serverCommand:
            {
                std::string commandType = data->readString();

                processCommand(clientEnvironment,commandType,data);

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
                        tmp->rotation = serverData->staticBricks.rotations[angleID];
                        tmp->material = material;
                        int width = data->readUInt(8);
                        int height = data->readUInt(8);
                        int length = data->readUInt(8);

                        bool collides = data->readBit();
                        tmp->shouldCollide = collides;

                        bool isPrint = data->readBit();
                        if(isPrint)
                        {
                            int mask = data->readUInt(6);
                            std::string printName = data->readString();

                            //TODO: Replace with server sending printID?
                            tmp->printID = clientEnvironment->prints->getPrintID(printName);
                            if(tmp->printID != 0)
                            {
                                tmp->hasPrint = true;
                                tmp->printMask = mask;
                            }
                        }

                        tmp->dimensions = glm::ivec4(width,height,length,0);
                        bool doNotCompile = false;
                        if(serverData->skippingCompileNextBricks > 0)
                        {
                            if(serverData->staticBricks.getBrickCount() < serverData->skippingCompileNextBricks)
                                doNotCompile = true;
                            else
                            {
                                serverData->skippingCompileNextBricks = 0;
                                serverData->staticBricks.recompileEverything();
                            }

                        }
                        serverData->staticBricks.addBasicBrick(tmp,angleID,0,serverData->world,doNotCompile);
                    }
                    else
                    {
                        int typeID = data->readUInt(10);

                        bool collides = data->readBit();

                        int mask = 0;
                        std::string printName = "";
                        bool isPrint = data->readBit();
                        if(isPrint)
                        {
                            mask = data->readUInt(6);
                            printName = data->readString();
                        }

                        bool foundSpecial = false;
                        for(int i = 0; i<serverData->staticBricks.blocklandTypes->specialBrickTypes.size(); i++)
                        {
                            if(serverData->staticBricks.blocklandTypes->specialBrickTypes[i]->serverID == typeID)
                            {
                                foundSpecial = true;
                                specialBrickRenderData *tmp = new specialBrickRenderData;
                                tmp->shouldCollide = collides;
                                tmp->serverID = serverID;
                                tmp->color = glm::vec4(r,g,b,al);
                                tmp->position = glm::vec3(x,y,z);
                                tmp->rotation = serverData->staticBricks.rotations[angleID];
                                tmp->material = material;

                                if(isPrint)
                                {
                                    //TODO: Replace with server sending printID
                                    for(unsigned int a = 0; a<clientEnvironment->prints->names.size(); a++)
                                    {
                                        if(clientEnvironment->prints->names[a] == printName)
                                        {
                                            tmp->printID = a;
                                            break;
                                        }
                                    }
                                }
                                else
                                    tmp->printID = -1;

                                bool doNotCompile = false;
                                if(serverData->skippingCompileNextBricks > 0)
                                {
                                    if(serverData->staticBricks.getBrickCount() < serverData->skippingCompileNextBricks)
                                        doNotCompile = true;
                                    else
                                    {
                                        serverData->skippingCompileNextBricks = 0;
                                        serverData->staticBricks.recompileEverything();
                                    }
                                }
                                serverData->staticBricks.addSpecialBrick(tmp,serverData->world,i,angleID,doNotCompile);
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

                    for(int a = 0; a<serverData->newDynamics.size(); a++)
                    {
                        if(serverData->newDynamics[a]->serverID == objectID)
                        {
                            error("Dynamic with serverID " + std::to_string(objectID) + " already exists!");
                            return;
                        }
                    }

                    newModel *type = 0;
                    for(int a = 0; a<serverData->newDynamicTypes.size(); a++)
                    {
                        if(serverData->newDynamicTypes[a]->serverID == typeID)
                        {
                            type = serverData->newDynamicTypes[a];
                            break;
                        }
                    }

                    if(type)
                    {
                        //TODO: Let server set scale
                        newDynamic *tmp = new newDynamic(type,type->networkScale);
                        /*tmp->addExtraTransform("Head");
                        tmp->addExtraTransform("Face1");

                        glm::mat4 final = glm::translate(glm::vec3(0,176.897,0)) * glm::rotate(glm::mat4(1.0),glm::radians(0.f),glm::vec3(1.0,0.0,0.0)) * glm::translate(glm::vec3(0,-176.897,0));
                        tmp->setExtraTransform("Head",final);
                        tmp->setExtraTransform("Face1",final);
                        tmp->colorTint = glm::vec3(red,green,blue);*/
                        tmp->serverID = objectID;
                        tmp->shapeNameColor = glm::vec3(red,green,blue);
                        tmp->shapeName = "";
                        serverData->newDynamics.push_back(tmp);
                    }
                    else
                        error("Could not find dynamic type: " + std::to_string(typeID));
                }
                else //remove
                {
                    for(int a = 0; a<serverData->newDynamics.size(); a++)
                    {
                        if(serverData->newDynamics[a]->serverID == objectID)
                        {
                            if(serverData->cameraTarget == serverData->newDynamics[a])
                                serverData->cameraTarget = 0;
                            if(serverData->currentPlayer == serverData->newDynamics[a])
                            {
                                if(serverData->newDynamics[a]->body)
                                {
                                    serverData->newDynamics[a]->world->removeRigidBody(serverData->newDynamics[a]->body);
                                    delete serverData->newDynamics[a]->defaultMotionState;
                                    serverData->newDynamics[a]->defaultMotionState = 0;
                                    delete serverData->newDynamics[a]->shape;
                                    serverData->newDynamics[a]->shape = 0;
                                    serverData->newDynamics[a]->body = 0;
                                }
                                serverData->currentPlayer = 0;
                            }

                            /*for(int b = 0; b<serverData->emitters.size(); b++)
                            {
                                if(serverData->emitters[b]->attachedToModel == serverData->newDynamics[a])
                                {
                                    serverData->emitters[b]->attachedToModel = 0;
                                    delete serverData->emitters[b];
                                    serverData->emitters[b] = 0;
                                    serverData->emitters.erase(serverData->emitters.begin() + b);
                                }
                                if(serverData->emitters[b]->attachedToItem == serverData->newDynamics[a])
                                {
                                    serverData->emitters[b]->attachedToItem = 0;
                                    delete serverData->emitters[b];
                                    serverData->emitters[b] = 0;
                                    serverData->emitters.erase(serverData->emitters.begin() + b);
                                }
                            }*/

                            std::vector<emitter*> justToDebugPrint;
                            int removedEmitters = 0;
                            auto iterEmitter = serverData->emitters.begin();
                            while(iterEmitter != serverData->emitters.end())
                            {
                                if((*iterEmitter)->attachedToModel == serverData->newDynamics[a])
                                {
                                    justToDebugPrint.push_back((*iterEmitter));
                                    delete (*iterEmitter);
                                    iterEmitter = serverData->emitters.erase(iterEmitter);
                                    removedEmitters++;
                                    continue;
                                }
                                if((*iterEmitter)->attachedToItem == serverData->newDynamics[a])
                                {
                                    justToDebugPrint.push_back((*iterEmitter));
                                    delete (*iterEmitter);
                                    iterEmitter = serverData->emitters.erase(iterEmitter);
                                    removedEmitters++;
                                    continue;
                                }
                                ++iterEmitter;
                            }
                            if(removedEmitters > 1)
                            {
                                std::cout<<"More than one removed emitter, removing dynamic!\n";
                                for(int z = 0; z<justToDebugPrint.size(); z++)
                                    std::cout<<justToDebugPrint[z]<<"\n";
                            }

                            for(int l = 0; l<location::locations.size(); l++)
                            {
                                if(location::locations[l]->dynamic == serverData->newDynamics[a])
                                    location::locations[l]->dynamic = 0;
                            }

                            delete serverData->newDynamics[a];
                            serverData->newDynamics.erase(serverData->newDynamics.begin() + a);
                            break;
                        }
                    }
                }

                return;
            }
            case packetType_cameraDetails:
            {
                serverData->adminCam = data->readBit();
                serverData->boundToObject = data->readBit();
                if(serverData->boundToObject)
                {
                    serverData->cameraTargetServerID = data->readUInt(dynamicObjectIDBits);
                    //std::cout<<"Got a camera details packet, bound to object "<<serverData->cameraTargetServerID<<"\n";
                    serverData->cameraLean = data->readBit();
                    serverData->cameraTarget = 0;
                    checkForCameraToBind(serverData);
                }
                else
                {
                    //std::cout<<"Got a camera details packet, not bound to any object!\n";
                    serverData->cameraTarget = 0;
                    float x = data->readFloat();
                    float y = data->readFloat();
                    float z = data->readFloat();
                    serverData->cameraLean = false;
                    serverData->cameraPos = glm::vec3(x,y,z);
                    serverData->freeLook = data->readBit();
                    if(!serverData->freeLook)
                    {
                        x = data->readFloat();
                        y = data->readFloat();
                        z = data->readFloat();
                        serverData->cameraDir = glm::vec3(x,y,z);
                    }
                }

                CEGUI::Window *saveLoadWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("SaveLoad");

                if(data->readBit())
                {
                    saveLoadWindow->getChild("SaveCar")->setDisabled(false);
                    serverData->drivenCarId = data->readUInt(10);
                }
                else
                {
                    saveLoadWindow->getChild("SaveCar")->setDisabled(true);
                    serverData->drivenCarId = -1;
                }
                return;
            }
            case packetType_connectionRepsonse:
            {
                bool nameOkay = data->readBit();
                if(!nameOkay)
                {
                    serverData->kicked = true;
                    serverData->waitingForServerResponse = false;
                    error("Your name was not acceptable or the server failed to respond!");
                    return;
                }
                interpolator::serverTimePoint = data->readUInt(32) - 50;
                interpolator::clientTimePoint = getServerTime();
                std::cout<<"Difference between server and client time: "<<(int)interpolator::serverTimePoint - (int) getServerTime()<<"\n";
                serverData->brickTypesToLoad = data->readUInt(10);
                serverData->dynamicTypesToLoad = data->readUInt(10);
                serverData->itemTypesToLoad = data->readUInt(10);

                CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
                joinServerWindow->getChild("StatusText")->setText("Connected!");

                info("Loading " + std::to_string(serverData->brickTypesToLoad) + " brick types and " + std::to_string(serverData->dynamicTypesToLoad) + " dynamic types from server!");

                finishLoadingTypesCheck(clientEnvironment);

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
                float scaleX = data->readFloat();
                float scaleY = data->readFloat();
                float scaleZ = data->readFloat();

                info("Loading dynamic type: " + filePath);

                newModel *tmp = new newModel(filePath);
                tmp->defaultFrame = standingFrame;
                tmp->serverID = serverID;
                tmp->eyeOffset = glm::vec3(eyeOffsetX,eyeOffsetY,eyeOffsetZ);
                tmp->networkScale = glm::vec3(scaleX,scaleY,scaleZ);

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
                if(serverData->newDynamicTypes.size() == 0)
                {
                    newAnimation grab;
                    //grab.endFrame = 51;
                    //grab.startFrame = 36;
                    grab.startFrame = 57;
                    grab.endFrame = 66;
                    grab.speedDefault = 0.03;
                    grab.name = "grab";
                    tmp->animations.push_back(grab);

                    newAnimation walk;
                    walk.endFrame = 30;
                    walk.startFrame = 0;
                    walk.speedDefault = 0.045;
                    walk.name = "walk";
                    tmp->animations.push_back(walk);
                }

                serverData->newDynamicTypes.push_back(tmp);

                for(unsigned int a = 0; a<serverData->itemTypes.size(); a++)
                {
                    if(serverData->itemTypes[a]->waitingForModel)
                    {
                        if(serverData->itemTypes[a]->waitingModel == serverID)
                        {
                            info("Added model to " + serverData->itemTypes[a]->uiName + " after the fact.");
                            serverData->itemTypes[a]->waitingForModel = false;
                            serverData->itemTypes[a]->type = tmp;

                            for(unsigned int b = 0; b<tmp->animations.size(); b++)
                            {
                                if(tmp->animations[b].serverID == serverData->itemTypes[a]->waitingForSwingAnim)
                                {
                                    serverData->itemTypes[a]->fireAnim = &tmp->animations[b];
                                }
                                if(tmp->animations[b].serverID == serverData->itemTypes[a]->waitingForSwitchAnim)
                                {
                                    serverData->itemTypes[a]->switchAnim = &tmp->animations[b];
                                }
                            }
                        }
                    }
                }

                finishLoadingTypesCheck(clientEnvironment);

                return;
            }
            case packetType_addSpecialBrickType:
            {
                int howMany = data->readUInt(6);
                debug("Loading another: " + std::to_string(howMany) + " brick types");

                for(int a = 0; a<howMany; a++)
                {
                    int typeID = data->readUInt(10);
                    std::string filePath = data->readString();
                    if(filePath.substr(0,8) != "add-ons/" && filePath.substr(0,6) != "cache/")
                        filePath = "assets/brick/types" + filePath;
                    replaceAll(filePath,"\\","/");
                    replaceAll(filePath,"add-ons/","cache/");

                    bool customMesh = data->readBit();

                    //std::cout<<"Adding type: "<<typeID<<" "<<filePath<<"\n";
                    serverData->staticBricks.blocklandTypes->addSpecialBrickType(filePath,clientEnvironment->brickSelector,typeID,customMesh);
                }

                finishLoadingTypesCheck(clientEnvironment);

                return;
            }
        }
    }
}



















