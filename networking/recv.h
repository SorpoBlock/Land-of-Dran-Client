#ifndef RECV_H_INCLUDED
#define RECV_H_INCLUDED

#include "code/networking/client.h"
#include "code/physics/player.h"
#include "code/graphics/newBrickType.h"
#include "code/physics/brickPhysics.h"
#include "code/gui/hud.h"
#include "code/audio/audio.h"
#include "code/graphics/camera.h"
#include "code/graphics/environment.h"
#include "code/gui/options.h"
#include "code/networking/quaternion.h"
#include "code/graphics/printLoader.h"
#include "code/gui/avatarPicker.h"
#include "code/graphics/newModel.h"
#include "code/graphics/animation.h"
#include "code/graphics/emitter.h"
#include "code/graphics/rope.h"
#include "code/graphics/light.h"

#define clientPacketType_requestName 0
#define clientPacketType_startLoadingStageTwo 1
#define clientPacketType_clicked 8

#define inventorySize 3

namespace syj
{
    struct brickFakeKills
    {
        basicBrickRenderData *basic = 0;
        specialBrickRenderData *special = 0;
        glm::vec3 linVel = glm::vec3(0,0,0);
        glm::vec3 angVel = glm::vec3(0,0,0);
        unsigned int startTime = 0;
        unsigned int endTime = 0;
    };

    struct heldDynamicPacket
    {
        int dynamicID = -1;
        texture *decal = 0;
        std::vector<glm::vec3> nodeColors;
        std::vector<std::string> nodeNames;
        //std::vector<int> nodePickingIDs;
        int deletionTime = 0;
    };

    struct heldLightPacket
    {
        int deletionTime = 0;
        int brickCarID = 0;
        light *l = 0;
    };

    struct heldClientPhysicsDataPacket
    {
        int dynamicID = -1;
        btVector3 finalHalfExtents;
        btVector3 finalOffset;
        unsigned int deletionTime = 0;
    };

    struct serverStuff
    {
        //For example, if you're driving a car:
        bool giveUpControlOfCurrentPlayer = false;
        newDynamic *currentPlayer = 0;

        /*unsigned int serverTimePoint = 0;
        unsigned int clientTimePoint = 0;*/

        int masterRevision = -1;
        int masterNetwork = -1;

        std::vector<std::string> typingPlayersNames;
        std::string typingPlayerNameString = "";

        heldItemType *paintCan = 0;
        item *fixedPaintCanItem = 0;
        bool usingPaint = false;

        std::vector<particleType*> particleTypes;
        std::vector<emitterType*> emitterTypes;
        std::vector<emitter*> emitters;
        std::vector<light*> lights;
        std::vector<heldLightPacket> heldLightPackets;
        emitterType *wheelDirtEmitter = 0;

        std::vector<heldClientPhysicsDataPacket> clientPhysicsPackets;

        std::vector<glm::vec3> debugLocations;
        std::vector<glm::vec3> debugColors;

        bool fatalNotifyStarted = false;
        void fatalNotify(std::string windowName,std::string windowText,std::string buttonText);

        bool waitingToPickServer = true;
        std::string wantedName = "Guest";
        std::string wantedIP = "127.0.0.1";
        glm::vec3 wantedColor = glm::vec3(1,1,1);

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
        preferenceFile *prefs = 0;
        textMessages bottomPrint;

        client *connection = 0;
        newBrickRenderer staticBricks;
        btDynamicsWorld *world = 0;
        paletteGUI *palette = 0;
        renderContext *context = 0;
        avatarPicker *picker = 0;

        printLoader *prints = 0;
        material *brickMat;//("assets/brick/otherBrickMat.txt");
        material *brickMatSide;//("assets/brick/sideBrickMat.txt");
        material *brickMatRamp;//("assets/brick/rampBrickMat.txt");
        material *brickMatBottom;//("assets/brick/bottomBrickMat.txt");

        bool adminCam = false;
        int cameraTargetServerID = -1;
        newDynamic *cameraTarget = 0;
        float totalLean = 0;
        bool cameraLean = false;
        glm::vec3 cameraPos = glm::vec3(0,0,0);
        glm::vec3 cameraDir = glm::vec3(0,0,0);
        bool freeLook = false;
        bool boundToObject = false;
        int drivenCarId = -1;

        std::vector<heldDynamicPacket> heldAppearancePackets;
        std::vector<item*> items;
        std::vector<animatedModel*> oldDynamicTypes;
        std::vector<newModel*> newDynamicTypes;
        std::vector<newDynamic*> newDynamics;
        std::vector<livingBrick*> livingBricks;
        std::vector<rope*> ropes;
        livingBrick *ghostCar = 0;
        std::vector<light*> ghostCarLights;
        bool loadCarAsCar = false;
        std::vector<heldItemType*> itemTypes;
        std::vector<brickFakeKills> fakeKills;
        bool inventoryOpen = true;
        unsigned int selectedSlot = 0;
        heldItemType *inventory[inventorySize] = {0,0,0};

        unsigned int brickTypesToLoad = 0;
        unsigned int dynamicTypesToLoad = 0;
        unsigned int itemTypesToLoad = 0;
        bool waitingForServerResponse = true;
        bool kicked = false;
        int skippingCompileNextBricks = -1;
        bool exitToWindows = false;

        environment *env =0;
        perspectiveCamera *playerCamera = 0;
        options *settings = 0;

        void tryApplyHeldPackets();

        void renderMeshes(uniformsHolder &unis,camera *altCam = 0);
        void renderBricks(uniformsHolder &unis,bool skipMats = false,camera *altCam = 0);
    };

    void checkForCameraToBind(serverStuff *ohWow);

    void recvHandle(client *theClient,packet *data,void *userData);
}

#endif // RECV_H_INCLUDED
