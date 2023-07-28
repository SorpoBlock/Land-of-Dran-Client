#include "selectionBox.h"

namespace syj
{
    selectionBox::selectionBox(btDynamicsWorld *_world,GLuint &_cubeVAO)
    {
        cubeVAO = _cubeVAO;
        world = _world;
        ms = new btDefaultMotionState(btTransform::getIdentity());
        pullShape = new btBoxShape(btVector3(0.5,0.5,0.5));

        float mass = 1.0; //Don't do 0.0 because AABB will not be recalculated for static shape
        leftPull = new btRigidBody(mass,ms,pullShape);
        rightPull = new btRigidBody(mass,ms,pullShape);
        downPull = new btRigidBody(mass,ms,pullShape);
        upPull = new btRigidBody(mass,ms,pullShape);
        forwardPull = new btRigidBody(mass,ms,pullShape);
        backwardPull = new btRigidBody(mass,ms,pullShape);

        leftPull->setUserIndex(100);
        rightPull->setUserIndex(101);
        downPull->setUserIndex(102);
        upPull->setUserIndex(103);
        forwardPull->setUserIndex(104);
        backwardPull->setUserIndex(105);

        leftPull->setGravity(btVector3(0,0,0));
        rightPull->setGravity(btVector3(0,0,0));
        downPull->setGravity(btVector3(0,0,0));
        upPull->setGravity(btVector3(0,0,0));
        forwardPull->setGravity(btVector3(0,0,0));
        backwardPull->setGravity(btVector3(0,0,0));

        world->addRigidBody(leftPull);
        world->addRigidBody(rightPull);
        world->addRigidBody(upPull);
        world->addRigidBody(downPull);
        world->addRigidBody(forwardPull);
        world->addRigidBody(backwardPull);

        leftPull->setActivationState(DISABLE_SIMULATION);
        rightPull->setActivationState(DISABLE_SIMULATION);
        upPull->setActivationState(DISABLE_SIMULATION);
        downPull->setActivationState(DISABLE_SIMULATION);
        forwardPull->setActivationState(DISABLE_SIMULATION);
        backwardPull->setActivationState(DISABLE_SIMULATION);
    }

            /*glm::vec3 up = middle + glm::vec3(0,1.0 + halfs.y,0);
            glm::vec3 planeNormal = glm::normalize(up - position);
            float length = 0;
            if(intersectPlane(planeNormal,up,position,direction,length))*/
    bool intersectPlane(const glm::vec3 &n, const glm::vec3 &p0, const glm::vec3 &l0, const glm::vec3 &l, float &t)
    {
        float denom = glm::dot(n, l);
        if (denom > 1e-6)
        {
            glm::vec3 p0l0 = p0 - l0;
            t = glm::dot(p0l0, n) / denom;
            return (t >= 0);
        }

        return false;
    }

