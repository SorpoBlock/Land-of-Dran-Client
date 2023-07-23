#include "avatarPicker.h"

namespace syj
{
    void colorSliderMoved()
    {
        CEGUI::Window *colorPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("ColorPicker");
        avatarPicker *picker = (avatarPicker*)colorPicker->getUserData();
        if(!picker)
            return;
        if(picker->sliderEventProtection)
            return;
        if(picker->settingColorFor == -1)
            return;

        picker->nodeColors[picker->settingColorFor].r = ((CEGUI::Slider*)colorPicker->getChild("RedSlider"))->getCurrentValue();
        picker->nodeColors[picker->settingColorFor].g = ((CEGUI::Slider*)colorPicker->getChild("GreenSlider"))->getCurrentValue();
        picker->nodeColors[picker->settingColorFor].b = ((CEGUI::Slider*)colorPicker->getChild("BlueSlider"))->getCurrentValue();
        glm::vec3 color = picker->nodeColors[picker->settingColorFor];
        colorPicker->getChild("Result")->setProperty("SwatchColour",
                            "FF" + charToHex(color.r*255) + charToHex(color.g*255) + charToHex(color.b*255));
    }

    bool buyDecal(const CEGUI::EventArgs &e)
    {
        const CEGUI::WindowEventArgs& wea = static_cast<const CEGUI::WindowEventArgs&>(e);
        CEGUI::Window *button = wea.window;
        if(!button->getUserData())
        {
            error("No user data for buy decal button!");
            return false;
        }
        int decalID = ((int*)button->getUserData())[0];

        CEGUI::Window *parent = button->getParent()->getParent()->getParent();

        avatarPicker *picker = (avatarPicker*)parent->getUserData();
        if(!picker)
            return false;

        if(decalID < 0 || decalID >= picker->faceDecals.size())
        {
            error("Decal ID " + std::to_string(decalID) + " out of range!");
            return false;
        }
        picker->chosenDecal = decalID;

        CEGUI::Window *decalPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("DecalPicker");
        decalPicker->setVisible(false);
        decalPicker->setAlwaysOnTop(false);

        return true;
    }

    void colorAccept()
    {
        CEGUI::Window *colorPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("ColorPicker");
        avatarPicker *picker = (avatarPicker*)colorPicker->getUserData();
        if(!picker)
            return;
        if(picker->settingColorFor == -1)
            return;

        colorPicker->setVisible(false);
        colorPicker->setAlwaysOnTop(false);
    }

    void colorRevert()
    {
        CEGUI::Window *colorPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("ColorPicker");
        avatarPicker *picker = (avatarPicker*)colorPicker->getUserData();
        if(!picker)
            return;
        if(picker->settingColorFor == -1)
            return;

        picker->nodeColors[picker->settingColorFor] = picker->preNodeColors[picker->settingColorFor];
        colorPicker->setVisible(false);
        colorPicker->setAlwaysOnTop(false);
    }

    void decalRevert()
    {
        CEGUI::Window *decalPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("DecalPicker");
        avatarPicker *picker = (avatarPicker*)decalPicker->getUserData();
        if(!picker)
            return;
        if(picker->settingColorFor == -1)
            return;

        picker->chosenDecal = picker->preChosenDecal;

        decalPicker->setVisible(false);
        decalPicker->setAlwaysOnTop(false);
    }

    void revertAll()
    {
        CEGUI::Window *colorPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("ColorPicker");
        avatarPicker *picker = (avatarPicker*)colorPicker->getUserData();
        if(!picker)
            return;

        picker->nodeColors.clear();
        for(int a = 0; a<picker->preNodeColors.size(); a++)
            picker->nodeColors.push_back(picker->preNodeColors[a]);

        picker->chosenDecal = picker->preChosenDecal;

        colorPicker->setVisible(false);
        colorPicker->setAlwaysOnTop(false);
        picker->buttons->setVisible(false);
        picker->decalPicker->setVisible(false);
        picker->decalPicker->setAlwaysOnTop(false);
        picker->picking = false;
    }

