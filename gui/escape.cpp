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
        serverStuff *ohWow = (serverStuff*)escapeMenu->getUserData();
        ohWow->exitToWindows = true;

        return true;
    }

    bool playerListButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *escapeMenu = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("EscapeMenu");
        serverStuff *ohWow = (serverStuff*)escapeMenu->getUserData();
        ohWow->playerList->setVisible(true);
        ohWow->playerList->moveToFront();

        return true;
    }

    bool escapeAvatarButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *escapeMenu = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("EscapeMenu");
        serverStuff *ohWow = (serverStuff*)escapeMenu->getUserData();
        uniformsHolder *basicShader = (uniformsHolder*)escapeMenu->getChild("AvatarPrefs")->getUserData();

        bool isMouseLocked = ohWow->context->getMouseLocked();
        ohWow->context->setMouseLock(false);
        //TODO: Update avatar picker code
        /*if(!ohWow->picker->playerModel)
            ohWow->picker->playerModel = ohWow->newDynamicTypes[0];*/

        ohWow->palette->window->setVisible(false);
        ohWow->inventoryBox->setVisible(false);
        ohWow->evalWindow->setVisible(false);
        ohWow->wheelWrench->setVisible(false);
        ohWow->wrench->setVisible(false);
        ohWow->playerList->setVisible(false);
        CEGUI::Window *saveLoadWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("SaveLoad");
        saveLoadWindow->setVisible(false);
        ohWow->brickSelector->setVisible(false);
        escapeMenu->setVisible(false);
        CEGUI::Window *optionsWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Options");
        optionsWindow->setVisible(false);

        ohWow->picker->picking = true;
        ohWow->picker->runPickCycle(ohWow->context,basicShader,ohWow->connection,ohWow->prefs);
        ohWow->context->setMouseLock(isMouseLocked);

        for(unsigned int a = 0; a<ohWow->newDynamics.size(); a++)
        {
            ohWow->newDynamics[a]->modelInterpolator.keyFrames.clear();
        }

        ohWow->palette->window->setVisible(true);

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

    CEGUI::Window *addEscapeMenu(serverStuff *ohWow,uniformsHolder *basicShader)
    {
        CEGUI::Window *escapeMenu = addGUIFromFile("escapeMenu.layout");
        escapeMenu->setUserData(ohWow);
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
