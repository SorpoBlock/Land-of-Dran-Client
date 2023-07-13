#include "player.h"

namespace syj
{

    bool sameSign(float a,float b)
    {
        return a>0 ? b>0 : b<0;
    }

    bool hasHit(btDynamicsWorld *dynamicsWorld,btVector3 from,btVector3 to,btRigidBody *toIgnore,float &dist)
    {
        btCollisionWorld::AllHitsRayResultCallback res(from,to);
        dynamicsWorld->rayTest(from,to, res);
        for(unsigned int a = 0; a<res.m_collisionObjects.size(); a++)
        {
            if(res.m_collisionObjects[a] != toIgnore)
            {
                dist = res.m_hitFractions[a];
                return true;
            }
        }
        return false;
    }

    /*btVector3 dynamic::getLinearVelocity()
    {
        return body->getLinearVelocity();
    }

    void dynamic::smoothLinearVelocity(btVector3 vel,float deltaMS)
    {
        float blend = deltaMS / 16.666; //standardize to 60 fps
        //32.33 ms, blend = 2
        blend = 1.0 / pow(2,blend);
        //blend pow = 1/4

        btVector3 oldVel = getLinearVelocity();
        setLinearVelocity(oldVel * blend + vel * (1.0-blend));
    }

    std::string bts(bool in){return in ? "True " : "False ";}

    void dynamic::control(float deltaMS,glm::vec3 direction,bool forward,bool backward,bool left,bool right,bool jump)
    {
        cameraFollowDistance = 30.0;
        btVector3 cameraEnd = getPosition() - glmToBt(direction) * cameraFollowDistance;
        btCollisionWorld::AllHitsRayResultCallback camera(getPosition(),cameraEnd);
        float lowestHitFrac = 1.0;
        world->rayTest(getPosition(),cameraEnd,camera);
        for(int a = 0; a<camera.m_collisionObjects.size(); a++)
        {
            if(camera.m_collisionObjects[a] != body)
            {
                if(camera.m_hitFractions[a] < lowestHitFrac && camera.m_hitFractions[a] > 0.08)
                    lowestHitFrac = camera.m_hitFractions[a];
            }
        }
        if(lowestHitFrac > 1.0)
            lowestHitFrac -= 0.3;
        cameraFollowDistance *= lowestHitFrac;
        cameraFollowDistance *= 0.9;

        float bodyLength = (scale.z * (max.z()-min.z()))/2.0;
        float bodyWidth = (scale.x * (max.x()-min.x()))/2.0;
        float footDown = (scale.y * (max.y()-min.y()))/2.0;

        int sides = 0;
        btVector3 directionSpeed(0,0,0);
        float yaw = atan2(direction.x,direction.z);
        setRotation(btQuaternion(3.1415 + yaw,0,0));

        //std::cout<<"Yaw: "<<yaw<<" forward: "<<(forward ? "Yes" : "No")<<"\n";

        bool sideWays = false;
        bool stopWalking = true;
        if(forward)
        {
            sides++;
            directionSpeed += btVector3(sin(yaw),0,cos(yaw));
            stopWalking = false;
        }
        else if(backward)
        {
            sides++;
            directionSpeed += btVector3(sin(yaw+3.1415),0,cos(yaw+3.1415));
            stopWalking = false;
        }
        if(left)
        {
            sides++;
            sideWays = true;
            directionSpeed += btVector3(sin(yaw-1.57),0,cos(yaw-1.57));
            stopWalking = false;
        }
        else if(right)
        {
            sides++;
            sideWays = true;
            directionSpeed += btVector3(sin(yaw+1.57),0,cos(yaw+1.57));
            stopWalking = false;
        }

        float middlex = directionSpeed.normalized().x();
        float middlez = directionSpeed.normalized().z();
        btVector3 dir = btVector3(sin(yaw),0,cos(yaw));
        btVector3 perpDir = dir.cross(btVector3(0,1,0));
        if(stopWalking)
            dir = btVector3(0,0,0);
        else
            dir = btVector3(middlex,0,middlez);

        bool onGround = false;
        btVector3 groundEnd = getPosition() - btVector3(0,footDown+0.4,0);
        btCollisionWorld::AllHitsRayResultCallback ground(getPosition(),groundEnd);
        world->rayTest(getPosition(),groundEnd,ground);
        //debugLocations[0] = BtToGlm(groundEnd);

        for(int a = 0; a<ground.m_collisionObjects.size(); a++)
        {
            if(ground.m_collisionObjects[a] != body)
            {
                onGround = true;
                break;
            }
        }

        if(!onGround)
        {
            btVector3 groundStart = getPosition() - perpDir * bodyWidth;
            btVector3 groundEnd = groundStart - btVector3(0,footDown+0.4,0);
            //debugLocations[1] = BtToGlm(groundEnd);
            btCollisionWorld::AllHitsRayResultCallback ground(groundStart,groundEnd);
            world->rayTest(groundStart,groundEnd,ground);
            for(int a = 0; a<ground.m_collisionObjects.size(); a++)
            {
                if(ground.m_collisionObjects[a] != body)
                {
                    onGround = true;
                    break;
                }
            }
        }

        if(!onGround)
        {
            btVector3 groundStart = getPosition() + perpDir * bodyWidth;
            btVector3 groundEnd = groundStart - btVector3(0,footDown+0.4,0);
            //debugLocations[2] = BtToGlm(groundEnd);
            btCollisionWorld::AllHitsRayResultCallback ground(groundStart,groundEnd);
            world->rayTest(groundStart,groundEnd,ground);
            for(int a = 0; a<ground.m_collisionObjects.size(); a++)
            {
                if(ground.m_collisionObjects[a] != body)
                {
                    onGround = true;
                    break;
                }
            }
        }

        if(stopWalking)
        {
            body->setDamping(0.1,0);
            if(SDL_GetTicks() - lastWalked < 50)
            {
                body->setFriction(5.0);
            }
            else
            {
                body->setFriction(0.5);
            }
        }
        else
        {
            lastWalked = SDL_GetTicks();
            body->setDamping(0,0);
            body->setFriction(0);

            btVector3 toSet = body->getLinearVelocity();
            toSet.setX(dir.x()*20);
            toSet.setZ(dir.z()*20);
            body->setLinearVelocity(toSet);
        }

        if(jump && onGround && body->getLinearVelocity().y() < 1)
        {
            body->applyCentralForce(btVector3(0,2000,0));
            return;
        }

        /*float maxSpeed = 15.0;
        float sideSpeed = btVector3(vel.x(),0,vel.z()).length();

        //Where we actually calculate walking and jumping

        if(sides == 2)
            maxSpeed *= 1.5;

        btVector3 walkVel = directionSpeed * maxSpeed * 0.7;
        if(sides == 2)
            walkVel *= 0.7;
        if(sideSpeed > maxSpeed)
        {
            if(sameSign(walkVel.x(),vel.x()) && fabs(walkVel.x()) > fabs(vel.x()))
                walkVel.setX(0);
            if(sameSign(walkVel.z(),vel.z()) && fabs(walkVel.z()) > fabs(vel.z()))
                walkVel.setZ(0);
        }

        if(stopWalking)
            body->setFriction(0.9);
        else
            body->setFriction(0.1);
        body->applyCentralForce(walkVel);*/
        /*if(onGround && !stopWalking)
        {
            btVector3 newVel = btVector3(vel * 0.2 + walkVel);
            newVel.setY(vel.y());
            setLinearVelocity(newVel);
        }
        else if(onGround && stopWalking)
        {
            btVector3 newVel = btVector3(vel * 0.2);
            newVel.setY(vel.y());
            setLinearVelocity(newVel);
        }
        else if(!onGround && !stopWalking)
        {
            btVector3 newVel = btVector3(vel * 0.75 + walkVel * 0.25);
            newVel.setY(vel.y());
            setLinearVelocity(newVel);
        }
        else if(!onGround && stopWalking)
        {

        }

        //sayVec3(body->getLinearVelocity());
        //End walking and jumping calculations

        if(stopWalking)
            stop();
        else
        {
            play("walk");

            if(!onGround)
                return;

            footDown -= 0.2;

            if(sideWays)
                std::swap(bodyLength,bodyWidth);
            bodyLength += 1.25;

            bool gotHit = false;
            float hitFrac;
            btVector3 rayHitPos;

            btVector3 footPosition = getPosition() - btVector3(0,footDown,0);
            btVector3 footEndPosition = footPosition + dir * bodyLength;
            btCollisionWorld::AllHitsRayResultCallback res(footPosition,footEndPosition);
            world->rayTest(footPosition,footEndPosition,res);

            for(int a = 0; a<res.m_collisionObjects.size(); a++)
            {
                if(res.m_collisionObjects[a] != body && (((btRigidBody*)res.m_collisionObjects[a])->getUserIndex() == userIndex_staticNormalBrick||((btRigidBody*)res.m_collisionObjects[a])->getUserIndex() == userIndex_staticSpecialBrick))
                {
                    gotHit = true;
                    rayHitPos = res.m_hitPointWorld[a];
                    hitFrac = res.m_hitFractions[a];
                }
            }

            if(!gotHit)
            {
                btVector3 footPosition = getPosition() - btVector3(0,footDown,0) - perpDir * bodyWidth;
                btVector3 footEndPosition = footPosition + dir * bodyLength;
                btCollisionWorld::AllHitsRayResultCallback res(footPosition,footEndPosition);
                world->rayTest(footPosition,footEndPosition,res);

                for(int a = 0; a<res.m_collisionObjects.size(); a++)
                {
                    if(res.m_collisionObjects[a] != body)
                    {
                        gotHit = true;
                        rayHitPos = res.m_hitPointWorld[a];
                        hitFrac = res.m_hitFractions[a];
                    }
                }
            }

            if(!gotHit)
            {
                btVector3 footPosition = getPosition() - btVector3(0,footDown,0) + perpDir * bodyWidth;
                btVector3 footEndPosition = footPosition + dir * bodyLength;
                btCollisionWorld::AllHitsRayResultCallback res(footPosition,footEndPosition);
                world->rayTest(footPosition,footEndPosition,res);

                for(int a = 0; a<res.m_collisionObjects.size(); a++)
                {
                    if(res.m_collisionObjects[a] != body && ((btRigidBody*)res.m_collisionObjects[a])->getUserIndex() != userIndex_livingBrick)
                    {
                        gotHit = true;
                        rayHitPos = res.m_hitPointWorld[a];
                        hitFrac = res.m_hitFractions[a];
                    }
                }
            }

            if(gotHit)
            {
                btVector3 start = rayHitPos + btVector3(0,footDown,0) + dir * 0.5;

                btCollisionWorld::AllHitsRayResultCallback res(start,rayHitPos);
                world->rayTest(start,rayHitPos,res);

                bool gotAnotherHit = false;
                float highestHeight = -99999.0;
                const btCollisionObject *lastBody = 0;
                btVector3 lastHit;

                for(int a = 0; a<res.m_collisionObjects.size(); a++)
                {
                    if(res.m_collisionObjects[a] != body && ((btRigidBody*)res.m_collisionObjects[a])->getUserIndex() != userIndex_livingBrick)
                    {
                        gotAnotherHit = true;
                        if(res.m_hitPointWorld[a].y() > highestHeight)
                        {
                            highestHeight = res.m_hitPointWorld[a].y();
                            lastHit = res.m_hitPointWorld[a];
                            lastBody = res.m_collisionObjects.at(a);
                        }
                    }
                }

                if(gotAnotherHit)
                {
                    if(highestHeight - footPosition.y() < 2.9 && highestHeight - footPosition.y() > 0.0)
                    {
                        float height = (scale.y * (max.y()-min.y()));
                        btCollisionWorld::AllHitsRayResultCallback colTest(lastHit,lastHit + btVector3(0,height,0));
                        world->rayTest(lastHit,lastHit + btVector3(0,height,0),colTest);

                        for(int a = 0; a<colTest.m_collisionObjects.size(); a++)
                        {
                            if(colTest.m_collisionObjects[a] != body)
                            {
                                if(colTest.m_collisionObjects[a] != lastBody)
                                {
                                    std::cout<<"We couldn't fit!\n";
                                    return;
                                }
                            }
                        }

                        std::cout<<"Setting position: "<<highestHeight - footPosition.y()<<"\n";
                        btVector3 pos = getPosition();
                        pos.setY(0.15 + pos.y() + (highestHeight - footPosition.y()));
                        pos.setX(pos.x() + 1.3 * dir.x());
                        pos.setZ(pos.z() + 1.3 * dir.z());
                        setPosition(pos);
                    }
                }
            }
        }
    }

    btVector3 dynamic::getPosition()
    {
        btTransform t = body->getWorldTransform();
        return t.getOrigin();
    }

    dynamic::dynamic(btDynamicsWorld *_world,animatedModel *_type,glm::vec3 baseScale)
    {
        scale = baseScale;
        type = _type;
        bool hasCollision = false;
        world = _world;

        nodeColors.clear();
        for(int a = 0; a<type->numPickingIDs; a++)
            nodeColors.push_back(glm::vec3(1,1,1));

        for(int a = 0; a<type->meshes.size(); a++)
        {
            if(type->meshes[a]->detectedCollisionMesh)
            {
                hasCollision = true;
                min = glmToBt(type->meshes[a]->detectedColMin);
                max = glmToBt(type->meshes[a]->detectedColMax);
            }
        }
        btVector3 size = max-min;
        size /= 2.0;
        size *= glmToBt(scale);

        btBoxShape *shape = new btBoxShape(size);
        btVector3 inertia;
        shape->calculateLocalInertia(1.0,inertia);
        btTransform startTrans = btTransform::getIdentity();
        startTrans.setOrigin(btVector3(0,0,0));
        btMotionState *ms = new btDefaultMotionState(startTrans);
        body = new btRigidBody(1.0,ms,shape,inertia);
        body->setActivationState(DISABLE_DEACTIVATION);
        body->setAngularFactor(btVector3(0,0,0));
        world->addRigidBody(body);
    }

    void dynamic::setPosition(glm::vec3 pos)
    {
        btTransform t = body->getWorldTransform();
        t.setOrigin(btVector3(pos.x,pos.y,pos.z));
        body->setWorldTransform(t);
    }

    void dynamic::setPosition(btVector3 pos)
    {
        btTransform t = body->getWorldTransform();
        t.setOrigin(pos);
        body->setWorldTransform(t);
    }

    void dynamic::setRotation(glm::quat rot)
    {
        btTransform t = body->getWorldTransform();
        t.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
        body->setWorldTransform(t);
    }

    void dynamic::setRotation(btQuaternion quat)
    {
        btTransform t = body->getWorldTransform();
        t.setRotation(quat);
        body->setWorldTransform(t);
    }

    void dynamic::setLinearVelocity(glm::vec3 vel)
    {
        body->setLinearVelocity(btVector3(vel.x,vel.y,vel.z));
    }

    void dynamic::setLinearVelocity(btVector3 vel)
    {
        body->setLinearVelocity(vel);
    }

    void dynamic::render(uniformsHolder *unis,float deltaMS,bool stopInterpolation,bool skipMats)
    {
        /*btTransform t;
        body->getMotionState()->getWorldTransform(t);*/
        /*btTransform t = body->getWorldTransform();
        btVector3 o = t.getOrigin();
        btQuaternion q = t.getRotation();
        glm::vec3 offset = scale * BtToGlm(max+min) / glm::vec3(2.0);
        //position = glm::vec3(o.x()-offset.x,o.y()-offset.y,o.z()-offset.z);
        //rotation = glm::quat(q.w(),q.x(),q.y(),q.z());

        glUniform1i(unis->target->getUniformLocation("useTint"),true);
        glUniform3f(unis->target->getUniformLocation("tint"),colorTint.r,colorTint.g,colorTint.b);
        renderInst(unis,deltaMS,stopInterpolation,skipMats);
        glUniform1i(unis->target->getUniformLocation("useTint"),false);
    }*/

