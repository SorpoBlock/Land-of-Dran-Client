#ifndef BRICKSELECTOR_H_INCLUDED
#define BRICKSELECTOR_H_INCLUDED

#include <iostream>
#include "code/utility/ceguiHelper.h"
#include "code/utility/fileOpeartions.h"

namespace syj
{
    CEGUI::Window *loadBrickSelector();

    int addBasicBrickToSelector(CEGUI::Window *brickSelector,std::string imageFilePath,int width,int height,int length);
    void addSpecialBrickToSelector(CEGUI::Window *brickSelector,std::string imageFilePath,std::string text,int pickingID);
}

#endif // BRICKSELECTOR_H_INCLUDED
