#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "code/physics/bulletIncludes.h"
#include "code/graphics/uniformsBasic.h"
#include "code/graphics/newModel.h"
#include "code/utility/debugGraphics.h"

extern glm::vec3 debugLocations[10];

namespace syj
{
    /*struct dynamic : animatedInstance
    {
        float cameraFollowDistance = 0.0;

        int serverID = -1;
        btDynamicsWorld *world = 0;
        btRigidBody *body = 0;
        btVector3 min,max;
        int lastWalked = 0;

        glm::vec3 colorTint = glm::vec3(1,1,1);

        btVector3 getPosition();
        btVector3 getLinearVelocity();
        void setPosition(glm::vec3 pos);
        void setPosition(btVector3 pos);
        void setRotation(glm::quat rot);
        void setRotation(btQuaternion quat);
        void smoothLinearVelocity(btVector3 vel,float deltaMS);
        void setLinearVelocity(glm::vec3 vel);
        void setLinearVelocity(btVector3 vel);

        void control(float deltaMS,glm::vec3 direction,bool forward,bool backward,bool left,bool right,bool jump);

        void render(uniformsHolder *unis,float deltaMS = 0,bool stopInterpolation = false,bool skipMats = false);

        dynamic(btDynamicsWorld *_world,animatedModel *_type,glm::vec3 baseScale = glm::vec3(1,1,1));
    };*/

    struct heldItemType
    {
        newModel *type = 0;
        newAnimation *switchAnim = 0;
        newAnimation *fireAnim = 0;
        glm::quat handRot = glm::quat(1.0,0.0,0.0,0.0);
        glm::vec3 handOffset = glm::vec3(0,0,0);
        std::string uiName = "";
        unsigned int serverID = 0;
        bool useDefaultSwing = true;

        bool waitingForModel = false;
        int waitingModel = -1;
        int waitingForSwitchAnim = -1;
        int waitingForSwingAnim = -1;
    };

    struct item : newDynamic
    {
        glm::vec3 bulletTrailColor = glm::vec3(1,0.5,0);
        float bulletTrailSpeed = 1.0;
        bool useBulletTrail = false;

        int nextFireAnim = -1;
        float nextFireAnimSpeed = 1.0;
        int nextFireSound = -1;
        float nextFireSoundPitch = 1.0;
        float nextFireSoundGain = 1.0;
        int nextFireEmitter = -1;
        std::string nextFireEmitterMesh = "";
        int fireCooldownMS = 0;
        int lastFire = 0;

        heldItemType *itemType = 0;
        newDynamic *heldBy = 0;
        float pitch = 0;
        bool swinging = false;
        item(btDynamicsWorld *_world,newModel *_type,glm::vec3 baseScale = glm::vec3(1,1,1));
        void updateTransform(bool useDir = false,float yaw = 0,float camPitch = 0);
        void advance(bool mouseIsDown,float deltaMS);
    };
}

#endif // PLAYER_H_INCLUDED
