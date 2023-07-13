#ifndef PALETTE_H_INCLUDED
#define PALETTE_H_INCLUDED

#include "code/utility/ceguiHelper.h"
#include "code/graphics/program.h"

namespace syj
{
    struct paletteGUI
    {
        float lastLocation = 0.0;
        float desiredLocation = -0.2;
        float actualLocation = -0.2;
        float lastSlide = 0;
        const float timeToSlide = 500;
        const float autoClose = 3000;

        const int columns = 5;
        const int rows = 8;
        std::vector<glm::vec4> colors;
        int selectedIndex = 0;
        CEGUI::Window *window = 0;
        void setColor(int index,glm::vec4 color);
        void scroll(int upOrDown);
        void advanceColumn();
        glm::vec4 getColor();
        void calcAnimation();
        void close();
        void open();
        paletteGUI(CEGUI::Window *hud);
    };
}

#endif // PALETTE_H_INCLUDED