    void selectionBox::drag(glm::vec3 position,glm::vec3 direction)
    {
        if(currentPhase != selectionPhase::stretching)
            return;

        if(draggedBox == selectionSide::none)
            return;

        glm::vec3 halfs = BtToGlm((maxExtents - minExtents) / 2.0);
        glm::vec3 middle = BtToGlm(minExtents + glmToBt(halfs));

        /*
        btVector3 left = middle - btVector3(1.0 + halfs.x(),0,0);
        btVector3 right = middle + btVector3(1.0 + halfs.x(),0,0);
        btVector3 down = middle - btVector3(0,1.0 + halfs.y(),0);
        btVector3 up = middle + btVector3(0,1.0 + halfs.y(),0);
        btVector3 forward = middle + btVector3(0,0,1.0 + halfs.z());
        btVector3 backward = middle - btVector3(0,0,1.0 + halfs.z());
        */

        if(draggedBox == selectionSide::up)
        {
            glm::vec3 up = middle + glm::vec3(0,1.0 + halfs.y,0); //position of dragging box
            glm::vec3 planeNormal = glm::normalize(up - position);
            float length = 0;
            if(intersectPlane(planeNormal,up,position,direction,length))
            {
                glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                if(hitPos.y > minExtents.getY())
                    maxExtents.setY(hitPos.y);
            }
        }
        else if(draggedBox == selectionSide::down)
        {
            glm::vec3 down = middle - glm::vec3(0,1.0 + halfs.y,0);
            glm::vec3 planeNormal = glm::normalize(down - position);
            float length = 0;
            if(intersectPlane(planeNormal,down,position,direction,length))
            {
                glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                if(hitPos.y < maxExtents.getY())
                    minExtents.setY(hitPos.y);
            }
        }
        else if(draggedBox == selectionSide::forward)
        {
            glm::vec3 forward = middle + glm::vec3(0,0,1.0 + halfs.z);
            glm::vec3 planeNormal = glm::vec3(0,-1,0);

            float length = 0;
            if(intersectPlane(planeNormal,forward,position,direction,length))
            {
                glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                if(hitPos.z > minExtents.getZ())
                    maxExtents.setZ(hitPos.z);
            }
            else
            {
                planeNormal = glm::vec3(0,1,0);
                length = 0;
                if(intersectPlane(planeNormal,forward,position,direction,length))
                {
                    glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                    if(hitPos.z > minExtents.getZ())
                        maxExtents.setZ(hitPos.z);
                }
            }
        }
        else if(draggedBox == selectionSide::backward)
        {
            glm::vec3 backward = middle - glm::vec3(0,0,1.0 + halfs.z);
            glm::vec3 planeNormal = glm::vec3(0,-1,0);

            float length = 0;
            if(intersectPlane(planeNormal,backward,position,direction,length))
            {
                glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                if(hitPos.z < maxExtents.getZ())
                    minExtents.setZ(hitPos.z);
            }
            else
            {
                planeNormal = glm::vec3(0,1,0);
                length = 0;
                if(intersectPlane(planeNormal,backward,position,direction,length))
                {
                    glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                    if(hitPos.z < maxExtents.getZ())
                        minExtents.setZ(hitPos.z);
                }
            }
        }
        else if(draggedBox == selectionSide::left)
        {
            glm::vec3 left = middle - glm::vec3(1.0 + halfs.x,0,0);
            glm::vec3 planeNormal = glm::vec3(0,-1,0);

            float length = 0;
            if(intersectPlane(planeNormal,left,position,direction,length))
            {
                glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                if(hitPos.x < maxExtents.getX())
                    minExtents.setX(hitPos.x);
            }
            else
            {
                length = 0;
                planeNormal = glm::vec3(0,1,0);
                if(intersectPlane(planeNormal,left,position,direction,length))
                {
                    glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                    if(hitPos.x < maxExtents.getX())
                        minExtents.setX(hitPos.x);
                }
            }
        }
        else if(draggedBox == selectionSide::right)
        {
            glm::vec3 right = middle + glm::vec3(1.0 + halfs.x,0,0);
            glm::vec3 planeNormal = glm::vec3(0,-1,0);

            float length = 0;
            if(intersectPlane(planeNormal,right,position,direction,length))
            {
                glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                if(hitPos.x > minExtents.getX())
                    maxExtents.setX(hitPos.x);
            }
            else
            {
                planeNormal = glm::vec3(0,1,0);

                length = 0;
                if(intersectPlane(planeNormal,right,position,direction,length))
                {
                    glm::vec3 hitPos = position + direction * glm::vec3(length,length,length);
                    if(hitPos.x > minExtents.getX())
                        maxExtents.setX(hitPos.x);
                }
            }
        }

        movePulls();
    }