    void applyAll()
    {
        CEGUI::Window *colorPicker = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("ColorPicker");
        avatarPicker *picker = (avatarPicker*)colorPicker->getUserData();
        if(!picker)
            return;

        picker->sendAvatarPrefs(picker->tmpClient,picker->tmpPrefs);
        colorPicker->setVisible(false);
        colorPicker->setAlwaysOnTop(false);
        picker->buttons->setVisible(false);
        picker->decalPicker->setVisible(false);
        picker->decalPicker->setAlwaysOnTop(false);
        picker->picking = false;
    }

    void avatarPicker::loadFromPrefs(preferenceFile *prefs)
    {
        nodeColors.clear();

        if(!playerModel)
            return;

        preference *tmp = prefs->getPreference("FACEDECAL");
        if(tmp)
        {
            std::string decalName = tmp->toString();
            for(int a = 0; a<faceDecalFilepaths.size(); a++)
            {
                if(faceDecalFilepaths[a] == decalName)
                {
                    chosenDecal = a;
                    break;
                }
            }
        }

        for(int a = 0; a<playerModel->meshes.size(); a++)
            nodeColors.push_back(glm::vec3(0,0,0));
        for(int a = 0; a<playerModel->meshes.size(); a++)
        {
            int id = playerModel->meshes[a]->pickingID;
            if(id < 0 || id >= nodeColors.size())
                continue;

            preference *tmp = prefs->getPreference(playerModel->meshes[a]->name + "_RED");
            if(tmp)
            {
                nodeColors[id].r = tmp->toInteger();
                nodeColors[id].r /= 255.0;
            }
            tmp = prefs->getPreference(playerModel->meshes[a]->name + "_GREEN");
            if(tmp)
            {
                nodeColors[id].g = tmp->toInteger();
                nodeColors[id].g /= 255.0;
            }
            tmp = prefs->getPreference(playerModel->meshes[a]->name + "_BLUE");
            if(tmp)
            {
                nodeColors[id].b = tmp->toInteger();
                nodeColors[id].b /= 255.0;
            }
        }
    }

    void avatarPicker::sendAvatarPrefs(client *connection,preferenceFile *prefs)
    {
        if(nodeColors.size() < 1)
            return;

        packet data;
        data.writeUInt(13,4);

        data.writeUInt(chosenDecal,7);

        if(chosenDecal >= 0 && chosenDecal < faceDecalFilepaths.size() && prefs)
        {
            std::cout<<"Setting file path for face decal: "<<chosenDecal<<" "<<faceDecalFilepaths[chosenDecal]<<"\n";
            if(prefs->getPreference("FACEDECAL"))
                prefs->set("FACEDECAL",faceDecalFilepaths[chosenDecal]);
            else
                prefs->addStringPreference("FACEDECAL",faceDecalFilepaths[chosenDecal]);
        }


        data.writeUInt(nodeColors.size(),8);
        for(int a = 0; a<nodeColors.size(); a++)
        {
            int mesh = -1;
            for(int b = 0; b<playerModel->meshes.size(); b++)
            {
                if(playerModel->meshes[b]->pickingID == a)
                {
                    mesh = b;
                    break;
                }
            }

            if(mesh != -1)
            {
                if(prefs)
                {
                    if(prefs->getPreference(playerModel->meshes[mesh]->name + "_RED"))
                        prefs->set(playerModel->meshes[mesh]->name + "_RED",(int)(nodeColors[a].r*255));
                    else
                        prefs->addIntegerPreference(playerModel->meshes[mesh]->name + "_RED",(int)(nodeColors[a].r*255));

                    if(prefs->getPreference(playerModel->meshes[mesh]->name + "_GREEN"))
                        prefs->set(playerModel->meshes[mesh]->name + "_GREEN",(int)(nodeColors[a].g*255));
                    else
                        prefs->addIntegerPreference(playerModel->meshes[mesh]->name + "_GREEN",(int)(nodeColors[a].g*255));

                    if(prefs->getPreference(playerModel->meshes[mesh]->name + "_BLUE"))
                        prefs->set(playerModel->meshes[mesh]->name + "_BLUE",(int)(nodeColors[a].b*255));
                    else
                        prefs->addIntegerPreference(playerModel->meshes[mesh]->name + "_BLUE",(int)(nodeColors[a].b*255));
                }
                data.writeString(playerModel->meshes[mesh]->name);
            }
            else
            {
                data.writeString("error");
                error("Could not find player model mesh w/ picking ID " + std::to_string(a));
            }

            data.writeUInt(nodeColors[a].r*255,8);
            data.writeUInt(nodeColors[a].g*255,8);
            data.writeUInt(nodeColors[a].b*255,8);
        }
        if(connection)
            connection->send(&data,true);

        if(prefs)
            prefs->exportToFile("config.txt");
    }

