#include "camera.h"

namespace syj
{
    void orthographicCamera::walkForward(float amount)
    {

    }

    void orthographicCamera::walkRight(float amount)
    {

    }

    void orthographicCamera::render(uniformsHolder *holder)
    {
        /*projectionMatrix = glm::ortho(orthoBoundNear.x,orthoBoundFar.x,orthoBoundNear.y,orthoBoundFar.y,orthoBoundNear.z,orthoBoundFar.z);
        viewMatrix =  glm::lookAt(position,position + direction,nominalUp);
        angleMatrix = glm::lookAt(glm::vec3(0,0,0),direction,nominalUp);*/

        glUniformMat(holder->viewMatrix(name),viewMatrix);
        glUniformMat(holder->angleMatrix(name),angleMatrix);
        glUniformMat(holder->projectionMatrix(name),projectionMatrix);
        glUniform3vec(holder->cameraDirection(name),direction);
        glUniform3vec(holder->cameraPosition(name),position);
    }

    void perspectiveCamera::walkForward(float amount)
    {
        setPosition(position + glm::vec3(direction.x * amount,direction.y * amount,direction.z * amount));
    }

    void perspectiveCamera::walkRight(float amount)
    {
        float y = atan2(direction.x,direction.z);
        y += 1.57079633;
        if(y > 6.28318531)
            y -= 6.28318531;

        setPosition(position + glm::vec3(sin(y) * amount,0,cos(y) * amount));
    }

    void perspectiveCamera::turn(float _pitch,float _yaw)
    {
        if(isnan(direction.y))
            direction.y = 0;
        if(isnan(direction.x))
            direction.x = 0;
        if(isnan(direction.z))
            direction.z = 0;
        float p = asin(direction.y);
        float y = atan2(direction.x,direction.z);
        float newP = p + _pitch;
        if(newP >= 1.57)
            newP = 1.57;
        if(newP <= -1.57)
            newP = -1.57;

        setDirection(newP,y + _yaw);
    }

    void orthographicCamera::setPosition(glm::vec3 newPos)
    {
        position = newPos;
        viewMatrix =  glm::lookAt(position,position + direction,nominalUp);
        angleMatrix = glm::lookAt(glm::vec3(0,0,0),direction,nominalUp);
    }

    void perspectiveCamera::setPosition(glm::vec3 newPos)
    {
        position = newPos;
        if(thirdPerson)
        {
            viewMatrix =  glm::lookAt(thirdPersonTarget - direction * thirdPersonDistance,thirdPersonTarget,nominalUp);
            angleMatrix = glm::lookAt(glm::vec3(0,0,0),direction * thirdPersonDistance,nominalUp);
        }
        else
        {
            viewMatrix  = glm::lookAt(position,position + direction,nominalUp);
            angleMatrix = glm::lookAt(glm::vec3(0,0,0),direction,nominalUp);
        }
    }

    glm::vec3 orthographicCamera::getPosition()
    {
        return position;
    }

    glm::vec3 perspectiveCamera::getPosition()
    {
        if(thirdPerson)
            return thirdPersonTarget - direction * thirdPersonDistance;
        else
            return position;
    }

    void orthographicCamera::setDirection(glm::vec3 newDir)
    {
        direction = newDir;
        viewMatrix  = glm::lookAt(position + direction,position,nominalUp);
        angleMatrix = glm::lookAt(direction,glm::vec3(0,0,0),nominalUp);
    }

