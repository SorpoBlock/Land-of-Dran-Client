#ifndef TEMPBRICK_H_INCLUDED
#define TEMPBRICK_H_INCLUDED

#include "code/graphics/newBrickType.h"
#include "code/gui/brickSelector.h"
#include "code/utility/inputMap.h"
#include "code/gui/palette.h"
#include "code/audio/audio.h"
#include "code/networking/client.h"

namespace syj
{
    struct tempBrick
    {
        specialBrickRenderData special;
        basicBrickRenderData basic;

        CEGUI::Window *cart = 0;

        int rotationID = 0;
        int brickSlotSelected = -1;
        //glm::vec3 position;

        short xPos,zPos;
        unsigned short uYPos;
        bool xHalfPos,yHalfPos,zHalfPos;

        float getX() { return ((float)xPos) + (xHalfPos ? 0.5 : 0.0); }
        float getY() { return (((float)uYPos) + (yHalfPos ? 0.5 : 0.0)) * 0.4; }
        float getZ() { return ((float)zPos) + (zHalfPos ? 0.5 : 0.0); }

        void rotCheck();

        bool basicChanged = false;
        bool specialChanged = false;
        bool isBasic = false;
        int resizeMode = 0; //0 = none, 1 = grow, 2 = shrink
        bool superShiftMode = false;

        int brickMoveCooldown = 80;
        int firstPressTime = -1;
        int lastBrickMove = 0;

        //Returns true if a brick type was selected
        //Doesn't handle changing palette color
        brickMaterial mat = none;
        void scroll(CEGUI::Window *hud,CEGUI::Window *brickSelector,newBrickRenderer &renderer,int mouseWheelChange);
        void selectBrick(inputMap &controls,CEGUI::Window *hud,CEGUI::Window *brickSelector,newBrickRenderer &renderer,bool &guiOpened,bool &guiClosed);
        void manipulate(inputMap &controls,CEGUI::Window *brickPopup,CEGUI::Window *brickSelector,float yaw,audioPlayer *speaker,newBrickRenderer &renderer);
        void plant(newBrickRenderer &renderer,client *serverConnection);

        void teleport(glm::vec3 pos);
        void update(newBrickRenderer &renderer,paletteGUI *palette);
        tempBrick(newBrickRenderer &renderer);
    };
}

#endif // TEMPBRICK_H_INCLUDED