    void avatarPicker::addDecalToPicker(std::string fileName)
    {
        texture *face = new texture;
        face->createFromFile("assets/decals/" + fileName);
        faceDecals.push_back(face);
        faceDecalFilepaths.push_back(fileName);

        CEGUI::ScrolledContainer *box = (CEGUI::ScrolledContainer *)((CEGUI::ScrollablePane*)decalPicker->getChild("Decals"))->getContentPane();

        const int columns = 3;
        int numBricks = box->getChildCount();
        float column = numBricks % columns;
        float row = numBricks / columns;
        float xOffset = column / ((float)columns);
        float yOffset = row / 2.0;
        float xSize = (1.0 / ((float)columns));

        CEGUI::Window *button = CEGUI::WindowManager::getSingleton().createWindow("GWEN/Button",fileName);
        button->setVisible(true);
        button->setArea(CEGUI::UDim(xOffset,0),CEGUI::UDim(yOffset,0),CEGUI::UDim(xSize,0),CEGUI::UDim(0.5,0));
        box->addChild(button);

        CEGUI::ImageManager::getSingleton().addFromImageFile(fileName, "assets/decals/" + fileName,"/");
        CEGUI::Window *image = CEGUI::WindowManager::getSingleton().createWindow("GWEN/StaticImage","Image");
        image->setProperty("Image",fileName);
        image->setMousePassThroughEnabled(true);
        button->addChild(image);

        int *data = new int[1];
        data[0] = numBricks;
        button->setUserData(data);
        image->setUserData(data);
        button->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&buyDecal));
    }

    avatarPicker::avatarPicker(int resX,int resY)
    {
        decalPicker = addGUIFromFile("facePicker.layout");
        decalPicker->setUserData(this);
        colorPicker = addGUIFromFile("colorPicker.layout");
        buttons = addGUIFromFile("avatarPickerButtons.layout");
        colorPicker->setUserData(this);
        buttons->setUserData(this);

        colorPicker->getChild("RedSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&colorSliderMoved));
        colorPicker->getChild("BlueSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&colorSliderMoved));
        colorPicker->getChild("GreenSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&colorSliderMoved));
        colorPicker->getChild("Accept")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&colorAccept));
        colorPicker->getChild("Revert")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&colorRevert));
        buttons->getChild("Apply")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&applyAll));
        buttons->getChild("Cancel")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&revertAll));
        decalPicker->getChild("Cancel")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&decalRevert));

        renderTarget::renderTargetSettings settings;
        settings.useColor = true;
        settings.resX = resX;
        settings.resY = resY;
        settings.numColorChannels = 1;
        settings.clearColor = glm::vec4(1,1,1,1);
        pickingTexture = new renderTarget(settings);

        float x = resX;
        float y = resY;

        pickingCamera.setAspectRatio(x/y);
        pickingCamera.setFieldOfVision(90);
        pickingCamera.setNearPlane(0.1);
        pickingCamera.setFarPlane(100);
        pickingCamera.setPosition(glm::vec3(0,3,4.3));
        pickingCamera.setDirection(glm::vec3(0,0,-1));
        pickingCamera.name = "Player";
    }

    void avatarPicker::runPickCycle(renderContext *context,uniformsHolder *graphics,client *connection,preferenceFile *prefs)
    {
        //Sometimes the decals haven't loaded by the time the other types have so put this here too
        preference *tmp = prefs->getPreference("FACEDECAL");
        if(tmp)
        {
            std::string decalName = tmp->toString();
            for(int a = 0; a<faceDecalFilepaths.size(); a++)
            {
                if(faceDecalFilepaths[a] == decalName)
                {
                    chosenDecal = a;
                    break;
                }
            }
        }

        tmpClient = connection;
        tmpPrefs = prefs;
        buttons->setVisible(true);
        buttons->moveToBack();

        preNodeColors.clear();
        for(int a = 0; a<nodeColors.size(); a++)
        {
            preNodeColors.push_back(nodeColors[a]);
        }
        preChosenDecal = chosenDecal;

        glDisable(GL_CULL_FACE);

        int dummyControlPacket = SDL_GetTicks();
        int selectedMesh = 0;

        while(picking)
        {
            const Uint8 *states = SDL_GetKeyboardState(NULL);
            int mx,my;
            Uint32 mouseState = SDL_GetMouseState(&mx,&my);
            my = pickingTexture->settings.resY - my;

            SDL_Event event;
            while(SDL_PollEvent(&event))
            {
                processEventsCEGUI(event,states);

                if(event.type == SDL_QUIT)
                {
                    picking = false;
                    colorPicker->setAlwaysOnTop(false);
                    colorPicker->setVisible(false);
                    buttons->setVisible(false);
                    decalPicker->setVisible(false);
                    decalPicker->setAlwaysOnTop(false);
                    break;
                }

                if(event.type == SDL_KEYDOWN)
                {
                    if(event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        if(colorPicker->isVisible())
                            colorRevert();
                        else if(decalPicker->isVisible())
                            decalRevert();
                        else
                            revertAll();
                        break;
                    }
                }

                if(event.type == SDL_MOUSEBUTTONDOWN)
                {
                    if(selectedMesh >= 0 && selectedMesh < nodeColors.size() && !colorPicker->isVisible() && !decalPicker->isVisible())
                    {
                        /*float r = rand() % 1000;
                        r /= 1000.0;
                        float g = rand() % 1000;
                        g /= 1000.0;
                        float b = rand() % 1000;
                        b /= 1000.0;
                        nodeColors[selectedMesh] = glm::vec3(r,g,b);*/
                        settingColorFor = selectedMesh;

                        bool isFace = false;

                        for(int a = 0; a<playerModel->meshes.size(); a++)
                        {
                            if(playerModel->meshes[a]->pickingID == settingColorFor)
                            {
                                if(playerModel->meshes[a]->name == "Face1")
                                {
                                    isFace = true;
                                }
                                break;
                            }
                        }

                        if(isFace)
                        {
                            /*glm::vec3 faceColor = glm::vec3(1,1,1);
                            for(int a = 0; a<playerModel->meshes.size(); a++)
                            {
                                if(playerModel->meshes[a]->name == "Head")
                                {
                                    if(playerModel->meshes[a]->pickingID >= 0 && playerModel->meshes[a]->pickingID < )
                                    break;
                                }
                            }*/
                            sliderEventProtection = true;
                            CEGUI::Slider *red = (CEGUI::Slider *)colorPicker->getChild("RedSlider");
                            CEGUI::Slider *green = (CEGUI::Slider *)colorPicker->getChild("GreenSlider");
                            CEGUI::Slider *blue = (CEGUI::Slider *)colorPicker->getChild("BlueSlider");
                            red->setCurrentValue(nodeColors[selectedMesh].r);
                            green->setCurrentValue(nodeColors[selectedMesh].g);
                            blue->setCurrentValue(nodeColors[selectedMesh].b);
                            colorPicker->getChild("Result")->setProperty("SwatchColour",
                                                                "FF" + charToHex(nodeColors[selectedMesh].r*255) + charToHex(nodeColors[selectedMesh].g*255) + charToHex(nodeColors[selectedMesh].b*255));

                            sliderEventProtection = false;
                            colorPicker->setVisible(true);
                            colorPicker->moveToFront();
                            colorPicker->setAlwaysOnTop(true);

                            decalPicker->setVisible(true);
                            decalPicker->moveToFront();
                            decalPicker->setAlwaysOnTop(true);
                        }
                        else
                        {
                            sliderEventProtection = true;
                            CEGUI::Slider *red = (CEGUI::Slider *)colorPicker->getChild("RedSlider");
                            CEGUI::Slider *green = (CEGUI::Slider *)colorPicker->getChild("GreenSlider");
                            CEGUI::Slider *blue = (CEGUI::Slider *)colorPicker->getChild("BlueSlider");
                            red->setCurrentValue(nodeColors[selectedMesh].r);
                            green->setCurrentValue(nodeColors[selectedMesh].g);
                            blue->setCurrentValue(nodeColors[selectedMesh].b);
                            colorPicker->getChild("Result")->setProperty("SwatchColour",
                                                                "FF" + charToHex(nodeColors[selectedMesh].r*255) + charToHex(nodeColors[selectedMesh].g*255) + charToHex(nodeColors[selectedMesh].b*255));

                            sliderEventProtection = false;
                            colorPicker->setVisible(true);
                            colorPicker->moveToFront();
                            colorPicker->setAlwaysOnTop(true);
                        }
                    }
                }

                if(event.type == SDL_MOUSEMOTION && !colorPicker->isVisible())
                {
                    if(mouseState & SDL_BUTTON_LEFT)
                    {
                        glm::vec2 res = context->getResolution();
                        float x = (((float)event.motion.xrel)/res.x);
                        float y = (((float)event.motion.yrel)/res.y);
                        x *= ((float)150) / 100.0;
                        y *= ((float)150) / 100.0;

                        modelYaw += x;
                        modelPitch += y;

                        while(modelYaw > 6.28)
                            modelYaw -= 6.28;
                        while(modelPitch > 6.28)
                            modelPitch -= 6.28;
                        while(modelYaw < 6.28)
                            modelYaw += 6.28;
                        while(modelPitch < 6.28)
                            modelPitch += 6.28;
                    }
                }
            }

            if(connection && (dummyControlPacket + 200 < SDL_GetTicks()))
            {
                dummyControlPacket = SDL_GetTicks();

                packet transPacket;
                transPacket.writeUInt(2,4);
                transPacket.writeUInt(0,5);
                transPacket.writeBit(false);
                transPacket.writeBit(false);
                transPacket.writeBit(false);
                transPacket.writeFloat(0);
                transPacket.writeFloat(0);
                transPacket.writeFloat(0);
                transPacket.writeUInt(7,3);
                transPacket.writeBit(false);
                transPacket.writeBit(false);
                transPacket.writeBit(false);

                connection->send(&transPacket,false);

                connection->run();
            }

            glm::mat4 rotMatrix = glm::rotate(modelYaw,glm::vec3(0,1,0));

            pickingTexture->bind();
            graphics->target->use();
                pickingCamera.render(*graphics);
                playerModel->renderForPicking(graphics,rotMatrix * glm::scale(glm::vec3(0.02)));

            glFlush();
            glFinish();

            pickingTexture->unbind();

            if(mx >= 0 && mx < pickingTexture->settings.resX && my >= 0 && my < pickingTexture->settings.resY)
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER,pickingTexture->frameBuffer);
                glReadBuffer(GL_COLOR_ATTACHMENT0);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                unsigned char data[1];
                glReadPixels(mx,my,1,1, GL_RED, GL_UNSIGNED_BYTE, data);
                selectedMesh = data[0];

                glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
            }

            context->clear(0.5,0.5,0.5);
            context->select();

            graphics->target->use();
                pickingCamera.render(*graphics);
                glUniform1i(graphics->target->getUniformLocation("selectedMesh"),selectedMesh);
                playerModel->render(graphics,rotMatrix * glm::scale(glm::vec3(0.02)),false,&nodeColors,faceDecals.size() > 0 ? faceDecals[chosenDecal] : 0);

            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            CEGUI::System::getSingleton().renderAllGUIContexts();
            context->swap();
            glEnable(GL_DEPTH_TEST);
        }

        glEnable(GL_CULL_FACE);
    }
}
