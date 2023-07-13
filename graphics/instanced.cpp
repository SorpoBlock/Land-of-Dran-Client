#include "instanced.h"

namespace syj
{
    std::vector<glm::vec2> defaultSquareUVs()
    {
        std::vector<glm::vec2> uvs;

        uvs.push_back(glm::vec2(0,1));
        uvs.push_back(glm::vec2(1,1));
        uvs.push_back(glm::vec2(1,0));
        uvs.push_back(glm::vec2(1,0));
        uvs.push_back(glm::vec2(0,0));
        uvs.push_back(glm::vec2(0,1));

        uvs.push_back(glm::vec2(1,1));
        uvs.push_back(glm::vec2(0,1));
        uvs.push_back(glm::vec2(0,0));
        uvs.push_back(glm::vec2(0,0));
        uvs.push_back(glm::vec2(1,0));
        uvs.push_back(glm::vec2(1,1));

        uvs.push_back(glm::vec2(0,1));
        uvs.push_back(glm::vec2(1,1));
        uvs.push_back(glm::vec2(1,0));
        uvs.push_back(glm::vec2(1,0));
        uvs.push_back(glm::vec2(0,0));
        uvs.push_back(glm::vec2(0,1));

        return uvs;
    }

    std::vector<glm::vec3> defaultSquareShape()
    {
        std::vector<glm::vec3> verts;
        verts.push_back(glm::vec3(-1,-1,0));
        verts.push_back(glm::vec3(1,-1,0));
        verts.push_back(glm::vec3(1,1,0));
        verts.push_back(glm::vec3(1,1,0));
        verts.push_back(glm::vec3(-1,1,0));
        verts.push_back(glm::vec3(-1,-1,0));

        verts.push_back(glm::vec3(-1,-1,-1));
        verts.push_back(glm::vec3(1,-1,1));
        verts.push_back(glm::vec3(1,1,1));
        verts.push_back(glm::vec3(1,1,1));
        verts.push_back(glm::vec3(-1,1,-1));
        verts.push_back(glm::vec3(-1,-1,-1));

        verts.push_back(glm::vec3(-1,-1,1));
        verts.push_back(glm::vec3(1,-1,-1));
        verts.push_back(glm::vec3(1,1,-1));
        verts.push_back(glm::vec3(1,1,-1));
        verts.push_back(glm::vec3(-1,1,1));
        verts.push_back(glm::vec3(-1,-1,1));
        return verts;
    }

    void sprite::updateOffsets(std::vector<glm::vec4> &offsets)
    {
        glGetError();

        strides.clear();

        for(unsigned int a = 0; a<offsets.size(); a++)
            strides.push_back(offsets[a]);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,strideBuffer);
        glBufferSubData(GL_ARRAY_BUFFER,0, strides.size() * sizeof(glm::vec3), &strides[0][0]);
        glBindVertexArray(0);
    }

    void sprite::render()
    {
        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES,0,shapeSize,strides.size());
        glBindVertexArray(0);
    }

    sprite::sprite(std::vector<glm::vec3> &shape,std::vector<glm::vec2> &shapeUVs,std::vector<glm::vec3> &shapeNormals,unsigned int _size)
    {
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vertexBuffer);
        glGenBuffers(1,&uvBuffer);
        glGenBuffers(1,&normalBuffer);
        glGenBuffers(1,&strideBuffer);

        glBindVertexArray(vao);

        shapeSize = shape.size();
        glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,shape.size() * sizeof(glm::vec3),&shape[0][0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(positions);
        glVertexAttribPointer(positions,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(positions, 0);

        glBindBuffer(GL_ARRAY_BUFFER,normalBuffer);
        glBufferData(GL_ARRAY_BUFFER,shapeNormals.size() * sizeof(glm::vec3),&shapeNormals[0][0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(normals);
        glVertexAttribPointer(normals,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(normals, 0);

        glBindBuffer(GL_ARRAY_BUFFER,uvBuffer);
        glBufferData(GL_ARRAY_BUFFER,shapeUVs.size() * sizeof(glm::vec2),&shapeUVs[0][0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(uvs);
        glVertexAttribPointer(uvs,2,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(uvs, 0);

        strides.resize(_size,glm::vec4(0,0,0,0));
        glBindBuffer(GL_ARRAY_BUFFER,strideBuffer);
        glBufferData(GL_ARRAY_BUFFER,strides.size() * sizeof(glm::vec4),&strides[0][0],GL_STREAM_DRAW);
        glEnableVertexAttribArray(boneWeightsOrSprites);
        glVertexAttribPointer(boneWeightsOrSprites,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(boneWeightsOrSprites,1);

        glBindVertexArray(0);
    }

    sprite::~sprite()
    {
        glDeleteVertexArrays(1,&vao);
        glDeleteBuffers(1,&vertexBuffer);
        glDeleteBuffers(1,&uvBuffer);
        glDeleteBuffers(1,&normalBuffer);
        glDeleteBuffers(1,&strideBuffer);
    }
}
