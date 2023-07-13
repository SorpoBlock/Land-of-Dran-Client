#ifndef SELECTIONBOX_H_INCLUDED
#define SELECTIONBOX_H_INCLUDED

#include "code/physics/bulletIncludes.h"
#include "code/utility/debugGraphics.h"

namespace syj
{
    struct selectionBox
    {
        enum selectionPhase
        {
            idle = 0,
            waitingForClick = 1,
            selecting = 2,
            stretching = 3,
            confirming = 4
        };

        enum selectionSide
        {
            left = 0,
            right = 1,
            down = 2,
            up = 3,
            forward = 4,
            backward = 5,
            none = 6
        };

        int draggedBox = selectionSide::none;
        int currentPhase = selectionPhase::idle;
        GLuint cubeVAO;

        btVector3 minExtents = btVector3(0,0,0);
        btVector3 maxExtents = btVector3(0,0,0);

        btDynamicsWorld *world = 0;

        btMotionState *ms = 0;
        btBoxShape *pullShape = 0;
        btRigidBody *leftPull = 0;
        btRigidBody *rightPull = 0;
        btRigidBody *downPull = 0;
        btRigidBody *upPull = 0;
        btRigidBody *forwardPull = 0;
        btRigidBody *backwardPull = 0;

        void performRaycast(glm::vec3 start,glm::vec3 dir,btRigidBody *ignore = 0);
        void render(uniformsHolder &unis);
        void drag(glm::vec3 position,glm::vec3 direction);
        void movePulls();

        selectionBox(btDynamicsWorld *_world,GLuint &_cubeVAO);
    };
}

#endif // SELECTIONBOX_H_INCLUDED
