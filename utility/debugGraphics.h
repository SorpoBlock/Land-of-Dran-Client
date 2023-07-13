#ifndef DEBUGGRAPHICS_H_INCLUDED
#define DEBUGGRAPHICS_H_INCLUDED

#include "code/utility/logger.h"
#include "code/graphics/program.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include "code/graphics/texture.h"
#include "code/graphics/uniformsBasic.h"
#include "code/physics/bulletIncludes.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

extern glm::vec3 debugLocations[10];

namespace syj
{
    typedef struct RgbColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } RgbColor;

    typedef struct HsvColor
    {
        unsigned char h;
        unsigned char s;
        unsigned char v;
    } HsvColor;

    RgbColor HsvToRgb(HsvColor hsv);
    HsvColor RgbToHsv(RgbColor rgb);

    void sayVec3(btVector3 a);
    glm::vec3 BtToGlm(btVector3 in);
    btVector3 glmToBt(glm::vec3 in);

    glm::vec3 multVec3ByMat(glm::mat4 rotation,glm::vec3 point);
    glm::vec3 multVec3ByMat(glm::quat rotation,glm::vec3 point);
    void storeTransform(glm::vec3 pos,glm::vec3 dir);
    void loadTransform(glm::vec3 &pos,glm::vec3 &dir);
    double drand(double min,double max);
    std::vector<glm::vec3> calculateNormals(std::vector<glm::vec3> &verts);
    GLuint createCubeVAO();
    void drawDebugLocations(uniformsHolder &unis,GLuint cubeVAO,std::vector<glm::vec3> debugLocations,std::vector<glm::vec3> debugColors);
    GLuint createQuadVAO();
    GLuint createBoxEdgesVAO();
    void printComputerStats();
}

#endif // DEBUGGRAPHICS_H_INCLUDED
