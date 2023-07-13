#ifndef RECTTOCUBE_H_INCLUDED
#define RECTTOCUBE_H_INCLUDED

#include "code/graphics/texture.h"
#include "code/graphics/camera.h"

namespace syj
{
    GLuint processEquirectangularMap(program &rectToCube,GLuint cubeVAO,std::string fileName,bool mipMaps = false);
}

#endif // RECTTOCUBE_H_INCLUDED
