#ifndef BULLETTRAILS_H_INCLUDED
#define BULLETTRAILS_H_INCLUDED

#include "code/graphics/material.h"
#include "code/utility/debugGraphics.h"

using namespace syj;

struct bulletTrail
{
    unsigned int deletionTime = 0;
    unsigned int creationTime = 0;
    glm::vec3 color = glm::vec3(1,1,0);
    glm::vec3 start = glm::vec3(0,0,0);
    glm::vec3 end = glm::vec3(1,1,1);
};

struct bulletTrailsHolder
{
    GLuint quadVAO;

    std::vector<bulletTrail> bulletTrails;

    void purge();
    void render(uniformsHolder *graphics);

    bulletTrailsHolder();
    ~bulletTrailsHolder();
};

#endif // BULLETTRAILS_H_INCLUDED
