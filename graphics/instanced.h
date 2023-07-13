#ifndef INSTANCED_H_INCLUDED
#define INSTANCED_H_INCLUDED

#include "code/graphics/uniformsBasic.h"

namespace syj
{
    std::vector<glm::vec2> defaultSquareUVs();
    std::vector<glm::vec3> defaultSquareShape();

    class sprite
    {
        private:
            GLuint vao;
            GLuint vertexBuffer;
            GLuint uvBuffer;
            GLuint normalBuffer;
            GLuint strideBuffer;

            int shapeSize = 0;
            std::vector<glm::vec4> strides;

        public:
            void updateOffsets(std::vector<glm::vec4> &offsets);
            void render();
            sprite(std::vector<glm::vec3> &shape,std::vector<glm::vec2> &shapeUVs,std::vector<glm::vec3> &shapeNormals,unsigned int _size);
            ~sprite();
    };
}

#endif // INSTANCED_H_INCLUDED