    void perspectiveCamera::setDirection(glm::vec3 newDir)
    {
        direction = newDir;
        if(thirdPerson)
        {
            viewMatrix =  glm::lookAt(thirdPersonTarget - direction * thirdPersonDistance,thirdPersonTarget,nominalUp);
            angleMatrix = glm::lookAt(glm::vec3(0,0,0),direction * thirdPersonDistance,nominalUp);
        }
        else
        {
            /*std::cout<<position.x<<","<<position.y<<","<<position.z<<"\n";
            std::cout<<direction.x<<","<<direction.y<<","<<direction.z<<"\n";
            std::cout<<nominalUp.x<<","<<nominalUp.y<<","<<nominalUp.z<<"\n\n";*/
            viewMatrix  = glm::lookAt(position,position + direction,nominalUp);
            angleMatrix = glm::lookAt(glm::vec3(0,0,0),direction,nominalUp);
        }
    }

    glm::vec3 camera::getDirection()
    {
        return direction;
    }

    void perspectiveCamera::setDirection(float pitch, float yaw)
    {
        setDirection(glm::vec3(cos(pitch)*sin(yaw),sin(pitch),cos(pitch)*cos(yaw)));
    }

    void perspectiveCamera::setAspectRatio(float newRatio)
    {
        aspectRatio = newRatio;
        projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
    }

    void perspectiveCamera::setNearPlane(float newNear)
    {
        nearPlane = newNear;
        projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
    }

    void perspectiveCamera::setFieldOfVision(float newFoV)
    {
        fieldOfVision = newFoV;
        projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
    }

    void perspectiveCamera::setFarPlane(float newFar)
    {
        farPlane = newFar;
        projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
    }

    void perspectiveCamera::render(uniformsHolder *holder)
    {
        glUniformMat(holder->viewMatrix(name),viewMatrix);
        glUniformMat(holder->angleMatrix(name),angleMatrix);
        glUniformMat(holder->projectionMatrix(name),projectionMatrix);
        glUniform3vec(holder->cameraDirection(name),direction);
        glUniform3vec(holder->cameraPosition(name),thirdPerson ? thirdPersonTarget - direction * thirdPersonDistance : position);
        glm::vec3 right =   glm::normalize(glm::cross(direction,nominalUp));
        glm::vec3 up =      -glm::normalize(glm::cross(direction,right));
        glUniform3vec(holder->cameraRight(name),right);
        glUniform3vec(holder->cameraUp(name),up);
    }

    void perspectiveCamera::renderReflection(uniformsHolder *holder,float height)
    {
        glm::vec3 start,end,dir;
        if(thirdPerson)
        {
            dir = direction * thirdPersonDistance;
            start = thirdPersonTarget - dir;
            end = thirdPersonTarget;
        }
        else
        {
            dir = direction;
            start = position;
            end = position + dir;
        }

        dir = glm::normalize(dir);

        start.y = height - (start.y - height);
        end.y = height - (end.y - height);
        float yaw = atan2(dir.x,dir.z);
        float pitch = -asin(dir.y);
        dir = glm::vec3(cos(pitch)*sin(yaw),sin(pitch),cos(pitch)*cos(yaw));

        /*glm::vec3 pos = playerCamera.getPosition();
                pos.y = waterLevel - (pos.y - waterLevel);
                playerCamera.setPosition(pos);

                glm::vec3 dir = playerCamera.getDirection();
                dir.y = -dir.y;
                playerCamera.setDirection(dir);*/

        glm::mat4 reflectViewMatrix =  glm::lookAt(start,end,glm::vec3(0,1,0));
        glm::mat4 reflectAngleMatrix = glm::lookAt(glm::vec3(0,0,0),dir,glm::vec3(0,1,0));

        glUniformMat(holder->viewMatrix(name),reflectViewMatrix);
        glUniformMat(holder->angleMatrix(name),reflectAngleMatrix);
        glUniformMat(holder->projectionMatrix(name),projectionMatrix);
        glUniform3vec(holder->cameraDirection(name),dir);
        glUniform3vec(holder->cameraPosition(name),start);
    }

    perspectiveCamera::perspectiveCamera()
    {
        //This just gives us a default projection matrix
        setFieldOfVision(fieldOfVision);

        //This initialized our view matrix
        setPosition(getPosition());
    }
}
