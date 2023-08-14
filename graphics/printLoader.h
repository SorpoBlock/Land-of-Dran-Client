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

        unsigned int getPrintID(std::string name)
        {
            for(int a = 0; a<names.size(); a++)
            {
                if(name == names[a])
                    return a;
            }
            return 0;
        }

        printLoader(std::string loadDir);
        ~printLoader();
    };

}

#endif // PRINTLOADER_H_INCLUDED
