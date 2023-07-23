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

    btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world)
    {
        btCollisionWorld::AllHitsRayResultCallback ground(start,end);
        world->rayTest(start,end,ground);

        float closestDist = 100;
        int closestIdx = -1;

        for(int a = 0; a<ground.m_collisionObjects.size(); a++)
        {
            float dist = (ground.m_hitPointWorld[a]-start).length();
            if(dist < closestDist)
            {
                closestIdx = a;
                closestDist = dist;
            }
        }

        if(closestIdx != -1)
            return (btRigidBody*)ground.m_collisionObjects[closestIdx];
        else
            return 0;
    }

    btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world,btRigidBody *ignore,btVector3 &hitPos)
    {
        btCollisionWorld::AllHitsRayResultCallback ground(start,end);
        world->rayTest(start,end,ground);

        float closestDist = 100;
        int closestIdx = -1;

        for(int a = 0; a<ground.m_collisionObjects.size(); a++)
        {
            if(ground.m_collisionObjects[a] == ignore)
                continue;

            float dist = (ground.m_hitPointWorld[a]-start).length();
            if(dist < closestDist)
            {
                hitPos = ground.m_hitPointWorld[a];
                closestIdx = a;
                closestDist = dist;
            }
        }

        if(closestIdx != -1)
            return (btRigidBody*)ground.m_collisionObjects[closestIdx];
        else
            return 0;
    }

    btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world,btRigidBody *ignore,btVector3 &hitPos,btVector3 &normal)
    {
        btCollisionWorld::AllHitsRayResultCallback ground(start,end);
        world->rayTest(start,end,ground);

        float closestDist = 100;
        int closestIdx = -1;

        for(int a = 0; a<ground.m_collisionObjects.size(); a++)
        {
            if(ground.m_collisionObjects[a] == ignore)
                continue;

            float dist = (ground.m_hitPointWorld[a]-start).length();
            if(dist < closestDist)
            {
                hitPos = ground.m_hitPointWorld[a];
                closestIdx = a;
                closestDist = dist;
                normal = ground.m_hitNormalWorld[a];
            }
        }

        if(closestIdx != -1)
            return (btRigidBody*)ground.m_collisionObjects[closestIdx];
        else
            return 0;
    }

    btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world,btVector3 &normal)
    {
        btCollisionWorld::AllHitsRayResultCallback ground(start,end);
        world->rayTest(start,end,ground);

        float closestDist = 100;
        int closestIdx = -1;

        for(int a = 0; a<ground.m_collisionObjects.size(); a++)
        {
            float dist = (ground.m_hitPointWorld[a]-start).length();
            if(dist < closestDist)
            {
                closestIdx = a;
                closestDist = dist;
                normal = ground.m_hitNormalWorld[a];
            }
        }

        if(closestIdx != -1)
            return (btRigidBody*)ground.m_collisionObjects[closestIdx];
        else
            return 0;
    }

    //Copied from server with some changes
    //Returns true if player has just jumped
    bool newDynamic::control(float yaw,bool forward,bool backward,bool left,bool right,bool jump,bool isJetting,bool allowTurning,bool relativeSpeed)
    {
        bool sideWays = false;
        btVector3 walkVelocity = btVector3(0,0,0);
        btVector3 walkVelocityDontTouch = btVector3(0,0,0);
        if(forward)
        {
            walkVelocity += btVector3(sin(yaw),0,cos(yaw));
            walkVelocityDontTouch += btVector3(sin(yaw),0,cos(yaw));
        }
        if(backward)
        {
            walkVelocity += btVector3(sin(yaw+3.1415),0,cos(yaw+3.1415));
            walkVelocityDontTouch += btVector3(sin(yaw+3.1415),0,cos(yaw+3.1415));
        }
        if(left)
        {
            sideWays = true;
            walkVelocity += btVector3(sin(yaw-1.57),0,cos(yaw-1.57));
            walkVelocityDontTouch += btVector3(sin(yaw-1.57),0,cos(yaw-1.57));
        }
        if(right)
        {
            sideWays = true;
            walkVelocity += btVector3(sin(yaw+1.57),0,cos(yaw+1.57));
            walkVelocityDontTouch += btVector3(sin(yaw+1.57),0,cos(yaw+1.57));
        }

        if(isJetting)
        {
            body->setGravity(btVector3(0,20,0));
            //applyCentralForce(walkVelocity * 20);
        }
        else
        {
            btVector3 gravity = world->getGravity();
            body->setGravity(gravity);
        }

        float speed = 16.0; //Needs to be a bit faster than the server I've found
        float blendTime = 50;

        if(!type)
        {
            error("Tried to control (i.e. like a player) dynamic without a type (i.e. a vehicle)?");
            return false;
        }

        if(allowTurning)
        {
            btTransform t = body->getWorldTransform();
            t.setRotation(btQuaternion(3.1415 + yaw,0,0));
            body->setWorldTransform(t);
        }

        btTransform rotMatrix = body->getWorldTransform();
        rotMatrix.setOrigin(btVector3(0,0,0));
        btVector3 position = body->getWorldTransform().getOrigin();

        btVector3 normal;
        btRigidBody *ground = 0;
        for(int gx = -1; gx<=1; gx++)
        {
            for(int gz = -1; gz<=1; gz++)
            {
                btVector3 lateralOffsets = btVector3(finalHalfExtents.x() * gx, 0, finalHalfExtents.z() * gz);
                lateralOffsets = rotMatrix * lateralOffsets;

                ground = getClosestBody(position+btVector3(0,finalHalfExtents.y()*0.1,0),position - btVector3(lateralOffsets.x(),finalHalfExtents.y()*0.1,lateralOffsets.z()),world,normal);

                if(ground)
                    break;
            }

            if(ground)
                break;
        }

        int deltaMS = SDL_GetTicks() - lastPlayerControl;
        lastPlayerControl = SDL_GetTicks();

        if(jump && ground && body->getLinearVelocity().y() < 1)
        {
            btVector3 vel = body->getLinearVelocity();
            vel.setY(45);
            body->setLinearVelocity(vel);
            return true;
        }

        if(walkVelocity.length2() < 0.01)
        {
            walking = false;
            body->setFriction(1.0);
            return false;
        }

        if(!ground)
        {
            walkVelocity *= speed;
            blendTime = 600;
        }
        else
        {
            walking = true;
            body->setFriction(0.0);
            btVector3 halfWay = (normal+walkVelocity)/2.0;
            float dot = fabs(btDot(normal,walkVelocity));
            walkVelocity = halfWay * fabs(dot) + walkVelocity * (1.0-fabs(dot)); //1.0 = use half way, 0.0 = use normal vel
            if(dot > 0)
                walkVelocity.setY(-walkVelocity.getY());
            btVector3 oldVelNoY = walkVelocity.normalized();
            oldVelNoY.setY(0);
            oldVelNoY.normalized();
            walkVelocity = oldVelNoY * speed + btVector3(0,walkVelocity.getY() * speed,0);
        }

        float blendCoefficient = deltaMS / blendTime;
        btVector3 velocity = body->getLinearVelocity();
        btVector3 endVel = (1.0-blendCoefficient) * velocity + blendCoefficient * walkVelocity;
        endVel.setY(velocity.getY());

        if(isinf(endVel.x()) || isinf(endVel.y()) || isinf(endVel.z()) || isnanf(endVel.x()) || isnanf(endVel.y()) || isnanf(endVel.z()))
        {
            std::cout<<"Player walking bad result...\n";
            std::cout<<"Normal "; sayVec3(normal);
            std::cout<<"Walk vel "; sayVec3(walkVelocity);
            std::cout<<"End "; sayVec3(endVel);
            std::cout<<"\n";
        }
        else
            body->setLinearVelocity(endVel);

        if(!ground)
            return false;

        btVector3 dir = walkVelocityDontTouch.normalize();
        btVector3 perpDir = walkVelocityDontTouch.cross(btVector3(0,1,0));

        /*colors.clear();
        poses.clear();

        colors.push_back(btVector3(1,1,1));
        poses.push_back(position);*/

        float bodyX = finalHalfExtents.x();
        float bodyZ = finalHalfExtents.z();
        if(sideWays)
            std::swap(bodyX,bodyZ);

        btVector3 footPos;
        btVector3 hitpos;
        btRigidBody *step;
        for(float footUp = 0; footUp <= 1.2; footUp += 0.3)
        {
            for(int across = -1; across <= 1; across++)
            {
                footPos = position + (2 * dir * bodyZ + perpDir * across * bodyX);
                footPos += btVector3(0,footUp,0);
                /*colors.push_back(btVector3(0,0,0));
                poses.push_back(footPos);*/

                step = getClosestBody(position,footPos,world,body,hitpos,normal);
                if(step)
                {
                    if(btDot(normal,btVector3(0,1,0)) < 0.15)
                    {
                        //float dist = (hitpos-position).length();
                        if(hitpos.y() <= position.y())
                            step = 0;
                        else
                            break;
                    }
                    else
                        step = 0;
                }
            }
            if(step)
                break;
        }

        if(step)
        {
            btVector3 rayStart = hitpos + btVector3(0,finalHalfExtents.y()*2,0) + walkVelocityDontTouch.normalized() * 0.1;

            btVector3 newHitPos;
            btRigidBody *secondCheck = getClosestBody(rayStart,hitpos,world,body,newHitPos);

            if(secondCheck != step)
                return false;

            float stepUp = newHitPos.getY() - position.getY();
            //std::cout<<"Step up: "<<stepUp<<"\n";
            if(stepUp > 0 && stepUp < 1.75)
            {
                btTransform t = body->getWorldTransform();
                btVector3 pos = t.getOrigin();
                pos.setY(pos.getY()+stepUp+0.4);
                t.setOrigin(pos);
                body->setWorldTransform(t);
            }
        }


        return false;
    }


    item::item(btDynamicsWorld *_world,newModel *_type,glm::vec3 baseScale)
        : newDynamic(_type,baseScale)
        {}

    //useDir now means it's a model (player) that has client simulated physics
    void item::updateTransform(bool useDir,float yaw)
    {
        if(hidden)
            return;

        if(heldBy)
        {
            float actualPitch = -0.785 * (sin(pitch)+1.0);
            glm::vec4 offset = glm::vec4(itemType->handOffset.x,itemType->handOffset.y,itemType->handOffset.z,1);
            glm::mat4 rot;
            glm::vec3 playerPos;
            //YOUR item, on a client physics simulated player model
            if(useDir && heldBy->body)
            {
                rot = glm::toMat4(glm::quat(glm::vec3(0,yaw+3.1415,0)));
                btVector3 bulletPos = heldBy->body->getWorldTransform().getOrigin();
                playerPos.x = bulletPos.x();
                playerPos.y = bulletPos.y();
                playerPos.z = bulletPos.z();
            }
            else //everyone else's tools
            {
                rot = glm::toMat4(heldBy->modelInterpolator.getRotation());
                playerPos = heldBy->modelInterpolator.getPosition();
            }

            offset = rot * offset;
            useGlobalTransform = true;
            globalTransform =
                        glm::translate(playerPos + glm::vec3(offset.x,offset.y,offset.z)) *
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



















