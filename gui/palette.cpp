#include "palette.h"

namespace syj
{
    glm::vec4 paletteGUI::getColor()
    {
        return colors[selectedIndex];
    }

    void paletteGUI::advanceColumn()
    {
        open();

        selectedIndex += 1 * rows;

        if(selectedIndex < 0)
            selectedIndex += columns * rows;
        if(selectedIndex >= columns * rows)
            selectedIndex -= columns * rows;

        int row = selectedIndex % rows;
        int column = selectedIndex / rows;

        float xStart = column;
        xStart *= 0.2;
        float yStart = row;
        yStart *= 0.1;
        yStart = 1.0 - yStart;
        yStart -= 0.1;


        window->getChild("Selector")->setArea(
                                              CEGUI::UDim(xStart,0),
                                              CEGUI::UDim(yStart,0),
                                              CEGUI::UDim(0.2,0),
                                              CEGUI::UDim(0.1,0));
    }

    void paletteGUI::scroll(int upOrDown)
    {
        open();

        if(upOrDown == 1)
        {
            if(selectedIndex % rows == rows - 1)
                selectedIndex -= (rows-1);
            else
                selectedIndex++;
        }
        else
        {
            if(selectedIndex == 0)
                selectedIndex += rows-1;
            else
                selectedIndex--;
        }

        if(selectedIndex < 0)
            selectedIndex += columns * rows;
        if(selectedIndex >= columns * rows)
            selectedIndex -= columns * rows;

        int row = selectedIndex % rows;
        int column = selectedIndex / rows;

        float xStart = column;
        xStart *= 0.2;
        float yStart = row;
        yStart *= 0.1;
        yStart = 1.0 - yStart;
        yStart -= 0.1;

        window->getChild("Selector")->setArea(
                                              CEGUI::UDim(xStart,0),
                                              CEGUI::UDim(yStart,0),
                                              CEGUI::UDim(0.2,0),
                                              CEGUI::UDim(0.1,0));

        window->getChild("Selector")->moveToFront();
    }

    void paletteGUI::close()
    {
        lastLocation = actualLocation;
        desiredLocation = -0.2;
        lastSlide = SDL_GetTicks();
    }

    void paletteGUI::open()
    {
        lastLocation = actualLocation;
        desiredLocation = 0.0;
        lastSlide = SDL_GetTicks();
    }

    void paletteGUI::calcAnimation()
    {
        float progress = (SDL_GetTicks() - lastSlide);
        progress /= timeToSlide;
        progress = std::clamp(progress,0.f,1.f);
        actualLocation = lastLocation + progress * (desiredLocation - lastLocation);
        CEGUI::UVector2 oldPos = window->getPosition();
        CEGUI::UDim x = oldPos.d_x;
        CEGUI::UDim y = oldPos.d_y;
        x.d_scale = actualLocation;
        window->setPosition(CEGUI::UVector2(x,y));

        if(SDL_GetTicks() - lastSlide > autoClose)
            close();
    }

    paletteGUI::paletteGUI(CEGUI::Window *hud)
    {
        window = hud->getChild("Palette");
        colors.resize(64);
        colors[0] = glm::vec4(1,0,0,1);
        colors[1] = glm::vec4(0,1,0,1);
        colors[2] = glm::vec4(0,0,1,1);
        colors[3] = glm::vec4(1,1,1,0.5);
    }

    void paletteGUI::setColor(int index,glm::vec4 color)
    {
        /*int column = index % columns;
        int row = index / columns;*/

        std::string paletteName = std::to_string(index);
        if(paletteName.length() == 1)
            paletteName = "0" + paletteName;

        if(index >= colors.size())
        {
            error("Color index " + std::to_string(index) + " for palette out of range!");
            return;
        }

        colors[index] = color;

        CEGUI::Window *swatch = window->getChild("Palette" + paletteName);
        if(!swatch)
        {
            error("Color swatch Palette" + paletteName + " not found!");
            return;
        }
        swatch->setProperty("SwatchColour",charToHex(color.a*255) + charToHex(color.r*255) + charToHex(color.g*255) + charToHex(color.b*255));
    }
}
