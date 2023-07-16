#include "options.h"

namespace syj
{
    void options::render(uniformsHolder &unis)
    {
        switch(shadowSoftness)
        {
            default:
            case shadowsHard:
                glUniform1i(unis.target->getUniformLocation("shadowSoftness"),0);
                break;
            case shadows2PCF:
                glUniform1i(unis.target->getUniformLocation("shadowSoftness"),1);
                break;
            case shadows3PCF:
                glUniform1i(unis.target->getUniformLocation("shadowSoftness"),2);
                break;
            case shadows5PCF:
                glUniform1i(unis.target->getUniformLocation("shadowSoftness"),3);
                break;
        }

        glUniform1i(unis.target->getUniformLocation("niceWater"),waterQuality != waterStatic);

        switch(godRayQuality)
        {
            case godRaysOff:
                glUniform1i(unis.target->getUniformLocation("godRaySamples"),0);
                break;
            case godRays32Samples:
                glUniform1i(unis.target->getUniformLocation("godRaySamples"),32);
                break;
            case godRays64Samples:
                glUniform1i(unis.target->getUniformLocation("godRaySamples"),64);
                break;
            case godRays96Samples:
                glUniform1i(unis.target->getUniformLocation("godRaySamples"),96);
                break;
            case godRays128Samples:
                glUniform1i(unis.target->getUniformLocation("godRaySamples"),128);
                break;
        }

        glUniform1i(unis.target->getUniformLocation("useShadows"),shadowResolution != shadowsOff);

        glUniform1f(unis.target->getUniformLocation("materialCutoff"),materialCutoff);
    }

    void setOptions(options *toSet)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *graphicsPlane = root->getChild("Options")->getChild("Graphics");
        CEGUI::Window *controlsPlane = root->getChild("Options")->getChild("Controls");
        CEGUI::Window *audioPlane = root->getChild("Options")->getChild("Audio");

        toSet->materialCutoff = ((CEGUI::Slider*)graphicsPlane->getChild("MaterialCutoff"))->getCurrentValue();
        toSet->waterQuality = (waterChoices)((CEGUI::Combobox*)graphicsPlane->getChild("WaterQualityDropdown"))->getSelectedItem()->getID();
        toSet->shadowResolution = (shadowResChoices)((CEGUI::Combobox*)graphicsPlane->getChild("ShadowResolutionDropdown"))->getSelectedItem()->getID();
        toSet->shadowSoftness = (shadowSoftnessChoices)((CEGUI::Combobox*)graphicsPlane->getChild("ShadowSoftnessDropdown"))->getSelectedItem()->getID();
        toSet->godRayQuality = (godRayChoices)((CEGUI::Combobox*)graphicsPlane->getChild("GodRayQualityDropdown"))->getSelectedItem()->getID();
        toSet->spriteDensity = (spriteDensityChoices)((CEGUI::Combobox*)graphicsPlane->getChild("SpriteDensityDropdown"))->getSelectedItem()->getID();
        toSet->antiAliasing = (antiAliasingChoices)((CEGUI::Combobox*)graphicsPlane->getChild("AASamplesDropdown"))->getSelectedItem()->getID();
        toSet->resolutionX = atoi(((CEGUI::Editbox*)graphicsPlane->getChild("xResEditbox"))->getText().c_str());
        toSet->resolutionY = atoi(((CEGUI::Editbox*)graphicsPlane->getChild("yResEditbox"))->getText().c_str());
        toSet->fullscreen = ((CEGUI::ToggleButton*)graphicsPlane->getChild("FullscreenCheckbox"))->isSelected();
        toSet->vsync = ((CEGUI::ToggleButton*)graphicsPlane->getChild("VSyncCheckbox"))->isSelected();
        toSet->coloredShadows = ((CEGUI::ToggleButton*)graphicsPlane->getChild("ColoredShadowsCheckbox"))->isSelected();
        toSet->fieldOfView = ((CEGUI::Slider*)graphicsPlane->getChild("FOVSlider"))->getCurrentValue();
        toSet->hudOpacity = ((CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider"))->getCurrentValue();
        toSet->mouseSensitivity = ((CEGUI::Slider*)controlsPlane->getChild("MouseSlider"))->getCurrentValue();
        toSet->invertMouseY = ((CEGUI::ToggleButton*)controlsPlane->getChild("InvertMouseCheckbox"))->isSelected();
        toSet->masterVolume = ((CEGUI::Slider*)audioPlane->getChild("MasterSlider"))->getCurrentValue();
        toSet->musicVolume = ((CEGUI::Slider*)audioPlane->getChild("MusicSlider"))->getCurrentValue();

        if(toSet->prefs)
        {
            toSet->overwrite(toSet->prefs);
            toSet->prefs->exportToFile("config.txt");
        }
    }

    bool closeOptionsMenu(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *graphicsPlane = root->getChild("Options");
        graphicsPlane->setVisible(false);

        return true;
    }

    bool applyButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");

        setOptions((options*)optionsWindow->getChild("ApplyButton")->getUserData());

        return true;
    }

