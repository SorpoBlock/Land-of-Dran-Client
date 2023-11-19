#include "newBrickType.h"

namespace syj
{
    int getAngleIDFromRot(glm::quat in)
    {
        glm::quat rotations[4] = {

        glm::quat(0,0,1,0),
        glm::quat(4.7122,0,1,0),
        glm::quat(3.1415,0,1,0),
        glm::quat(1.5708,0,1,0)};

        int idx = 0;
        float dist = glm::length(in - rotations[0]);
        for(int i = 1; i<4; i++)
        {
            float newdist = glm::length(in-rotations[i]);
            if(newdist < dist)
            {
                dist = newdist;
                idx = i;
            }
        }

        return idx;
    }

    std::string getBrickMatName(brickMaterial mat)
    {
        switch(mat)
        {
            case none: return "Normal";
            case undulo: return "Undulo";
            case peral: return "Peral";
            case chrome: return "Chrome";
            case glow: return "Glow";
            case blink: return "Blink";
            case rainbow: return "Rainbow";
            case bob: return "Bouncy";
            case swirl: return "Hologram";
            case slippery: return "Slippery";
            case foil: return "Foil";
        }
    }

    /*
    SPECIAL BRICK STUFF-
    SPECIAL BRICK STARTING
    */

    void newBrickRenderer::addSpecialBrick(specialBrickRenderData *theBrick,btDynamicsWorld *world,int typeIdx,int rotationID,bool doNotCompile)
    {
        theBrick->rotation = rotations[rotationID];
        theBrick->typeID = typeIdx;
        if((theBrick->color.a > 0.01 && theBrick->color.a < 0.99) || blocklandTypes->specialBrickTypes[typeIdx]->hasTransparency)
        {
            theBrick->placedInTransparentBuffer = true;
            transparentSpecialBricks.push_back(theBrick);
            if(typeIdx >= transparentSpecialBrickTypes.size())
            {
                error("Missing (transparent) special brick instance holders in brick renderer!");
                return;
            }
            theBrick->type = transparentSpecialBrickTypes[typeIdx];
        }
        else
        {
            theBrick->placedInTransparentBuffer = false;
            opaqueSpecialBricks.push_back(theBrick);
            if(typeIdx >= specialBrickTypes.size())
            {
                error("Missing (opaque) special brick instance holders in brick renderer!");
                return;
            }
            theBrick->type = specialBrickTypes[typeIdx];
        }
        theBrick->oldType = theBrick->type;

        if(world && theBrick->shouldCollide)
        {
            theBrick->body = addBrickToWorld(theBrick->position,rotationID,theBrick->type->type->width,theBrick->type->type->height,theBrick->type->type->length,theBrick->type->type->shape,world,(brickMaterial)theBrick->material,theBrick->type->type->isModTerrain);
            theBrick->body->setUserPointer(theBrick);
            theBrick->body->setUserIndex(userIndex_staticSpecialBrick);
        }

        if(doNotCompile)
            return;

        theBrick->brickOffsets = theBrick->type->instances.size();
        theBrick->type->instances.push_back(theBrick);
        theBrick->type->recompileInstances();
    }

    //Luckily we don't need to do separate searches for transparent or opaque bricks
    //Since the type it references is already specific to that
    bool newBrickRenderer::removeSpecialBrick(specialBrickRenderData *theBrick,btDynamicsWorld *world,bool noDelete)
    {
        bool found = false;

        if(!theBrick->oldType)
        {
            std::cout<<"No type to remove!\n";
            return false;
        }

        if(theBrick->body && world)
        {
            world->removeRigidBody(theBrick->body);
            delete theBrick->body;
            theBrick->body = 0;
        }

        auto iter = theBrick->oldType->instances.begin();
        while(iter != theBrick->oldType->instances.end())
        {
            if(*iter == theBrick)
            {
                iter = theBrick->oldType->instances.erase(iter);
                found = true;
            }
            else
                ++iter;
        }
        theBrick->oldType->recompileInstances();

        bool deletedOpaque = false;
        for(unsigned int a = 0; a<opaqueSpecialBricks.size(); a++)
        {
            if(opaqueSpecialBricks[a] == theBrick)
            {
                deletedOpaque = true;
                opaqueSpecialBricks.erase(opaqueSpecialBricks.begin() + a);
                break;
            }
        }

        if(!deletedOpaque)
        {
            for(unsigned int a = 0; a<transparentSpecialBricks.size(); a++)
            {
                if(transparentSpecialBricks[a] == theBrick)
                {
                    transparentSpecialBricks.erase(transparentSpecialBricks.begin() + a);
                    break;
                }
            }
        }

        if(!noDelete)
            delete theBrick;

        return found;
    }

    void newBrickRenderer::updateSpecialBrick(specialBrickRenderData *theBrick,btDynamicsWorld *world,int rotationID)
    {
        if(!theBrick->type)
            return;

        if(theBrick->type != theBrick->oldType)
        {
            removeSpecialBrick(theBrick,world,true);
            addSpecialBrick(theBrick,world,theBrick->typeID,rotationID);
        }
        else
        {
            if(!theBrick->shouldCollide)
            {
                if(theBrick->body && world)
                {
                    world->removeRigidBody(theBrick->body);
                    delete theBrick->body;
                    theBrick->body = 0;
                }
            }
            else if(!theBrick->markedForDeath)
            {
                if(world && !theBrick->body)
                {
                    theBrick->body = addBrickToWorld(theBrick->position,rotationID,theBrick->type->type->width,theBrick->type->type->height,theBrick->type->type->length,theBrick->type->type->shape,world,(brickMaterial)theBrick->material,theBrick->type->type->isModTerrain);
                    theBrick->body->setUserPointer(theBrick);
                    theBrick->body->setUserIndex(userIndex_staticSpecialBrick);
                }
            }
            theBrick->type->update(theBrick);
        }
    }

    void newBrickRenderer::recompileOpaqueSpecialBricks()
    {
        for(int a = 0; a<specialBrickTypes.size(); a++)
        {
            specialBrickTypes[a]->instances.clear();
        }

        for(int a = 0; a<opaqueSpecialBricks.size(); a++)
        {
            specialBrickRenderData *tmp = opaqueSpecialBricks[a];
            if(!tmp)
                continue;
            if(!tmp->type)
                continue; //crashes here

            tmp->brickOffsets = tmp->type->instances.size();
            tmp->type->instances.push_back(tmp);
        }

        for(unsigned int a = 0; a<blocklandTypes->specialBrickTypes.size(); a++)
        {
            specialBrickTypes[a]->recompileInstances();
        }
    }

