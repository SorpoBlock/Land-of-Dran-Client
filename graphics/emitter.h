#ifndef EMITTER_H_INCLUDED
#define EMITTER_H_INCLUDED

#include "code/graphics/uniformsBasic.h"
#include "code/graphics/texture.h"
#include <algorithm>
#include <glm/gtx/norm.hpp>
#include "code/graphics/newModel.h"
#include "code/physics/player.h"
#include "code/graphics/newBrickType.h"
#include "code/utility/debugGraphics.h"
#include "code/physics/brickPhysics.h"

#define particleAllocSize 500

namespace syj
{
    enum particleLayout
    {
        vertexBuffer       = 0,
        startingPosAndTime = 1,
        startingVel        = 2
    };

    struct particleType;

    struct particle
    {
        particleType *type = 0;
        glm::vec3 startingPos;
        glm::vec3 startingVel;
        float creationTimeMS;
        float lifetimeRandomness;
    };

    struct particleType
    {
        unsigned int serverID = 0;

        glm::vec3 dragCoefficient = glm::vec3(0.1,0.1,0.1);
        glm::vec3 gravityCoefficient = glm::vec3(0,0.705,0);
        glm::vec3 inheritedVelFactor = glm::vec3(0,0,0);
        float lifetimeMS = 1100;
        int lifetimeVarianceMS = 000;
        texture *baseTexture = 0;
        float spinSpeed = 1000;
        float sizes[4] = {0,0.99,0.59,1};
        glm::vec4 colors[4] = { glm::vec4(1,1,0.26,0) , glm::vec4(1,1,0.26,1) , glm::vec4(0.6,0,0,0) , glm::vec4(0.6,0,0,0) };
        float times[4] = {0,0.2,1,1};
        bool useInvAlpha = false;
        bool needSorting = false;

        GLuint vao;
        GLuint buffers[3];
        unsigned int allocatedParticleSpace = 0;
        int emptySlots = 0;
        unsigned int lastCheckTime = 0;

        particleType();
        ~particleType();
        void recompile();
        void scaleToBlockland();
        void addParticle(glm::vec3 startingPos,glm::vec3 startingVelocity);
        void render(uniformsHolder &emitterUnis);
        void deadParticlesCheck(glm::vec3 cameraPos = glm::vec3(0,0,0),float deltaT = 0);

        std::vector<particle*> particles;
    };

    struct emitterType
    {
        unsigned int serverID = 0;
        std::string uiName = "";

        float lifetimeMS = 0;
        int ejectionPeriodMS;
        int periodVarianceMS;
        float ejectionVelocity;
        float velocityVariance;
        float ejectionOffset;
        float ejectionOffsetRandomMin = 0;
        float ejectionOffsetRandomMax = 0;
        float thetaMin;
        float thetaMax;
        float phiReferenceVel;
        float phiVariance;
        /*bool useEmitterColors;
        bool orientParticles;*/
        std::vector<particleType*> particleTypes;
    };

    struct emitter
    {
        unsigned int serverID = 0;
        emitterType *type = 0;

        glm::vec3 color = glm::vec3(1,1,1);
        bool shootFromHand = false;
        glm::vec3 position;
        glm::vec3 attachedVelocity = glm::vec3(0,0,0);
        glm::vec3 emitterRange = glm::vec3(0,0,0);
        std::string meshName = "";
        newDynamic *attachedToModel = 0;
        item *attachedToItem = 0;
        basicBrickRenderData *attachedToBasicBrick = 0;
        specialBrickRenderData *attachedToSpecialBrick = 0;
        livingBrick *attachedToCar = 0;
        int whichWheel = 0;
        bool justAttached = true;
        bool enabled = true;

        int lastEmission = 0;
        int realLastEmission = 0;
        float currentPhi = 0;
        unsigned int creationTime = 0;

        void update(glm::vec3 eyePos,glm::vec3 eyeDir,bool useBodyPos = false,bool firstPerson = false,float cameraYaw=0,float cameraPitch=0);
    };
}

#endif // EMITTER_H_INCLUDED
