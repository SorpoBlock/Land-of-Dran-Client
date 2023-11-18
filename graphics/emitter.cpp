#include "emitter.h"

namespace syj
{
    void emitter::update(glm::vec3 eyePos,glm::vec3 eyeDir,bool useBodyPos,bool firstPerson,float cameraYaw,float cameraPitch)
    {
        if(!enabled)
        {
            lastEmission = SDL_GetTicks();
            realLastEmission = SDL_GetTicks();
            return;
        }

        if(attachedToBasicBrick)
        {
            emitterRange = glm::vec3(attachedToBasicBrick->dimensions.x,0,attachedToBasicBrick->dimensions.z);
            position = attachedToBasicBrick->position;
        }
        else if(attachedToSpecialBrick)
        {
            emitterRange = glm::vec3(attachedToSpecialBrick->type->type->width,0,attachedToSpecialBrick->type->type->length);
            position = attachedToSpecialBrick->position;
        }
        else if(attachedToModel)
        {
            emitterRange = glm::vec3(0,0,0);
            if(useBodyPos && attachedToModel->body)
            {
                btVector3 o = attachedToModel->body->getWorldTransform().getOrigin();
                position = glm::vec3(o.x(),o.y(),o.z());
            }
            else
                position = attachedToModel->modelInterpolator.getPosition();
            //TODO: THERE IS (was?) A CRASH HERE
            if(meshName != "")
            {
                for(unsigned int a = 0; a<attachedToModel->type->instancedMeshes.size(); a++)
                {
                    newMesh *tmp = attachedToModel->type->instancedMeshes[a];
                    if(tmp->name == meshName)
                    {
                        glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                        center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                        center *= attachedToModel->scale;
                        center = multVec3ByMat(attachedToModel->modelInterpolator.getRotation(),center);
                        position += center;
                        break;
                    }
                }
            }
        }
        else if(attachedToItem)
        {
            if(attachedToItem->hidden)
                return;

            if(attachedToItem->heldBy)
            {
                //Copied from item::updateTransform

                glm::mat4 rot;
                glm::vec3 playerPos;

                glm::vec4 offset;
                if(firstPerson)
                {
                    offset = glm::vec4(attachedToItem->itemType->handOffset.x,attachedToItem->itemType->handOffset.y,0,1);
                    offset += glm::toMat4(glm::quat(glm::vec3(cameraPitch,0,0))) * glm::vec4(0,0,attachedToItem->itemType->handOffset.z,1);
                    offset.w = 1.0;
                }
                else
                    offset = glm::vec4(attachedToItem->itemType->handOffset.x,attachedToItem->itemType->handOffset.y,attachedToItem->itemType->handOffset.z,1);

                if(useBodyPos && attachedToItem->heldBy->body)
                {
                    rot = glm::toMat4(glm::quat(glm::vec3(0,cameraYaw+3.1415,0)));
                    btVector3 bulletPos = attachedToItem->heldBy->body->getWorldTransform().getOrigin();
                    playerPos.x = bulletPos.x();
                    playerPos.y = bulletPos.y();
                    playerPos.z = bulletPos.z();
                }
                else
                {
                    rot = glm::toMat4(attachedToItem->heldBy->modelInterpolator.getRotation());
                    playerPos = attachedToItem->heldBy->modelInterpolator.getPosition();
                }

                glm::vec4 meshOffset = glm::vec4(0,1,-2.3,1);
                if(meshName != "")
                {
                    for(unsigned int a = 0; a<attachedToItem->type->instancedMeshes.size(); a++)
                    {
                        newMesh *tmp = attachedToItem->type->instancedMeshes[a];
                        if(tmp->name == meshName)
                        {
                            glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                            center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                            center *= attachedToItem->scale;
                            center = multVec3ByMat(attachedToItem->modelInterpolator.getRotation(),center);
                            meshOffset.x += center.x;
                            meshOffset.y += center.y;
                            meshOffset.z += center.z;
                            break;
                        }
                    }
                }

                offset = rot * offset;
                glm::vec4 result = glm::translate(playerPos + glm::vec3(offset.x,offset.y,offset.z)) *
                                    rot *
                                    glm::toMat4(glm::quat(glm::vec3(cameraPitch,0,0)))  *
                                    glm::toMat4(attachedToItem->itemType->handRot)*
                                    meshOffset;

                position.x = result.x;
                position.y = result.y;
                position.z = result.z;

            }
            else
            {
                position = attachedToItem->modelInterpolator.getPosition();
                //TODO: THERE IS (was?) A CRASH HERE (i copied this comment lol)
                if(meshName != "")
                {
                    for(unsigned int a = 0; a<attachedToItem->type->instancedMeshes.size(); a++)
                    {
                        newMesh *tmp = attachedToItem->type->instancedMeshes[a];
                        if(tmp->name == meshName)
                        {
                            glm::vec3 center = tmp->rawMaxExtents - tmp->rawMinExtents;
                            center = center / glm::vec3(2,2,2) + tmp->rawMinExtents;
                            center *= attachedToItem->scale;
                            center = multVec3ByMat(attachedToItem->modelInterpolator.getRotation(),center);
                            position += center;
                            break;
                        }
                    }
                }
            }
        }
        else if(attachedToCar)
        {
            emitterRange = glm::vec3(1,0,1);
            //position = attachedToCar->wheels[whichWheel]->getPosition() - glm::vec3(0,attachedToCar->wheelBrickData[whichWheel].wheelScale,0);
            position = attachedToCar->newWheels[whichWheel]->modelInterpolator.getPosition() - glm::vec3(0,attachedToCar->wheelBrickData[whichWheel].wheelScale,0);
        }
        justAttached = false;

        if((unsigned)(lastEmission + type->ejectionPeriodMS) > SDL_GetTicks())
            return;

        float deltaT = SDL_GetTicks() - realLastEmission;

        realLastEmission = SDL_GetTicks();
        if(type->periodVarianceMS != 0)
            lastEmission = SDL_GetTicks() + (rand() % type->periodVarianceMS*2) - type->periodVarianceMS;
        else
            lastEmission = realLastEmission;

        currentPhi += type->phiReferenceVel * deltaT;
        currentPhi = fmod(currentPhi,6.28318530718);

        float theta = drand(type->thetaMin,type->thetaMax);
        float finalPhi = drand(-type->phiVariance,type->phiVariance) + currentPhi;

        glm::vec3 normal = glm::vec3(cos(finalPhi)*sin(theta),cos(theta),sin(finalPhi)*sin(theta));
        float posLength = type->ejectionOffset + drand(type->ejectionOffsetRandomMin,type->ejectionOffsetRandomMax);
        glm::vec3 offset = position + normal * posLength;
        float velLength = type->ejectionVelocity + drand(-type->velocityVariance,type->velocityVariance);
        glm::vec3 velocity = normal * velLength;

        if(shootFromHand)
        {
            glm::vec3 eyeTarget = eyePos + eyeDir * glm::vec3(10,10,10);
            glm::vec3 finalDir = eyeTarget - offset;
            velocity = glm::normalize(finalDir) * glm::vec3(25,25,25);
        }

        for(unsigned int a = 0; a<type->particleTypes.size(); a++)
            type->particleTypes[a]->addParticle(offset,velocity + type->particleTypes[a]->inheritedVelFactor * attachedVelocity);
    }

