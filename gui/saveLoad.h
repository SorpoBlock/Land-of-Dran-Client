#ifndef SAVELOAD_H_INCLUDED
#define SAVELOAD_H_INCLUDED

#include "code/utility/ceguiHelper.h"
#include "code/networking/recv.h"
#include <filesystem>

using namespace std::filesystem;

namespace syj
{
    void sendBrickCarToServer(serverStuff *common,livingBrick *car,glm::vec3 origin);
    CEGUI::Window *loadSaveLoadWindow(serverStuff *common);
}


#endif // SAVELOAD_H_INCLUDED