    item::item(btDynamicsWorld *_world,newModel *_type,glm::vec3 baseScale)
        : newDynamic(_type,baseScale)
        {}

    void item::render(uniformsHolder &unis,bool useDir,float yaw)
    {
        if(hidden)
            return;

        if(heldBy)
        {
            float actualPitch = -0.785 * (sin(pitch)+1.0);
            glm::vec4 offset = glm::vec4(itemType->handOffset.x,itemType->handOffset.y,itemType->handOffset.z,1);
            glm::mat4 rot;
            if(useDir)
                rot = glm::toMat4(glm::quat(glm::vec3(0,yaw+3.1415,0)));
            else
                rot = glm::toMat4(heldBy->modelInterpolator.getRotation());
            offset = rot * offset;
            useGlobalTransform = true;
            globalTransform =
                        glm::translate(heldBy->modelInterpolator.getPosition() + glm::vec3(offset.x,offset.y,offset.z)) *
                        rot *
                        glm::toMat4(glm::quat(glm::vec3(actualPitch,0,0)));
            /*type->render(&unis,0,
                        glm::translate(heldBy->modelInterpolator.getPosition() + glm::vec3(offset.x,offset.y,offset.z)) *
                        rot *
                        glm::toMat4(glm::quat(glm::vec3(actualPitch,0,0))) *
                        glm::scale(scale));*/
        }
        else
        {
            useGlobalTransform = false;
            //renderInst(&unis,0);
        }
    }

    void item::advance(bool mouseIsDown,float deltaMS)
    {
        if(!heldBy)
            modelInterpolator.advance(deltaMS);

        if(itemType->useDefaultSwing)
        {
            if(mouseIsDown || swinging)
                pitch += deltaMS * 0.03;
            else
                pitch = -1;
        }
    }
}



















