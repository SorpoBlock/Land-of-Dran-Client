#include "evalWindow.h"

namespace syj
{
    bool closeEvalWindow(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *evalWindow = root->getChild("HUD/EvalWindow");
        evalWindow->setVisible(false);

        return true;
    }

    bool submitEvalString(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *evalWindow = root->getChild("HUD/EvalWindow");
        std::string code = evalWindow->getChild("Code/Editbox")->getText().c_str();
        evalWindow->getChild("Code/Editbox")->setText("");
        clientStuff *clientEnvironment = (clientStuff*)evalWindow->getUserData();

        packet data;
        data.writeUInt(7,4);
        data.writeString(clientEnvironment->serverData->lastGuessedEvalPass);
        data.writeString(code);
        clientEnvironment->serverData->connection->send(&data,true);

        return true;
    }

    bool guessLuaPassword(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *evalWindow = root->getChild("HUD/EvalWindow");
        CEGUI::Editbox *pass = (CEGUI::Editbox*)evalWindow->getChild("Login/Editbox");
        clientStuff *clientEnvironment = (clientStuff*)evalWindow->getUserData();

        std::string guess = pass->getText().c_str();
        if(guess.length() < 1)
            return true;

        unsigned char hash[32];
        br_sha256_context shaContext;
        br_sha256_init(&shaContext);
        br_sha256_update(&shaContext,guess.c_str(),guess.length());
        br_sha256_out(&shaContext,hash);
        std::string hexStr = GetHexRepresentation(hash,32);

        clientEnvironment->serverData->lastGuessedEvalPass = hexStr;

        packet data;
        data.writeUInt(6,4);
        data.writeString(hexStr);
        clientEnvironment->serverData->connection->send(&data,true);

        return true;
    }

    CEGUI::Window* configureEvalWindow(CEGUI::Window *hud,clientStuff *clientEnvironment)
    {
        CEGUI::Window *evalWindow = hud->getChild("EvalWindow");

        evalWindow->setUserData(clientEnvironment);
        evalWindow->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeEvalWindow));
        evalWindow->getChild("Login/Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&guessLuaPassword));
        evalWindow->getChild("Code/Editbox")->subscribeEvent(CEGUI::Editbox::EventTextAccepted,CEGUI::Event::Subscriber(&submitEvalString));
        evalWindow->getChild("Login/Editbox")->subscribeEvent(CEGUI::Editbox::EventTextAccepted,CEGUI::Event::Subscriber(&guessLuaPassword));
        dynamic_cast<CEGUI::Listbox*>(evalWindow->getChild("Code/Listbox"))->getVertScrollbar()->setEndLockEnabled(true);
        return evalWindow;
    }
}
