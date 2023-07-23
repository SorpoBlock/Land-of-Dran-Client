#ifndef INTERPOLATOR_H_INCLUDED
#define INTERPOLATOR_H_INCLUDED

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include "code/networking/packet.h"
#include <chrono>
#include <utility>

using namespace std::chrono;

namespace syj
{
    unsigned int getServerTime();

    struct modelTransform
    {
        unsigned int packetTime = 0;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 velocity;
    };

    struct interpolator
    {
        static bool useTripleInterpolation;
        static unsigned int clientTimePoint;
        static unsigned int serverTimePoint;

        public:
        glm::vec3 lastPositionReturned = glm::vec3(0,0,0);
        glm::quat lastQuatReturned = glm::quat(1,0,0,0);
        float progressToNextFrame = 0;
        std::vector<modelTransform> keyFrames;
        unsigned int highestProcessed = 0;
        glm::vec3 scale = glm::vec3(1,1,1);

        public:
        void addTransform(unsigned int packetTime,glm::vec3 pos,glm::quat rotation,glm::vec3 vel = glm::vec3(0,0,0));
        //void addTransform(packet *data,int id,float ms);
        void advance(float deltaMS);

        void getFrames(std::vector<modelTransform> &p,float &progress);
        glm::vec3 getPosition();
        glm::quat getRotation();
        glm::vec3 guessVelocity();
    };
}

#endif // INTERPOLATOR_H_INCLUDED
