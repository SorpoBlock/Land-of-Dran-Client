#include "joinServer.h"

namespace syj
{
    //This is called with a dummy/empty event so don't use the argument
    bool playerColorSlider(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
        unsigned char r = ((CEGUI::Slider*)joinServerWindow->getChild("RedSlider"))->getCurrentValue();
        unsigned char g = ((CEGUI::Slider*)joinServerWindow->getChild("GreenSlider"))->getCurrentValue();
        unsigned char b = ((CEGUI::Slider*)joinServerWindow->getChild("BlueSlider"))->getCurrentValue();
        joinServerWindow->getChild("ColorResult")->setProperty("SwatchColour","FF" + charToHex(r) + charToHex(g) + charToHex(b));
        return true;
    }

    bool optionsButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *optionsWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Options");
        optionsWindow->setVisible(true);
    }

    bool connectToServer(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
        std::string ip = joinServerWindow->getChild("IPBox")->getText().c_str();
        std::string name = joinServerWindow->getChild("UsernameBox")->getText().c_str();
        if(ip.length() < 1)
            return true;
        if(name.length() < 1)
            return true;
        serverStuff *ohWow = (serverStuff*)joinServerWindow->getUserData();
        if(!ohWow)
            return true;
        ohWow->prefs->set("Name",name);
        ohWow->prefs->set("IP",ip);
        joinServerWindow->getChild("StatusText")->setText("Connecting...");

        ohWow->wantedIP = ip;
        ohWow->wantedName = name;
        ohWow->waitingToPickServer = false;

        unsigned char r = ((CEGUI::Slider*)joinServerWindow->getChild("RedSlider"))->getCurrentValue();
        unsigned char g = ((CEGUI::Slider*)joinServerWindow->getChild("GreenSlider"))->getCurrentValue();
        unsigned char b = ((CEGUI::Slider*)joinServerWindow->getChild("BlueSlider"))->getCurrentValue();
        ohWow->prefs->set("PlayerColorRed",r);
        ohWow->prefs->set("PlayerColorGreen",g);
        ohWow->prefs->set("PlayerColorBlue",b);
        ohWow->wantedColor = glm::vec3(r,g,b);
        ohWow->wantedColor /= glm::vec3(255);

        ohWow->prefs->exportToFile("config.txt");

        return true;
    }

    CEGUI::Window *loadJoinServer(serverStuff *ohWow)
    {
        CEGUI::Window *joinServerWindow = addGUIFromFile("joinServer.layout");
        joinServerWindow->setUserData(ohWow);
        if(ohWow->prefs->getPreference("Name"))
            joinServerWindow->getChild("UsernameBox")->setText(ohWow->prefs->getPreference("Name")->toString());
        if(ohWow->prefs->getPreference("IP"))
            joinServerWindow->getChild("IPBox")->setText(ohWow->prefs->getPreference("IP")->toString());

        if(ohWow->prefs->getPreference("PlayerColorRed"))
            ((CEGUI::Slider*)joinServerWindow->getChild("RedSlider"))->setCurrentValue(ohWow->prefs->getPreference("PlayerColorRed")->toInteger());
        if(ohWow->prefs->getPreference("PlayerColorGreen"))
            ((CEGUI::Slider*)joinServerWindow->getChild("GreenSlider"))->setCurrentValue(ohWow->prefs->getPreference("PlayerColorGreen")->toInteger());
        if(ohWow->prefs->getPreference("PlayerColorBlue"))
            ((CEGUI::Slider*)joinServerWindow->getChild("BlueSlider"))->setCurrentValue(ohWow->prefs->getPreference("PlayerColorBlue")->toInteger());

        joinServerWindow->getChild("ConnectButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&connectToServer));
        joinServerWindow->getChild("OptionsButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&optionsButton));

        joinServerWindow->getChild("RedSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));
        joinServerWindow->getChild("GreenSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));
        joinServerWindow->getChild("BlueSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));

        CEGUI::EventArgs empty;
        playerColorSlider(empty);

        return joinServerWindow;
    }
}