    particleType::particleType()
    {
        glGenVertexArrays(1,&vao);
        glGenBuffers(3,buffers);

        std::vector<glm::vec2> verts;
        float low = -0.5;
        float high = 0.5;

        //Right bottom triangle
        verts.push_back(glm::vec2(low,low));
        verts.push_back(glm::vec2(high,low));
        verts.push_back(glm::vec2(high,high));

        //Left top triangle
        verts.push_back(glm::vec2(high,high));
        verts.push_back(glm::vec2(low,high));
        verts.push_back(glm::vec2(low,low));

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[vertexBuffer]);
        glBufferData(GL_ARRAY_BUFFER,verts.size() * sizeof(glm::vec2),&verts[0],GL_STATIC_DRAW);
        glVertexAttribPointer(vertexBuffer,2,GL_FLOAT,GL_FALSE,0,0);
        glVertexAttribDivisor(vertexBuffer,0);
        glEnableVertexAttribArray(vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindVertexArray(0);
    }

    particleType::~particleType()
    {
        glDeleteBuffers(3,buffers);
        glDeleteVertexArrays(1,&vao);
    }

    glm::vec3 camPos = glm::vec3(0,0,0);

    bool sortParticles(particle *a,particle *b)
    {
        if(!a && b)
            return true;
        else if(!b && a)
            return false;
        else if(!b && !a)
            return false;
        else
            return glm::distance2(a->startingPos,camPos) > glm::distance2(b->startingPos,camPos);
    }

