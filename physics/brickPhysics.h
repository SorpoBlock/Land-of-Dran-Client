#ifndef BRICKPHYSICS_H_INCLUDED
#define BRICKPHYSICS_H_INCLUDED

#include "code/physics/bulletIncludes.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include "code/graphics/newBrickType.h"
#include "code/utility/debugGraphics.h"
#include "code/graphics/newModel.h"
#include "code/networking/interpolator.h"

namespace syj
{
    struct AllButMeAabbResultCallback : public btBroadphaseAabbCallback
    {
        AllButMeAabbResultCallback(btRigidBody* me) : me(me)
        {

        }

        virtual bool process(const btBroadphaseProxy* proxy)
        {
            btCollisionObject* collisionObject = static_cast<btCollisionObject*>(proxy->m_clientObject);
            if (collisionObject == this->me)
            {
                return true;
            }
            results.push_back(collisionObject);
            return true;
        }

        btRigidBody* me;
        std::vector<btCollisionObject*> results;
    };

    struct extraWheelData
    {
        int typeID = 0;
        float wheelScale = 1.0;

        float carX,carY,carZ;

        float breakForce = 400;
        float steerAngle = 0.5;
        float engineForce = 100;
        float suspensionLength = 0.7;
        float suspensionStiffness = 100;
        float dampingCompression = 6;
        float dampingRelaxation = 10;
        float frictionSlip = 1.2;
        float rollInfluence = 1.0;

        void *dirtEmitter = 0;
    };

    struct livingBrick : newBrickRenderer
    {
        float lastSpeed = 0;
        int serverID = -1;
        btVehicleRaycaster* vehicleRayCaster = 0;
        btRaycastVehicle *vehicle = 0;
        btRaycastVehicle::btVehicleTuning tuning;
        interpolator carTransform;

        std::vector<extraWheelData> wheelBrickData;
        std::vector<newDynamic*> newWheels;
        //std::vector<interpolator*> wheels;

        //btRigidBody *body = 0;
        bool compiled = false;

        //void renderWheels(uniformsHolder *unis,model *wheelModel);
        void addWheels(btVector3 &halfExtents);
        void compile();
        void renderAlive(uniformsHolder *unis,bool skipMats = false,float deltaT = 0,bool disableInterpolation = false);
        void renderTransparentAlive(uniformsHolder *unis,bool skipMats = false,float deltaT = 0);
    };
}

#endif // BRICKPHYSICS_H_INCLUDED