    bool graphicsTab(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");

        optionsWindow->getChild("Controls")->setVisible(false);
        optionsWindow->getChild("Audio")->setVisible(false);
        optionsWindow->getChild("Graphics")->setVisible(true);
        optionsWindow->getChild("Graphics")->moveToFront();

        return true;
    }

    bool controlsTab(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");

        optionsWindow->getChild("Audio")->setVisible(false);
        optionsWindow->getChild("Graphics")->setVisible(false);
        optionsWindow->getChild("Controls")->setVisible(true);
        optionsWindow->getChild("Controls")->moveToFront();

        return true;
    }

    bool audioTab(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");

        optionsWindow->getChild("Graphics")->setVisible(false);
        optionsWindow->getChild("Controls")->setVisible(false);
        optionsWindow->getChild("Audio")->setVisible(true);
        optionsWindow->getChild("Audio")->moveToFront();

        return true;
    }

    bool materialCutoffSliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *graphicsPlane = root->getChild("Options")->getChild("Graphics");

        ((options*)((CEGUI::Slider*)graphicsPlane->getChild("MaterialCutoff"))->getUserData())->materialCutoff = ((CEGUI::Slider*)graphicsPlane->getChild("MaterialCutoff"))->getCurrentValue();

        return true;
    }

    bool fovSliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *graphicsPlane = root->getChild("Options")->getChild("Graphics");

        graphicsPlane->getChild("FOVText")->setText(std::to_string(((CEGUI::Slider*)graphicsPlane->getChild("FOVSlider"))->getCurrentValue()) + " degrees");
        ((options*)((CEGUI::Slider*)graphicsPlane->getChild("FOVSlider"))->getUserData())->fieldOfView = ((CEGUI::Slider*)graphicsPlane->getChild("FOVSlider"))->getCurrentValue();

        return true;
    }

    bool mouseSliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *controlsPlane = root->getChild("Options")->getChild("Controls");

        controlsPlane->getChild("MouseText")->setText(std::to_string(((CEGUI::Slider*)controlsPlane->getChild("MouseSlider"))->getCurrentValue()) + "%");
        ((options*)((CEGUI::Slider*)controlsPlane->getChild("MouseSlider"))->getUserData())->mouseSensitivity = ((CEGUI::Slider*)controlsPlane->getChild("MouseSlider"))->getCurrentValue();

        return true;
    }

    bool masterSliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *audio = root->getChild("Options")->getChild("Audio");

        ((options*)((CEGUI::Slider*)audio->getChild("MasterSlider"))->getUserData())->masterVolume = ((CEGUI::Slider*)audio->getChild("MasterSlider"))->getCurrentValue();

        return true;
    }

    bool musicSliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *audio = root->getChild("Options")->getChild("Audio");

        ((options*)((CEGUI::Slider*)audio->getChild("MusicSlider"))->getUserData())->musicVolume = ((CEGUI::Slider*)audio->getChild("MusicSlider"))->getCurrentValue();

        return true;
    }

    bool hudOpacitySliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *graphicsPlane = root->getChild("Options")->getChild("Graphics");

        graphicsPlane->getChild("OpacityText")->setText(std::to_string(((CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider"))->getCurrentValue()) + "%");
        ((options*)((CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider"))->getUserData())->hudOpacity = ((CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider"))->getCurrentValue();
        float alpha = ((options*)((CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider"))->getUserData())->hudOpacity;
        alpha /= 100;
        graphicsPlane->getParent()->setAlpha(alpha);

        return true;
    }

    void options::overwrite(preferenceFile *prefs)
    {
        prefs->set("RESX",resolutionX);
        prefs->set("RESY",resolutionY);
        prefs->set("FULLSCREEN",fullscreen);
        prefs->set("VSYNC",vsync);
        prefs->set("FOV",fieldOfView);
        prefs->set("HUDOPACITY",hudOpacity);
        prefs->set("WATERQUALITY",waterQuality);
        prefs->set("SHADOWRESOLUTION",shadowResolution);
        prefs->set("SHADOWSOFTNESS",shadowSoftness);
        prefs->set("COLOREDSHADOWS",coloredShadows);
        prefs->set("GODRAYQUALITY",godRayQuality);
        prefs->set("SPRITEDENSITY",spriteDensity);
        prefs->set("ANTIALIASING",antiAliasing);
        prefs->set("INVERTMOUSEY",invertMouseY);
        prefs->set("MOUSESENSITIVITY",mouseSensitivity);
        prefs->set("MASTERVOLUME",masterVolume);
        prefs->set("MUSICVOLUME",musicVolume);
        prefs->set("MATERIALCUTOFF",(int)materialCutoff);
    }

    void options::setDefaults(preferenceFile *prefs)
    {
        prefs->addIntegerPreference("RESX",resolutionX);
        prefs->addIntegerPreference("RESY",resolutionY);
        prefs->addBoolPreference("FULLSCREEN",fullscreen);
        prefs->addBoolPreference("VSYNC",vsync);
        prefs->addIntegerPreference("FOV",fieldOfView);
        prefs->addIntegerPreference("HUDOPACITY",hudOpacity);
        prefs->addIntegerPreference("WATERQUALITY",waterQuality);
        prefs->addIntegerPreference("SHADOWRESOLUTION",shadowResolution);
        prefs->addIntegerPreference("SHADOWSOFTNESS",shadowSoftness);
        prefs->addBoolPreference("COLOREDSHADOWS",coloredShadows);
        prefs->addIntegerPreference("GODRAYQUALITY",godRayQuality);
        prefs->addIntegerPreference("SPRITEDENSITY",spriteDensity);
        prefs->addIntegerPreference("ANTIALIASING",antiAliasing);
        prefs->addBoolPreference("INVERTMOUSEY",invertMouseY);
        prefs->addIntegerPreference("MOUSESENSITIVITY",mouseSensitivity);
        prefs->addStringPreference("IP","dran.land");
        prefs->addStringPreference("Name","Guest");
        prefs->addIntegerPreference("PlayerColorRed",rand() % 255);
        prefs->addIntegerPreference("PlayerColorGreen",rand() % 255);
        prefs->addIntegerPreference("PlayerColorBlue",rand() % 255);
        prefs->addIntegerPreference("MASTERVOLUME",80);
        prefs->addIntegerPreference("MUSICVOLUME",80);
        prefs->addIntegerPreference("MATERIALCUTOFF",400);

        prefs->addIntegerPreference("NETWORK",0);
        prefs->addIntegerPreference("REVISION",0);
    }

    void options::loadFromFile(preferenceFile *prefs)
    {
        preference *tmp;

        tmp = prefs->getPreference("RESX");
        if(tmp)
            resolutionX = tmp->toInteger();

        tmp = prefs->getPreference("RESY");
        if(tmp)
            resolutionY = tmp->toInteger();

        tmp = prefs->getPreference("FULLSCREEN");
        if(tmp)
            fullscreen = tmp->toBool();

        tmp = prefs->getPreference("VSYNC");
        if(tmp)
            vsync = tmp->toBool();

        tmp = prefs->getPreference("FOV");
        if(tmp)
            fieldOfView = tmp->toInteger();

        tmp = prefs->getPreference("HUDOPACITY");
        if(tmp)
            hudOpacity = tmp->toInteger();

        tmp = prefs->getPreference("WATERQUALITY");
        if(tmp)
            waterQuality = (waterChoices)tmp->toInteger();

        tmp = prefs->getPreference("SHADOWRESOLUTION");
        if(tmp)
            shadowResolution = (shadowResChoices)tmp->toInteger();

        tmp = prefs->getPreference("SHADOWSOFTNESS");
        if(tmp)
            shadowSoftness = (shadowSoftnessChoices)tmp->toInteger();

        tmp = prefs->getPreference("COLOREDSHADOWS");
        if(tmp)
            coloredShadows = tmp->toBool();

        tmp = prefs->getPreference("GODRAYQUALITY");
        if(tmp)
            godRayQuality = (godRayChoices)tmp->toInteger();

        tmp = prefs->getPreference("SPRITEDENSITY");
        if(tmp)
            spriteDensity = (spriteDensityChoices)tmp->toInteger();

        tmp = prefs->getPreference("ANTIALIASING");
        if(tmp)
            antiAliasing = (antiAliasingChoices)tmp->toInteger();

        tmp = prefs->getPreference("INVERTMOUSEY");
        if(tmp)
            invertMouseY = tmp->toBool();

        tmp = prefs->getPreference("MOUSESENSITIVITY");
        if(tmp)
            mouseSensitivity = tmp->toInteger();

        tmp = prefs->getPreference("MASTERVOLUME");
        if(tmp)
            masterVolume = tmp->toInteger();

        tmp = prefs->getPreference("MUSICVOLUME");
        if(tmp)
            musicVolume = tmp->toInteger();

        tmp = prefs->getPreference("MATERIALCUTOFF");
        if(tmp)
            materialCutoff = tmp->toInteger();
    }

    bool rebindKey(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");
        CEGUI::Window *controls = optionsWindow->getChild("Controls");

        CEGUI::MultiColumnList* keyBindsList = (CEGUI::MultiColumnList*)controls->getChild("KeyBindsList");
        int action = keyBindsList->getFirstSelectedItem()->getID();
        controls->getChild("BindingText")->setText("Press any key to bind for " + inputCommandToString((inputCommand)action));

        SDL_Event s;
        int foundKey = 0;
        bool binding = true;
        while(binding)
        {
            while(SDL_PollEvent(&s))
            {
                if(s.type == SDL_QUIT)
                {
                    binding = false;
                    break;
                }
                if(s.type == SDL_KEYDOWN)
                {
                    foundKey = s.key.keysym.scancode;
                    binding = false;
                    break;
                }
            }
        }

        ((inputMap*)keyBindsList->getUserData())->bindKey((inputCommand)action,(SDL_Scancode)foundKey);
        std::string keyName = SDL_GetScancodeName((SDL_Scancode)foundKey);
        keyBindsList->getNextSelected(keyBindsList->getFirstSelectedItem())->setText(keyName);
        ((inputMap*)keyBindsList->getUserData())->toPrefs(((options*)optionsWindow->getChild("ApplyButton")->getUserData())->prefs);
        ((options*)optionsWindow->getChild("ApplyButton")->getUserData())->prefs->exportToFile("config.txt");
        keyBindsList->handleUpdatedItemData();

        return true;
    }

    void buildLaptopBinds()
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");
        CEGUI::Window *controls = optionsWindow->getChild("Controls");
        inputMap *keyBinds = (inputMap*)controls->getChild("LaptopBuild")->getUserData();

        keyBinds->bindKey(hideBricks,SDL_SCANCODE_0);
        keyBinds->bindKey(moveBrickForward,SDL_SCANCODE_I);
        keyBinds->bindKey(moveBrickBackward,SDL_SCANCODE_K);
        keyBinds->bindKey(moveBrickLeft,SDL_SCANCODE_J);
        keyBinds->bindKey(moveBrickRight,SDL_SCANCODE_L);
        keyBinds->bindKey(moveBrickUp,SDL_SCANCODE_PERIOD);
        keyBinds->bindKey(moveBrickDown,SDL_SCANCODE_COMMA);
        keyBinds->bindKey(rotateBrick,SDL_SCANCODE_U);
        keyBinds->bindKey(rotateBrickBackwards,SDL_SCANCODE_KP_7);
        keyBinds->bindKey(plantBrick,SDL_SCANCODE_RETURN);
        keyBinds->bindKey(moveBrickUpThree,SDL_SCANCODE_P);
        keyBinds->bindKey(moveBrickDownThree,SDL_SCANCODE_SEMICOLON);

        keyBinds->toPrefs(((options*)optionsWindow->getChild("ApplyButton")->getUserData())->prefs);
        ((options*)optionsWindow->getChild("ApplyButton")->getUserData())->prefs->exportToFile("config.txt");

        CEGUI::MultiColumnList* keyBindsList = (CEGUI::MultiColumnList*)controls->getChild("KeyBindsList");
        keyBindsList->resetList();
        for(int a = toggleMouseLock; a < endOfCommandEnum; a++)
        {
            keyBindsList->addRow(a);

            CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem(inputCommandToString((inputCommand)a),a);
            cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
            //cell->setSelectionColours(CEGUI::Colour(0.3,0,1,1));
            cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
            keyBindsList->setItem(cell,0,a-1);

            std::string keyName = SDL_GetScancodeName(keyBinds->getBoundKey((inputCommand)a));
            cell = new CEGUI::ListboxTextItem(keyName,a);
            cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
            //cell->setSelectionColours(CEGUI::Colour(0.3,0,0,1));
            cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
            keyBindsList->setItem(cell,1,a-1);
        }
    }

    void buildNormalBinds()
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *optionsWindow = root->getChild("Options");
        CEGUI::Window *controls = optionsWindow->getChild("Controls");
        inputMap *keyBinds = (inputMap*)controls->getChild("NormalBuild")->getUserData();

        keyBinds->bindKey(hideBricks,SDL_SCANCODE_KP_0);
        keyBinds->bindKey(moveBrickForward,SDL_SCANCODE_KP_8);
        keyBinds->bindKey(moveBrickBackward,SDL_SCANCODE_KP_2);
        keyBinds->bindKey(moveBrickLeft,SDL_SCANCODE_KP_4);
        keyBinds->bindKey(moveBrickRight,SDL_SCANCODE_KP_6);
        keyBinds->bindKey(moveBrickUp,SDL_SCANCODE_KP_3);
        keyBinds->bindKey(moveBrickDown,SDL_SCANCODE_KP_1);
        keyBinds->bindKey(rotateBrick,SDL_SCANCODE_KP_9);
        keyBinds->bindKey(rotateBrickBackwards,SDL_SCANCODE_KP_7);
        keyBinds->bindKey(plantBrick,SDL_SCANCODE_KP_ENTER);
        keyBinds->bindKey(moveBrickUpThree,SDL_SCANCODE_KP_PLUS);
        keyBinds->bindKey(moveBrickDownThree,SDL_SCANCODE_KP_5);

        keyBinds->toPrefs(((options*)optionsWindow->getChild("ApplyButton")->getUserData())->prefs);
        ((options*)optionsWindow->getChild("ApplyButton")->getUserData())->prefs->exportToFile("config.txt");

        CEGUI::MultiColumnList* keyBindsList = (CEGUI::MultiColumnList*)controls->getChild("KeyBindsList");
        keyBindsList->resetList();
        for(int a = toggleMouseLock; a < endOfCommandEnum; a++)
        {
            keyBindsList->addRow(a);

            CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem(inputCommandToString((inputCommand)a),a);
            cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
            //cell->setSelectionColours(CEGUI::Colour(0.3,0,1,1));
            cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
            keyBindsList->setItem(cell,0,a-1);

            std::string keyName = SDL_GetScancodeName(keyBinds->getBoundKey((inputCommand)a));
            cell = new CEGUI::ListboxTextItem(keyName,a);
            cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
            //cell->setSelectionColours(CEGUI::Colour(0.3,0,0,1));
            cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
            keyBindsList->setItem(cell,1,a-1);
        }
    }

    CEGUI::Window* loadOptionsGUI(options *defaults,preferenceFile &prefs,inputMap &keybinds)
    {
        CEGUI::Window *optionsWindow = addGUIFromFile("options.layout");
        CEGUI::Window *graphicsPlane = optionsWindow->getChild("Graphics");
        CEGUI::Window *controlsPlane = optionsWindow->getChild("Controls");
        CEGUI::Window *audioPlane = optionsWindow->getChild("Audio");

        optionsWindow->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeOptionsMenu));

        //Graphics options stuff:

        CEGUI::Combobox *waterQualityDropdown = (CEGUI::Combobox*)graphicsPlane->getChild("WaterQualityDropdown");
        dropBoxAdd(waterQualityDropdown,"No reflections",waterStatic);
        dropBoxAdd(waterQualityDropdown,"1/4 resolution",waterQuarter);
        dropBoxAdd(waterQualityDropdown,"1/2 resolution",waterHalf);
        dropBoxAdd(waterQualityDropdown,"Full Resolution",waterFull);
        waterQualityDropdown->setItemSelectState(defaults->waterQuality,true);

        CEGUI::Combobox *shadowResolutionDropdown = (CEGUI::Combobox*)graphicsPlane->getChild("ShadowResolutionDropdown");
        dropBoxAdd(shadowResolutionDropdown,"Shadows Off",shadowsOff);
        dropBoxAdd(shadowResolutionDropdown,"Shadows 1k",shadow1k);
        dropBoxAdd(shadowResolutionDropdown,"Shadows 2k",shadow2k);
        dropBoxAdd(shadowResolutionDropdown,"Shadows 4k",shadow4k);
        shadowResolutionDropdown->setItemSelectState(defaults->shadowResolution,true);

        CEGUI::Combobox *shadowSoftnessDropdown = (CEGUI::Combobox*)graphicsPlane->getChild("ShadowSoftnessDropdown");
        dropBoxAdd(shadowSoftnessDropdown,"Hard Shadows",shadowsHard);
        dropBoxAdd(shadowSoftnessDropdown,"2x PCF",shadows2PCF);
        dropBoxAdd(shadowSoftnessDropdown,"3x PCF",shadows3PCF);
        dropBoxAdd(shadowSoftnessDropdown,"5x PCF",shadows5PCF);
        shadowSoftnessDropdown->setItemSelectState(defaults->shadowSoftness,true);

        CEGUI::Combobox *godRayQualityDropdown = (CEGUI::Combobox*)graphicsPlane->getChild("GodRayQualityDropdown");
        dropBoxAdd(godRayQualityDropdown,"God Rays Off",godRaysOff);
        dropBoxAdd(godRayQualityDropdown,"32 Samples",godRays32Samples);
        dropBoxAdd(godRayQualityDropdown,"64 Samples",godRays64Samples);
        dropBoxAdd(godRayQualityDropdown,"96 Samples",godRays96Samples);
        dropBoxAdd(godRayQualityDropdown,"128 Samples",godRays128Samples);
        godRayQualityDropdown->setItemSelectState(defaults->godRayQuality,true);

        CEGUI::Combobox *spriteDensityDropdown = (CEGUI::Combobox*)graphicsPlane->getChild("SpriteDensityDropdown");
        dropBoxAdd(spriteDensityDropdown,"Sprites Off",spritesOff);
        dropBoxAdd(spriteDensityDropdown,"500x sprites",sprites500);
        dropBoxAdd(spriteDensityDropdown,"1000x sprites",sprites1000);
        dropBoxAdd(spriteDensityDropdown,"2000x sprites",sprites2000);
        dropBoxAdd(spriteDensityDropdown,"4000x sprites",sprites4000);
        spriteDensityDropdown->setItemSelectState(defaults->spriteDensity,true);

        CEGUI::Combobox *AASamplesDropdown = (CEGUI::Combobox*)graphicsPlane->getChild("AASamplesDropdown");
        dropBoxAdd(AASamplesDropdown,"No Anti-Aliasing",aaOff);
        dropBoxAdd(AASamplesDropdown,"2x Anti-Aliasing",aa2x);
        dropBoxAdd(AASamplesDropdown,"4x Anti-Aliasing",aa4x);
        dropBoxAdd(AASamplesDropdown,"8x Anti-Aliasing",aa8x);
        dropBoxAdd(AASamplesDropdown,"16x Anti-Aliasing",aa16x);
        AASamplesDropdown->setItemSelectState(defaults->antiAliasing,true);

        CEGUI::ToggleButton* vsync = (CEGUI::ToggleButton*)graphicsPlane->getChild("VSyncCheckbox");
        vsync->setSelected(defaults->vsync);

        CEGUI::ToggleButton* fullscreen = (CEGUI::ToggleButton*)graphicsPlane->getChild("FullscreenCheckbox");
        fullscreen->setSelected(defaults->fullscreen);

        CEGUI::ToggleButton* coloredShadows = (CEGUI::ToggleButton*)graphicsPlane->getChild("ColoredShadowsCheckbox");
        coloredShadows->setSelected(defaults->coloredShadows);

        CEGUI::Editbox* resolutionX = (CEGUI::Editbox*)graphicsPlane->getChild("xResEditbox");
        resolutionX->setText(std::to_string(defaults->resolutionX));

        CEGUI::Editbox* resolutionY = (CEGUI::Editbox*)graphicsPlane->getChild("yResEditbox");
        resolutionY->setText(std::to_string(defaults->resolutionY));

        CEGUI::Slider* FOVSlider = (CEGUI::Slider*)graphicsPlane->getChild("FOVSlider");
        FOVSlider->setCurrentValue(defaults->fieldOfView);
        FOVSlider->setUserData(defaults);
        FOVSlider->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&fovSliderMoved));
        graphicsPlane->getChild("FOVText")->setText(std::to_string(((CEGUI::Slider*)graphicsPlane->getChild("FOVSlider"))->getCurrentValue()) + " degrees");

        CEGUI::Slider* materialCutoffSlider = (CEGUI::Slider*)graphicsPlane->getChild("MaterialCutoff");
        materialCutoffSlider->setCurrentValue(defaults->materialCutoff);
        materialCutoffSlider->setUserData(defaults);
        materialCutoffSlider->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&materialCutoffSliderMoved));

        CEGUI::Slider* HUDOpacitySlider = (CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider");
        HUDOpacitySlider->setCurrentValue(defaults->hudOpacity);
        HUDOpacitySlider->setUserData(defaults);
        float alpha = defaults->hudOpacity;
        alpha /= 100;
        graphicsPlane->getParent()->setAlpha(alpha);
        HUDOpacitySlider->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&hudOpacitySliderMoved));
        graphicsPlane->getChild("OpacityText")->setText(std::to_string(((CEGUI::Slider*)graphicsPlane->getChild("HUDOpacitySlider"))->getCurrentValue()) + "%");

        //Non tab specific buttons:

        optionsWindow->getChild("ApplyButton")->setUserData(defaults);
        optionsWindow->getChild("ApplyButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&applyButton));

        optionsWindow->getChild("GraphicsButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&graphicsTab));
        optionsWindow->getChild("ControlsButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&controlsTab));
        optionsWindow->getChild("AudioButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&audioTab));

        //Controls tab stuff:

        CEGUI::Slider* mouseSlider = (CEGUI::Slider*)controlsPlane->getChild("MouseSlider");
        mouseSlider->setCurrentValue(defaults->mouseSensitivity);
        mouseSlider->setUserData(defaults);
        mouseSlider->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&mouseSliderMoved));
        controlsPlane->getChild("MouseText")->setText(std::to_string(((CEGUI::Slider*)controlsPlane->getChild("MouseSlider"))->getCurrentValue()) + "%");

        CEGUI::ToggleButton* invertY = (CEGUI::ToggleButton*)controlsPlane->getChild("InvertMouseCheckbox");
        invertY->setSelected(defaults->invertMouseY);

        controlsPlane->getChild("NormalBuild")->setUserData(&keybinds);
        controlsPlane->getChild("NormalBuild")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&buildNormalBinds));
        controlsPlane->getChild("LaptopBuild")->setUserData(&keybinds);
        controlsPlane->getChild("LaptopBuild")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&buildLaptopBinds));

        CEGUI::MultiColumnList* keyBindsList = (CEGUI::MultiColumnList*)controlsPlane->getChild("KeyBindsList");
        keyBindsList->setUserData(&keybinds);
        keyBindsList->addColumn("Action",0,CEGUI::UDim(0.5,0));
        keyBindsList->addColumn("Key",1,CEGUI::UDim(0.4,0.4));
        for(int a = toggleMouseLock; a < endOfCommandEnum; a++)
        {
            keyBindsList->addRow(a);

            CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem(inputCommandToString((inputCommand)a),a);
            cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
            //cell->setSelectionColours(CEGUI::Colour(0.3,0,1,1));
            cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
            keyBindsList->setItem(cell,0,a-1);

            std::string keyName = SDL_GetScancodeName(keybinds.getBoundKey((inputCommand)a));
            cell = new CEGUI::ListboxTextItem(keyName,a);
            cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
            //cell->setSelectionColours(CEGUI::Colour(0.3,0,0,1));
            cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
            keyBindsList->setItem(cell,1,a-1);
        }
        controlsPlane->getChild("BindKeyButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&rebindKey));

        //Audio tab stuff:

        CEGUI::Slider *masterSlider = (CEGUI::Slider*)audioPlane->getChild("MasterSlider");
        masterSlider->setCurrentValue(defaults->masterVolume);
        masterSlider->setUserData(defaults);
        masterSlider->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&masterSliderMoved));

        CEGUI::Slider *musicSlider = (CEGUI::Slider*)audioPlane->getChild("MusicSlider");
        musicSlider->setCurrentValue(defaults->musicVolume);
        musicSlider->setUserData(defaults);
        musicSlider->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&musicSliderMoved));

        return optionsWindow;
    }
}













