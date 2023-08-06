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
                statusText->setText("No account by that name. Register at https://dran.land");
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

    size_t writeServerList(void *buffer,size_t size,size_t nmemb,void *userp)
    {
        if(!userp)
            return nmemb;
        if(nmemb == 0)
            return nmemb;
        std::ofstream *file = (std::ofstream*)userp;
        file->write((char*)buffer,nmemb);
        return nmemb;
    }

    void refreshServerList()
    {
        CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
        CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList");
        serverListGUI->clearAllSelections();
        serverListGUI->resetList();

        std::ofstream serverList("serverList.tmp");
        if(!serverList.is_open())
        {
            error("Could not open serverList.tmp for writing!");
            return;
        }

        CURL *curlHandle = curl_easy_init();
        curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER , 0);
        curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST , 0);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &serverList);
        curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,writeServerList);
        std::string url = "https://dran.land/rawList.php";
        curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
        CURLcode res = curl_easy_perform(curlHandle);
        curl_easy_cleanup(curlHandle);

        serverList.close();

        std::ifstream serverListIn("serverList.tmp");
        if(!serverListIn.is_open())
        {
            error("Could not open serverList.tmp for reading!");
            return;
        }

        std::string line = "";
        while(!serverListIn.eof())
        {
            getline(serverListIn,line);
            if(line == "" || line == " ")
                continue;

            std::vector<std::string> fields;
            std::istringstream iss(line);
            std::string token;
            while(std::getline(iss,token,'\t'))
                fields.push_back(token);
            if(fields.size() != 5)
            {
                error("Malformed server list entry!");
                continue;
            }

            int idx = serverListGUI->getRowCount();
            serverListGUI->addRow(idx);

            for(int a = 0; a<5; a++)
            {
                //Mature 0-1 turns into no-yes:
                if(a == 4)
                {
                    if(fields[a] == "0")
                        fields[a] = "No";
                    else if(fields[a] == "1")
                        fields[a] = "Yes";
                }

                CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem(fields[a],idx);
                cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
                cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
                cell->setID(idx);
                serverListGUI->setItem(cell,a,idx);
            }
        }

        serverListIn.close();

        remove("serverList.tmp");
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

    bool copyServerListIP(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *joinServerWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer");
        CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList");
        CEGUI::Editbox *ipBox = (CEGUI::Editbox*)joinServerWindow->getChild("IPBox");

        if(serverListGUI->getSelectedCount() < 2)
            return true;

        CEGUI::ListboxItem *first = serverListGUI->getFirstSelectedItem();
        if(!first)
            return true;
        CEGUI::ListboxItem *second = serverListGUI->getNextSelected(first);
        if(!second)
            return true;
        std::string ip = second->getText().c_str();
        ipBox->setText(ip);

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
        joinServerWindow->getChild("Refresh")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&refreshServerList));

        joinServerWindow->getChild("ServerList")->subscribeEvent(CEGUI::MultiColumnList::EventSelectionChanged,CEGUI::Event::Subscriber(&copyServerListIP));

        joinServerWindow->getChild("RedSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));
        joinServerWindow->getChild("GreenSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));
        joinServerWindow->getChild("BlueSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&playerColorSlider));

        ((CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList"))->addColumn("Server Name",0,CEGUI::UDim(0.5,0));
        ((CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList"))->addColumn("IP Address",1,CEGUI::UDim(0.2,0));
        ((CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList"))->addColumn("Players",2,CEGUI::UDim(0.1,0));
        ((CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList"))->addColumn("Max Players",3,CEGUI::UDim(0.1,0));
        ((CEGUI::MultiColumnList*)joinServerWindow->getChild("ServerList"))->addColumn("Mature",4,CEGUI::UDim(0.09,0));

        CEGUI::EventArgs empty;
        playerColorSlider(empty);

        refreshServerList();

        return joinServerWindow;
    }
}
