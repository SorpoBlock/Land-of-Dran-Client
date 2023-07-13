#include "printLoader.h"

namespace syj
{
    printLoader::printLoader(std::string loadDir)
    {
        for (recursive_directory_iterator i(loadDir.c_str()), end; i != end; ++i)
        {
            if (!is_directory(i->path()))
            {
                if(i->path().parent_path().string().find("prints") != std::string::npos && i->path().parent_path().string().find("icons") == std::string::npos)
                {
                    if(i->path().extension() == ".png")
                    {
                        std::string addonName = i->path().parent_path().string();
                        addonName = addonName.substr(0,addonName.length() - 7);
                        int pos = addonName.find("\\Print_")+7;
                        addonName = addonName.substr(pos,addonName.length() - pos);
                        if(addonName.find("_Default") != std::string::npos)
                            addonName = addonName.substr(0,addonName.length() - 8);
                        addonName += "/" + i->path().stem().string();

                        names.push_back(addonName);
                        material *tmp = new material;
                        tmp->addOnlyAlbedo(i->path().string());
                        textures.push_back(tmp);
                    }
                }
            }
        }

        std::cout<<names.size()<<" prints loaded!\n";
    }

    printLoader::~printLoader()
    {
        for(int a = 0; a<textures.size(); a++)
            delete textures[a];
    }
}
