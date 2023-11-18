#ifndef SERVERSTUFF_H_INCLUDED
#define SERVERSTUFF_H_INCLUDED

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
#include "code/graphics/bulletTrails.h"

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
        btVector3 initPos;
        unsigned int deletionTime = 0;
    };

    struct heldSetItemPropertiesPacket
    {
        int itemID = -1;
        unsigned int deletionTime = 0;

        bool setAnim = false;
        bool hasAnim = false;
        int animID = 0;
        float animSpeed = 1.0;

        bool setSound = false;
        bool hasSound = false;
        int soundID = 0;
        float soundPitch = 1.0;
        float soundGain = 1.0;

        bool setEmitter = false;
        bool hasEmitter = false;
        int emitterID = 0;
        std::string emitterMesh = "";

        bool setCooldown = false;
        unsigned int cooldown = 1.0;
    };

    struct serverStuff
    {
        bool waitingForServerResponse = true;
        avatarPicker *picker = 0;
        std::string lastGuessedEvalPass = "";
        bulletTrailsHolder *bulletTrails = 0;

        float waterLevel = 15.0;

        //For example, if you're driving a car:
        bool giveUpControlOfCurrentPlayer = false;
        newDynamic *currentPlayer = 0;

        /*unsigned int serverTimePoint = 0;
        unsigned int clientTimePoint = 0;*/

        std::vector<std::string> typingPlayersNames;
        std::string typingPlayerNameString = "";

        heldItemType *paintCan = 0;
        item *fixedPaintCanItem = 0;
        //bool usingPaint = false;

        std::vector<particleType*> particleTypes;
        std::vector<emitterType*> emitterTypes;
        std::vector<emitter*> emitters;
        std::vector<light*> lights;
        emitterType *wheelDirtEmitter = 0;

        std::vector<heldClientPhysicsDataPacket> clientPhysicsPackets;
        std::vector<heldLightPacket> heldLightPackets;
        std::vector<heldSetItemPropertiesPacket> heldItemPackets;

        std::vector<glm::vec3> debugLocations;
        std::vector<glm::vec3> debugColors;

        client *connection = 0;
        newBrickRenderer staticBricks;
        btDynamicsWorld *world = 0;

        bool canJet = true;
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
        //std::vector<animatedModel*> oldDynamicTypes;
        std::vector<newModel*> newDynamicTypes;
        std::vector<newDynamic*> newDynamics;
        std::vector<livingBrick*> livingBricks;
        std::vector<rope*> ropes;
        livingBrick *ghostCar = 0;
        std::vector<light*> ghostCarLights;
        bool loadCarAsCar = false;
        std::vector<heldItemType*> itemTypes;
        //std::vector<newDynamic*> itemIcons;
        std::vector<brickFakeKills> fakeKills;
        //bool inventoryOpen = true;
        unsigned int selectedSlot = 0;
        heldItemType *inventory[inventorySize] = {0,0,0};

        unsigned int brickTypesToLoad = 0;
        unsigned int dynamicTypesToLoad = 0;
        unsigned int itemTypesToLoad = 0;
        bool kicked = false;
        int skippingCompileNextBricks = -1;

        environment *env =0;
        perspectiveCamera *playerCamera = 0;

        void tryApplyHeldPackets();

        void renderMeshes(uniformsHolder &unis,camera *altCam = 0);
        void renderBricks(uniformsHolder &unis,bool skipMats = false,camera *altCam = 0);
    };
}
#endif // SERVERSTUFF_H_INCLUDED
