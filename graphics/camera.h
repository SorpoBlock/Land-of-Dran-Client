#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "code/graphics/uniformsBasic.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <math.h>

namespace syj
{

    class camera
    {
        public:
        //protected:
            glm::mat4 viewMatrix = glm::mat4(1.0);
            glm::mat4 angleMatrix = glm::mat4(1.0);
            glm::mat4 projectionMatrix = glm::mat4(1.0);
            glm::vec3 position = glm::vec3(0,0,0);
            glm::vec3 direction = glm::vec3(0,0,1);
            glm::vec3 nominalUp = glm::vec3(0,1,0);

        public:
            float debugPitch=0,debugYaw=0;
            std::string name = "";

            glm::mat4 getProjectionMatrix(){return projectionMatrix;}
            glm::mat4 getViewMatrix(){return viewMatrix;}

            virtual void walkForward(float amount) = 0;
            virtual void walkRight(float amount) = 0;
            virtual void setDirection(glm::vec3 newDir) = 0;
            glm::vec3 getDirection();
            virtual void setPosition(glm::vec3 newPos) = 0;
            virtual glm::vec3 getPosition() = 0;
            virtual void render(uniformsHolder *holder) = 0;
            float getYaw(){return atan2(direction.x,direction.z);}
            float getPitch(){return asin(direction.y);}

    };

    class perspectiveCamera : public camera
    {
        private:
            float fieldOfVision = 90.0;
            float nearPlane = 0.5;
            float farPlane = 1000.0;
            float aspectRatio = 1.0;

        public:
            float getAspectRatio(){return aspectRatio;}
            float getFieldOfVision(){return fieldOfVision;}
            float getNearPlane(){return nearPlane;}
            float getFarPlane(){return farPlane;}

            bool thirdPerson = false;
            glm::vec3 thirdPersonTarget = glm::vec3(0,0,0);
            float thirdPersonDistance = 0;

            void turn(float _pitch,float _yaw);
            void setAspectRatio(float newRatio);
            void setNearPlane(float newNear);
            void setFarPlane(float newFar);
            void setFieldOfVision(float newFoV);
            void walkForward(float amount);
            void walkRight(float amount);
            void setDirection(glm::vec3 newDir);
            void setPosition(glm::vec3 newPos);
            void setDirection(float pitch,float yaw);
            glm::vec3 getPosition();

            void renderReflection(uniformsHolder *holder,float height);
            void render(uniformsHolder *holder);

            perspectiveCamera();
    };

    class orthographicCamera : public camera
    {
        public:

        glm::vec3 orthoBoundNear;
        glm::vec3 orthoBoundFar;

        void render(uniformsHolder *holder);
        void walkForward(float amount);
        void walkRight(float amount);
        void setDirection(glm::vec3 newDir);
        void setPosition(glm::vec3 newPos);
        glm::vec3 getPosition();
    };

}

#endif // CAMERA_H_INCLUDED