    void selectionBox::performRaycast(glm::vec3 start,glm::vec3 dir,btRigidBody *ignore)
    {
        if(currentPhase != selectionPhase::selecting)
            return;

        btVector3 raystart = glmToBt(start);
        btVector3 rayend = glmToBt(start + dir * glm::vec3(30.0));
        btCollisionWorld::AllHitsRayResultCallback res(raystart,rayend);
        res.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
        res.m_collisionFilterMask = btBroadphaseProxy::AllFilter;
        world->rayTest(raystart,rayend,res);

        int idx = -1;
        float dist = 9999999;
        btRigidBody *body = 0;

        for(int a = 0; a<res.m_collisionObjects.size(); a++)
        {
            body = (btRigidBody*)res.m_collisionObjects[a];
            if(body == ignore)
                continue;
            if(body->getUserIndex() < 100 || body->getUserIndex() > 105)
                continue;

            if(fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-start)) < dist)
            {
                dist = fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-start));
                idx = a;
            }
        }

        if(idx == -1)
        {
            draggedBox = selectionSide::none;
            return;
        }

        body = (btRigidBody*)res.m_collisionObjects[idx];

        if(!body)
        {
            draggedBox = selectionSide::none;
            return;
        }

        if(body->getUserIndex() == 100)
            draggedBox = selectionSide::left;
        else if(body->getUserIndex() == 101)
            draggedBox = selectionSide::right;
        else if(body->getUserIndex() == 102)
            draggedBox = selectionSide::down;
        else if(body->getUserIndex() == 103)
            draggedBox = selectionSide::up;
        else if(body->getUserIndex() == 104)
            draggedBox = selectionSide::forward;
        else if(body->getUserIndex() == 105)
            draggedBox = selectionSide::backward;
        else
            draggedBox = selectionSide::none;
    }

    void updateAABB(btRigidBody *rigidBody,btDynamicsWorld *world)
    {
        btVector3 aabbMin, aabbMax;
        rigidBody->getCollisionShape()->getAabb(rigidBody->getWorldTransform(), aabbMin, aabbMax);

        btDbvtBroadphase* broadphase = (btDbvtBroadphase*)world->getBroadphase();
        if (broadphase)
            broadphase->setAabb(rigidBody->getBroadphaseHandle(), aabbMin, aabbMax, world->getDispatcher());
    }

    void selectionBox::movePulls()
    {
        btVector3 halfs = (maxExtents - minExtents) / 2.0;
        btVector3 middle = minExtents + halfs;

        btVector3 left = middle - btVector3(1.0 + halfs.x(),0,0);
        btVector3 right = middle + btVector3(1.0 + halfs.x(),0,0);
        btVector3 down = middle - btVector3(0,1.0 + halfs.y(),0);
        btVector3 up = middle + btVector3(0,1.0 + halfs.y(),0);
        btVector3 forward = middle + btVector3(0,0,1.0 + halfs.z());
        btVector3 backward = middle - btVector3(0,0,1.0 + halfs.z());

        btTransform t = btTransform::getIdentity();

        t.setOrigin(left);
        leftPull->setWorldTransform(t);
        updateAABB(leftPull,world);

        t.setOrigin(right);
        rightPull->setWorldTransform(t);
        updateAABB(rightPull,world);

        t.setOrigin(down);
        downPull->setWorldTransform(t);
        updateAABB(downPull,world);

        t.setOrigin(up);
        upPull->setWorldTransform(t);
        updateAABB(upPull,world);

        t.setOrigin(backward);
        backwardPull->setWorldTransform(t);
        updateAABB(backwardPull,world);

        t.setOrigin(forward);
        forwardPull->setWorldTransform(t);
        updateAABB(forwardPull,world);
    }

    void selectionBox::render(uniformsHolder &unis)
    {
        if(!(currentPhase == selecting || currentPhase == stretching))
            return;

        movePulls();

        glBindVertexArray(cubeVAO);
        btVector3 half = btVector3(0.5,0.5,0.5);
        glm::vec4 selectedNubColor = glm::vec4(0.8,0.8,1.0,1.0);
        glm::vec4 idleNubColor = glm::vec4(0.2,0.2,1.0,1.0);

        btVector3 pos = leftPull->getWorldTransform().getOrigin();
        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(pos - half));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(pos + half));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),draggedBox == left ? selectedNubColor : idleNubColor);
        glDrawArrays(GL_TRIANGLES,0,36);

        pos = rightPull->getWorldTransform().getOrigin();
        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(pos - half));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(pos + half));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),draggedBox == right ? selectedNubColor : idleNubColor);
        glDrawArrays(GL_TRIANGLES,0,36);

        pos = downPull->getWorldTransform().getOrigin();
        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(pos - half));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(pos + half));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),draggedBox == down ? selectedNubColor : idleNubColor);
        glDrawArrays(GL_TRIANGLES,0,36);

        pos = upPull->getWorldTransform().getOrigin();
        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(pos - half));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(pos + half));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),draggedBox == up ? selectedNubColor : idleNubColor);
        glDrawArrays(GL_TRIANGLES,0,36);

        pos = forwardPull->getWorldTransform().getOrigin();
        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(pos - half));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(pos + half));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),draggedBox == forward ? selectedNubColor : idleNubColor);
        glDrawArrays(GL_TRIANGLES,0,36);

        pos = backwardPull->getWorldTransform().getOrigin();
        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(pos - half));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(pos + half));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),draggedBox == backward ? selectedNubColor : idleNubColor);
        glDrawArrays(GL_TRIANGLES,0,36);

        glUniform3vec(unis.target->getUniformLocation("start"),BtToGlm(minExtents));
        glUniform3vec(unis.target->getUniformLocation("end"),BtToGlm(maxExtents));
        glUniform4vec(unis.target->getUniformLocation("boxColor"),glm::vec4(1,0.3,0.3,0.5));
        glDrawArrays(GL_TRIANGLES,0,36);

        glBindVertexArray(0);
    }
}





















