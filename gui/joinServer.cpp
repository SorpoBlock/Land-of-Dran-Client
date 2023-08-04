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

    bool updaterButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *updater = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Updater");
        updater->setVisible(true);
    }

    bool optionsButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *optionsWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Options");
        optionsWindow->setVisible(true);
    }

    size_t getLoginResponse(void *buffer,size_t size,size_t nmemb,void *userp)
    {
        serverStuff *ohWow = (serverStuff*)userp;
        if(!ohWow)
            return nmemb;

        if(nmemb > 0)
        {
            CEGUI::Window *statusText = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/StatusText");

            std::string response;
            response.assign((char*)buffer,nmemb);

            int space = response.find(" ");
            if(space == std::string::npos)
            {
                statusText->setText("Malformed login server response.");
                error("Login response: " + response);
                return nmemb;
            }

            std::string beforeSpace = response.substr(0,space);

            if(beforeSpace == "NOACCOUNT")
                statusText->setText("No account by that name. Register at http://dran.land");
            else if(beforeSpace == "GOOD")
            {
                statusText->setText(std::string("Welcome, ") + std::string(CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/UsernameBox")->getText().c_str()));
                CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/ConnectButton")->setText("Join");
                std::string afterSpace = response.substr(space+1,response.length() - (space+1));
                std::cout<<"Session token: "<<afterSpace<<"\n";
                ohWow->loggedIn = true;
                ohWow->sessionToken = afterSpace;
            }
            else if(beforeSpace == "BAD")
                statusText->setText("Incorrect password!");
            else
            {
                statusText->setText("Malformed login server response.");
                error("Login response: " + response);
            }
        }

        return nmemb;
    }

    bool loginButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
        serverStuff *ohWow = (serverStuff*)joinServerWindow->getUserData();
        std::string username = joinServerWindow->getChild("UsernameBox")->getText().c_str();
        std::string password = joinServerWindow->getChild("PasswordBox")->getText().c_str();

        if(username.length() < 1 || password.length() < 1)
        {
            joinServerWindow->getChild("StatusText")->setText("You need a name and pass to login!");
            error("You need a name and pass to login!");
            return true;
        }

        ohWow->loggedName = username;

        CURL *curlHandle = curl_easy_init();
        curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER , 0);
        curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST , 0);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, ohWow);
        curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,getLoginResponse);
        std::string url = "https://dran.land/getSessionToken.php";
        std::string args = "pass=" + password + "&name=" + username;
        curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
        curl_easy_setopt(curlHandle,CURLOPT_POSTFIELDS,args.c_str());
        CURLcode res = curl_easy_perform(curlHandle);
        curl_easy_cleanup(curlHandle);
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
        joinServerWindow->getChild("UpdateButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&updaterButton));
        joinServerWindow->getChild("LoginButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&loginButton));

        joinServerWindow->getChild("RedSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));
        joinServerWindow->getChild("GreenSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));
        joinServerWindow->getChild("BlueSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));

        CEGUI::EventArgs empty;
        playerColorSlider(empty);

        return joinServerWindow;
    }
}