    void newBrickRenderer::recompileTransparentSpecialBricks()
    {
        for(int a = 0; a<transparentSpecialBrickTypes.size(); a++)
        {
            transparentSpecialBrickTypes[a]->instances.clear();
        }

        for(int a = 0; a<transparentSpecialBricks.size(); a++)
        {
            specialBrickRenderData *tmp = transparentSpecialBricks[a];
            if(!tmp)
                continue;
            if(!tmp->type)
                continue;

            tmp->brickOffsets = tmp->type->instances.size();
            tmp->type->instances.push_back(tmp);
        }

        for(unsigned int a = 0; a<blocklandTypes->specialBrickTypes.size(); a++)
        {
            transparentSpecialBrickTypes[a]->recompileInstances();
        }
    }

    void newBrickRenderer::addBlocklandCompatibility(blocklandCompatibility *source)
    {
        blocklandTypes = source;

        int startTime = SDL_GetTicks();

        for(int a = 0; a<source->basicTypes.size(); a++)
        {
            basicBrickType *tmp = &source->basicTypes[a];
            collisionShapes->set(tmp->width,tmp->height,tmp->length,tmp);
        }

        for(int a = 0; a<source->specialBrickTypes.size(); a++)
        {
            specialBrickTypeInstanceHolder *tmp = new specialBrickTypeInstanceHolder(source->specialBrickTypes[a]);
            specialBrickTypes.push_back(tmp);

            tmp = new specialBrickTypeInstanceHolder(source->specialBrickTypes[a]);
            transparentSpecialBrickTypes.push_back(tmp);
        }

        std::cout<<"Associated Blockland compatability in "<<SDL_GetTicks() - startTime<<"ms\n";
    }

    //END SPECIAL BRICK STUFF
    //BOTH SPECIAL AND BASIC BRICK STUFF:

    btRigidBody *addBrickToWorld(glm::vec3 pos,int rotation,int w,int h,int l,btCollisionShape *shape,btDynamicsWorld *world,brickMaterial material,bool isModTer)
    {
        if(!shape)
        {
            error("No shape for brick physics!");
            return 0;
        }
        if(!world)
        {
            error("No physics world!");
            return 0;
        }

        float vx=pos.x,vy=pos.y,vz=pos.z;

        if(rotation == 0 || rotation == 2)
        {
            vx -= w/2.0;
            vy -= h/2.0;
            vz -= l/2.0;
        }
        else
        {
            vz -= w/2.0;
            vy -= h/2.0;
            vx -= l/2.0;
        }
        btRigidBody *body = 0;
        btTransform startTrans = btTransform::getIdentity();

        if(rotation == 0 || rotation == 2)
            startTrans.setOrigin(btVector3(vx+(w/2.0),vy+(h/2.0),vz+(l/2.0)));
        else
            startTrans.setOrigin(btVector3(vx+(l/2.0),vy+(h/2.0),vz+(w/2.0)));

        if(isModTer)
        {
            if(rotation == 0)
                startTrans.setRotation(btQuaternion(0,0,0));
            if(rotation == 1)
                startTrans.setRotation(btQuaternion(1.5708,0,0));
            if(rotation == 2)
                startTrans.setRotation(btQuaternion(3.1415,0,0));
            if(rotation == 3)
                startTrans.setRotation(btQuaternion(4.7122,0,0));
        }

        if(!isModTer && (rotation == 1 || rotation == 3))
            startTrans.setRotation(btQuaternion(1.57,0,0));

        btMotionState* ms = new btDefaultMotionState(startTrans);
        btRigidBody::btRigidBodyConstructionInfo info(0.0,ms,shape,btVector3(0,0,0));
        body = new btRigidBody(info);
        body->setCollisionFlags( btCollisionObject::CF_STATIC_OBJECT);
        delete ms;

        int matAfterShapeFx = material;
        if(matAfterShapeFx >= 2000)
            matAfterShapeFx -= 2000;
        if(matAfterShapeFx >= 1000)
            matAfterShapeFx -= 1000;

        body->setFriction(matAfterShapeFx == slippery ? 0.3 : 1.0);
        body->setRestitution((material >= 2000) ? 2.0 : 0.0);
        body->forceActivationState(ISLAND_SLEEPING);
        //body->setActivationState(ISLAND_SLEEPING);
        world->addRigidBody(body);
        //world->addCollisionObject(body
        return body;
    }

    void newBrickRenderer::renderTransparent(uniformsHolder *unis,bool skipMats,float deltaT)
    {
        glEnable(GL_BLEND);

        for(int a = 0; a<4; a++)
        {
            if(transparentPerTextureInstances[a] < 1)
                continue;

            if(!skipMats)
                theTextures[a]->use(unis);
            glBindVertexArray(transparentPerTextureVAO[a]);
            //glDrawArraysInstanced(GL_TRIANGLES,0,6,transparentPerTextureInstances[a]);
            glDrawArraysInstanced(GL_TRIANGLE_FAN,0,4,transparentPerTextureInstances[a]);
        }

        glUniform1i(unis->target->getUniformLocation("isPrint"),true);

        for(int a = 4; a<transparentPerTextureVAO.size(); a++)
        {
            if(transparentPerTextureInstances[a] < 1)
                continue;

            if(!skipMats)
                theTextures[a]->use(unis);

            glBindVertexArray(transparentPerTextureVAO[a]);
            //glDrawArraysInstanced(GL_TRIANGLES, 0,6,transparentPerTextureInstances[a]);
            glDrawArraysInstanced(GL_TRIANGLE_FAN,0,4,transparentPerTextureInstances[a]);
        }

        glUniform1i(unis->target->getUniformLocation("isPrint"),false);

        glUniform1i(unis->target->getUniformLocation("specialBricks"),true);

        if(!skipMats)
        {
            theTextures[BRICKTEX_STUDS]->useManuelOffset(doNotUseBrickTexture,topNormal,topMohr);
            theTextures[BRICKTEX_SIDES]->useManuelOffset(doNotUseBrickTexture,sideNormal,sideMohr);
            theTextures[BRICKTEX_BOTTOM]->useManuelOffset(doNotUseBrickTexture,bottomNormal,bottomMohr);
            theTextures[BRICKTEX_RAMP]->useManuelOffset(doNotUseBrickTexture,rampNormal,rampMohr);
        }


        //std::cout<<"BL Types: "<<blocklandTypes->specialBrickTypes.size()<<"\n";
        //std::cout<<"Our types: "<<specialBrickTypes.size()<<"\n";

        for(int a = 0; a<blocklandTypes->specialBrickTypes.size(); a++)
        {
            if(transparentSpecialBrickTypes[a]->numCompiledInstances == 0)
                continue;

            //std::cout<<"Rendering: "<<specialBrickTypes[a]->vao<<" verts: "<<blocklandTypes->specialBrickTypes[a]->vertexCount<<" instances: "<<specialBrickTypes[a]->numCompiledInstances<<"\n";

            glBindVertexArray(transparentSpecialBrickTypes[a]->vao);
            glDrawArraysInstanced(GL_TRIANGLES,0,blocklandTypes->specialBrickTypes[a]->vertexCount,transparentSpecialBrickTypes[a]->numCompiledInstances);
        }

        glUniform1i(unis->target->getUniformLocation("specialBricks"),false);

        glDisable(GL_BLEND);
    }

