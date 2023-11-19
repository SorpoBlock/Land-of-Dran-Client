#include "bulletTrails.h"

void bulletTrailsHolder::purge()
{
    auto iter = bulletTrails.begin();
    while(iter != bulletTrails.end())
    {
        bulletTrail *trail = &(*iter);

        if(trail->deletionTime < SDL_GetTicks())
        {
            iter = bulletTrails.erase(iter);
            continue;
        }

        ++iter;
    }
}

void bulletTrailsHolder::render(uniformsHolder *graphics)
{
    glEnable( GL_LINE_SMOOTH );
    glLineWidth(2.0);
    glBindVertexArray(quadVAO);
    for(int a = 0; a<bulletTrails.size(); a++)
    {
        bulletTrail *trail = &bulletTrails[a];

        glUniform3f(graphics->bulletTrailStart,trail->start.x,trail->start.y,trail->start.z);
        glUniform3f(graphics->bulletTrailEnd,trail->end.x,trail->end.y,trail->end.z);
        glUniform3f(graphics->bulletTrailColor,trail->color.r,trail->color.g,trail->color.b);

        float progress = trail->deletionTime - trail->creationTime;
        float timeElapsed = SDL_GetTicks() - trail->creationTime;
        progress = timeElapsed / progress;

        glUniform1f(graphics->bulletTrailProgress,progress);

        glDrawArrays(GL_LINES,0,2);
    }
    glBindVertexArray(0);
}

bulletTrailsHolder::bulletTrailsHolder()
{
    //quadVAO = createQuadVAO();

    std::vector<glm::vec3> tmp;
    tmp.push_back(glm::vec3(0,0,0));
    tmp.push_back(glm::vec3(1,1,1));

    glCreateVertexArrays(1,&quadVAO);
    glBindVertexArray(quadVAO);

    glCreateBuffers(1,&buffer);

    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glBufferData(GL_ARRAY_BUFFER,
                 tmp.size() * sizeof(glm::vec3),
                 &tmp[0][0],
                 GL_STATIC_DRAW);

    glBindVertexArray(0);
}

bulletTrailsHolder::~bulletTrailsHolder()
{
    glDeleteVertexArrays(1,&quadVAO);
    glDeleteBuffers(1,&buffer);
}
