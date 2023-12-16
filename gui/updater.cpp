#include "updater.h"

namespace syj
{
    //TODO: uh, std::tuples and pairs exist lol
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

    SDL_sem *whichFileLock = 0;
    unsigned int bytesInFile = 0;

    size_t getFileListFile(void *buffer,size_t size,size_t nmemb,void *userp)
    {
        if(nmemb > 0)
        {
            std::ofstream *file = (std::ofstream*)userp;
            file->write((char*)buffer,nmemb);

            if(whichFileLock)
                SDL_SemWait(whichFileLock);
            bytesInFile += nmemb;
            if(whichFileLock)
                SDL_SemPost(whichFileLock);
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
        textBoxAdd(updater->getChild("Listbox"),"[colour='FFFF0000']" + message,0,false);
    }

    int whichFile = 0;
    bool threadWantsDie = false;

    int updateThread(void *data)
    {
        CURL *curlHandle = curl_easy_init();

        std::vector<std::string> *pathsToDownload = (std::vector<std::string>*)data;

        for(int a = 0; a<pathsToDownload->size(); a++)
        {
            std::string asdf = pathsToDownload->at(a);
            replaceAll(asdf,"\\","/");
            std::string folder = getFolderFromPath(asdf.substr(1,asdf.length()-1));
            folder = folder.substr(1,folder.length()-1);
            if(folder.length() > 0)
                std::filesystem::create_directories(folder.c_str());

            std::cout<<pathsToDownload->at(a)<<"\n";
            if(pathsToDownload->at(a) == ".\\LandOfDran.exe")
            {
                updateLog("Renaming old .exe, will be deleted on next start-up.");
                rename("LandOfDran.exe","oldlandofdran.exe");
            }

            std::ofstream tmp(pathsToDownload->at(a).c_str(),std::ios::binary);

            if(tmp.is_open())
            {
                std::string url = "http://dran.land/repo" + pathsToDownload->at(a).substr(1,pathsToDownload->at(a).size()-1);
                replaceAll(url,"\\","/");
                replaceAll(url," ","%20");

                curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
                curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,getFileListFile);
                curl_easy_setopt(curlHandle,CURLOPT_WRITEDATA,&tmp);
                CURLcode res = curl_easy_perform(curlHandle);

                //It gives CURLE_URL_MALFORMAT error when we have a space in a file name to download
                if(res != CURLE_OK)// && res != CURLE_URL_MALFORMAT)
                    updateError("Could not get file " + url + " error " + std::to_string(res));

                tmp.close();
            }
            else
                updateError("Could not open file " + pathsToDownload->at(a) + " for writing.");

            SDL_SemWait(whichFileLock);
            bytesInFile = 0;
            whichFile = a;
            bool threadWantsDieCopy = threadWantsDie;
            SDL_SemPost(whichFileLock);
            if(threadWantsDieCopy)
                break;
        }

        curl_easy_cleanup(curlHandle);

        return 0;
    }

    bool updateButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *updater = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Updater");
        updater->getChild("Button")->setDisabled(true);
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/ConnectButton")->setDisabled(true);

        CEGUI::Window *text = updater->getChild("StaticText");
        text->setVisible(true);
        CEGUI::Window *bar = updater->getChild("ProgressBar/ProgressBar");
        bar->setSize(CEGUI::USize(CEGUI::UDim(0,0),CEGUI::UDim(0.8275,0)));

        CEGUI::Window *topbar = updater->getChild("SubProgress/ProgressBar");
        topbar->setSize(CEGUI::USize(CEGUI::UDim(0,0),CEGUI::UDim(0.8275,0)));

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
            updater->getChild("Button")->setDisabled(false);
            CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/ConnectButton")->setDisabled(false);
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

        curl_easy_cleanup(curlHandle);

        std::vector<std::string> pathsToDownload;
        std::vector<unsigned int> sizes;
        unsigned int bytesInFileSafe = 0;

        std::string line = "";
        while(!fileListFileIn.eof())
        {
            lodGetLine(fileListFileIn,line);
            if(line == "")
                continue;
            if(line == "\n")
                continue;
            replaceAll(line,"\r","");

            int firstTab = line.find("\t");
            std::string fileName = line.substr(0,firstTab);
            std::string afterFirstTab = line.substr(firstTab+1,line.length()-(firstTab+1));

            if(afterFirstTab.find("\t") == std::string::npos)
            {
                if(!doesFileExist(fileName))
                {
                    updateLog("Missing " + fileName);
                    sizes.push_back(atoi(afterFirstTab.c_str()));
                    pathsToDownload.push_back(fileName);
                }
            }
            else
            {
                //std::string fileName = line.substr(0,line.find("\t"));
                std::string sizeStr = afterFirstTab.substr(0,afterFirstTab.find("\t"));

                if(!doesFileExist(fileName))
                {
                    updateLog("Missing " + fileName);
                    sizes.push_back(atoi(sizeStr.c_str()));
                    pathsToDownload.push_back(fileName);
                }
                else
                {
                    std::string checksum = afterFirstTab.substr(afterFirstTab.find("\t")+1,afterFirstTab.length() - (afterFirstTab.find("\t")+1));
                    unsigned int latest = atoi(checksum.c_str());
                    unsigned int ours = getFileChecksum(fileName.c_str());
                    if(latest != ours)
                    {
                        updateLog("Checksum " + std::to_string(ours) + " does not match " + std::to_string(latest) + " for " + fileName);
                        sizes.push_back(atoi(sizeStr.c_str()));
                        pathsToDownload.push_back(fileName);
                    }
                }
            }
        }

