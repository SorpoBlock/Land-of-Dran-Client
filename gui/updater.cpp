#include "updater.h"

namespace syj
{
    struct literallyJustTwoInts
    {
        int revision = -1;
        int network = -1;
    };

    size_t getVersionFile(void *buffer,size_t size,size_t nmemb,void *userp)
    {
        if(nmemb > 0)
        {
            std::string versions = std::string((char*)buffer).substr(0,nmemb);

            literallyJustTwoInts *data = (literallyJustTwoInts*)userp;

            std::vector<std::string> words;
            split(versions,words);

            if(words.size() > 1)
            {
                if(words[0] == "REVISION")
                    data->revision = atoi(words[1].c_str());
                else if(words[0] == "NETWORK")
                    data->network = atoi(words[1].c_str());
            }
            if(words.size() > 3)
            {
                if(words[2] == "REVISION")
                    data->revision = atoi(words[3].c_str());
                else if(words[2] == "NETWORK")
                    data->network = atoi(words[3].c_str());
            }
        }
        return nmemb;
    }

    size_t getFileListFile(void *buffer,size_t size,size_t nmemb,void *userp)
    {
        if(nmemb > 0)
        {
            std::ofstream *file = (std::ofstream*)userp;
            file->write((char*)buffer,nmemb);
        }

        return nmemb;
    }

    void getVersions(int &releaseVersion,int &networkVersion,std::string domain)
    {
        CURL *curlHandle = curl_easy_init();

        std::string url = "http://" + domain + "/repo/versions.txt";
        literallyJustTwoInts userData;

        curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
        curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,getVersionFile);
        curl_easy_setopt(curlHandle,CURLOPT_WRITEDATA,&userData);
        CURLcode res = curl_easy_perform(curlHandle);
        if(res != CURLE_OK)
        {
            releaseVersion = -1;
            networkVersion = -1;
            std::cout<<"Failed to get version info from " + domain + " master server "<<curl_easy_strerror(res)<<"\n";
        }
        else
        {
            releaseVersion = userData.revision;
            networkVersion = userData.network;
        }

        curl_easy_cleanup(curlHandle);
    }

    bool closeUpdater(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *updater = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Updater");
        updater->setVisible(false);

        return true;
    }

    void updateLog(std::string message)
    {
        CEGUI::Window *updater = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Updater");
        info(message);
        replaceAll(message,"\\","/");
        textBoxAdd(updater->getChild("Listbox"),message,0,false);
    }

    void updateError(std::string message)
    {
        CEGUI::Window *updater = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Updater");
        error(message);
        replaceAll(message,"\\","/");
        textBoxAdd(updater->getChild("Listbox"),"[Colour='FFFF0000']" + message,0,false);
    }

    bool updateButton(const CEGUI::EventArgs &e)
    {

        CEGUI::Window *updater = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Updater");

        CEGUI::Window *text = updater->getChild("StaticText");
        text->setVisible(true);
        CEGUI::Window *bar = updater->getChild("ProgressBar/ProgressBar");
        bar->setSize(CEGUI::USize(CEGUI::UDim(0,0),CEGUI::UDim(0.8275,0)));

        updateLog("Checking for updates...");

        //Todo: make configurable
        std::string url = "http://dran.land/repo/updateInfo.txt";

        std::ofstream fileListFile("filelist.tmp");
        CURL *curlHandle = curl_easy_init();
        curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
        curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,getFileListFile);
        curl_easy_setopt(curlHandle,CURLOPT_WRITEDATA,&fileListFile);
        CURLcode res = curl_easy_perform(curlHandle);
        if(res != CURLE_OK)
        {
            updateError("Could not get files list from master server.");
            fileListFile.close();
            curl_easy_cleanup(curlHandle);
            return true;
        }
        else
            updateLog("Got file list from master server.");

        fileListFile.close();

        std::ifstream fileListFileIn("filelist.tmp");

        if(!fileListFileIn.is_open())
        {
            updateError("Could not open filelist.tmp");
            curl_easy_cleanup(curlHandle);
            return true;
        }

        std::vector<std::string> pathsToDownload;

        std::string line = "";
        while(!fileListFileIn.eof())
        {
            getline(fileListFileIn,line);
            if(line == "")
                continue;
            if(line == "\n")
                continue;
            replaceAll(line,"\r","");

            if(line.find("\t") == std::string::npos)
            {
                if(!doesFileExist(line))
                {
                    updateLog("Missing " + line);
                    pathsToDownload.push_back(line);
                }
            }
            else
            {
                std::string fileName = line.substr(0,line.find("\t"));

                if(!doesFileExist(fileName))
                {
                    updateLog("Missing " + fileName);
                    pathsToDownload.push_back(fileName);
                }
                else
                {
                    std::string checksum = line.substr(line.find("\t")+1,line.length() - (line.find("\t")+1));
                    unsigned int latest = atoi(checksum.c_str());
                    unsigned int ours = getFileChecksum(fileName.c_str());
                    if(latest != ours)
                    {
                        updateLog("Checksum " + std::to_string(ours) + " does not match " + std::to_string(latest) + " for " + fileName);
                        pathsToDownload.push_back(fileName);
                    }
                }
            }
        }

        fileListFileIn.close();

        remove("filelist.tmp");

        for(int a = 0; a<pathsToDownload.size(); a++)
        {
            std::string msg = "Downloading file " + std::to_string(a) + " / " + std::to_string(pathsToDownload.size());
            text->setText(msg);
            info(msg);

            std::string asdf = pathsToDownload[a];
            replaceAll(asdf,"\\","/");
            std::string folder = getFolderFromPath(asdf.substr(1,asdf.length()-1));
            folder = folder.substr(1,folder.length()-1);
            std::filesystem::create_directories(folder.c_str());

            std::ofstream tmp(pathsToDownload[a].c_str());

            if(tmp.is_open())
            {
                std::string url = "http://dran.land/repo" + pathsToDownload[a].substr(1,pathsToDownload[a].size()-1);
                replaceAll(url,"\\","/");

                curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
                curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,getFileListFile);
                curl_easy_setopt(curlHandle,CURLOPT_WRITEDATA,&tmp);
                CURLcode res = curl_easy_perform(curlHandle);
                if(res != CURLE_OK)
                    updateError("Could not get file " + url);

                tmp.close();
            }
            else
                updateError("Could not open file " + pathsToDownload[a] + " for writing.");

        }

        curl_easy_cleanup(curlHandle);
    }

    CEGUI::Window *addUpdater()
    {
        CEGUI::Window *updater = addGUIFromFile("updater.layout");
        updater->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeUpdater));
        updater->getChild("Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&updateButton));

        return updater;
    }
}





















