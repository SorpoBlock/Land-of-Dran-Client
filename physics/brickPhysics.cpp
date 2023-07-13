#include "brickPhysics.h"

namespace syj
{
    glm::mat4 makeRotateMatrix(glm::vec3 axis, float angle)
    {
        axis = normalize(axis);
        float s = sin(angle);
        float c = cos(angle);
        float oc = 1.0 - c;

        return glm::mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                    oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                    oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                    0.0,                                0.0,                                0.0,                                1.0);
    }

    /*
    float gVehicleSteering = 0.f;
    float steeringIncrement = 0.04f;
    float steeringClamp = 0.3f;
    float wheelRadius = 0.5f;
    float wheelWidth = 0.4f;
    float wheelFriction = 1000;  //BT_LARGE_FLOAT;
    float suspensionStiffness = 20.f;
    float suspensionDamping = 2.3f;
    float suspensionCompression = 4.4f;
    float rollInfluence = 0.1f;  //1.0f;
    btScalar suspensionRestLength(0.6);
    */

    void livingBrick::addWheels(btVector3 &halfExtents)
    {
        //The direction of the raycast, the btRaycastVehicle uses raycasts instead of simiulating the wheels with rigid bodies
        btVector3 wheelDirectionCS0(0, -1, 0);
        //The axis which the wheel rotates arround
        btVector3 wheelAxleCS(-1, 0, 0);

        btScalar suspensionRestLength(0.7);
        btScalar wheelWidth(0.4);
        btScalar wheelRadius(1.2);

        //The height where the wheels are connected to the chassis
        btScalar connectionHeight(0.5);

        //All the wheel configuration assumes the vehicle is centered at the origin and a right handed coordinate system is used
        btVector3 wheelConnectionPoint(halfExtents.x() + wheelRadius, connectionHeight - halfExtents.y(), halfExtents.z() - wheelWidth);

        //Adds the front wheels
        vehicle->addWheel(wheelConnectionPoint, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, true);

        vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, 1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, true);

        //Adds the rear wheels
        vehicle->addWheel(wheelConnectionPoint* btVector3(1, 1, -1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, false);

        vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, -1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, false);

        //Configures each wheel of our vehicle, setting its friction, damping compression, etc.
        //For more details on what each parameter does, refer to the docs
        for (int i = 0; i < vehicle->getNumWheels(); i++)
        {
            btWheelInfo& wheel = vehicle->getWheelInfo(i);
            wheel.m_suspensionStiffness = 100;
            wheel.m_wheelsDampingCompression = btScalar(0.3) * 2 * btSqrt(wheel.m_suspensionStiffness);//btScalar(0.8);
            wheel.m_wheelsDampingRelaxation = btScalar(0.5) * 2 * btSqrt(wheel.m_suspensionStiffness);//1;
            //Larger friction slips will result in better handling
            wheel.m_frictionSlip = btScalar(1.2);
            wheel.m_rollInfluence = 1;
        }
    }

    void livingBrick::compile(btDynamicsWorld *world)
    {
        float minX = 999999;
        float minY = 999999;
        float minZ = 999999;
        float maxX = -999999;
        float maxY = -999999;
        float maxZ = -999999;

        for(int a = 0; a<opaqueBasicBricks.size(); a++)
        {
            basicBrickRenderData *brick = opaqueBasicBricks[a];

            std::cout<<"\nBefore: "<<brick->dimensions.x<<","<<brick->dimensions.z<<"\n";
            std::cout<<"Quat: "<<brick->rotation.w<<","<<brick->rotation.x<<","<<brick->rotation.y<<","<<brick->rotation.z<<"\n";
            glm::vec4 dims = glm::vec4(brick->dimensions.x,brick->dimensions.y,brick->dimensions.z,0.0);
            dims = makeRotateMatrix(glm::vec3(brick->rotation.x,brick->rotation.y,brick->rotation.z),brick->rotation.w) * dims;
            std::cout<<"After: "<<dims.x<<","<<dims.z<<"\n";

            float x = dims.x / 2.0 + brick->position.x;
            float y = brick->dimensions.y / (2.0*2.5) + brick->position.y;
            float z = dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;

            x = -dims.x / 2.0 + brick->position.x;
            y = -brick->dimensions.y / (2.0*2.5) + brick->position.y;
            z = -dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;
        }

        for(int a = 0; a<transparentBasicBricks.size(); a++)
        {
            basicBrickRenderData *brick = transparentBasicBricks[a];

            /*std::cout<<"\nBefore: "<<brick->dimensions.x<<","<<brick->dimensions.z<<"\n";
            std::cout<<"Quat: "<<brick->rotation.w<<","<<brick->rotation.x<<","<<brick->rotation.y<<","<<brick->rotation.z<<"\n";*/
            glm::vec4 dims = glm::vec4(brick->dimensions.x,brick->dimensions.y,brick->dimensions.z,0.0);
            dims = makeRotateMatrix(glm::vec3(brick->rotation.x,brick->rotation.y,brick->rotation.z),brick->rotation.w) * dims;
            //std::cout<<"After: "<<dims.x<<","<<dims.z<<"\n";

            float x = dims.x / 2.0 + brick->position.x;
            float y = brick->dimensions.y / (2.0*2.5) + brick->position.y;
            float z = dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;

            x = -dims.x / 2.0 + brick->position.x;
            y = -brick->dimensions.y / (2.0*2.5) + brick->position.y;
            z = -dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;
        }

        for(int a = 0; a<opaqueSpecialBricks.size(); a++)
        {
            specialBrickRenderData *brick = opaqueSpecialBricks[a];
            glm::vec4 dims = glm::vec4(brick->type->type->width,brick->type->type->height,brick->type->type->length,0.0);
            dims = makeRotateMatrix(glm::vec3(brick->rotation.x,brick->rotation.y,brick->rotation.z),brick->rotation.w) * dims;

            float x = dims.x / 2.0 + brick->position.x;
            float y = dims.y / (2.0*2.5) + brick->position.y;
            float z = dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;

            x = -dims.x / 2.0 + brick->position.x;
            y = -dims.y / (2.0*2.5) + brick->position.y;
            z = -dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;
        }

        for(int a = 0; a<transparentSpecialBricks.size(); a++)
        {
            specialBrickRenderData *brick = transparentSpecialBricks[a];
            glm::vec4 dims = glm::vec4(brick->type->type->width,brick->type->type->height,brick->type->type->length,0.0);
            dims = makeRotateMatrix(glm::vec3(brick->rotation.x,brick->rotation.y,brick->rotation.z),brick->rotation.w) * dims;

            float x = dims.x / 2.0 + brick->position.x;
            float y = dims.y / (2.0*2.5) + brick->position.y;
            float z = dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;

            x = -dims.x / 2.0 + brick->position.x;
            y = -dims.y / (2.0*2.5) + brick->position.y;
            z = -dims.z / 2.0 + brick->position.z;

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;
        }

        float sizeX = maxX - minX;
        float sizeY = maxY - minY;
        float sizeZ = maxZ - minZ;

        float midX = minX + sizeX / 2.0;
        float midY = minY + sizeY / 2.0;
        float midZ = minZ + sizeZ / 2.0;

        std::cout<<"Bricks, opaque basic: "<<opaqueBasicBricks.size()<<" Transparent basic: "<<transparentBasicBricks.size()<<"\n";
        std::cout<<"Bricks, opaque special: "<<opaqueSpecialBricks.size()<<" Transparent special: "<<transparentSpecialBricks.size()<<"\n";
        std::cout<<"Size: "<<sizeX<<","<<sizeY<<","<<sizeZ<<"\n";
        std::cout<<"Center: "<<midX<<","<<midY<<","<<midZ<<"\n";

        for(int a = 0; a<opaqueBasicBricks.size(); a++)
        {
            basicBrickRenderData *brick = opaqueBasicBricks[a];
            brick->position -= glm::vec3(midX,midY,midZ);
        }

        for(int a = 0; a<transparentBasicBricks.size(); a++)
        {
            basicBrickRenderData *brick = transparentBasicBricks[a];
            brick->position -= glm::vec3(midX,midY,midZ);
        }

        for(int a = 0; a<opaqueSpecialBricks.size(); a++)
            opaqueSpecialBricks[a]->position -= glm::vec3(midX,midY,midZ);

        for(int a = 0; a<transparentSpecialBricks.size(); a++)
            transparentSpecialBricks[a]->position -= glm::vec3(midX,midY,midZ);

        recompileEverything();

        btVector3 halfExtents = btVector3(sizeX/2.0,sizeY/2.0,sizeZ/2.0);
        btBoxShape *tmp = new btBoxShape(halfExtents);

        btVector3 inertia;
        float mass = 5;//325;
        tmp->calculateLocalInertia(mass,inertia);

        btTransform startTrans = btTransform::getIdentity();
        startTrans.setOrigin(btVector3(midX,midY,midZ));
        btMotionState *ms = new btDefaultMotionState(startTrans);

/*        body = new btRigidBody(mass,ms,tmp,inertia);
        world->addRigidBody(body);
        body->setUserIndex(userIndex_livingBrick);
        body->setFriction(0.9);
        body->setLinearVelocity(btVector3(0,20,0));

        compiled = true;

        vehicleRayCaster = new btDefaultVehicleRaycaster(world);
        vehicle = new btRaycastVehicle(tuning,body,vehicleRayCaster);
        vehicle->setCoordinateSystem(0,1,2);
        body->setActivationState(DISABLE_DEACTIVATION);
        world->addVehicle(vehicle);
        addWheels(halfExtents);*/
    }

    glm::mat4 btScalar2mat4(btScalar* matrix) {
    return glm::mat4(
        matrix[0], matrix[1], matrix[2], matrix[3],
        matrix[4], matrix[5], matrix[6], matrix[7],
        matrix[8], matrix[9], matrix[10], matrix[11],
        matrix[12], matrix[13], matrix[14], matrix[15]);
    }

    void livingBrick::renderWheels(uniformsHolder *unis,model *wheelModel)
    {
        glDisable(GL_CULL_FACE);
        for(int a = 0; a<4; a++)
        {
            /*btTransform t = vehicle->getWheelInfo(a).m_worldTransform;

            btVector3 o = t.getOrigin();
            btQuaternion q = t.getRotation();

            glm::mat4 translate = glm::translate(BtToGlm(o));
            glm::mat4 rotate = glm::toMat4(glm::quat(q.w(),q.x(),q.y(),q.z()));
            glm::mat4 modelMatrix = translate * rotate * glm::scale(glm::vec3(0.02));*/

            vehicle->updateWheelTransform(a,false);

            btScalar tmp[16];
            vehicle->getWheelInfo(a).m_worldTransform.getOpenGLMatrix(tmp);
            glm::mat4 myMat = btScalar2mat4(tmp);

            wheelModel->render(unis,myMat * glm::scale(glm::vec3(0.06)));
        }
        glEnable(GL_CULL_FACE);
    }

    void livingBrick::renderAlive(uniformsHolder &unis,bool skipMats,float deltaT,bool disableInterpolation)
    {
        if(!compiled)
            return;

        glUniform1i(unis.target->getUniformLocation("livingBricks"),true);

/*        btTransform t = body->getWorldTransform();
        btVector3 o = t.getOrigin();
        btQuaternion q = t.getRotation();*/

        glm::mat4 translate;
        if(disableInterpolation)
            translate = glm::translate(carTransform.keyFrames[0].position);
        else
            translate = glm::translate(carTransform.getPosition());
        glm::mat4 rotate = glm::toMat4(carTransform.getRotation());
        glm::mat4 modelMatrix = translate * rotate;

        unis.setModelMatrix(modelMatrix);

        renderEverything(unis,skipMats,0,deltaT);
        glUniform1i(unis.target->getUniformLocation("livingBricks"),false);
    }

    void livingBrick::renderTransparentAlive(uniformsHolder &unis,bool skipMats,float deltaT)
    {
        if(!compiled)
            return;

        glUniform1i(unis.target->getUniformLocation("livingBricks"),true);

        /*btTransform t = body->getWorldTransform();
        btVector3 o = t.getOrigin();
        btQuaternion q = t.getRotation();*/

        glm::mat4 translate = glm::translate(carTransform.getPosition());
        glm::mat4 rotate = glm::toMat4(carTransform.getRotation());
        glm::mat4 modelMatrix = translate * rotate;

        //std::cout<<"Position: "; sayVec3(o); std::cout<<"\n";

        unis.setModelMatrix(modelMatrix);

        renderTransparent(unis,skipMats,deltaT);
        glUniform1i(unis.target->getUniformLocation("livingBricks"),false);
    }
}