    void newBrickRenderer::renderEverything(uniformsHolder *unis,bool skipMats,material *specialPrintBrickMaterial,float deltaT)
    {
        glEnable(GL_CULL_FACE);

        glUniform1f(unis->target->getUniformLocation("deltaT"),deltaT);

        for(int a = 0; a<4; a++)
        {
            if(perTextureInstances[a] < 1)
                continue;

            if(!skipMats)
                theTextures[a]->use(unis);

            glBindVertexArray(perTextureVAO[a]);
            //glDrawArraysInstanced(GL_TRIANGLES, 0,6,perTextureInstances[a]);
            glDrawArraysInstanced(GL_TRIANGLE_FAN,0,4,perTextureInstances[a]);
        }

        glUniform1i(unis->target->getUniformLocation("isPrint"),true);

        for(int a = 4; a<perTextureVAO.size(); a++)
        {
            if(perTextureInstances[a] < 1)
                continue;

            if(!skipMats)
                theTextures[a]->use(unis);

            glBindVertexArray(perTextureVAO[a]);
            glDrawArraysInstanced(GL_TRIANGLE_FAN,0,4,perTextureInstances[a]);
        }

        glUniform1i(unis->target->getUniformLocation("isPrint"),false);

        glUniform1i(unis->target->getUniformLocation("specialBricks"),true);

        if(!skipMats)
        {
            theTextures[BRICKTEX_STUDS]->useManuelOffset(doNotUseBrickTexture,topNormal,topMohr);
            theTextures[BRICKTEX_SIDES]->useManuelOffset(doNotUseBrickTexture,sideNormal,sideMohr);
            theTextures[BRICKTEX_BOTTOM]->useManuelOffset(doNotUseBrickTexture,bottomNormal,bottomMohr);
            theTextures[BRICKTEX_RAMP]->useManuelOffset(doNotUseBrickTexture,rampNormal,rampMohr);
        }

        //std::cout<<"BL Types: "<<blocklandTypes->specialBrickTypes.size()<<"\n";
        //std::cout<<"Our types: "<<specialBrickTypes.size()<<"\n";

        if(specialPrintBrickMaterial)
            specialPrintBrickMaterial->useManuelOffset(doNotUseBrickTexture,printTextureSpecialBrickNormal,printTexture);

        for(int a = 0; a<blocklandTypes->specialBrickTypes.size(); a++)
        {
            if(specialBrickTypes[a]->numCompiledInstances == 0)
                continue;

            if(specialPrintBrickMaterial)
                glUniform1i(unis->target->getUniformLocation("isPrint"),specialBrickTypes[a]->type->hasPrint);

            glBindVertexArray(specialBrickTypes[a]->vao);
            glDrawArraysInstanced(GL_TRIANGLES,0,blocklandTypes->specialBrickTypes[a]->vertexCount,specialBrickTypes[a]->numCompiledInstances);
        }

        if(specialPrintBrickMaterial)
            glUniform1i(unis->target->getUniformLocation("isPrint"),0);

        glUniform1i(unis->target->getUniformLocation("specialBricks"),false);

        renderTransparent(unis,skipMats,deltaT);

        glBindVertexArray(0);
    }

    //END BOTH SPECIAL AND BASIC PART
    //BEGIN BASIC PART


    //Changed alpha wasn't actually implemented
    void newBrickRenderer::updateBasicBrick(basicBrickRenderData *theBrick,btDynamicsWorld *world,bool changedAlpha)
    {
        if(!theBrick)
        {
            std::cout<<"No brick in updateBasicBrick!\n";
            return;
        }
        if(theBrick->bufferOffset == -1)
        {
            std::cout<<"No buffer offset!\n";
            return;
        }
        glm::vec4 pos = glm::vec4(theBrick->position.x,theBrick->position.y,theBrick->position.z,theBrick->material);

        if(theBrick->shouldCollide && !theBrick->markedForDeath)
        {
            if(!theBrick->body && world)
            {
                btCollisionShape *shape = 0;

                if(!shape)
                {
                    basicBrickType *tmp = collisionShapes->at((int)theBrick->dimensions.x,(int)theBrick->dimensions.y,(int)theBrick->dimensions.z);
                    if(tmp)
                    shape = tmp->shape;
                }

                if(!shape)
                {
                    basicBrickType *shapeHolder = new basicBrickType;
                    shapeHolder->init(theBrick->dimensions.x,theBrick->dimensions.y,theBrick->dimensions.z);
                    collisionShapes->set(shapeHolder->width,shapeHolder->height,shapeHolder->length,shapeHolder);
                    shape = shapeHolder->shape;
                }

                if(shape)
                {
                    theBrick->body = addBrickToWorld(theBrick->position,getAngleIDFromRot(theBrick->rotation),theBrick->dimensions.x,theBrick->dimensions.y,theBrick->dimensions.z,shape,world,(brickMaterial)theBrick->material,false);
                    theBrick->body->setUserPointer(theBrick);
                    theBrick->body->setUserIndex(userIndex_staticNormalBrick);
                }
            }
        }
        else
        {
            if(theBrick->body && world)
            {
                world->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }
        }

        if(theBrick->placedInTransparentBuffer)//theBrick->color.a < 1)
        {
            if(theBrick->bufferOffset >= transparentBasicAlloc[0])
            {
                std::cout<<"Brick beyond transparent buffer!\n";
                return;
            }

            glm::uvec4 dimensions = theBrick->dimensions;
            if(theBrick->hasPrint)
                dimensions.w = theBrick->printMask;

            glBindVertexArray(transparentPerTextureVAO[BRICKTEX_STUDS]);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&dimensions);


            if(theBrick->hasPrint)
            {
                dimensions.w = dimensions.w ^ 0b111111;

                if(theBrick->oldPrintID != theBrick->printID)
                {
                    if(theBrick->printBufferOffset != -1)
                    {
                        if(theBrick->oldPrintID != -1)
                        {
                            glm::vec4 noColor = glm::vec4(0,0,0,0);
                            glm::uvec4 noDims = dimensions;
                            noDims.w = 0b11111111;

                            glBindVertexArray(transparentPerTextureVAO[theBrick->oldPrintID+4]);

                            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

                            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

                            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&noColor);

                            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&noDims);

                            glBindVertexArray(0);
                        }
                    }

                    theBrick->oldPrintID = theBrick->printID;

                    if(perTextureInstances[theBrick->printID+4]/6 < opaqueBasicAlloc[theBrick->printID+4])
                    {
                        theBrick->printBufferOffset = perTextureInstances[theBrick->printID+4]/6;
                        perTextureInstances[theBrick->printID+4]+=6;

                        glBindVertexArray(transparentPerTextureVAO[theBrick->printID+4]);

                        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

                        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

                        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);

                        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&dimensions);