        fileListFileIn.close();

        remove("filelist.tmp");

        bool replacedExe = false;
        for(int a = 0; a<pathsToDownload.size(); a++)
        {
            if(pathsToDownload[a] == ".\\LandOfDran.exe")
            {
                replacedExe = true;
                break;
            }
        }

        if(pathsToDownload.size() == 0)
        {
            CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/ConnectButton")->setDisabled(false);
            updater->getChild("Button")->setDisabled(false);
            updateLog("No files need updating!");
            return true;
        }


        float hue = 0;
        float lastTick = SDL_GetTicks();
        float deltaT = 0;
        float horBounceDir = 1;
        float vertBounceDir = 1;
        int totalFiles = pathsToDownload.size();
        CEGUI::Window *bounceText = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("Bouncer");
        renderContext *context = ((clientStuff*)updater->getUserData())->context;

        whichFileLock = SDL_CreateSemaphore(1);
        SDL_Thread *thread = SDL_CreateThread(updateThread,"UpdaterThread",&pathsToDownload);

        bool cont = true;
        while(cont)
        {
            SDL_SemWait(whichFileLock);
            float progress = whichFile;
            std::string msg = "Downloading file " + std::to_string(whichFile) + " / " + std::to_string(totalFiles);
            bytesInFileSafe = bytesInFile;
            SDL_SemPost(whichFileLock);

            if(whichFile+1 == totalFiles)
                cont = false;
            text->setText(msg);

            const Uint8 *states = SDL_GetKeyboardState(NULL);
            SDL_Event e;
            while(SDL_PollEvent(&e))
            {
                if(e.type == SDL_QUIT)
                {
                    SDL_SemWait(whichFileLock);
                    threadWantsDie = true;
                    SDL_SemPost(whichFileLock);
                    cont = false;
                    break;
                }

                processEventsCEGUI(e,states);
            }
            progress /= ((float)totalFiles);

            bar->setSize(CEGUI::USize(CEGUI::UDim(progress,0),CEGUI::UDim(0.8275,0)));

            if(whichFile+1 < sizes.size())
            {
                float subProgress = bytesInFileSafe;
                subProgress /= ((float)sizes[whichFile+1]);
                topbar->setSize(CEGUI::USize(CEGUI::UDim(subProgress,0),CEGUI::UDim(0.8275,0)));
            }

            deltaT = SDL_GetTicks() - lastTick;
            lastTick = SDL_GetTicks();

            CEGUI::UVector2 pos = bounceText->getPosition();
            if(pos.d_x.d_scale > 0.75)
                horBounceDir = -1;
            if(pos.d_y.d_scale > 0.9)
                vertBounceDir = -1;

            if(pos.d_x.d_scale < -0.1)
                horBounceDir = 1;
            if(pos.d_y.d_scale < -0.05)
                vertBounceDir = 1;
            pos += CEGUI::UVector2(CEGUI::UDim(horBounceDir * deltaT * 0.0002,0),CEGUI::UDim(vertBounceDir * deltaT * 0.0001,0));
            bounceText->setPosition(pos);

            hue += deltaT * 0.00003;
            if(hue > 1)
                hue -= 1;

            HsvColor hsv;
            hsv.h = hue*255;
            hsv.s = 0.5*255;
            hsv.v = 0.5*255;
            RgbColor rgb = HsvToRgb(hsv);

            context->clear(((float)rgb.r)/255.0,((float)rgb.g)/255.0,((float)rgb.b)/255.0);
            context->select();
            CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context->getResolution().x,context->getResolution().y));
            glViewport(0,0,context->getResolution().x,context->getResolution().y);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            CEGUI::System::getSingleton().renderAllGUIContexts();
            context->swap();
            glEnable(GL_DEPTH_TEST);
        }

        SDL_WaitThread(thread,NULL);
        SDL_DestroySemaphore(whichFileLock);
        whichFileLock = 0;

        updateLog("Update complete!");
        text->setText("Update complete!");

        if(replacedExe)
        {
            updateError("You will need to restart the game for this update to take full effect.");
            CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/UpdateText")->setText("[colour='FFFF0000']Restart to complete update!");
        }
        else
        {
            CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/ConnectButton")->setDisabled(false);
            CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/UpdateText")->setText("[colour='FF00CC00']Your game appears up to date!");
        }

        updater->getChild("Button")->setDisabled(false);

        ((clientStuff*)updater->getUserData())->prefs->set("REVISION",((clientStuff*)updater->getUserData())->masterRevision);
        ((clientStuff*)updater->getUserData())->prefs->set("NETWORK",((clientStuff*)updater->getUserData())->masterNetwork);
        ((clientStuff*)updater->getUserData())->prefs->exportToFile("config.txt");
    }

    CEGUI::Window *addUpdater(clientStuff *clientEnvironment)
    {
        CEGUI::Window *updater = addGUIFromFile("updater.layout");
        updater->setUserData(clientEnvironment);
        updater->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeUpdater));
        updater->getChild("Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&updateButton));

        return updater;
    }
}





















