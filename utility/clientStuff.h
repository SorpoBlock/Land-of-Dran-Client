#ifndef CLIENTSTUFF_H_INCLUDED
#define CLIENTSTUFF_H_INCLUDED

#include "code/networking/client.h"
#include "code/gui/hud.h"
#include "code/audio/audio.h"
#include "code/gui/options.h"
#include "code/networking/quaternion.h"
#include "code/graphics/printLoader.h"
#include "code/gui/avatarPicker.h"
#include "code/utility/serverStuff.h"

namespace syj
{

    enum hudType
    {
        allClosed = 0,
        paintCan = 1,
        brickBar = 2,
        inventory = 3
    };

    //Split off everything from serverStuff that's not particular to a given server connection
    struct clientStuff
    {
        avatarPicker *picker = 0;
        hudType currentlyOpen = allClosed;

        bool fatalNotifyStarted = false;
        void fatalNotify(std::string windowName,std::string windowText,std::string buttonText);

        serverStuff *serverData = 0;

        options *settings = 0;
        preferenceFile *prefs = 0;

        int masterRevision = -1;
        int masterNetwork = -1;

        renderContext *context = 0;
        audioPlayer *speaker = 0;

        CEGUI::Window *evalWindow = 0;
        CEGUI::Window *brickSelector = 0;
        CEGUI::Window *wrench = 0;
        CEGUI::Window *wheelWrench = 0;
        CEGUI::Window *steeringWrench = 0;
        CEGUI::Window *inventoryBox = 0;
        CEGUI::Window *messageBox = 0;
        CEGUI::Window *playerList = 0;
        CEGUI::Window *whosTyping = 0;
        CEGUI::Listbox *chat = 0;
        textMessages bottomPrint;

        std::string loggedName = "";
        bool loggedIn = false;
        std::string sessionToken = "";

        bool exitToWindows = false;
        bool exitToMenu = false;
        bool waitingToPickServer = true;
        bool clickedMainMenuExit = false;

        std::string wantedName = "Guest";
        std::string wantedIP = "127.0.0.1";
        glm::vec3 wantedColor = glm::vec3(1,1,1);

        uniformsHolder *instancedShader = 0;
        uniformsHolder *nonInstancedShader = 0;

        paletteGUI *palette = 0;

        printLoader *prints = 0;
        material *brickMat=0;//("assets/brick/otherBrickMat.txt");
        material *brickMatSide=0;//("assets/brick/sideBrickMat.txt");
        material *brickMatRamp=0;//("assets/brick/rampBrickMat.txt");
        material *brickMatBottom=0;//("assets/brick/bottomBrickMat.txt");
        newModel *newWheelModel = 0;

        glm::vec3 vignetteColor = glm::vec3(1,0,0);
        float vignetteStrength = 0.0;
    };

}

#endif // CLIENTSTUFF_H_INCLUDED
