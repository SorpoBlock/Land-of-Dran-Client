#ifndef QUATERNION_H_INCLUDED
#define QUATERNION_H_INCLUDED

#include "code/networking/packet.h"
#ifndef __linux__ 
    #define BT_USE_DOUBLE_PRECISION
#endif
#include <btBulletDynamicsCommon.h>
#include "code/utility/logger.h"

using namespace syj;

unsigned char greatestOfQuat(btQuaternion quat);
btQuaternion readQuat(packet *data);
void writeQuat(btQuaternion quat,packet *toWrite);

#endif // QUATERNION_H_INCLUDED