    void particleType::deadParticlesCheck(glm::vec3 cameraPos,float deltaT)
    {
        if(particles.size() < 1)
            return;

        if(needSorting)
        {
            deltaT /= 1000;
            for(unsigned int a = 0; a<particles.size(); a++)
            {
                if(!particles[a])
                    continue;

                particles[a]->startingVel -= deltaT * (particles[a]->startingVel * dragCoefficient);
                particles[a]->startingVel += gravityCoefficient * deltaT;
                particles[a]->startingPos += particles[a]->startingVel*deltaT;

                /*std::cout<<"Particle "<<a<<":\n";
                std::cout<<"Pos: "<<particles[a]->startingPos.x<<","<<particles[a]->startingPos.y<<","<<particles[a]->startingPos.z<<"\n";
                std::cout<<"Vel: "<<particles[a]->startingVel.x<<","<<particles[a]->startingVel.y<<","<<particles[a]->startingVel.z<<"\n\n";*/

                glm::vec4 vec = glm::vec4(particles[a]->startingPos.x,particles[a]->startingPos.y,particles[a]->startingPos.z,particles[a]->creationTimeMS);

                glBindBuffer(GL_ARRAY_BUFFER,buffers[startingPosAndTime]);
                glBufferSubData(GL_ARRAY_BUFFER,a * sizeof(glm::vec4),sizeof(glm::vec4),&vec);
            }
        }

        //std::cout<<"Particles: "<<particles.size()<<"-"<<emptySlots<<" = "<<particles.size()-emptySlots<<"\n";

        int msBetweenChecks = 100;
        if(particles.size() < 100)
            msBetweenChecks = 0;
        else if(particles.size() < 250)
            msBetweenChecks = 25;
        else if(particles.size() < 1000)
            msBetweenChecks = 100;
        else
            msBetweenChecks = 150;

        if(lastCheckTime + msBetweenChecks > SDL_GetTicks())
            return;
        lastCheckTime = SDL_GetTicks();

        glm::vec4 emptyVec = glm::vec4(0,0,0,-1);

        for(unsigned int a = 0; a<particles.size(); a++)
        {
            if(!particles[a])
                continue;

            if(particles[a]->creationTimeMS + lifetimeMS + particles[a]->lifetimeRandomness < SDL_GetTicks())
            {
                glBindBuffer(GL_ARRAY_BUFFER,buffers[startingPosAndTime]);
                glBufferSubData(GL_ARRAY_BUFFER,a * sizeof(glm::vec4),sizeof(glm::vec4),&emptyVec);
                delete particles[a];
                particles[a] = 0;
                emptySlots++;
            }
        }

        if(needSorting)
        {
            camPos = cameraPos;

            sort(particles.begin(),particles.end(),sortParticles);
            recompile();

            return;
        }
    }

    void particleType::render(uniformsHolder *emitterUnis)
    {
        if(particles.size() < 1)
            return;

        if(baseTexture)
            baseTexture->bind(albedo);

        glUniform1f(emitterUnis->lifetimeMS,lifetimeMS);
        glUniform3f(emitterUnis->drag,dragCoefficient.x,dragCoefficient.y,dragCoefficient.z);
        glUniform3f(emitterUnis->gravity,gravityCoefficient.x*2.0,gravityCoefficient.y*2.0,gravityCoefficient.z*2.0);
        glUniform1f(emitterUnis->spinSpeed,spinSpeed);
        for(int a = 0; a<4; a++)
        {
            glUniform1f(emitterUnis->sizes[a],sizes[a]);
            glUniform4f(emitterUnis->colors[a],colors[a].r,colors[a].g,colors[a].b,colors[a].a);
            glUniform1f(emitterUnis->times[a],times[a]);
        }
        glUniform1i(emitterUnis->useInvAlpha,useInvAlpha);
        glUniform1f(emitterUnis->currentTimeMS,SDL_GetTicks());
        glUniform1i(emitterUnis->calculateMovement,!needSorting);

        if(useInvAlpha)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        else
            glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES,0,6,particles.size());
        glBindVertexArray(0);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void particleType::addParticle(glm::vec3 startingPos,glm::vec3 startingVelocity)
    {
        particle *tmp = new particle;
        tmp->type = this;
        tmp->startingPos = startingPos;
        tmp->startingVel = startingVelocity;
        tmp->creationTimeMS = SDL_GetTicks();
        if(lifetimeVarianceMS != 0)
            tmp->lifetimeRandomness = (rand() % (lifetimeVarianceMS*2))-lifetimeVarianceMS;
        else
            tmp->lifetimeRandomness = 0;

        glm::vec4 posAndTime = glm::vec4(tmp->startingPos.x,tmp->startingPos.y,tmp->startingPos.z,tmp->creationTimeMS);
        glm::vec4 velAndLifeRandom = glm::vec4(tmp->startingVel.x,tmp->startingVel.y,tmp->startingVel.z,tmp->lifetimeRandomness);

        if(emptySlots > 0)
        {
            for(unsigned int a = 0; a<particles.size(); a++)
            {
                if(!particles[a])
                {
                    particles[a] = tmp;

                    glBindBuffer(GL_ARRAY_BUFFER,buffers[startingPosAndTime]);
                    glBufferSubData(GL_ARRAY_BUFFER,a * sizeof(glm::vec4),sizeof(glm::vec4),&posAndTime);

                    glBindBuffer(GL_ARRAY_BUFFER,buffers[startingVel]);
                    glBufferSubData(GL_ARRAY_BUFFER,a * sizeof(glm::vec4),sizeof(glm::vec4),&velAndLifeRandom);

                    emptySlots--;

                    return;
                }
            }
        }

        if(particles.size() + 1 < allocatedParticleSpace)
        {
            int offset = particles.size();
            particles.push_back(tmp);

            glBindBuffer(GL_ARRAY_BUFFER,buffers[startingPosAndTime]);
            glBufferSubData(GL_ARRAY_BUFFER,offset * sizeof(glm::vec4),sizeof(glm::vec4),&posAndTime);

            glBindBuffer(GL_ARRAY_BUFFER,buffers[startingVel]);
            glBufferSubData(GL_ARRAY_BUFFER,offset * sizeof(glm::vec4),sizeof(glm::vec4),&velAndLifeRandom);

            return;
        }

        particles.push_back(tmp);
        recompile();
    }

