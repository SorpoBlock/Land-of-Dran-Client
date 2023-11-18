#include "escape.h"

namespace syj
{
    bool closeEscapeMenu(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *escapeMenu = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("EscapeMenu");
        escapeMenu->setVisible(false);

        return true;
    }

    bool exitToWindowsButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *escapeMenu = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("EscapeMenu");
        clientStuff *clientEnvironment = (clientStuff*)escapeMenu->getUserData();
        clientEnvironment->exitToWindows = true;

        return true;
    }

    bool playerListButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *escapeMenu = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("EscapeMenu");
        clientStuff *clientEnvironment = (clientStuff*)escapeMenu->getUserData();
        clientEnvironment->playerList->setVisible(true);
        clientEnvironment->playerList->moveToFront();

        return true;
    }

    bool escapeAvatarButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *escapeMenu = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("EscapeMenu");
        clientStuff *clientEnvironment = (clientStuff*)escapeMenu->getUserData();
        //uniformsHolder *basicShader = (uniformsHolder*)escapeMenu->getChild("AvatarPrefs")->getUserData();

        bool isMouseLocked = clientEnvironment->context->getMouseLocked();
        clientEnvironment->context->setMouseLock(false);

        clientEnvironment->palette->window->setVisible(false);
        clientEnvironment->inventoryBox->setVisible(false);
        clientEnvironment->evalWindow->setVisible(false);
        clientEnvironment->wheelWrench->setVisible(false);
        clientEnvironment->wrench->setVisible(false);
        clientEnvironment->playerList->setVisible(false);
        CEGUI::Window *saveLoadWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("SaveLoad");
        saveLoadWindow->setVisible(false);
        clientEnvironment->brickSelector->setVisible(false);
        escapeMenu->setVisible(false);
        CEGUI::Window *optionsWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Options");
        optionsWindow->setVisible(false);

        CEGUI::Window *brickPopup = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("HUD/BrickPopup");
        CEGUI::UVector2 oldPos = brickPopup->getPosition();
        CEGUI::UDim x = oldPos.d_x;
        CEGUI::UDim y = oldPos.d_y;
        y.d_scale = 100;
        brickPopup->setPosition(CEGUI::UVector2(x,y));

        clientEnvironment->picker->picking = true;
        clientEnvironment->picker->runPickCycle(clientEnvironment->context,clientEnvironment->instancedShader,clientEnvironment->nonInstancedShader,clientEnvironment->serverData->connection,clientEnvironment->prefs);
        clientEnvironment->context->setMouseLock(isMouseLocked);

        for(unsigned int a = 0; a<clientEnvironment->serverData->newDynamics.size(); a++)
        {
            clientEnvironment->serverData->newDynamics[a]->modelInterpolator.keyFrames.clear();
        }

        clientEnvironment->palette->window->setVisible(true);

        return true;
    }

    bool escapeOptionsButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *options = root->getChild("Options");
        options->setVisible(true);
        options->moveToFront();

        return true;
    }

    bool escapeAdminButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *adminWindow = root->getChild("HUD/EvalWindow");
        adminWindow->setVisible(true);
        adminWindow->moveToFront();

        return true;
    }

    bool brickSelectorButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *brickSelector = root->getChild("BrickSelector");
        brickSelector->setVisible(true);
        brickSelector->moveToFront();

        return true;
    }

    bool saveLoadButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
        saveLoadWindow->setVisible(true);
        saveLoadWindow->moveToFront();

        return true;
    }

    CEGUI::Window *addEscapeMenu(clientStuff *clientEnvironment,uniformsHolder *basicShader)
    {
        CEGUI::Window *escapeMenu = addGUIFromFile("escapeMenu.layout");
        escapeMenu->setUserData(clientEnvironment);
        escapeMenu->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeEscapeMenu));

        escapeMenu->getChild("ExitToWindows/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&exitToWindowsButton));
        escapeMenu->getChild("Options/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&escapeOptionsButton));
        escapeMenu->getChild("Admin/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&escapeAdminButton));
        escapeMenu->getChild("Brick/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&brickSelectorButton));
        escapeMenu->getChild("SaveLoad/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&saveLoadButton));
        escapeMenu->getChild("AvatarPrefs/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&escapeAvatarButton));
        escapeMenu->getChild("AvatarPrefs")->setUserData(basicShader);
        escapeMenu->getChild("PlayerList/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&playerListButton));

        return escapeMenu;
    }
}
