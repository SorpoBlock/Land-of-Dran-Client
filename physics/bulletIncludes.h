#ifndef BULLETINCLUDES_H_INCLUDED
#define BULLETINCLUDES_H_INCLUDED

//I'd use a preprocessor directive, but codeblocks IDE doesn't correctly lint them in downstream files
const bool useClientPhysics = true;

#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#define userIndex_livingBrick 77
#define userIndex_staticNormalBrick 88
#define userIndex_staticSpecialBrick 99
//TODO: Why the hell are these different from the server
#define userIndex_dynamic 109

#endif // BULLETINCLUDES_H_INCLUDED
