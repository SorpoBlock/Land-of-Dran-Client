#ifndef PRINTLOADER_H_INCLUDED
#define PRINTLOADER_H_INCLUDED

#include "code/graphics/material.h"
#include <filesystem>

using namespace std::filesystem;

namespace syj
{
    struct printLoader
    {
        std::vector<std::string> names;
        std::vector<material*> textures;

        printLoader(std::string loadDir);
        ~printLoader();
    };

}

#endif // PRINTLOADER_H_INCLUDED
