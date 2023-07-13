#include "rope.h"

namespace syj
{
    rope::rope(int _serverID,int _nodes)
    {
        serverID = _serverID;
        nodes = _nodes;
        std::vector<float> dists;
        for(int a = 0; a<nodes; a++)
        {
            nodePositions.push_back(glm::vec3(0,a,0));
            dists.push_back(a);
        }

        glCreateVertexArrays(1,&vao);
        glCreateBuffers(1,&positionsBuffer);
        glCreateBuffers(1,&distBuffer);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER,positionsBuffer);
        glBufferData(GL_ARRAY_BUFFER,nodePositions.size() * sizeof(glm::vec3),&nodePositions[0][0],GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindBuffer(GL_ARRAY_BUFFER,distBuffer);
        glBufferData(GL_ARRAY_BUFFER,dists.size() * sizeof(float),&dists[0],GL_STREAM_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindVertexArray(0);
    }

    void rope::render()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_LINE_STRIP,0,nodePositions.size());
        glBindVertexArray(0);
    }

    rope::~rope()
    {
        glDeleteBuffers(1,&positionsBuffer);
        glDeleteBuffers(1,&distBuffer);
        glDeleteVertexArrays(1,&vao);
    }
}
