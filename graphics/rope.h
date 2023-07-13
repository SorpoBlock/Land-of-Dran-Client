#ifndef ROPE_H_INCLUDED
#define ROPE_H_INCLUDED

#include "code/graphics/uniformsBasic.h"
#include "code/graphics/texture.h"
#include <algorithm>
#include <glm/gtx/norm.hpp>
#include "code/utility/debugGraphics.h"

namespace syj
{
    struct rope
    {
        GLuint vao;
        GLuint positionsBuffer;
        GLuint distBuffer;

        int serverID = 0;
        int nodes = 0;
        std::vector<glm::vec3> nodePositions;

        rope(int _serverID,int _nodes);
        ~rope();
        void render();
    };
}

#endif // ROPE_H_INCLUDED
