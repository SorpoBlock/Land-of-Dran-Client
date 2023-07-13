#include "hud.h"

namespace syj
{
    bool sendChat()
    {
        CEGUI::Window *chat = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("HUD/Chat");
        client *connection = (client*)chat->getChild("Editbox")->getUserData();

        if(!connection)
            return false;

        std::string message = chat->getChild("Editbox")->getText().c_str();

        chat->getChild("Editbox")->setText("");
        chat->deactivate();
        chat->moveToBack();
        chat->setMousePassThroughEnabled(true);

        if(message.length() < 1)
            return false;

        packet data;
        data.writeUInt(12,4);
        data.writeString(message);
        connection->send(&data,true);

        return true;
    }

    bool messageBoxAccept(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *messageBox = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("MessageBox");
        int *status = (int*)messageBox->getUserData();
        if(status)
            status[0] = 0;
        messageBox->setVisible(false);
        return true;
    }

    void notify(std::string windowName,std::string windowText,std::string buttonText)
    {
        CEGUI::Window *messageBox = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("MessageBox");
        int *status = (int*)messageBox->getUserData();
        if(status)
            status[0] = 1;
        messageBox->setText(windowName);
        messageBox->getChild("MessageText")->setText(windowText);
        messageBox->getChild("MessageButton")->setText(buttonText);
        messageBox->setVisible(true);
    }

    void setBrickLoadProgress(float progress,int bricks)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *brickLoadingWindow = root->getChild("HUD/LoadingBricks");
        brickLoadingWindow->setVisible(progress < 0.99);
        CEGUI::ProgressBar *loadingProgress = (CEGUI::ProgressBar*)brickLoadingWindow->getChild("ProgressBar");
        loadingProgress->setSize(CEGUI::USize(CEGUI::UDim(progress,0),CEGUI::UDim(0.43,0)));
        brickLoadingWindow->getChild("StaticText")->setText("Brick count: " + std::to_string(bricks));
    }

    bool closeBrickLoadingBar(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *brickLoadingBar = root->getChild("HUD/LoadingBricks");
        brickLoadingBar->setVisible(false);

        return true;
    }

    bool closePlayersList(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *playerList = root->getChild("HUD/PlayerList");
        playerList->setVisible(false);

        return true;
    }

    CEGUI::Window *initHud(CEGUI::Window *hud)
    {
        int *messageBoxStatus = new int[1];
        messageBoxStatus[0] = 0;

        CEGUI::Window *brickLoadingWindow = hud->getChild("LoadingBricks");
        brickLoadingWindow->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeBrickLoadingBar));
        CEGUI::Window *messageBox = addGUIFromFile("messageBox.layout");
        messageBox->setUserData((int*)messageBoxStatus);
        messageBox->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&messageBoxAccept));
        hud->getChild("PlayerList")->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closePlayersList));
        ((CEGUI::MultiColumnList*)hud->getChild("PlayerList/List"))->addColumn("Name",0,CEGUI::UDim(1,0));
        messageBox->getChild("MessageButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&messageBoxAccept));
        hud->getChild("Chat/Editbox")->subscribeEvent(CEGUI::Editbox::EventTextAccepted,CEGUI::Event::Subscriber(&sendChat));
        return messageBox;
    }

    void textMessages::checkForTimeouts()
    {
        if(textExpiry < SDL_GetTicks())
        {
            textBar->setText("");
            return;
        }
    }

    void textMessages::setText(std::string text,int timeoutMS)
    {
        textExpiry = SDL_GetTicks() + timeoutMS;
        textBar->setText(text);
    }
}
