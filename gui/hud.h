#ifndef HUD_H_INCLUDED
#define HUD_H_INCLUDED

#include <iostream>
#include "code/utility/ceguiHelper.h"
#include "code/networking/client.h"

namespace syj
{
    bool sendChat();
    CEGUI::Window *initHud(CEGUI::Window *hud);
    void setBrickLoadProgress(float progress,int bricks);
    void notify(std::string windowName,std::string windowText,std::string buttonText);

    struct textMessages
    {
        int textExpiry = 0;
        CEGUI::Window *textBar = 0;

        void checkForTimeouts();
        void setText(std::string text,int timeoutMS);
    };
}

#endif // HUD_H_INCLUDED
