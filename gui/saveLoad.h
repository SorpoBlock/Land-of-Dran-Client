#ifndef SAVELOAD_H_INCLUDED
#define SAVELOAD_H_INCLUDED

#include "code/utility/ceguiHelper.h"
#include "code/networking/recv.h"
#include <filesystem>
#include "code/utility/clientStuff.h"

using namespace std::filesystem;

namespace syj
{
    void sendBrickCarToServer(clientStuff *common,livingBrick *car,glm::vec3 origin);
    CEGUI::Window *loadSaveLoadWindow(clientStuff *common);
}


#endif // SAVELOAD_H_INCLUDED