                        glBindVertexArray(0);
                    }
                    else
                        recompileOpaqueBasicBricks();
                }
                else
                {
                    glBindVertexArray(transparentPerTextureVAO[theBrick->printID+4]);

                    glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

                    glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

                    glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);

                    glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&dimensions);

                    glBindVertexArray(0);
                }
            }
        }
        else
        {
            if(theBrick->bufferOffset >= opaqueBasicAlloc[0])
            {
                std::cout<<"Brick beyond opaque buffer!\n";
                std::cout<<theBrick->bufferOffset<<" "<<opaqueBasicBricks.size()<<"\n";
                return;
            }

            glm::uvec4 dimensions = theBrick->dimensions;
            if(theBrick->hasPrint)
                dimensions.w = theBrick->printMask;

            glBindVertexArray(perTextureVAO[BRICKTEX_STUDS]);

            glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

            glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

            glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);

            glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[BRICKTEX_STUDS]);
            glBufferSubData(GL_ARRAY_BUFFER,theBrick->bufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&theBrick->dimensions);

            glBindVertexArray(0);

            if(theBrick->hasPrint)
            {
                dimensions.w = dimensions.w ^ 0b111111;

                if(theBrick->oldPrintID != theBrick->printID)
                {
                    if(theBrick->printBufferOffset != -1)
                    {
                        if(theBrick->oldPrintID != -1)
                        {
                            glm::vec4 noColor = glm::vec4(0,0,0,0);
                            glm::uvec4 noDims = dimensions;
                            noDims.w = 0b11111111;

                            glBindVertexArray(perTextureVAO[theBrick->oldPrintID+4]);

                            glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

                            glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

                            glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&noColor);

                            glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[theBrick->oldPrintID+4]);
                            glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&noDims);

                            glBindVertexArray(0);
                        }
                    }

                    theBrick->oldPrintID = theBrick->printID;

                    if(perTextureInstances[theBrick->printID+4]/6 < opaqueBasicAlloc[theBrick->printID+4])
                    {

                        theBrick->printBufferOffset = perTextureInstances[theBrick->printID+4]/6;
                        perTextureInstances[theBrick->printID+4]+=6;

                        glBindVertexArray(perTextureVAO[theBrick->printID+4]);

                        glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

                        glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

                        glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);

                        glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[theBrick->printID+4]);
                        glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&dimensions);

                        glBindVertexArray(0);
                    }
                    else
                        recompileOpaqueBasicBricks();
                }
                else
                {
                    glBindVertexArray(perTextureVAO[theBrick->printID+4]);

                    glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&pos);

                    glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);

                    glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);

                    glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[theBrick->printID+4]);
                    glBufferSubData(GL_ARRAY_BUFFER,theBrick->printBufferOffset * sizeof(glm::uvec4),sizeof(glm::uvec4),&dimensions);

                    glBindVertexArray(0);
                }
            }
        }
    }

    void newBrickRenderer::addBasicBrick(basicBrickRenderData *theBrick,int rotationID,btCollisionShape *shape,btDynamicsWorld *world,bool doNotCompile)
    {
        theBrick->oldPrintID = theBrick->printID;

        if(world && theBrick->shouldCollide)
        {
            if(!shape)
            {
                basicBrickType *tmp = collisionShapes->at((int)theBrick->dimensions.x,(int)theBrick->dimensions.y,(int)theBrick->dimensions.z);
                if(tmp)
                    shape = tmp->shape;
            }

            if(!shape)
            {
                basicBrickType *shapeHolder = new basicBrickType;
                shapeHolder->init(theBrick->dimensions.x,theBrick->dimensions.y,theBrick->dimensions.z);
                collisionShapes->set(shapeHolder->width,shapeHolder->height,shapeHolder->length,shapeHolder);
                shape = shapeHolder->shape;
            }

            if(shape)
            {
                theBrick->body = addBrickToWorld(theBrick->position,rotationID,theBrick->dimensions.x,theBrick->dimensions.y,theBrick->dimensions.z,shape,world,(brickMaterial)theBrick->material,false);
                theBrick->body->setUserPointer(theBrick);
                theBrick->body->setUserIndex(userIndex_staticNormalBrick);
            }
        }

        if(theBrick->color.a < 1)
        {
            theBrick->placedInTransparentBuffer = true;
            transparentBasicBricks.push_back(theBrick);
            if(doNotCompile)
                return;

            //No reason why top should have different amount of bricks than sides or bottom
            if(transparentPerTextureInstances[BRICKTEX_STUDS] < transparentBasicAlloc[0])
            {
                theBrick->bufferOffset = currentTransparentBasicBufferOffset;
                currentTransparentBasicBufferOffset++;
                //theBrick->bufferOffset = transparentBasicBricks.size()-1;
                transparentPerTextureInstances[BRICKTEX_STUDS]++;
                //Uh, except for this one lol:
                transparentPerTextureInstances[BRICKTEX_SIDES]+=4;
                transparentPerTextureInstances[BRICKTEX_BOTTOM]++;
                if(!theBrick->hasPrint)
                    updateBasicBrick(theBrick,world);
            }
            else
                recompileTransparentBasicBricks();

            if(theBrick->hasPrint)
            {
                if(transparentPerTextureInstances[theBrick->printID+4]/6 < transparentBasicAlloc[theBrick->printID+4])
                {
                    theBrick->printBufferOffset = transparentPerTextureInstances[theBrick->printID+4];
                    transparentPerTextureInstances[theBrick->printID+4]+=6;
                    updateBasicBrick(theBrick,world);
                }
                else
                    recompileTransparentBasicBricks();
            }
        }
        else
        {
            theBrick->placedInTransparentBuffer = false;
            opaqueBasicBricks.push_back(theBrick);
            if(doNotCompile)
                return;

            if(perTextureInstances[BRICKTEX_STUDS] < opaqueBasicAlloc[0])
            {
                theBrick->bufferOffset = currentOpaqueBasicBufferOffset;
                currentOpaqueBasicBufferOffset++;
                //theBrick->bufferOffset = opaqueBasicBricks.size()-1;
                perTextureInstances[BRICKTEX_STUDS]++;
                perTextureInstances[BRICKTEX_SIDES]+=4;
                perTextureInstances[BRICKTEX_BOTTOM]++;
                if(!theBrick->hasPrint)
                    updateBasicBrick(theBrick,world);
            }
            else
                recompileOpaqueBasicBricks();

            if(theBrick->hasPrint)
            {
                if(perTextureInstances[theBrick->printID+4]/6 < opaqueBasicAlloc[theBrick->printID+4])
                {
                    theBrick->printBufferOffset = perTextureInstances[theBrick->printID+4];
                    perTextureInstances[theBrick->printID+4]+=6;
                    updateBasicBrick(theBrick,world);
                }
                else
                    recompileOpaqueBasicBricks();
            }
        }
    }

    void newBrickRenderer::removeBasicBrick(basicBrickRenderData *theBrick,btDynamicsWorld *world)
    {
        //TODO: Update for prints
        if(theBrick->body && world)
        {
            world->removeRigidBody(theBrick->body);
            delete theBrick->body;
            theBrick->body = 0;
        }

        theBrick->position = glm::vec3(0);
        theBrick->dimensions = glm::uvec4(0);
        updateBasicBrick(theBrick,world);

        /*if(theBrick->color.a == 1)
            opaqueBasicBricks[theBrick->bufferOffset] = 0;
        else
            transparentBasicBricks[theBrick->bufferOffset] = 0;*/

        bool deletedOpaque = false;
        for(unsigned int a = 0; a<opaqueBasicBricks.size(); a++)
        {
            if(opaqueBasicBricks[a] == theBrick)
            {
                opaqueBasicBricks.erase(opaqueBasicBricks.begin() + a);
                deletedOpaque = true;
                break;
            }
        }

        if(!deletedOpaque)
        {
            for(unsigned int a = 0; a<transparentBasicBricks.size(); a++)
            {
                if(transparentBasicBricks[a] == theBrick)
                {
                    transparentBasicBricks.erase(transparentBasicBricks.begin() + a);
                    break;
                }
            }
        }

        delete theBrick;
    }

    //Called once on start-up, creates 6 verts that make two triangles / one square we repeat
    //for each face of each brick
    void newBrickRenderer::allocateVertBuffer()
    {
        collisionShapes = new Octree<basicBrickType*>(256);

        //Left in memory of a really weird bug
        //glGenVertexArrays(1,&vertBuffer);

        glGenBuffers(1,&vertBuffer);

        std::vector<glm::vec3> verts;
        float low = -0.5;
        float high = 0.5;

        //Right bottom triangle
        /*verts.push_back(glm::vec3(low,low,0));
        verts.push_back(glm::vec3(high,low,0));
        verts.push_back(glm::vec3(high,high,0));

        //Left top triangle
        verts.push_back(glm::vec3(high,high,0));
        verts.push_back(glm::vec3(low,high,0));
        verts.push_back(glm::vec3(low,low,0));*/

        verts.push_back(glm::vec3(low,high,0));
        verts.push_back(glm::vec3(low,low,0));
        verts.push_back(glm::vec3(high,low,0));
        verts.push_back(glm::vec3(high,high,0));

        glBindBuffer(GL_ARRAY_BUFFER,vertBuffer);
        glBufferData(GL_ARRAY_BUFFER,verts.size() * sizeof(glm::vec3),&verts[0],GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,0);
    }

    //Since swapping textures is more expensive than glDrawArraysInstanced we group faces by texture
    //to minimize swapping. This is called once per texture we will use for our bricks
    //at the start it should be called for the top, bottom, side, and ramp textures at least, plus any prints later on
    int newBrickRenderer::allocatePerTexture(material* mat,bool sideOrBottomTexture,bool sideTexture,bool printTexture)
    {
        if(sideTexture && !sideOrBottomTexture)
            error("Side texture but not sideOrBottomTexture???");

        //Allocate buffers and object:
        GLuint vao;
        glGenVertexArrays(1,&vao);

        //Don't need 0-3 for side or bottom
        GLuint buffers[5];
        glGenBuffers(5,buffers);

        //What it's all about:
        theTextures.push_back(mat);

        opaqueBasicAlloc.push_back(0);

        //Store them so we can iterate through all faces by texture later:
        perTextureVAO.push_back(vao);
        perTexturePositionMatBuffer.push_back(  buffers[positionMatBuffer]);
        perTextureRotationBuffer.push_back(  buffers[rotationBuffer]);
        perTexturePaintColorBuffer.push_back(buffers[paintColorBuffer]);
        perTextureDimensionBuffer.push_back( buffers[dimensionBuffer]);
        perTextureDirectionBuffer.push_back( buffers[directionBuffer]);

        //How many faces with this texture will we need to render:
        perTextureInstances.push_back(0);

        //Start actually putting stuff in the VAO
        glBindVertexArray(vao);

        //The basic square we'll repeat over and over again
        glBindBuffer(GL_ARRAY_BUFFER,vertBuffer);
        glEnableVertexAttribArray(BRICKLAYOUT_VERTS);
        glVertexAttribDivisor(BRICKLAYOUT_VERTS,0);
        glVertexAttribPointer(BRICKLAYOUT_VERTS,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        //The position and material of each brick
        //We can optimize a bit by grouping the 4 sides of each brick together
        //Until we re-implement interior face culling, that is...
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers[positionMatBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_POSITIONMAT);
        glVertexAttribPointer(BRICKLAYOUT_POSITIONMAT,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_POSITIONMAT,printTexture ? 6 : (sideTexture ? 4 : 1));

        //The rotation of each brick, same thing with the sides
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers[rotationBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_ROTATION);
        glVertexAttribPointer(BRICKLAYOUT_ROTATION,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_ROTATION,printTexture ? 6 : (sideTexture ? 4 : 1));

        //The paint color of each brick, same thing with the sides
        //There is also per vertex color for special bricks, and print texture color
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers[paintColorBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_PAINTCOLOR);
        glVertexAttribPointer(BRICKLAYOUT_PAINTCOLOR,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_PAINTCOLOR,printTexture ? 6 : (sideTexture ? 4 : 1));

        //Brick dimensions in studs/plates, same thing with the sides
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers[dimensionBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_DIMENSION);
        glVertexAttribIPointer(BRICKLAYOUT_DIMENSION,4,GL_UNSIGNED_INT,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_DIMENSION,printTexture ? 6 : (sideTexture ? 4 : 1));

        //Which face of the brick is this, only really relevant for the sides
        //Note this one is exempt from the instancing seen above, it is always per face
        glBindBuffer(GL_ARRAY_BUFFER,buffers[directionBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_DIRECTION);
        glVertexAttribIPointer(BRICKLAYOUT_DIRECTION,1,GL_UNSIGNED_BYTE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_DIRECTION,1);

        glBindVertexArray(0);

        //Do it all again for transparent bricks:

        //Allocate buffers and object:
        GLuint vao2;
        glGenVertexArrays(1,&vao2);

        //Don't need 0-3 for side or bottom
        GLuint buffers2[5];
        glGenBuffers(5,buffers2);

        transparentBasicAlloc.push_back(0);

        //Store them so we can iterate through all faces by texture later:
        transparentPerTextureVAO.push_back(vao2);
        transparentPerTexturePositionMatBuffer.push_back(  buffers2[positionMatBuffer]);
        transparentPerTextureRotationBuffer.push_back(  buffers2[rotationBuffer]);
        transparentPerTexturePaintColorBuffer.push_back(buffers2[paintColorBuffer]);
        transparentPerTextureDimensionBuffer.push_back( buffers2[dimensionBuffer]);
        transparentPerTextureDirectionBuffer.push_back( buffers2[directionBuffer]);

        //How many faces with this texture will we need to render:
        transparentPerTextureInstances.push_back(0);

        //Start actually putting stuff in the VAO
        glBindVertexArray(vao2);

        //The basic square we'll repeat over and over again
        glBindBuffer(GL_ARRAY_BUFFER,vertBuffer);
        glEnableVertexAttribArray(BRICKLAYOUT_VERTS);
        glVertexAttribDivisor(BRICKLAYOUT_VERTS,0);
        glVertexAttribPointer(BRICKLAYOUT_VERTS,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        //The position of each brick
        //We can optimize a bit by grouping the 4 sides of each brick together
        //Until we re-implement interior face culling, that is...
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers2[positionMatBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_POSITIONMAT);
        glVertexAttribPointer(BRICKLAYOUT_POSITIONMAT,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_POSITIONMAT,printTexture ? 6 : (sideTexture ? 4 : 1));

        //The rotation of each brick, same thing with the sides
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers2[rotationBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_ROTATION);
        glVertexAttribPointer(BRICKLAYOUT_ROTATION,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_ROTATION,printTexture ? 6 : (sideTexture ? 4 : 1));

        //The paint color of each brick, same thing with the sides
        //There is also per vertex color for special bricks, and print texture color
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers2[paintColorBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_PAINTCOLOR);
        glVertexAttribPointer(BRICKLAYOUT_PAINTCOLOR,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_PAINTCOLOR,printTexture ? 6 : (sideTexture ? 4 : 1));

        //Brick dimensions in studs/plates, same thing with the sides
        if(sideOrBottomTexture)
            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[BRICKTEX_STUDS]);
        else
            glBindBuffer(GL_ARRAY_BUFFER,buffers2[dimensionBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_DIMENSION);
        glVertexAttribIPointer(BRICKLAYOUT_DIMENSION,4,GL_UNSIGNED_INT,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_DIMENSION,printTexture ? 6 : (sideTexture ? 4 : 1));

        //Which face of the brick is this, only really relevant for the sides
        //Note this one is exempt from the instancing seen above, it is always per face
        glBindBuffer(GL_ARRAY_BUFFER,buffers2[directionBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_DIRECTION);
        glVertexAttribIPointer(BRICKLAYOUT_DIRECTION,1,GL_UNSIGNED_BYTE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_DIRECTION,1);

        glBindVertexArray(0);

        return perTextureVAO.size() - 1;
    }

    void newBrickRenderer::recompileOpaquePrints(int extraAllocPrint)
    {
        for(int tex = 4; tex<theTextures.size(); tex++)
        {
            std::vector<glm::vec4> positionsAndMats;
            std::vector<glm::quat> rotations;
            std::vector<glm::vec4> paintColors;
            std::vector<glm::uvec4> dimensions;
            std::vector<unsigned char> directions;

            for(unsigned int i = 0; i<opaqueBasicBricks.size(); i++)
            {
                basicBrickRenderData *brick = opaqueBasicBricks[i];

                if(!brick->hasPrint)
                    continue;

                if(brick->printID+4 != tex)
                    continue;

                glm::uvec4 dimension = brick->dimensions;
                dimension.w = brick->printMask ^ 0b111111;

                //This is set so we can easily update just this bricks attributes later
                //The offset doesn't tell you if it's transparent or opaque
                //It is the same for all sides though
                brick->printBufferOffset = positionsAndMats.size();

                positionsAndMats.push_back(glm::vec4(brick->position.x,brick->position.y,brick->position.z,brick->material));
                rotations.push_back(brick->rotation);
                paintColors.push_back(brick->color);
                dimensions.push_back(dimension);

                for(int i = 0; i<6; i++)
                    directions.push_back((faceDirection)i);
            }

            for(unsigned int i = 0; i<extraAllocPrint; i++)
            {
                positionsAndMats.push_back(glm::vec4(0));
                rotations.push_back(glm::quat(0,0,0,0));
                paintColors.push_back(glm::vec4(0));
                dimensions.push_back(glm::uvec4(0));
                for(int i = 0; i<6; i++)
                    directions.push_back((faceDirection)i);
            }

            opaqueBasicAlloc[tex] = positionsAndMats.size();
            perTextureInstances[tex] = directions.size() - (extraAllocPrint*6);

            glBindVertexArray(perTextureVAO[tex]);

            glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,positionsAndMats.size() * sizeof(glm::vec4),&positionsAndMats[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,rotations.size() * sizeof(glm::quat),&rotations[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,paintColors.size() * sizeof(glm::vec4),&paintColors[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,dimensions.size() * sizeof(glm::uvec4),&dimensions[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,perTextureDirectionBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);
        }

        glBindVertexArray(0);
    }

    void newBrickRenderer::recompileTransparentPrints(int extraAllocPrint)
    {
        for(int tex = 4; tex<theTextures.size(); tex++)
        {
            std::vector<glm::vec4> positionsAndMats;
            std::vector<glm::quat> rotations;
            std::vector<glm::vec4> paintColors;
            std::vector<glm::uvec4> dimensions;
            std::vector<unsigned char> directions;

            for(unsigned int i = 0; i<transparentBasicBricks.size(); i++)
            {
                basicBrickRenderData *brick = transparentBasicBricks[i];

                if(!brick->hasPrint)
                    continue;

                if(brick->printID+4 != tex)
                    continue;

                glm::uvec4 dimension = brick->dimensions;
                dimension.w = ~brick->printMask;

                //This is set so we can easily update just this bricks attributes later
                //The offset doesn't tell you if it's transparent or opaque
                //It is the same for all sides though
                brick->printBufferOffset = positionsAndMats.size();

                positionsAndMats.push_back(glm::vec4(brick->position.x,brick->position.y,brick->position.z,brick->material));
                rotations.push_back(brick->rotation);
                paintColors.push_back(brick->color);
                dimensions.push_back(dimension);

                for(int i = 0; i<6; i++)
                    directions.push_back((faceDirection)i);
            }

            for(unsigned int i = 0; i<extraAllocPrint; i++)
            {
                positionsAndMats.push_back(glm::vec4(0));
                rotations.push_back(glm::quat(0,0,0,0));
                paintColors.push_back(glm::vec4(0));
                dimensions.push_back(glm::uvec4(0));
                for(int i = 0; i<6; i++)
                    directions.push_back((faceDirection)i);
            }

            transparentBasicAlloc[tex] = positionsAndMats.size();
            transparentPerTextureInstances[tex] = directions.size() - (extraAllocPrint*6);

            glBindVertexArray(transparentPerTextureVAO[tex]);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,positionsAndMats.size() * sizeof(glm::vec4),&positionsAndMats[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,rotations.size() * sizeof(glm::quat),&rotations[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,paintColors.size() * sizeof(glm::vec4),&paintColors[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,dimensions.size() * sizeof(glm::uvec4),&dimensions[0],GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDirectionBuffer[tex]);
            glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);
        }

        glBindVertexArray(0);
    }

    void newBrickRenderer::recompileOpaqueBasicBricks(int extraAlloc,int extraAllocPrint)
    {
        //We're going to start with just the basic bricks
        //Anything that's not a basic rectangular prism with only a top, bottom, and sides
        //Print bricks or any brick with a custom model or per-vertex coloring is "special"

        //These are the empty spaces for bricks removed from the remove command, lets get rid of them for good:
        auto iter = opaqueBasicBricks.begin();
        while(iter != opaqueBasicBricks.end())
        {
            if(*iter == 0)
                iter = opaqueBasicBricks.erase(iter);
            else
                ++iter;
        }

        std::vector<glm::vec4> positionsAndMats;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec4> paintColors;
        std::vector<glm::uvec4> dimensions;
        std::vector<unsigned char> directions;

        //Tops of bricks:

        for(unsigned int i = 0; i<opaqueBasicBricks.size(); i++)
        {
            basicBrickRenderData *brick = opaqueBasicBricks[i];
            if(brick->hasPrint)
                brick->dimensions.w = brick->printMask;

            //This is set so we can easily update just this bricks attributes later
            //The offset doesn't tell you if it's transparent or opaque
            //It is the same for all sides though
            brick->bufferOffset = i;

            positionsAndMats.push_back(glm::vec4(brick->position.x,brick->position.y,brick->position.z,brick->material));
            rotations.push_back(brick->rotation);
            paintColors.push_back(brick->color);
            dimensions.push_back(brick->dimensions);
            directions.push_back(FACE_UP);
        }

        currentOpaqueBasicBufferOffset = opaqueBasicBricks.size();

        for(unsigned int i = 0; i<extraAlloc; i++)
        {
            positionsAndMats.push_back(glm::vec4(0));
            rotations.push_back(glm::quat(0,0,0,0));
            paintColors.push_back(glm::vec4(0));
            dimensions.push_back(glm::uvec4(0));
            directions.push_back(FACE_UP);
        }

        //Only need to do this once for all sides because they all have the same amount of bricks
        //The total amount of bricks we have VRAM space allocated for:
        opaqueBasicAlloc[0] = positionsAndMats.size();

        glBindVertexArray(perTextureVAO[BRICKTEX_STUDS]);

        glBindBuffer(GL_ARRAY_BUFFER,perTexturePositionMatBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,positionsAndMats.size() * sizeof(glm::vec4),&positionsAndMats[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,perTextureRotationBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,rotations.size() * sizeof(glm::quat),&rotations[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,perTexturePaintColorBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,paintColors.size() * sizeof(glm::vec4),&paintColors[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,perTextureDimensionBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,dimensions.size() * sizeof(glm::uvec4),&dimensions[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,perTextureDirectionBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        //We use directions.size() because it will be correct even for the sides of bricks that use extra instancing
        perTextureInstances[BRICKTEX_STUDS] = directions.size()-extraAlloc;

        //Sides of bricks:

        directions.clear();

        for(unsigned int i = 0; i<opaqueBasicBricks.size(); i++)
        {
            //Man, if I ever change the order of my side's enum this will really break lol
            for(unsigned int side = FACE_NORTH; side <= FACE_WEST; side++)
                directions.push_back((faceDirection)side);
        }

        for(unsigned int i = 0; i<extraAlloc; i++)
        {
            for(unsigned int side = FACE_NORTH; side <= FACE_WEST; side++)
                directions.push_back((faceDirection)side);
        }

        glBindVertexArray(perTextureVAO[BRICKTEX_SIDES]);

        glBindBuffer(GL_ARRAY_BUFFER,perTextureDirectionBuffer[BRICKTEX_SIDES]);
        glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        perTextureInstances[BRICKTEX_SIDES] = directions.size()-(extraAlloc*4);

        //Bottoms of bricks:

        directions.clear();

        for(unsigned int i = 0; i<opaqueBasicBricks.size(); i++)
        {
            directions.push_back(FACE_DOWN);
        }

        for(unsigned int i = 0; i<extraAlloc; i++)
        {
            directions.push_back(FACE_DOWN);
        }

        glBindVertexArray(perTextureVAO[BRICKTEX_BOTTOM]);

        glBindBuffer(GL_ARRAY_BUFFER,perTextureDirectionBuffer[BRICKTEX_BOTTOM]);
        glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        perTextureInstances[BRICKTEX_BOTTOM] = directions.size()-extraAlloc;

        recompileOpaquePrints(extraAllocPrint);
    }

    void newBrickRenderer::recompileTransparentBasicBricks(int extraAlloc,int extraAllocPrints)
    {
        //These are the empty spaces for bricks removed from the remove command, lets get rid of them for good:
        auto iter = transparentBasicBricks.begin();
        while(iter != transparentBasicBricks.end())
        {
            if(*iter == 0)
                iter = transparentBasicBricks.erase(iter);
            else
                ++iter;
        }

        std::vector<glm::vec4> positionsAndMats;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec4> paintColors;
        std::vector<glm::uvec4> dimensions;
        std::vector<unsigned char> directions;

        //Tops of bricks:

        for(unsigned int i = 0; i<transparentBasicBricks.size(); i++)
        {
            basicBrickRenderData *brick = transparentBasicBricks[i];

            //This is set so we can easily update just this bricks attributes later
            //The offset doesn't tell you if it's transparent or opaque
            //It is the same for all sides though
            brick->bufferOffset = i;

            positionsAndMats.push_back(glm::vec4(brick->position.x,brick->position.y,brick->position.z,brick->material));
            rotations.push_back(brick->rotation);
            paintColors.push_back(brick->color);
            dimensions.push_back(brick->dimensions);
            directions.push_back(FACE_UP);
        }

        currentTransparentBasicBufferOffset = transparentBasicBricks.size();

        for(unsigned int i = 0; i<extraAlloc; i++)
        {
            positionsAndMats.push_back(glm::vec4(0));
            rotations.push_back(glm::quat(0,0,0,0));
            paintColors.push_back(glm::vec4(0));
            dimensions.push_back(glm::uvec4(0));
            directions.push_back(FACE_UP);
        }

        //Only need to do this once for all sides because they all have the same amount of bricks
        //The total amount of bricks we have VRAM space allocated for:
        transparentBasicAlloc[0] = positionsAndMats.size();

        glBindVertexArray(transparentPerTextureVAO[BRICKTEX_STUDS]);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePositionMatBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,positionsAndMats.size() * sizeof(glm::vec4),&positionsAndMats[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureRotationBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,rotations.size() * sizeof(glm::quat),&rotations[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTexturePaintColorBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,paintColors.size() * sizeof(glm::vec4),&paintColors[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDimensionBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,dimensions.size() * sizeof(glm::uvec4),&dimensions[0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDirectionBuffer[BRICKTEX_STUDS]);
        glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        //We use directions.size() because it will be correct even for the sides of bricks that use extra instancing
        transparentPerTextureInstances[BRICKTEX_STUDS] = directions.size() - extraAlloc;

        //Sides of bricks:

        directions.clear();

        for(unsigned int i = 0; i<transparentBasicBricks.size(); i++)
        {
            //Man, if I ever change the order of my side's enum this will really break lol
            for(unsigned int side = FACE_NORTH; side <= FACE_WEST; side++)
                directions.push_back((faceDirection)side);
        }

        for(unsigned int i = 0; i<extraAlloc; i++)
        {
            for(unsigned int side = FACE_NORTH; side <= FACE_WEST; side++)
                directions.push_back((faceDirection)side);
        }

        glBindVertexArray(transparentPerTextureVAO[BRICKTEX_SIDES]);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDirectionBuffer[BRICKTEX_SIDES]);
        glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        transparentPerTextureInstances[BRICKTEX_SIDES] = directions.size() - (extraAlloc*4);

        //Bottoms of bricks:

        directions.clear();

        for(unsigned int i = 0; i<transparentBasicBricks.size(); i++)
        {
            directions.push_back(FACE_DOWN);
        }

        for(unsigned int i = 0; i<extraAlloc; i++)
        {
            directions.push_back(FACE_DOWN);
        }

        glBindVertexArray(transparentPerTextureVAO[BRICKTEX_BOTTOM]);

        glBindBuffer(GL_ARRAY_BUFFER,transparentPerTextureDirectionBuffer[BRICKTEX_BOTTOM]);
        glBufferData(GL_ARRAY_BUFFER,directions.size() * sizeof(unsigned char),&directions[0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        transparentPerTextureInstances[BRICKTEX_BOTTOM] = directions.size() - extraAlloc;

        recompileTransparentPrints(extraAllocPrints);
    }

    void newBrickRenderer::recompileEverything()
    {
        std::cout<<"Opaque basic bricks: "<<opaqueBasicBricks.size()<<" Transparent basic bricks: "<<transparentBasicBricks.size()<<"\n";
        std::cout<<"Opaque special bricks: "<<opaqueSpecialBricks.size()<<" Transparent special bricks: "<<transparentSpecialBricks.size()<<"\n";

        int startTime = SDL_GetTicks();

        recompileOpaqueBasicBricks();

        recompileTransparentBasicBricks();

        recompileOpaqueSpecialBricks();

        recompileTransparentSpecialBricks();

        info("Recompiled all brick faces in: " + std::to_string(SDL_GetTicks() - startTime) + " ms");
    }
}
























