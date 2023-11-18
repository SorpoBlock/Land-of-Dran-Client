#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "code/utility/preference.h"
#include "code/graphics/uniformsBasic.h"
#include "code/utility/ceguiHelper.h"
#include "code/utility/inputMap.h"

namespace syj
{

    enum waterChoices{waterStatic=0,waterQuarter=1,waterHalf=2,waterFull=3};
    enum shadowResChoices{shadowsOff=0,shadow1k=1,shadow2k=2,shadow4k=3};
    enum shadowSoftnessChoices{shadowsHard=0,shadows2PCF=1,shadows3PCF=2,shadows5PCF=3};
    enum godRayChoices{godRaysOff=0,godRays32Samples=1,godRays64Samples=2,godRays96Samples=3,godRays128Samples=4};
    enum spriteDensityChoices{spritesOff=0,sprites500=1,sprites1000=2,sprites2000=3,sprites4000=4};
    enum antiAliasingChoices{aaOff=0,aa2x=1,aa4x=2,aa8x=3,aa16x=4};
    enum guiScalingChoices{smaller=0,normalScaling=1,bigger=2,biggest=3};

    struct options
    {
        float materialCutoff = 400.0;
        int resolutionX = 1280;
        int resolutionY = 720;
        bool fullscreen = false;
        bool vsync = false;
        int fieldOfView = 90;
        int hudOpacity = 80;
        waterChoices waterQuality = waterHalf;
        shadowResChoices shadowResolution = shadow2k;
        shadowSoftnessChoices shadowSoftness = shadows2PCF;
        bool coloredShadows = true;
        godRayChoices godRayQuality = godRaysOff;
        spriteDensityChoices spriteDensity = sprites2000;
        antiAliasingChoices antiAliasing = aa4x;
        guiScalingChoices guiScaling = normalScaling;

        int mouseSensitivity = 100;
        bool invertMouseY = false;

        int masterVolume = 80;
        int musicVolume = 80;

        preferenceFile *prefs = 0;

        void setDefaults(preferenceFile *prefs);
        void overwrite(preferenceFile *prefs);
        void loadFromFile(preferenceFile *prefs);
        void render(uniformsHolder *unis);

        bool guiScalingChanged = true;
    };

    CEGUI::Window* loadOptionsGUI(options *defaults,preferenceFile &prefs,inputMap &keybinds);
    void setOptions(options &toSet);
    bool applyButton(const CEGUI::EventArgs &e);
    bool fovSliderMoved(const CEGUI::EventArgs &e);
    bool hudOpacitySliderMoved(const CEGUI::EventArgs &e);
    bool closeOptionsMenu(const CEGUI::EventArgs &e);
    bool controlsTab(const CEGUI::EventArgs &e);
    bool graphicsTab(const CEGUI::EventArgs &e);

}


#endif // OPTIONS_H_INCLUDED