    void particleType::scaleToBlockland()
    {
        gravityCoefficient.x *= 20.0;
        gravityCoefficient.y *= 20.0;
        gravityCoefficient.z *= 20.0;
        for(int a = 0; a<4; a++)
            sizes[a] *= 3.0;

        if(dragCoefficient.x == 0)
            dragCoefficient.x = 0.001;
        if(dragCoefficient.y == 0)
            dragCoefficient.y = 0.001;
        if(dragCoefficient.z == 0)
            dragCoefficient.z = 0.001;
    }

    void particleType::recompile()
    {
        std::vector<particle*> swapVector;

        for(unsigned int a = 0; a<particles.size(); a++)
            if(particles[a])
                swapVector.push_back(particles[a]);
        particles.clear();

        for(unsigned int a = 0; a<swapVector.size(); a++)
            particles.push_back(swapVector[a]);

        std::vector<glm::vec4> startingPosAndTimes;
        std::vector<glm::vec4> startingVelocitiesAndLifeRandom;

        for(unsigned int a = 0; a<particles.size(); a++)
        {
            particle *tmp = particles[a];
            /*int howBig = startingPosAndTimes.size();
            int howBigOther = particles.size();*/
            //TODO: There was a crash here 11sep2023

            glm::vec4 posAndTime = glm::vec4(
                                                    tmp->startingPos.x,
                                                    tmp->startingPos.y,
                                                    tmp->startingPos.z,
                                                    tmp->creationTimeMS);

            glm::vec4 velAndLifeRandom = glm::vec4(
                                                   tmp->startingVel.x,
                                                   tmp->startingVel.y,
                                                   tmp->startingVel.z,
                                                   tmp->lifetimeRandomness);

            startingPosAndTimes.push_back(posAndTime);
            startingVelocitiesAndLifeRandom.push_back(velAndLifeRandom);

        }
        emptySlots = 0;

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER,buffers[startingPosAndTime]);

        if(particles.size() >= allocatedParticleSpace)
            glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec4) * (startingPosAndTimes.size() + particleAllocSize),(void*)0,GL_STREAM_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(glm::vec4) * startingPosAndTimes.size(),&startingPosAndTimes[0][0]);
        glVertexAttribPointer(startingPosAndTime,4,GL_FLOAT,GL_FALSE,0,0);
        glVertexAttribDivisor(startingPosAndTime,1);
        glEnableVertexAttribArray(startingPosAndTime);

        glBindBuffer(GL_ARRAY_BUFFER,buffers[startingVel]);

        if(particles.size() >= allocatedParticleSpace)
        {
            glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec4) * (startingVelocitiesAndLifeRandom.size() + particleAllocSize),(void*)0,GL_STREAM_DRAW);
            allocatedParticleSpace = particles.size() + particleAllocSize;
        }

        glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(glm::vec4) * startingVelocitiesAndLifeRandom.size(),&startingVelocitiesAndLifeRandom[0][0]);
        glVertexAttribPointer(startingVel,4,GL_FLOAT,GL_FALSE,0,0);
        glVertexAttribDivisor(startingVel,1);
        glEnableVertexAttribArray(startingVel);

        glBindVertexArray(0);
    }
}
