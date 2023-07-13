#ifndef TERRAIN_H_INCLUDED
#define TERRAIN_H_INCLUDED

#include "code/physics/bulletIncludes.h"
#include "code/graphics/uniformsBasic.h"
#include "code/graphics/tessellation.h"

namespace syj
{
    struct terrain : tessellation
    {
        btRigidBody *body = 0;
        btTriangleMesh *mTriMesh = 0;
        btCollisionShape *shape = 0;
        terrain(std::string heightMapFilePath,btDynamicsWorld *world);
    };
}

#endif // TERRAIN_H_INCLUDED
