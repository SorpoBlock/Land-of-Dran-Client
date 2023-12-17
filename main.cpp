#include "code/utility/logger.h"
#include "code/graphics/program.h"
#include "code/graphics/renderContext.h"
#include "code/utility/debugGraphics.h"
#include "code/graphics/uniformsBasic.h"
#include "code/graphics/texture.h"
#include "code/graphics/animation.h"
#include "code/graphics/camera.h"
#include "code/utility/inputMap.h"
#include "code/graphics/tessellation.h"
#include "code/graphics/instanced.h"
#include "code/graphics/renderTarget.h"
#include "code/graphics/environment.h"
#include "code/physics/terrain.h"
#include "code/physics/player.h"
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include "code/physics/bulletIncludes.h"
#include "code/graphics/printLoader.h"
#include "code/graphics/newBrickType.h"
#include "code/physics/brickPhysics.h"
#include "code/utility/ceguiHelper.h"
#include "code/gui/options.h"
#include "code/gui/brickSelector.h"
#include "code/gui/palette.h"
#include "code/physics/tempBrick.h"
#include "code/utility/brdfLookup.h"
#include "code/networking/common.h"
#include "code/networking/client.h"
#include "code/networking/recv.h"
#include "code/gui/evalWindow.h"
#include "code/gui/joinServer.h"
#include "code/gui/hud.h"
#include "code/graphics/fontRender.h"
#include "code/audio/audio.h"
#include "code/gui/wrenchDialog.h"
#include "code/gui/escape.h"
#include "code/gui/saveLoad.h"
#include "code/gui/avatarPicker.h"
#include "code/graphics/newModel.h"
#include "code/graphics/emitter.h"
#include <chrono>
#include "code/physics/selectionBox.h"
#include <curl/curl.h>
#include "code/gui/updater.h"
#include <bearssl/bearssl_hash.h>
#include "code/graphics/bulletTrails.h"
#include "code/gui/contentDownload.h"

#define hardCodedNetworkVersion 10015

#define cammode_firstPerson 0
#define cammode_thirdPerson 1
#define cammode_adminCam 2

using namespace syj;
using namespace std::filesystem;
using namespace std::chrono;

void gotKicked(client *theClient,unsigned int reason,void *userData)
{
    clientStuff *clientEnvironment = (clientStuff*)userData;
    clientEnvironment->serverData->kicked = true;
    clientEnvironment->fatalNotify("Disconnected!","Connection with server lost, reason: " + std::to_string(reason) + ".","Exit");
}

bool godRayButton(const CEGUI::EventArgs &e)
{
    CEGUI::Window *godRayWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("GodRays");
    clientStuff *clientEnvironment = (clientStuff*)godRayWindow->getUserData();

    clientEnvironment->serverData->env->godRayDecay = atof(godRayWindow->getChild("Decay")->getText().c_str());
    clientEnvironment->serverData->env->godRayDensity = atof(godRayWindow->getChild("Density")->getText().c_str());
    clientEnvironment->serverData->env->godRayExposure = atof(godRayWindow->getChild("Exposure")->getText().c_str());
    clientEnvironment->serverData->env->godRayWeight = atof(godRayWindow->getChild("Weight")->getText().c_str());
    clientEnvironment->serverData->env->sunDistance = atof(godRayWindow->getChild("Distance")->getText().c_str());
}

enum gameState
{
    STATE_MAINMENU,STATE_GETCONTENTLIST,STATE_GETCONTENT,STATE_DOWNLOADCONTENT,STATE_CONNECTING,STATE_PLAYING,STATE_AVATARPICKER,STATE_QUITTING,STATE_CLEANUP,STATE_LOADING
};

int main(int argc, char *argv[])
{
    client *serverConnection = 0;
    clientStuff clientEnvironment;

    preferenceFile prefs;
    clientEnvironment.prefs = &prefs;
    clientEnvironment.settings = new options;
    std::string ip = "127.0.0.1";

    clientEnvironment.settings->prefs = &prefs;
    clientEnvironment.settings->setDefaults(&prefs);
    prefs.importFromFile("config.txt");
    clientEnvironment.settings->loadFromFile(&prefs);
    prefs.exportToFile("config.txt");
    if(prefs.getPreference("IP"))
        ip = prefs.getPreference("IP")->toString();

    logger::setErrorFile("Logs/error.txt");
    logger::setInfoFile("Logs/log.txt");
    syj::log().setDebug(false);
    scope("Main");
    info("Starting up!");
    if(argc > 0)
        info(argv[0]);

    create_directory("Logs"); //CEGUI on Linux needs the L to be capital
    create_directory("saves");
    create_directory("add-ons");
    create_directory("assets");
    create_directory("assets/brick");
    create_directory("assets/brick/types");
    create_directory("assets/brick/prints");
    remove("oldlandofdran.exe");
    int revisionVersion = -1;
    int networkVersion = -1;
    info("Retrieving version info from master server...");
    getVersions(revisionVersion,networkVersion);
    clientEnvironment.masterRevision = revisionVersion;
    clientEnvironment.masterNetwork = networkVersion;
    info("Most recent revision available on master server: " + std::to_string(revisionVersion));
    int ourRevisionVersion = -1;
    int ourNetworkVersion = -1;
    if(prefs.getPreference("REVISION"))
        ourRevisionVersion = prefs.getPreference("REVISION")->toInteger();
    if(prefs.getPreference("NETWORK"))
        ourNetworkVersion = prefs.getPreference("NETWORK")->toInteger();
    info("Our revision: " + std::to_string(ourRevisionVersion));

    renderContextOptions renderOptions;
    renderOptions.name = "Rev " + std::to_string(ourRevisionVersion) + " - " + __DATE__;
    renderOptions.startingResX = clientEnvironment.settings->resolutionX;
    renderOptions.startingResY = clientEnvironment.settings->resolutionY;
    renderOptions.useVSync = clientEnvironment.settings->vsync;
    renderOptions.useFullscreen = clientEnvironment.settings->fullscreen;
    switch(clientEnvironment.settings->antiAliasing)
    {
        case aaOff:
            renderOptions.multiSampleSamples = 1;
            break;
        case aa2x:
            renderOptions.multiSampleSamples = 2;
            break;
        default:
        case aa4x:
            renderOptions.multiSampleSamples = 4;
            break;
        case aa8x:
            renderOptions.multiSampleSamples = 8;
            break;
        case aa16x:
            renderOptions.multiSampleSamples = 16;
            break;
    }
    renderContext context(renderOptions);
    clientEnvironment.context = &context;

    ALCdevice* device = alcOpenDevice(NULL);
    if(!device)
    {
        error("Could not find/open OpenAL device.");
        return 0;
    }
    ALCcontext *audioContext = alcCreateContext(device,NULL);
    alcMakeContextCurrent(audioContext);

    ALCenum err = alcGetError(device);
    if(err != ALC_NO_ERROR)
        error("OpenAL error: " + std::to_string(err));

    clientEnvironment.speaker = new audioPlayer;

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        error("FREETYPE: Could not init FreeType Library");
        return -1;
    }

    fontRenderer defaultFont(ft,"gui/fonts/DejaVuSans.ttf");

    printComputerStats();

    inputMap playerInput;
    playerInput.fromPrefs(&prefs);
    playerInput.toPrefs(&prefs);
    prefs.exportToFile("config.txt");

    initLoadCEGUISkin("GWEN",context.getWidth(),context.getHeight());
    //Default resolution is 800x800, this is needed to make the GUI work correctly on other resolutions:
    CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));

    CEGUI::Window *optionsWindow = loadOptionsGUI(clientEnvironment.settings,prefs,playerInput);
    clientEnvironment.brickSelector = loadBrickSelector();
    CEGUI::Window *hud = addGUIFromFile("hud.layout");
    CEGUI::Window *brickHighlighter = hud->getChild("BrickPopup/Selector");
    CEGUI::Window *chat = hud->getChild("Chat");
    CEGUI::Window *updater = addUpdater(&clientEnvironment);
    chat->moveToBack();
    clientEnvironment.playerList = hud->getChild("PlayerList");
    clientEnvironment.chat = (CEGUI::Listbox*)hud->getChild("Chat/Listbox");
    clientEnvironment.whosTyping = hud->getChild("WhosTyping");
    CEGUI::Window *chatEditbox = hud->getChild("Chat/Editbox");
    CEGUI::Window *chatScrollArrow = hud->getChild("Chat/DidScroll");
    clientEnvironment.inventoryBox = hud->getChild("Inventory");
    setUpWrenchDialogs(hud,&clientEnvironment);
    clientEnvironment.wrench = hud->getChild("Wrench");
    clientEnvironment.wheelWrench = hud->getChild("WheelWrench");
    clientEnvironment.steeringWrench = hud->getChild("SteeringWrench");
    CEGUI::Window *stats = hud->getChild("Stats");
    CEGUI::Window *crossHair = hud->getChild("Crosshair");
    CEGUI::Window *evalWindow = configureEvalWindow(hud,&clientEnvironment);
    CEGUI::Window *joinServer = loadJoinServer(&clientEnvironment);
    CEGUI::Window *godRayDebug = addGUIFromFile("godRayDebug.layout");
    godRayDebug->setUserData(&clientEnvironment);
    godRayDebug->getChild("Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&godRayButton));
    clientEnvironment.evalWindow = evalWindow;
    clientEnvironment.messageBox = initHud(hud);
    clientEnvironment.palette = new paletteGUI(hud);
    clientEnvironment.bottomPrint.textBar = hud->getChild("BottomPrint");
    CEGUI::Window *saveLoadWindow = loadSaveLoadWindow(&clientEnvironment);
    CEGUI::Window *brickPopup = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("HUD")->getChild("BrickPopup");

    CEGUI::Window *contentMenu = loadContentMenu(&clientEnvironment);

    if(ourRevisionVersion == -1 || ourRevisionVersion == 0)
    {
        joinServer->getChild("UpdateText")->setText("[colour='FFFF0000']New installs must run updater first ->");
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("JoinServer/ConnectButton")->setDisabled(true);
    }
    else
    {
        if(revisionVersion == -1)
            joinServer->getChild("UpdateText")->setText("[colour='FFFF0000']Could not retrieve version info!");
        else
        {
            if(revisionVersion > ourRevisionVersion)
                joinServer->getChild("UpdateText")->setText("[colour='FFFF9900']Your game may be out of date!");
            else
                joinServer->getChild("UpdateText")->setText("[colour='FF00CC00']Your game appears up to date!");
        }
    }

    checkForSessionKey(&clientEnvironment,prefs);

    if(SDLNet_Init())
        error("Could not initalize SDLNet");

    std::string shaderErrorStr = "";
    std::vector<uniformsHolder*> programUnis = loadAllShaders(shaderErrorStr);
    if(shaderErrorStr.length() > 0)
        notify("Shader Failed to Compile",shaderErrorStr,"Close");

    uniformsHolder  *boxEdgesUnis=0,*brickUnis=0,*bulletUnis=0,
                    *emitterUnis=0,*fontUnis=0,*godPrePassBrickUnis=0,
                    *godPrePassSunUnis=0,*modelUnis=0,*modelShadowUnis=0,
                    *oldModelUnis=0,*rectToCubeUnis=0,*screenOverlaysUnis=0,
                    *shadowBrickUnis=0,*simpleBrickUnis=0,*waterUnis=0,
                    *brickDNCUnis=0,*modelDNCUnis=0;

    info(std::to_string(programUnis.size()) + " shaders loaded.");
    for(int a = 0; a<programUnis.size(); a++)
    {
        std::string programName = programUnis[a]->name;
        if(programName == "boxEdges")
            boxEdgesUnis = programUnis[a];
        else if(programName == "brick")
            brickUnis = programUnis[a];
        else if(programName == "bullet")
            bulletUnis = programUnis[a];
        else if(programName == "emitter")
            emitterUnis = programUnis[a];
        else if(programName == "font")
            fontUnis = programUnis[a];
        else if(programName == "godPrePassBrick")
            godPrePassBrickUnis = programUnis[a];
        else if(programName == "godPrePassSun")
            godPrePassSunUnis = programUnis[a];
        else if(programName == "model")
            modelUnis = programUnis[a];
        else if(programName == "modelShadow")
            modelShadowUnis = programUnis[a];
        else if(programName == "oldModel")
            oldModelUnis = programUnis[a];
        else if(programName == "rectToCube")
            rectToCubeUnis = programUnis[a];
        else if(programName == "screenOverlays")
            screenOverlaysUnis = programUnis[a];
        else if(programName == "shadowBrick")
            shadowBrickUnis = programUnis[a];
        else if(programName == "simpleBrick")
            simpleBrickUnis = programUnis[a];
        else if(programName == "water")
            waterUnis = programUnis[a];
        else if(programName == "brickDNC")
            brickDNCUnis = programUnis[a];
        else if(programName == "modelDNC")
            modelDNCUnis = programUnis[a];
        else
        {
            error("Invalid program: " + programName);
            continue;
        }
    }

    CEGUI::Window *escapeMenu = addEscapeMenu(&clientEnvironment,modelUnis);
    clientEnvironment.nonInstancedShader = oldModelUnis;
    clientEnvironment.instancedShader = modelUnis;
    clientEnvironment.rectToCubeUnis = rectToCubeUnis;

    clientEnvironment.brickMat = new material("assets/brick/otherBrickMat.txt");
    clientEnvironment.brickMatSide = new material("assets/brick/sideBrickMat.txt");
    clientEnvironment.brickMatRamp = new material("assets/brick/rampBrickMat.txt");
    clientEnvironment.brickMatBottom = new material("assets/brick/bottomBrickMat.txt");

    GLuint quadVAO = createQuadVAO();
    GLuint cubeVAO = createCubeVAO();
    clientEnvironment.prints = new printLoader("./assets/brick/prints");
    clientEnvironment.cubeVAO = cubeVAO;
    texture *bdrf = generateBDRF(quadVAO);
    material grass("assets/ground/grass1/grass.txt");
    clientEnvironment.newWheelModel = new newModel("assets/ball/ball.txt");

    renderTarget *waterReflection = 0;
    renderTarget *waterRefraction = 0;
    renderTarget *waterDepth = 0;
    texture *dudvTexture = new texture;
    dudvTexture->setWrapping(GL_REPEAT);
    dudvTexture->createFromFile("assets/dudv.png");
    tessellation water(0);

    //blocklandCompatibility blocklandHolder("assets/brick/types/test.cs","./assets/brick/types",clientEnvironment.brickSelector,true);
    blocklandCompatibility *blocklandHolder = 0;

    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();

    clientEnvironment.picker = new avatarPicker(context.getResolution().x,context.getResolution().y);

    btDefaultCollisionConfiguration *collisionConfiguration = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);

    btDynamicsWorld *world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

    btCollisionShape *plane = 0;
    btDefaultMotionState* planeState = 0;
    btRigidBody *groundPlane = 0;

    btVector3 gravity = btVector3(0,-70,0);
    world->setGravity(gravity);
    world->setForceUpdateAllAabbs(false); //Remove this and static bricks will lag the physics implementation a ton

    plane = new btStaticPlaneShape(btVector3(0,1,0),0);
    planeState = new btDefaultMotionState();
    btRigidBody::btRigidBodyConstructionInfo planeCon(0,planeState,plane);
    groundPlane = new btRigidBody(planeCon);
    groundPlane->setFriction(1.0);
    //groundPlane->setUserIndex(bodyUserIndex_plane);
    world->addRigidBody(groundPlane);

    CEGUI::Window *bounceText = addGUIFromFile("justText.layout");
    bounceText->setVisible(true);
    bounceText->moveToBack();

    float hue = 0;
    float lastTick = SDL_GetTicks();
    float deltaT = 0;
    float horBounceDir = 1;
    float vertBounceDir = 1;
    float bottomBarOpen = 0;
    float bottomBarClose = 0.165;
    float bottomBarOpenTime = 500;
    float bottomBarLastAct = SDL_GetTicks();
    bool bottomBarShouldBeOpen = false;
    glm::vec3 lastCamPos = glm::vec3(0,0,0);
    unsigned int last10Secs = SDL_GetTicks();
    unsigned int frames = 0;
    int waterFrame = 0;
    int camMode = cammode_firstPerson;
    float desiredFov = clientEnvironment.settings->fieldOfView;
    float currentZoom = clientEnvironment.settings->fieldOfView;
    double totalSteps = 0;
    glm::vec3 lastPlayerDir = glm::vec3(0,0,0);
    int lastPlayerControlMask = 0;
    int transSendInterval = 30;
    int lastSentTransData = 0;
    bool doJump = false;
    bool showPreview = false;
    bool hitDebug = true;
    bool justTurnOnChat = false;
    float lastPhysicsStep = 0.0;
    unsigned int debugMode = 3;
    bool connected = false;

    TCPsocket cdnClient = 0;
    SDLNet_SocketSet cdnSocketSet = 0;
    customFileDescriptor *downloading = 0;
    std::ofstream currentDownloadFile;

    //Start connect screen
    //Start-up has finished

    context.setMouseLock(false);

    gameState currentState = STATE_MAINMENU;
    while(currentState != STATE_QUITTING)
    {
        switch(currentState)
        {
            case STATE_AVATARPICKER:
            {
                //TODO: To be moved from avatarPicker.cpp
                break;
            }
            case STATE_CLEANUP:
            {
                clientEnvironment.ignoreGamePackets = true;

                serverStuff *serverData = clientEnvironment.serverData;

                //Clear chats:
                CEGUI::Listbox* chat = (CEGUI::Listbox*)clientEnvironment.chat;
                chat->resetList();

                //Clear player list:
                CEGUI::MultiColumnList *playerList = (CEGUI::MultiColumnList*)clientEnvironment.playerList->getChild("List");
                playerList->resetList();

                //Clear decal gui elements
                CEGUI::ScrolledContainer *decalBox = (CEGUI::ScrolledContainer *)((CEGUI::ScrollablePane*)clientEnvironment.picker->decalPicker->getChild("Decals"))->getContentPane();
                while(decalBox->getChildCount() > 0)
                {
                    CEGUI::Window *child = decalBox->getChildAtIdx(0);
                    if(CEGUI::ImageManager::getSingleton().isDefined(child->getChild("Image")->getProperty("Image")))
                    {
                        if(child->getUserData())
                            delete child->getUserData();
                        std::string iconName = child->getChild("Image")->getProperty("Image").c_str();
                        CEGUI::ImageManager::getSingleton().destroy(iconName);
                        CEGUI::System::getSingleton().getRenderer()->destroyTexture(iconName);
                    }
                    decalBox->removeChild(child);
                    CEGUI::WindowManager::getSingleton().destroyWindow(child);
                }

                for(int a = 0; a<clientEnvironment.picker->faceDecals.size(); a++)
                    delete clientEnvironment.picker->faceDecals[a];
                clientEnvironment.picker->faceDecals.clear();
                clientEnvironment.picker->faceDecalFilepaths.clear();
                //Decal gui elements clear

                //Clear up brick type GUI elements

                CEGUI::Window *brickPopup = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("HUD")->getChild("BrickPopup");
                for(int a = 1; a<=9; a++)
                {
                    CEGUI::Window *cart = clientEnvironment.brickSelector->getChild("Cart" + std::to_string(a));
                    if(cart->getChild("BrickText")->getText() != "")
                    {
                        brickPopup->getChild("Cart" + std::to_string(a))->setProperty("Image","");
                        cart->getChild("BrickImage")->setProperty("Image","");
                        cart->getChild("BrickText")->setText("");
                        cart->setUserData(0);
                    }
                }

                CEGUI::ScrolledContainer *brickBox = (CEGUI::ScrolledContainer*)((CEGUI::ScrollablePane*)clientEnvironment.brickSelector->getChild("BasicBricks"))->getContentPane();
                while(brickBox->getChildCount() > 0)
                {
                    CEGUI::Window *child = brickBox->getChildAtIdx(0);
                    if(CEGUI::ImageManager::getSingleton().isDefined(child->getChild("BrickImage")->getProperty("Image")))
                    {
                        std::string iconName = child->getChild("BrickImage")->getProperty("Image").c_str();
                        CEGUI::ImageManager::getSingleton().destroy(iconName);
                        CEGUI::System::getSingleton().getRenderer()->destroyTexture(iconName);
                        if(child->getUserData())
                            delete child->getUserData();
                    }
                    brickBox->removeChild(child);
                    CEGUI::WindowManager::getSingleton().destroyWindow(child);
                }
                brickBox = (CEGUI::ScrolledContainer*)((CEGUI::ScrollablePane*)clientEnvironment.brickSelector->getChild("SpecialBricks"))->getContentPane();
                while(brickBox->getChildCount() > 0)
                {
                    CEGUI::Window *child = brickBox->getChildAtIdx(0);
                    if(CEGUI::ImageManager::getSingleton().isDefined(child->getChild("BrickImage")->getProperty("Image")))
                    {
                        std::string iconName = child->getChild("BrickImage")->getProperty("Image").c_str();
                        CEGUI::ImageManager::getSingleton().destroy(iconName);
                        CEGUI::System::getSingleton().getRenderer()->destroyTexture(iconName);
                        if(child->getUserData())
                            delete child->getUserData();
                    }
                    brickBox->removeChild(child);
                    CEGUI::WindowManager::getSingleton().destroyWindow(child);
                }

                //Brick type gui elements cleared

                if(blocklandHolder)
                    delete blocklandHolder;
                blocklandHolder = 0;

                CEGUI::ScrolledContainer *box = (CEGUI::ScrolledContainer *)((CEGUI::ScrollablePane*)clientEnvironment.picker->decalPicker->getChild("Decals"))->getContentPane();
                while(box->getChildCount() > 0)
                    box->removeChild(box->getChildAtIdx(0));

                //Audio clean-up
                while(clientEnvironment.speaker->allLoops.size() > 0)
                    clientEnvironment.speaker->removeLoop(clientEnvironment.speaker->allLoops[0].serverId);
                for(int a = 0; a<32; a++)
                {
                    alSourcei( clientEnvironment.speaker->generalSounds[a], AL_BUFFER, 0);
                    alSourceStop(clientEnvironment.speaker->generalSounds[a]);
                }
                for(int a = 0; a<clientEnvironment.speaker->sounds.size(); a++)
                    delete clientEnvironment.speaker->sounds[a];
                clientEnvironment.speaker->sounds.clear();
                //Finish audio clean-up

                context.setMouseLock(false);
                clientEnvironment.waitingToPickServer = true;

                bounceText->setVisible(true);
                bounceText->moveToBack();

                delete serverData;
                serverData = 0;
                clientEnvironment.serverData = 0;

                if(waterDepth)
                    delete waterDepth;
                waterDepth=0;
                if(waterReflection)
                    delete waterReflection;
                waterReflection = 0;
                if(waterRefraction)
                    delete waterRefraction;
                waterRefraction = 0;

                currentState = STATE_MAINMENU;
                break;
            }
            case STATE_MAINMENU:
            {
                clientEnvironment.ignoreGamePackets = true;

                if(clientEnvironment.serverData)
                {
                    delete clientEnvironment.serverData;
                    clientEnvironment.serverData = 0;
                }

                hud->setVisible(false);
                joinServer->setVisible(true);

                const Uint8 *states = SDL_GetKeyboardState(NULL);

                SDL_Event e;
                while(SDL_PollEvent(&e))
                {
                    if(e.type == SDL_QUIT)
                    {
                        currentState = STATE_QUITTING;
                        break;
                    }

                    processEventsCEGUI(e,states);

                    if(e.type == SDL_WINDOWEVENT)
                    {
                        if(e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            context.setSize(e.window.data1,e.window.data2);
                        }
                    }
                }

                deltaT = SDL_GetTicks() - lastTick;

                //After auto-updating, land of dran text may dissapear without this:
                if(deltaT > 20)
                    deltaT = 20;

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

                context.clear(((float)rgb.r)/255.0,((float)rgb.g)/255.0,((float)rgb.b)/255.0);
                context.select();
                CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));
                glViewport(0,0,context.getResolution().x,context.getResolution().y);
                glDisable(GL_DEPTH_TEST);
                glActiveTexture(GL_TEXTURE0);
                CEGUI::System::getSingleton().renderAllGUIContexts();
                context.swap();
                glEnable(GL_DEPTH_TEST);

                if(clientEnvironment.clickedMainMenuExit)
                {
                    currentState = STATE_QUITTING;
                    break;
                }

                SDL_Delay(1);

                if(!clientEnvironment.waitingToPickServer)
                {
                    bounceText->setVisible(false);
                    currentState = STATE_GETCONTENTLIST;

                    clientEnvironment.waitingOnContentList = true;
                    clientEnvironment.cancelCustomContent = false;
                    clientEnvironment.expectedCustomFiles = -1;

                    ((CEGUI::MultiColumnList*)contentMenu->getChild("List"))->clearAllSelections();
                    ((CEGUI::MultiColumnList*)contentMenu->getChild("List"))->resetList();
                    contentMenu->moveToFront();
                    contentMenu->getChild("Size")->setText("");
                    contentMenu->getChild("Number")->setText("");

                    if(contentMenu->getUserData())
                    {
                        std::vector<customFileDescriptor*> *contentList = (std::vector<customFileDescriptor*> *)contentMenu->getUserData();
                        for(int a = 0; a<contentList->size(); a++)
                            delete contentList->at(a);
                        contentList->clear();
                    }

                    networkingPreferences netPrefs;
                    netPrefs.timeoutMS = 10000;
                    serverConnection = new client(netPrefs,clientEnvironment.wantedIP);
                    serverConnection->userData = &clientEnvironment;
                    serverConnection->receiveHandle = recvHandle;
                    serverConnection->kickHandle = gotKicked;

                    packet requestContent;
                    requestContent.writeUInt(clientPacketType_requestName,4);
                    requestContent.writeUInt(hardCodedNetworkVersion,32);
                    requestContent.writeBit(true);
                    serverConnection->send(&requestContent,true);

                    info("Retrieving custom content list...");
                }

                break;
            }
            case STATE_GETCONTENT:
            {
                const Uint8 *states = SDL_GetKeyboardState(NULL);
                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_QUIT)
                    {
                        currentState = STATE_QUITTING;
                        break;
                    }

                    processEventsCEGUI(event,states);

                    if(event.type == SDL_WINDOWEVENT)
                    {
                        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            context.setSize(event.window.data1,event.window.data2);
                        }
                    }
                }

                std::vector<customFileDescriptor*> *contentList = ((std::vector<customFileDescriptor*> *)contentMenu->getUserData());
                if(contentList)
                {
                    if(contentList->size() > 0)
                    {
                        if(!downloading)
                        {
                            for(int a = 0; a<contentList->size(); a++)
                            {
                                if(!contentList->at(a)->selectable)
                                    continue;
                                if(!contentList->at(a)->enabled)
                                    continue;
                                if(contentList->at(a)->doneDownloading)
                                    continue;
                                downloading = contentList->at(a);
                                info("Downloading add-ons/"+downloading->path);

                                std::filesystem::path pathToCreateFoldersFor("add-ons/"+downloading->path);
                                create_directories(pathToCreateFoldersFor.parent_path());

                                currentDownloadFile = std::ofstream("add-ons/"+downloading->path,std::ios::binary);
                                if(!currentDownloadFile.is_open())
                                    error("Error opening file add-ons/" + downloading->path + " for write!");
                                break;
                            }
                        }

                        if(!downloading)
                        {
                            error("No possible download candidate file.");
                            if(cdnClient)
                            {
                                SDLNet_TCP_Close(cdnClient);
                                cdnClient = 0;
                            }
                            if(cdnSocketSet)
                            {
                                SDLNet_FreeSocketSet(cdnSocketSet);
                                cdnSocketSet = 0;
                            }
                            delete serverConnection;
                            serverConnection = 0;
                            currentState = STATE_CONNECTING;
                            contentMenu->setVisible(false);
                            break;
                        }

                        int cdnReady = SDLNet_CheckSockets(cdnSocketSet,0);
                        if(cdnReady > 0)
                        {
                            int actualMax = std::min(1024,downloading->size-downloading->bytesReceived);

                            char *buf = new char[actualMax];
                            int recvBytes = SDLNet_TCP_Recv(cdnClient,buf,actualMax);

                            if(recvBytes < 1)
                            {
                                error("CDN Server closed connection!");
                                if(cdnClient)
                                {
                                    SDLNet_TCP_Close(cdnClient);
                                    cdnClient = 0;
                                }
                                if(cdnSocketSet)
                                {
                                    SDLNet_FreeSocketSet(cdnSocketSet);
                                    cdnSocketSet = 0;
                                }
                                delete serverConnection;
                                serverConnection = 0;
                                currentState = STATE_CONNECTING;
                                contentMenu->setVisible(false);
                                delete buf;
                                currentDownloadFile.close();
                                break;
                            }
                            else
                            {
                                //info("Downloaded: " + std::to_string(downloading->bytesReceived) + " / " + std::to_string(downloading->size));
                                currentDownloadFile.write(buf,recvBytes);
                                downloading->bytesReceived += recvBytes;
                                if(downloading->bytesReceived > downloading->size)
                                    error("Somehow downloaded too many bytes: " + std::to_string(downloading->bytesReceived));
                                if(downloading->bytesReceived >= downloading->size)
                                {
                                    currentDownloadFile.close();
                                    downloading->doneDownloading = true;
                                    downloading = 0;
                                }
                            }

                            delete buf;
                        }
                        else if(cdnReady == -1)
                            error("CDN CheckSockets failed!");
                    }
                    else
                    {
                        if(cdnClient)
                        {
                            SDLNet_TCP_Close(cdnClient);
                            cdnClient = 0;
                        }
                        if(cdnSocketSet)
                        {
                            SDLNet_FreeSocketSet(cdnSocketSet);
                            cdnSocketSet = 0;
                        }
                        delete serverConnection;
                        serverConnection = 0;
                        currentState = STATE_CONNECTING;
                        contentMenu->setVisible(false);
                        break;
                    }
                }
                else
                {
                    error("No content list vector?");
                    if(cdnClient)
                    {
                        SDLNet_TCP_Close(cdnClient);
                        cdnClient = 0;
                    }
                    if(cdnSocketSet)
                    {
                        SDLNet_FreeSocketSet(cdnSocketSet);
                        cdnSocketSet = 0;
                    }
                    delete serverConnection;
                    serverConnection = 0;
                    currentState = STATE_CONNECTING;
                    contentMenu->setVisible(false);
                    break;
                }

                context.clear(0.5,0.5,1.0,1.0);
                context.select();
                CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));
                glViewport(0,0,context.getResolution().x,context.getResolution().y);
                glDisable(GL_DEPTH_TEST);
                glActiveTexture(GL_TEXTURE0);
                CEGUI::System::getSingleton().renderAllGUIContexts();
                context.swap();

                break;
            }
            case STATE_GETCONTENTLIST:
            {
                clientEnvironment.ignoreGamePackets = true;

                serverConnection->run();

                if(clientEnvironment.cancelCustomContent)
                {
                    delete serverConnection;
                    serverConnection = 0;
                    clientEnvironment.cancelCustomContent = false;
                    clientEnvironment.waitingToPickServer = true;
                    currentState = STATE_MAINMENU;
                    break;
                }

                if(!clientEnvironment.waitingOnContentList)
                {
                    bool oneEnabled = false;
                    for(int a = 0; a<((std::vector<customFileDescriptor*> *)contentMenu->getUserData())->size(); a++)
                    {
                        if(((std::vector<customFileDescriptor*> *)contentMenu->getUserData())->at(a)->enabled)
                        {
                            oneEnabled = true;
                            break;
                        }
                    }

                    //We selected none of the available downloads...
                    if(!oneEnabled)
                    {
                        delete serverConnection;
                        serverConnection = 0;
                        currentState = STATE_CONNECTING;
                        contentMenu->setVisible(false);
                        break;
                    }
                    else
                    {
                        //gotta download at least one file...

                        IPaddress cdnServerAddr;
                        SDLNet_ResolveHost(&cdnServerAddr,clientEnvironment.wantedIP.c_str(),20001);

                        cdnClient = SDLNet_TCP_Open(&cdnServerAddr);
                        cdnSocketSet = SDLNet_AllocSocketSet(1);
                        SDLNet_TCP_AddSocket(cdnSocketSet,cdnClient);

                        std::vector<customFileDescriptor*> *possibleFiles = ((std::vector<customFileDescriptor*> *)contentMenu->getUserData());

                        std::string contentRequest = "";
                        int numNeededFiles = 0;

                        for(int a = 0; a<possibleFiles->size(); a++)
                        {
                            if(possibleFiles->at(a)->enabled)
                            {
                                contentRequest += std::to_string(possibleFiles->at(a)->id) + "\n";
                                numNeededFiles++;
                            }
                        }

                        contentRequest = "FILES" + std::to_string(numNeededFiles) + "\n" + contentRequest + "END\n";

                        SDLNet_TCP_Send(cdnClient,contentRequest.c_str(),contentRequest.length());

                        currentState = STATE_GETCONTENT;
                        break;
                    }
                }

                //We don't actually have any new files we even *could* download if we wanted to, just skip to connecting...
                if(clientEnvironment.expectedCustomFiles == ((std::vector<customFileDescriptor*> *)contentMenu->getUserData())->size())
                {
                    bool oneSelectable = false;
                    for(int a = 0; a<((std::vector<customFileDescriptor*> *)contentMenu->getUserData())->size(); a++)
                    {
                        if(((std::vector<customFileDescriptor*> *)contentMenu->getUserData())->at(a)->selectable)
                        {
                            oneSelectable = true;
                            break;
                        }
                    }

                    if(!oneSelectable)
                    {
                        info("There were no new custom files to download, skipping...");
                        delete serverConnection;
                        serverConnection = 0;
                        currentState = STATE_CONNECTING;
                        contentMenu->setVisible(false);
                        break;
                    }
                }

                const Uint8 *states = SDL_GetKeyboardState(NULL);
                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_QUIT)
                    {
                        currentState = STATE_QUITTING;
                        break;
                    }

                    processEventsCEGUI(event,states);

                    if(event.type == SDL_WINDOWEVENT)
                    {
                        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            context.setSize(event.window.data1,event.window.data2);
                        }
                    }
                }

                context.clear(0.5,0.5,1.0,1.0);
                context.select();
                CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));
                glViewport(0,0,context.getResolution().x,context.getResolution().y);
                glDisable(GL_DEPTH_TEST);
                glActiveTexture(GL_TEXTURE0);
                CEGUI::System::getSingleton().renderAllGUIContexts();
                context.swap();

                break;
            }
            case STATE_PLAYING:
            {
                clientEnvironment.ignoreGamePackets = false;

                hud->setVisible(true);
                joinServer->setVisible(false);
                updater->setVisible(false);

                serverStuff *serverData = clientEnvironment.serverData;
                btDynamicsWorld *world = serverData->world;

                if(clientEnvironment.exitToMenu)
                {
                    clientEnvironment.ignoreGamePackets = true;

                    packet quitPacket;
                    quitPacket.writeUInt(clientPacketType_requestName,4);
                    quitPacket.writeUInt(0,32);
                    serverConnection->send(&quitPacket,true);
                    serverConnection->run();

                    clientEnvironment.exitToMenu = false;
                    clientEnvironment.waitingToPickServer = true;
                    currentState = STATE_CLEANUP;

                    break;
                }

                if(clientEnvironment.exitToWindows)
                {
                    currentState = STATE_QUITTING;
                    break;
                }

                if(clientEnvironment.fatalNotifyStarted)
                {
                    int *status = (int*)clientEnvironment.messageBox->getUserData();
                    if(status)
                    {
                        if(status[0] == 0)
                        {
                            currentState = STATE_QUITTING;
                            break;
                        }
                    }
                }

                //Options:

                hud->getChild("Inventory")->setVisible(clientEnvironment.currentlyOpen == inventory);
                if(clientEnvironment.currentlyOpen != paintCan)
                    clientEnvironment.palette->close();

                if(clientEnvironment.settings->guiScalingChanged)
                {
                    switch(clientEnvironment.settings->guiScaling)
                    {
                        case biggest:
                        clientEnvironment.inventoryBox->setArea(makeRelArea(1.0-((1.0-0.855)*0.9), 0.03875, 1, 0.70375*0.9));
                        ((CEGUI::Listbox*)chat->getChild("Listbox"))->setFont("OpenSans-30");
                        bottomBarClose = 0.165;
                        brickPopup->setArea(makeRelArea(0.5-((0.5 - 0.18125)*1.0),0,0.5+((0.8-0.5)*1.0),0.165 * 1.0));
                        break;

                        case bigger:
                        clientEnvironment.inventoryBox->setArea(makeRelArea(1.0-((1.0-0.855)*0.8), 0.03875, 1, 0.70375*0.8));
                        ((CEGUI::Listbox*)chat->getChild("Listbox"))->setFont("OpenSans-20");
                        bottomBarClose = 0.165 * 0.9;
                        brickPopup->setArea(makeRelArea(0.5-((0.5 - 0.18125)*0.9),0,0.5+((0.8-0.5)*0.9),0.165 * 0.9));
                        break;

                        case normalScaling:
                        clientEnvironment.inventoryBox->setArea(makeRelArea(1.0-((1.0-0.855)*0.7), 0.03875, 1, 0.70375*0.7));
                        ((CEGUI::Listbox*)chat->getChild("Listbox"))->setFont("DejaVuSans-12");
                        bottomBarClose = 0.165 * 0.8;
                        brickPopup->setArea(makeRelArea(0.5-((0.5 - 0.18125)*0.8),0,0.5+((0.8-0.5)*0.8),0.165 * 0.8));
                        break;

                        case smaller:
                        clientEnvironment.inventoryBox->setArea(makeRelArea(1.0-((1.0-0.855)*0.6), 0.03875, 1, 0.70375*0.6));
                        ((CEGUI::Listbox*)chat->getChild("Listbox"))->setFont("DejaVuSans-10");
                        bottomBarClose = 0.165 * 0.7;
                        brickPopup->setArea(makeRelArea(0.5-((0.5 - 0.18125)*0.6),0,0.5+((0.8-0.5)*0.6),0.165 * 0.6));
                        break;
                    }

                    clientEnvironment.settings->guiScalingChanged = false;
                }

                //serverData->playerCamera->setFieldOfVision(clientEnvironment.settings->fieldOfView);
                clientEnvironment.brickSelector->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                escapeMenu->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                evalWindow->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                saveLoadWindow->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                clientEnvironment.wrench->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                clientEnvironment.wheelWrench->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                clientEnvironment.steeringWrench->setAlpha(((float)clientEnvironment.settings->hudOpacity) / 100.0);
                clientEnvironment.playerList->setAlpha(((float)clientEnvironment.settings->hudOpacity)/100.0);
                godRayDebug->setAlpha(((float)clientEnvironment.settings->hudOpacity)/100.0);
                //chat->moveToBack();

                float gain = clientEnvironment.settings->masterVolume;
                gain /= 100.0;
                float musicGain = clientEnvironment.settings->musicVolume;
                musicGain /= 100.0;
                clientEnvironment.speaker->setVolumes(gain,musicGain);

                //Input:

                GLenum error = glGetError();
                if(error != GL_NO_ERROR)
                {
                    std::cout<<"Error: "<<error<<"\n";
                }

                lastCamPos = serverData->playerCamera->getPosition();

                frames++;
                if(last10Secs < SDL_GetTicks())
                {
                    double fps = frames;
                    totalSteps /= ((double)frames);
                    if(serverData->cameraTarget)
                        stats->setText("Sent/recv: " + std::to_string(serverConnection->numPacketsSent) + "/" + std::to_string(serverConnection->numPacketsReceived) + " FPS: " + std::to_string(fps)  + " Player Keyframes: " + std::to_string(serverData->cameraTarget->modelInterpolator.keyFrames.size()));
                    last10Secs = SDL_GetTicks() + 1000;
                    frames = 0;
                    totalSteps = 0;
                }

                deltaT = SDL_GetTicks() - lastTick;
                lastTick = SDL_GetTicks();

                /*for(unsigned int a = 0; a<32; a++)
                {
                    if(clientEnvironment.speaker->carToTrack[a])
                    {
                        glm::vec3 pos = clientEnvironment.speaker->carToTrack[a]->carTransform.getPosition();
                        glm::vec3 vel = clientEnvironment.speaker->carToTrack[a]->carTransform.guessVelocity() * glm::vec3(0.4);
                        alSource3f(clientEnvironment.speaker->loopSources[a],AL_POSITION,pos.x,pos.y,pos.z);
                        alSource3f(clientEnvironment.speaker->loopSources[a],AL_VELOCITY,vel.x,vel.y,vel.z);
                    }
                }*/

                Uint32 mouseState = SDL_GetMouseState(NULL,NULL);
                //SDL's macros are fucked up
                int leftDown = 0, rightDown = 0;
                if(mouseState == 1)
                    leftDown = 1;
                else if(mouseState == 4)
                    rightDown = 1;
                else if(mouseState == 5)
                {
                    leftDown = 1;
                    rightDown = 1;
                }

                if(useClientPhysics)
                {
                    //Buoyancy and jetting
                    if(serverData->currentPlayer)
                    {
                        if(rightDown && serverData->canJet)
                        {
                            serverData->currentPlayer->body->setGravity(btVector3(0,20,0));
                        }
                        else
                        {
                            btTransform t = serverData->currentPlayer->body->getWorldTransform();
                            btVector3 o = t.getOrigin();

                            if(o.y() < serverData->waterLevel-2.0)
                            {
                                serverData->currentPlayer->body->setDamping(0.4,0.0);
                                serverData->currentPlayer->body->setGravity(btVector3(0,-0.5,0) * world->getGravity());
                            }
                            else if(o.y() < serverData->waterLevel)
                            {
                                serverData->currentPlayer->body->setDamping(0.4,0.0);
                                serverData->currentPlayer->body->setGravity(btVector3(0,0,0));
                            }
                            else
                            {
                                serverData->currentPlayer->body->setDamping(0.0,0.0);
                                serverData->currentPlayer->body->setGravity(world->getGravity());
                            }
                        }
                    }

                    //if(SDL_GetTicks() - lastPhysicsStep >= 15)
                    //{
                        float physicsDeltaT = SDL_GetTicks() - lastPhysicsStep;
                        //if(!showPreview)
                            world->stepSimulation(physicsDeltaT / 1000.0);
                        lastPhysicsStep = SDL_GetTicks();
                    //}
                }

                const Uint8 *states = SDL_GetKeyboardState(NULL);
                playerInput.getKeyStates();

                serverData->box->performRaycast(serverData->playerCamera->getPosition(),serverData->playerCamera->getDirection(),0);

                //Clicking ray cast
                btVector3 raystart = glmToBt(serverData->playerCamera->getPosition());
                btVector3 rayend = glmToBt(serverData->playerCamera->getPosition() + serverData->playerCamera->getDirection() * glm::vec3(30.0));
                btCollisionWorld::AllHitsRayResultCallback res(raystart,rayend);
                world->rayTest(raystart,rayend,res);

                int idx = -1;
                float dist = 9999999;

                if(serverData->cameraTarget)
                {
                    for(int a = 0; a<res.m_collisionObjects.size(); a++)
                    {
                        //if(res.m_collisionObjects[a] != serverData->cameraTarget->body)
                        //{
                            if(fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-serverData->playerCamera->getPosition())) < dist)
                            {
                                dist = fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-serverData->playerCamera->getPosition()));
                                idx = a;
                            }
                        //}
                    }
                }
                else if(camMode == cammode_firstPerson)
                    camMode = cammode_thirdPerson;

                if(serverData->ghostCar)
                {
                    serverData->ghostCar->carTransform.keyFrames.clear();
                    serverData->ghostCar->carTransform.highestProcessed = 0;
                    if(idx != -1)
                        serverData->ghostCar->carTransform.addTransform(0,BtToGlm(res.m_hitPointWorld[idx]) + glm::vec3(0,5,0),glm::quat(1,0,0,0));
                    else
                        serverData->ghostCar->carTransform.addTransform(0,BtToGlm(rayend) + glm::vec3(0,5,0),glm::quat(1,0,0,0));
                }

                bool supress = evalWindow->isActive() && evalWindow->isVisible();
                supress |= clientEnvironment.wheelWrench->isActive() && clientEnvironment.wheelWrench->isVisible();
                supress |= clientEnvironment.wrench->isActive() && clientEnvironment.wrench->isVisible();
                supress |= saveLoadWindow->getChild("CarFilePath")->isActive();
                supress |= saveLoadWindow->getChild("BuildFilePath")->isActive();

                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    playerInput.handleInput(event);

                    if(event.type == SDL_QUIT)
                    {
                        currentState = STATE_QUITTING;
                        break;
                    }

                    processEventsCEGUI(event,states);

                    if(event.type == SDL_WINDOWEVENT)
                    {
                        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            context.setSize(event.window.data1,event.window.data2);
                        }
                    }

                    if(event.type == SDL_KEYDOWN)
                    {
                        if(event.key.keysym.sym == SDLK_BACKQUOTE)
                        {
                            debugMode++;
                            if(debugMode > 3)
                                debugMode = 1;
                        }

                        if(event.key.keysym.sym == SDLK_PAGEUP)
                        {
                            clientEnvironment.chat->getVertScrollbar()->setStepSize(36.3222);
                            //float maxScroll = chat->getVertScrollbar()->getDocumentSize() - chat->getVertScrollbar()->getPageSize();
                            if(clientEnvironment.chat->getVertScrollbar()->getScrollPosition() >= 36.3222)
                                clientEnvironment.chat->getVertScrollbar()->setScrollPosition(clientEnvironment.chat->getVertScrollbar()->getScrollPosition() - 36.3222);
                        }
                        if(event.key.keysym.sym == SDLK_PAGEDOWN)
                        {
                            clientEnvironment.chat->getVertScrollbar()->setStepSize(36.3222);
                            //float maxScroll = chat->getVertScrollbar()->getDocumentSize() - chat->getVertScrollbar()->getPageSize();
                            clientEnvironment.chat->getVertScrollbar()->setScrollPosition(clientEnvironment.chat->getVertScrollbar()->getScrollPosition() + 36.3222);
                        }
                        if(event.key.keysym.sym == SDLK_t && !supress)
                        {
                            if(!chatEditbox->isActive())
                            {
                                chatEditbox->activate();
                                justTurnOnChat = true;
                                chatEditbox->setMousePassThroughEnabled(false);
                            }
                        }

                        if(event.key.keysym.sym == SDLK_RETURN && chatEditbox->isActive())
                        {
                            sendChat();
                        }

                        if(event.key.keysym.sym == SDLK_ESCAPE)
                        {
                            if(chatEditbox->isActive())
                            {
                                chatEditbox->deactivate();
                                chatEditbox->setText("");
                                chatEditbox->moveToBack();
                                chatEditbox->setMousePassThroughEnabled(true);
                            }
                            else
                            {
                                bool anyVisible = false;
                                if(escapeMenu->isVisible())
                                    anyVisible = true;
                                if(optionsWindow->isVisible())
                                   anyVisible = true;
                                if(clientEnvironment.brickSelector->isVisible())
                                   anyVisible = true;
                                if(evalWindow->isVisible())
                                   anyVisible = true;
                                if(clientEnvironment.wrench->isVisible())
                                   anyVisible = true;
                                if(clientEnvironment.wheelWrench->isVisible())
                                   anyVisible = true;
                                if(clientEnvironment.steeringWrench->isVisible())
                                   anyVisible = true;
                                if(saveLoadWindow->isVisible())
                                    anyVisible = true;
                                if(clientEnvironment.playerList->isVisible())
                                    anyVisible = true;
                                if(godRayDebug->isVisible())
                                    anyVisible = true;


                                if(anyVisible)
                                {
                                    context.setMouseLock(true);
                                    escapeMenu->setVisible(false);
                                    clientEnvironment.brickSelector->setVisible(false);
                                    optionsWindow->setVisible(false);
                                    evalWindow->setVisible(false);
                                    clientEnvironment.wrench->setVisible(false);
                                    clientEnvironment.wheelWrench->setVisible(false);
                                    clientEnvironment.steeringWrench->setVisible(false);
                                    saveLoadWindow->setVisible(false);
                                    clientEnvironment.playerList->setVisible(false);
                                    godRayDebug->setVisible(false);
                                }
                                else
                                {
                                    context.setMouseLock(false);
                                    escapeMenu->setVisible(true);
                                    escapeMenu->moveToFront();
                                }
                            }
                        }


                        if(event.key.keysym.sym == SDLK_F5)
                        {
                            godRayDebug->setVisible(true);
                        }

                        if(states[SDL_SCANCODE_LCTRL] && event.key.keysym.sym == SDLK_z)
                        {
                            packet undoPacket;
                            undoPacket.writeUInt(4,4);
                            serverData->connection->send(&undoPacket,true);
                        }

                        if(states[SDL_SCANCODE_LCTRL] && event.key.keysym.sym == SDLK_w)
                        {
                            packet data;
                            data.writeUInt(9,4);
                            if(clientEnvironment.currentlyOpen == inventory)
                                data.writeUInt(serverData->selectedSlot,3);
                            else
                                data.writeUInt(7,3);
                            serverData->connection->send(&data,true);
                        }
                    }

                    if(event.type == SDL_MOUSEMOTION)
                    {
                        if(context.getMouseLocked() && (serverData->freeLook || serverData->boundToObject))
                        {
                            glm::vec2 res = context.getResolution();
                            float x = (((float)event.motion.xrel)/res.x);
                            float y = (((float)event.motion.yrel)/res.y);
                            x *= ((float)clientEnvironment.settings->mouseSensitivity) / 100.0;
                            y *= ((float)clientEnvironment.settings->mouseSensitivity) / 100.0;
                            serverData->playerCamera->turn(clientEnvironment.settings->invertMouseY ? y : -y,-x);

                            serverData->box->drag(serverData->playerCamera->getPosition(),serverData->playerCamera->getDirection());
                        }
                    }

                    if(event.type == SDL_MOUSEWHEEL)
                    {
                        //if(!clientEnvironment.brickSelector->isMouseContainedInArea() && !evalWindow->isMouseContainedInArea())
                        if(context.getMouseLocked())
                        {
                            if(clientEnvironment.currentlyOpen == inventory)
                            {
                                if(event.wheel.y > 0)
                                {
                                    serverData->selectedSlot--;
                                    if(serverData->selectedSlot < 0 || serverData->selectedSlot >= inventorySize)
                                        serverData->selectedSlot = inventorySize - 1;
                                }
                                else if(event.wheel.y < 0)
                                {
                                    serverData->selectedSlot++;
                                    if(serverData->selectedSlot >= inventorySize)
                                        serverData->selectedSlot = 0;
                                }
                            }
                            else if(clientEnvironment.currentlyOpen == brickBar)
                            {
                                serverData->ourTempBrick->scroll(hud,clientEnvironment.brickSelector,serverData->staticBricks,event.wheel.y);
                            }
                            else
                            {
                                clientEnvironment.palette->scroll(event.wheel.y);
                                serverData->ourTempBrick->basicChanged = serverData->ourTempBrick->basicChanged || serverData->ourTempBrick->isBasic;
                                serverData->ourTempBrick->specialChanged = serverData->ourTempBrick->specialChanged || !serverData->ourTempBrick->isBasic;
                            }
                        }
                    }

                    if(event.type == SDL_MOUSEBUTTONUP)
                    {
                        if(event.button.button == SDL_BUTTON_LEFT)
                        {
                            if(serverData->box->currentPhase == selectionBox::selectionPhase::stretching)
                                serverData->box->currentPhase = selectionBox::selectionPhase::selecting;
                        }
                    }

                    if(event.type == SDL_MOUSEBUTTONDOWN)
                    {
                        if(context.getMouseLocked())
                        {
                            packet data;
                            data.writeUInt(clientPacketType_clicked,4);
                            data.writeBit(event.button.button == SDL_BUTTON_LEFT);
                            data.writeFloat(serverData->playerCamera->getPosition().x);
                            data.writeFloat(serverData->playerCamera->getPosition().y);
                            data.writeFloat(serverData->playerCamera->getPosition().z);
                            data.writeFloat(serverData->playerCamera->getDirection().x);
                            data.writeFloat(serverData->playerCamera->getDirection().y);
                            data.writeFloat(serverData->playerCamera->getDirection().z);
                            if(clientEnvironment.currentlyOpen == inventory)
                                data.writeUInt(serverData->selectedSlot,3);
                            else
                                data.writeUInt(7,3);
                            serverData->connection->send(&data,true);

                            if(event.button.button == SDL_BUTTON_LEFT && serverData->currentPlayer)
                            {
                                item *currentlyHeldItem = 0;
                                if(clientEnvironment.currentlyOpen == inventory && serverData->cameraTarget)
                                {
                                    for(int a = 0; a<serverData->items.size(); a++)
                                    {
                                        if(serverData->items[a]->heldBy == serverData->cameraTarget && !serverData->items[a]->hidden)
                                        {
                                            currentlyHeldItem = serverData->items[a];
                                            break;
                                        }
                                    }
                                }

                                if(currentlyHeldItem)
                                {
                                    if(currentlyHeldItem->lastFire + currentlyHeldItem->fireCooldownMS < SDL_GetTicks())
                                    {
                                        currentlyHeldItem->lastFire = SDL_GetTicks();
                                        if(currentlyHeldItem->nextFireAnim != -1)
                                            currentlyHeldItem->play(currentlyHeldItem->nextFireAnim,true,currentlyHeldItem->nextFireAnimSpeed,false);
                                        if(currentlyHeldItem->nextFireSound != -1)
                                            clientEnvironment.speaker->playSound2D(currentlyHeldItem->nextFireSound,currentlyHeldItem->nextFireSoundPitch,currentlyHeldItem->nextFireSoundGain);

                                        if(currentlyHeldItem->nextFireEmitter != -1)
                                        {
                                            for(int a = 0; a<serverData->emitterTypes.size(); a++)
                                            {
                                                if(serverData->emitterTypes[a]->serverID == currentlyHeldItem->nextFireEmitter)
                                                {
                                                    emitter *e = new emitter;
                                                    e->creationTime = SDL_GetTicks();
                                                    e->type = serverData->emitterTypes[a];
                                                    e->attachedToItem = currentlyHeldItem;
                                                    e->justAttached = true;
                                                    e->meshName = currentlyHeldItem->nextFireEmitterMesh;
                                                    serverData->emitters.push_back(e);
                                                    break;
                                                }
                                            }
                                        }

                                        if(currentlyHeldItem->useBulletTrail)
                                        {
                                            bulletTrail trail;
                                            trail.color = currentlyHeldItem->bulletTrailColor;

                                            trail.end = serverData->playerCamera->getPosition() + (serverData->playerCamera->getDirection() * glm::vec3(1.0));
                                            trail.start = serverData->playerCamera->getPosition() + (serverData->playerCamera->getDirection() * glm::vec3(60.0));

                                            trail.creationTime = SDL_GetTicks();
                                            trail.deletionTime = SDL_GetTicks() + glm::length(trail.end-trail.start) * 13 * currentlyHeldItem->bulletTrailSpeed;

                                            serverData->bulletTrails->bulletTrails.push_back(trail);
                                        }
                                    }
                                }
                                else
                                {
                                    serverData->currentPlayer->stop("grab");
                                    serverData->currentPlayer->play("grab",true);
                                }
                            }
                        }

                        if(event.button.button == SDL_BUTTON_RIGHT)
                        {
                            if(serverData->ghostCar)
                            {
                                delete serverData->ghostCar;
                                serverData->ghostCar = 0;
                            }

                            serverData->box->currentPhase = selectionBox::selectionPhase::idle;
                        }

                        if(event.button.button == SDL_BUTTON_LEFT && context.getMouseLocked())
                        {
                            if(serverData->ghostCar)
                            {
                                if(idx != -1)
                                    sendBrickCarToServer(&clientEnvironment,serverData->ghostCar,BtToGlm(res.m_hitPointWorld[idx]));
                                else
                                    sendBrickCarToServer(&clientEnvironment,serverData->ghostCar,BtToGlm(rayend) + glm::vec3(0,5,0));
                                delete serverData->ghostCar;
                                serverData->ghostCar = 0;
                            }

                            //We actually clicked on something in the world...
                            if(idx != -1 && serverData->cameraTarget)
                            {
                                if(clientEnvironment.currentlyOpen == brickBar)
                                    serverData->ourTempBrick->teleport(BtToGlm(res.m_hitPointWorld[idx]));

                                if(serverData->box->currentPhase == selectionBox::selectionPhase::waitingForClick)
                                {
                                    serverData->box->minExtents = res.m_hitPointWorld[idx];
                                    serverData->box->maxExtents = res.m_hitPointWorld[idx];
                                    serverData->box->currentPhase = selectionBox::selectionPhase::selecting;
                                    serverData->box->movePulls();
                                }
                                else if(serverData->box->currentPhase == selectionBox::selectionPhase::selecting)
                                {
                                    serverData->box->currentPhase = selectionBox::selectionPhase::stretching;
                                }
                            }
                        }
                    }
                }

                //In case we quit or returned to main menu during event polling
                if(currentState != STATE_PLAYING)
                    break;

                if(justTurnOnChat)
                {
                    justTurnOnChat = false;
                    chatEditbox->setText("");
                }
                supress |= chatEditbox->isActive();

                playerInput.supress(supress);

                if(playerInput.commandPressed(toggleGUI))
                    hud->setVisible(!hud->isVisible());

                if(playerInput.commandPressed(debugInfo))
                {
                    if(serverData->currentPlayer)
                    {
                        btTransform t = serverData->currentPlayer->body->getWorldTransform();
                        btVector3 v = t.getOrigin();
                        std::cout<<"Physics pos: "<<v.x()<<","<<v.y()<<","<<v.z()<<"\n";
                    }

                    std::cout<<"Position: "<<serverData->playerCamera->getPosition().x<<","<<serverData->playerCamera->getPosition().y<<","<<serverData->playerCamera->getPosition().z<<"\n";
                    std::cout<<"Direction: "<<serverData->playerCamera->getDirection().x<<","<<serverData->playerCamera->getDirection().y<<","<<serverData->playerCamera->getDirection().z<<"\n";

                    std::cout<<"Num dynamics: "<<serverData->newDynamics.size()<<"\n";
                    std::cout<<"Num items: "<<serverData->items.size()<<"\n";
                    std::cout<<"Num emitters: "<<serverData->emitters.size()<<"\n";
                    std::cout<<"Num lights: "<<serverData->lights.size()<<"\n";
                    std::cout<<"Num cars: "<<serverData->livingBricks.size()<<"\n";
                    std::cout<<"Last crit packet ID: "<<serverData->connection->nextPacketID<<"\n";
                    std::cout<<"Highest server crit ID: "<<serverData->connection->highestServerCritID<<"\n";
                    std::cout<<"Sound locations: "<<location::locations.size()<<"\n";

                    std::cout<<"Program open for "<<SDL_GetTicks()<<"ms\n";

                    //for(int a = 0; a<32; a++)
                    //    std::cout<<"Of packet type "<<a<<" received "<<serverData->numGottenPackets[a]<<" packets.\n";
                }

                if(playerInput.commandPressed(playersListButton))
                {
                    clientEnvironment.playerList->setVisible(true);
                    clientEnvironment.playerList->moveToFront();
                }

                if(playerInput.commandPressed(dropCameraAtPlayer))
                {
                    packet data;
                    data.writeUInt(14,4);
                    data.writeBit(true);
                    serverData->connection->send(&data,true);
                }

                if(playerInput.commandPressed(dropPlayerAtCamera))
                {
                    packet data;
                    data.writeUInt(14,4);
                    data.writeBit(false);
                    data.writeFloat(serverData->playerCamera->getPosition().x);
                    data.writeFloat(serverData->playerCamera->getPosition().y);
                    data.writeFloat(serverData->playerCamera->getPosition().z);
                    serverData->connection->send(&data,true);
                }

                if(playerInput.commandPressed(changeMaterial))
                {
                    clientEnvironment.currentlyOpen = paintCan;

                    switch(serverData->ourTempBrick->mat)
                    {
                        case brickMaterial::none: default: serverData->ourTempBrick->mat = undulo; break;
                        case brickMaterial::undulo: serverData->ourTempBrick->mat = bob; break;
                        case brickMaterial::bob: serverData->ourTempBrick->mat = peral; break;
                        case brickMaterial::peral: serverData->ourTempBrick->mat = chrome; break;
                        case brickMaterial::chrome: serverData->ourTempBrick->mat = glow; break;
                        case brickMaterial::glow: serverData->ourTempBrick->mat = blink; break;
                        case brickMaterial::blink: serverData->ourTempBrick->mat = swirl; break;
                        case brickMaterial::swirl: serverData->ourTempBrick->mat = slippery; break;
                        case brickMaterial::slippery: serverData->ourTempBrick->mat = foil; break;
                        case brickMaterial::foil: serverData->ourTempBrick->mat = none; break;
                    }

                    clientEnvironment.palette->window->getChild("PaintName")->setText(getBrickMatName(serverData->ourTempBrick->mat));

                    clientEnvironment.palette->open();
                }
                if(playerInput.commandPressed(startSelection))
                    serverData->box->currentPhase = selectionBox::selectionPhase::waitingForClick;
                if(playerInput.commandPressed(dropPlayerAtCamera) && serverData->cameraTarget)
                {
                    //serverData->cameraTarget->setPosition(btVector3(serverData->playerCamera->getPosition().x,serverData->playerCamera->getPosition().y,serverData->playerCamera->getPosition().z));
                    //serverData->cameraTarget->setLinearVelocity(glm::vec3(0,0,0));
                }
                if(playerInput.commandPressed(changeCameraMode))
                {
                    if(camMode == cammode_firstPerson)
                        camMode = cammode_thirdPerson;
                    else if(camMode == cammode_thirdPerson)
                        camMode = cammode_firstPerson;
                    /*camMode++;
                    if(camMode > 2)
                        camMode = 0;*/
                }
                if(playerInput.commandPressed(openBrickSelector))
                {
                    context.setMouseLock(false);
                    clientEnvironment.brickSelector->setVisible(true);
                    clientEnvironment.brickSelector->moveToFront();
                }
                if(playerInput.commandPressed(openInventory))
                {
                    if(clientEnvironment.currentlyOpen == inventory)
                        clientEnvironment.currentlyOpen = allClosed;
                    else
                        clientEnvironment.currentlyOpen = inventory;
                }
                if(playerInput.commandPressed(openEvalWindow))
                {
                    context.setMouseLock(false);
                    evalWindow->setVisible(true);
                    evalWindow->moveToFront();
                    if(evalWindow->getChild("Code")->isVisible())
                    {
                        CEGUI::Listbox *codeBox = (CEGUI::Listbox*)evalWindow->getChild("Code/Editbox");
                        codeBox->activate();
                    }
                }

                if(playerInput.commandPressed(openOptions))
                {
                    context.setMouseLock(false);
                    optionsWindow->moveToFront();
                    optionsWindow->setVisible(true);
                }
                if(playerInput.commandPressed(toggleMouseLock))
                    context.setMouseLock(!context.getMouseLocked());

                if(playerInput.commandPressed(changePaintColumn))
                {
                    clientEnvironment.currentlyOpen = paintCan;
                    glm::vec3 pos = serverData->playerCamera->getPosition();
                    //clientEnvironment.speaker->playSound("Rattle",false,pos.x,pos.y,pos.z);
                    clientEnvironment.speaker->playSound2D(clientEnvironment.speaker->resolveSound("Rattle"));
                    clientEnvironment.palette->advanceColumn();
                    serverData->ourTempBrick->basicChanged = serverData->ourTempBrick->basicChanged || serverData->ourTempBrick->isBasic;
                    serverData->ourTempBrick->specialChanged = serverData->ourTempBrick->specialChanged || !serverData->ourTempBrick->isBasic;
                }

                bottomBarShouldBeOpen = clientEnvironment.currentlyOpen == brickBar; //serverData->ourTempBrick->brickSlotSelected != -1;
                float bottomBarPos = std::clamp(((float)SDL_GetTicks() - bottomBarLastAct) / bottomBarOpenTime,0.f,1.f);
                if(bottomBarShouldBeOpen)
                    bottomBarPos = bottomBarClose + bottomBarPos * (bottomBarOpen - bottomBarClose);
                else
                    bottomBarPos = bottomBarOpen + bottomBarPos * (bottomBarClose - bottomBarOpen);
                CEGUI::UVector2 oldPos = brickPopup->getPosition();
                CEGUI::UDim x = oldPos.d_x;
                CEGUI::UDim y = oldPos.d_y;
                y.d_scale = bottomBarPos;
                brickPopup->setPosition(CEGUI::UVector2(x,y));

                if(serverData->ourTempBrick->brickSlotSelected != -1)
                {
                    CEGUI::UDim dx = brickHighlighter->getPosition().d_x;
                    CEGUI::UDim dy = brickHighlighter->getPosition().d_y;
                    dx.d_scale = 0.0020202 + (serverData->ourTempBrick->brickSlotSelected-1) * 0.111;
                    brickHighlighter->setPosition(CEGUI::UVector2(dx,dy));
                }

                bool guiOpened=false,guiClosed=false;
                serverData->ourTempBrick->selectBrick(playerInput,hud,clientEnvironment.brickSelector,serverData->staticBricks,guiOpened,guiClosed);
                if(guiOpened && !guiClosed)
                {
                    if(clientEnvironment.currentlyOpen != brickBar)
                        bottomBarLastAct = SDL_GetTicks();
                    clientEnvironment.currentlyOpen = brickBar;

                }
                if(guiClosed && !guiOpened)
                {
                    if(clientEnvironment.currentlyOpen == brickBar)
                        bottomBarLastAct = SDL_GetTicks();
                    clientEnvironment.currentlyOpen = allClosed;
                }

                if(serverData->ourTempBrick->brickSlotSelected != -1)
                    serverData->ourTempBrick->manipulate(playerInput,hud,clientEnvironment.brickSelector,serverData->playerCamera->getYaw(),clientEnvironment.speaker,serverData->staticBricks);

                if(playerInput.commandPressed(plantBrick))
                {
                    if(serverData->box->currentPhase == selectionBox::selectionPhase::selecting)
                    {
                        packet data;
                        data.writeUInt(5,4);
                        data.writeFloat(serverData->box->minExtents.x());
                        data.writeFloat(serverData->box->minExtents.y());
                        data.writeFloat(serverData->box->minExtents.z());
                        data.writeFloat(serverData->box->maxExtents.x());
                        data.writeFloat(serverData->box->maxExtents.y());
                        data.writeFloat(serverData->box->maxExtents.z());
                        serverData->connection->send(&data,true);

                        serverData->box->currentPhase = selectionBox::selectionPhase::idle;
                    }
                    else
                        serverData->ourTempBrick->plant(serverData->staticBricks,serverData->connection);
                }

                serverData->ourTempBrick->update(serverData->staticBricks,clientEnvironment.palette);

                if(serverData->adminCam && camMode != cammode_adminCam)
                    camMode = cammode_adminCam;
                else if(!serverData->adminCam && camMode == cammode_adminCam)
                    camMode = cammode_firstPerson;

                if(camMode == cammode_adminCam) //admin cam
                {
                    crossHair->setVisible(false);
                    serverData->playerCamera->thirdPerson = false;
                    float cameraSpeed = 0.035;
                    if(states[SDL_SCANCODE_LCTRL] == SDL_PRESSED)
                        cameraSpeed = 0.2;
                    if(playerInput.commandKeyDown(walkForward))
                        serverData->playerCamera->walkForward(deltaT * cameraSpeed);
                    if(playerInput.commandKeyDown(walkBackward))
                        serverData->playerCamera->walkForward(-deltaT * cameraSpeed);
                    if(playerInput.commandKeyDown(walkLeft))
                        serverData->playerCamera->walkRight(-deltaT * cameraSpeed);
                    if(playerInput.commandKeyDown(walkRight))
                        serverData->playerCamera->walkRight(deltaT * cameraSpeed);
                }
                else
                {
                    if(serverData->boundToObject && serverData->cameraTarget)
                    {
                        if(camMode == cammode_firstPerson) //1st person
                        {
                            crossHair->setVisible(true);
                            serverData->playerCamera->thirdPerson = false;

                            glm::vec3 pos;
                            if(serverData->currentPlayer && !serverData->giveUpControlOfCurrentPlayer)
                            {
                                btTransform t = serverData->currentPlayer->body->getWorldTransform();
                                btVector3 v = t.getOrigin();
                                pos.x = v.x();
                                pos.y = v.y();
                                pos.z = v.z();
                            }
                            else
                            {
                                if(serverData->currentPlayer)
                                    serverData->currentPlayer->useGlobalTransform = false;
                                pos = serverData->cameraTarget->modelInterpolator.getPosition();
                            }

                            pos += serverData->cameraTarget->type->eyeOffset;
                            serverData->playerCamera->setPosition( pos );
                        }
                        else //3rd person
                        {
                            crossHair->setVisible(false);
                            serverData->playerCamera->thirdPerson = true;

                            //3rd person camera follow distance backwards raycast:
                            btVector3 v;
                            if(serverData->currentPlayer && serverData->currentPlayer->body)
                            {
                                btTransform t = serverData->currentPlayer->body->getWorldTransform();
                                v = t.getOrigin();
                            }
                            else
                            {
                                v = glmToBt(serverData->cameraTarget->modelInterpolator.getPosition());
                            }

                            v += glmToBt(serverData->cameraTarget->type->eyeOffset / glm::vec3(2.0,2.0,2.0));

                            btVector3 raystart = glmToBt(glm::vec3(v.x(),v.y(),v.z()));
                            btVector3 rayend = glmToBt(glm::vec3(v.x(),v.y(),v.z()) - serverData->playerCamera->getDirection() * glm::vec3(30.0));
                            btCollisionWorld::ClosestRayResultCallback res(raystart,rayend);
                            world->rayTest(raystart,rayend,res);

                            /*int idx = -1;
                            float dist = 9999999;

                            if(serverData->cameraTarget)
                            {
                                for(int a = 0; a<res.m_collisionObjects.size(); a++)
                                {
                                    if(res.m_collisionObjects[a] != serverData->cameraTarget->body)
                                    {
                                        if(fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-serverData->playerCamera->getPosition())) < dist)
                                        {
                                            dist = fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-serverData->playerCamera->getPosition()));
                                            idx = a;
                                        }
                                    }
                                }
                            }*/

                            if(serverData->currentPlayer && serverData->currentPlayer->body && !serverData->giveUpControlOfCurrentPlayer)
                            {
                                btTransform t = serverData->currentPlayer->body->getWorldTransform();
                                v = t.getOrigin();
                                serverData->playerCamera->thirdPersonTarget = glm::vec3(v.x(),v.y(),v.z()) + serverData->cameraTarget->type->eyeOffset / glm::vec3(2.0,2.0,2.0);
                                serverData->playerCamera->thirdPersonDistance = 30 * res.m_closestHitFraction * 0.98;
                                serverData->playerCamera->setPosition(glm::vec3(v.x(),v.y(),v.z()));
                                serverData->playerCamera->turn(0,0);
                                serverData->currentPlayer->useGlobalTransform = true;
                                btQuaternion bulletRot = t.getRotation();
                                glm::quat rot = glm::quat(bulletRot.w(),bulletRot.x(),bulletRot.y(),bulletRot.z());
                                serverData->currentPlayer->globalTransform = glm::translate(glm::vec3(v.x(),v.y(),v.z())) * glm::toMat4(rot);
                            }
                            else
                            {
                                if(serverData->currentPlayer)
                                    serverData->currentPlayer->useGlobalTransform = false;

                                serverData->playerCamera->thirdPersonTarget = serverData->cameraTarget->modelInterpolator.getPosition() + serverData->cameraTarget->type->eyeOffset / glm::vec3(2.0,2.0,2.0);
                                serverData->playerCamera->thirdPersonDistance = 30 * res.m_closestHitFraction * 0.98;
                                serverData->playerCamera->setPosition(serverData->cameraTarget->modelInterpolator.getPosition());
                                serverData->playerCamera->turn(0,0);
                            }
                        }
                        //TODO: Should just store which item is the current held not-hidden item, loop is bad
                        for(int a = 0; a<serverData->items.size(); a++)
                        {
                            if(serverData->currentPlayer && serverData->items[a]->heldBy == serverData->currentPlayer && !serverData->items[a]->hidden)
                            {
                                if(serverData->giveUpControlOfCurrentPlayer)
                                    serverData->items[a]->updateTransform(false,serverData->playerCamera->getYaw(),serverData->playerCamera->getPitch());
                                else
                                    serverData->items[a]->updateTransform(true,serverData->playerCamera->getYaw(),serverData->playerCamera->getPitch());
                                serverData->items[a]->calculateMeshTransforms(deltaT); //The item we are currently holding
                            }
                        }
                    }
                    else //static cam
                    {
                        serverData->playerCamera->thirdPerson = false;
                        serverData->playerCamera->setPosition(serverData->cameraPos);
                        if(!serverData->freeLook)
                        {
                            serverData->playerCamera->setDirection(serverData->cameraDir);
                            serverData->playerCamera->turn(0,0);
                        }
                    }
                }

                if(playerInput.commandKeyDown(zoom))
                    desiredFov = 15;
                else
                    desiredFov = clientEnvironment.settings->fieldOfView;

                currentZoom += (desiredFov - currentZoom) * deltaT * 0.0025;
                serverData->playerCamera->setFieldOfVision(currentZoom);

                glm::vec3 microphoneDir = glm::vec3(sin(serverData->playerCamera->getYaw()),0,cos(serverData->playerCamera->getYaw()));
                //clientEnvironment.speaker->microphone(serverData->playerCamera->getPosition()+glm::vec3(0.05,0.05,0.05),microphoneDir);
                clientEnvironment.speaker->update(serverData->playerCamera->getPosition(),microphoneDir);

                if(playerInput.commandKeyDown(jump))
                    doJump = true;

                for(unsigned int a = 0; a<serverData->items.size(); a++)
                {
                    heldItemType *type = serverData->items[a]->itemType;
                    if(type->useDefaultSwing)
                    {
                        if(serverData->items[a]->heldBy == serverData->cameraTarget)
                            serverData->items[a]->advance(context.getMouseLocked() && mouseState & SDL_BUTTON_LEFT,deltaT);
                        else
                            serverData->items[a]->advance(false,deltaT);
                    }
                    else
                        serverData->items[a]->advance(false,deltaT);
                    serverData->items[a]->bufferSubData();
                }

                for(int a = 0; a<serverData->newDynamics.size(); a++)
                {
                    serverData->newDynamics[a]->hidden = (serverData->newDynamics[a] == serverData->cameraTarget) && (camMode == cammode_firstPerson);
                    serverData->newDynamics[a]->calculateMeshTransforms(deltaT);
                    serverData->newDynamics[a]->bufferSubData();
                }
                for(int a = 0; a<serverData->items.size(); a++)
                    if(!(serverData->currentPlayer && serverData->items[a]->heldBy == serverData->currentPlayer && !serverData->items[a]->hidden))
                        serverData->items[a]->calculateMeshTransforms(deltaT); //Any item we are not currently holding

                auto emitterIter = serverData->emitters.begin();
                while(emitterIter != serverData->emitters.end())
                {
                    emitter *e = *emitterIter;
                    if(e->type->lifetimeMS != 0 && !e->attachedToCar && !e->attachedToBasicBrick && !e->attachedToSpecialBrick)
                    {
                        if(e->creationTime + e->type->lifetimeMS < SDL_GetTicks())
                        {
                            delete e;
                            emitterIter = serverData->emitters.erase(emitterIter);
                            continue;
                        }
                    }
                    bool usePhysicsPos = serverData->currentPlayer && !serverData->giveUpControlOfCurrentPlayer;
                    e->update(serverData->playerCamera->position,
                              serverData->playerCamera->direction,
                              usePhysicsPos,
                              (camMode == cammode_firstPerson) && (serverData->currentPlayer == e->attachedToModel || (e->attachedToItem && serverData->currentPlayer == e->attachedToItem->heldBy)),
                              serverData->playerCamera->getYaw(),
                              serverData->playerCamera->getPitch());
                    ++emitterIter;
                }

                for(unsigned int a = 0; a<serverData->particleTypes.size(); a++)
                    serverData->particleTypes[a]->deadParticlesCheck(serverData->playerCamera->getPosition(),deltaT);

                sortLights(serverData->lights,serverData->playerCamera->getPosition(),deltaT);

                //Networking:

                serverData->tryApplyHeldPackets();

                checkForCameraToBind(serverData);

                if(connected)
                {
                    glm::vec3 up = glm::vec3(0,1,0);
                    serverData->playerCamera->nominalUp = up;

                    if(serverData->cameraLean && camMode == 0 && serverData->cameraTarget)
                    {
                        serverData->totalLean += playerInput.commandKeyDown(walkLeft) ? deltaT * 0.001 : 0;
                        serverData->totalLean -= playerInput.commandKeyDown(walkRight) ? deltaT * 0.001 : 0;
                        if(!(playerInput.commandKeyDown(walkRight) || playerInput.commandKeyDown(walkLeft)))
                            serverData->totalLean *= 0.9;

                        if(serverData->totalLean > 0.17)
                            serverData->totalLean = 0.17;
                        if(serverData->totalLean < -0.17)
                            serverData->totalLean = -0.17;

                        glm::vec3 playerForward = glm::vec3(0,0,1);//serverData->playerCamera->getDirection();
                        glm::vec3 playerRight = glm::normalize(glm::cross(up,playerForward));

                        glm::vec3 dir = glm::normalize(glm::vec3(serverData->totalLean * playerRight.x,1,serverData->totalLean * playerRight.z));
                        glm::vec4 afterDir = glm::toMat4(serverData->cameraTarget->modelInterpolator.getRotation()) * glm::vec4(dir.x,dir.y,dir.z,0);
                        serverData->playerCamera->nominalUp = glm::vec3(afterDir.x,afterDir.y,afterDir.z);
                    }

                    bool didJump = false;
                    int netControlState = 0;
                    netControlState |= (!states[SDL_SCANCODE_LCTRL] && playerInput.commandKeyDown(walkForward)) ? 1 : 0;
                    netControlState |= playerInput.commandKeyDown(walkBackward) ? 2 : 0;
                    netControlState |= playerInput.commandKeyDown(walkLeft) ? 4 : 0;
                    netControlState |= playerInput.commandKeyDown(walkRight) ? 8 : 0;
                    netControlState |= doJump ? 16 : 0;
                    //if(camMode == 2)
                      //  netControlState = 0;
                    if(evalWindow->isActive() && evalWindow->isVisible())
                        netControlState = 0;

                    //Move client player for client physics:
                    if(serverData->currentPlayer && !serverData->giveUpControlOfCurrentPlayer && camMode != cammode_adminCam)
                    {
                        if(SDL_GetTicks() - serverData->currentPlayer->flingPreventionStartTime > 100)
                        {
                            if(serverData->currentPlayer->control(atan2(serverData->playerCamera->getDirection().x,serverData->playerCamera->getDirection().z),netControlState & 1,netControlState & 2,netControlState & 4,netControlState & 8,netControlState &16,serverData->canJet && rightDown))
                                didJump = true;
                            if(didJump)
                                clientEnvironment.speaker->playSound2D(clientEnvironment.speaker->resolveSound("Jump"));
                                //clientEnvironment.speaker->playSound("Jump",false,serverData->playerCamera->getPosition().x,serverData->playerCamera->getPosition().y-1.0,serverData->playerCamera->getPosition().z);
                        }
                    }

                    int fullControlState  = netControlState + (leftDown << 5);
                    fullControlState |= (clientEnvironment.currentlyOpen == inventory) ? 0b1000000 : 0;
                    fullControlState |= serverData->selectedSlot << 7;
                    fullControlState += rightDown << 11;
                    fullControlState |= (clientEnvironment.currentlyOpen == paintCan) ? (1 << 12) : 0;
                    fullControlState |= chatEditbox->isActive() ? (1 << 13) : 0;

                    glm::vec3 playerDirDiff = serverData->playerCamera->getDirection() - lastPlayerDir;
                    if(fabs(glm::length(playerDirDiff)) > 0.02 || fullControlState != lastPlayerControlMask || doJump || SDL_GetTicks() > (unsigned)(lastSentTransData + 1000))
                    {
                        if(SDL_GetTicks() > (unsigned)(lastSentTransData + 5))
                        {
                            lastPlayerControlMask = fullControlState;

                            packet transPacket;
                            transPacket.writeUInt(2,4);
                            if(camMode == 2)
                                netControlState = 0;
                            transPacket.writeUInt(netControlState,5);
                            transPacket.writeBit(didJump);//This one controls if the sound is played
                            transPacket.writeBit(context.getMouseLocked() && leftDown);
                            transPacket.writeBit(context.getMouseLocked() && rightDown);
                            transPacket.writeBit(clientEnvironment.currentlyOpen == paintCan);
                            if(clientEnvironment.currentlyOpen == paintCan)
                            {
                                glm::vec4 paintColor = clientEnvironment.palette->getColor();
                                transPacket.writeFloat(paintColor.r);
                                transPacket.writeFloat(paintColor.g);
                                transPacket.writeFloat(paintColor.b);
                                transPacket.writeFloat(paintColor.a);
                            }

                            doJump = false;

                            lastPlayerDir = serverData->playerCamera->getDirection();
                            transPacket.writeFloat(serverData->playerCamera->getDirection().x);
                            transPacket.writeFloat(serverData->playerCamera->getDirection().y);
                            transPacket.writeFloat(serverData->playerCamera->getDirection().z);
                            if(clientEnvironment.currentlyOpen == inventory)
                                transPacket.writeUInt(serverData->selectedSlot,3);
                            else
                                transPacket.writeUInt(7,3);

                            transPacket.writeBit(serverData->adminCam);
                            if(serverData->adminCam)
                            {
                                transPacket.writeFloat(serverData->playerCamera->getPosition().x);
                                transPacket.writeFloat(serverData->playerCamera->getPosition().y);
                                transPacket.writeFloat(serverData->playerCamera->getPosition().z);
                            }
                            transPacket.writeBit(chatEditbox->isActive());

                            bool actuallyUseClientPhysics = useClientPhysics;
                            if(!serverData->currentPlayer)
                                actuallyUseClientPhysics = false;
                            if(serverData->giveUpControlOfCurrentPlayer)
                                actuallyUseClientPhysics = false;

                            transPacket.writeBit(actuallyUseClientPhysics);
                            if(actuallyUseClientPhysics)
                            {
                                btTransform t = serverData->currentPlayer->body->getWorldTransform();
                                btVector3 o = t.getOrigin();
                                transPacket.writeFloat(o.x());
                                transPacket.writeFloat(o.y());
                                transPacket.writeFloat(o.z());
                                btVector3 v = serverData->currentPlayer->body->getLinearVelocity();
                                transPacket.writeFloat(v.x());
                                transPacket.writeFloat(v.y());
                                transPacket.writeFloat(v.z());
                            }

                            serverConnection->send(&transPacket,false);

                            lastSentTransData = SDL_GetTicks();
                        }
                    }

                    float progress = ((float)serverData->staticBricks.getBrickCount()) / ((float)serverData->skippingCompileNextBricks);
                    setBrickLoadProgress(progress,serverData->staticBricks.getBrickCount());

                    serverConnection->run();
                }

                for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                {
                    //std::cout<<"Advancing: "<<deltaT<<"\n";
                    serverData->livingBricks[a]->carTransform.advance(deltaT);
                    //std::cout<<"\n";

                    for(int wheel = 0; wheel<serverData->livingBricks[a]->newWheels.size(); wheel++)
                    {
                        serverData->livingBricks[a]->newWheels[wheel]->calculateMeshTransforms(deltaT);
                        serverData->livingBricks[a]->newWheels[wheel]->bufferSubData();
                    }
                }

                for(unsigned int a = 0; a<serverData->items.size(); a++)
                {
                    if(!(!serverData->giveUpControlOfCurrentPlayer && serverData->currentPlayer && serverData->items[a]->heldBy == serverData->currentPlayer && !serverData->items[a]->hidden))
                        serverData->items[a]->updateTransform();
                }

                //std::cout<<serverData->staticBricks.opaqueBasicBricks.size()<<" bricks\n";

                auto fakeKillIter = serverData->fakeKills.begin();
                while(fakeKillIter != serverData->fakeKills.end())
                {
                    if((*fakeKillIter).endTime < SDL_GetTicks())
                    {
                        if((*fakeKillIter).basic)
                            serverData->staticBricks.removeBasicBrick((*fakeKillIter).basic,serverData->world);
                        if((*fakeKillIter).special)
                            serverData->staticBricks.removeSpecialBrick((*fakeKillIter).special,serverData->world);
                        fakeKillIter = serverData->fakeKills.erase(fakeKillIter);
                    }
                    else
                    {
                        (*fakeKillIter).linVel.y -= deltaT / 2500.0;
                        float progress = (*fakeKillIter).endTime - (*fakeKillIter).startTime;
                        progress = ((float)SDL_GetTicks() - (float)(*fakeKillIter).startTime) / progress;
                        if((*fakeKillIter).basic)
                        {
                            (*fakeKillIter).basic->position += (*fakeKillIter).linVel;
                            (*fakeKillIter).basic->rotation = glm::slerp((*fakeKillIter).basic->rotation,glm::quat((*fakeKillIter).angVel),progress);
                            serverData->staticBricks.updateBasicBrick((*fakeKillIter).basic,serverData->world);
                        }
                        if((*fakeKillIter).special)
                        {
                            (*fakeKillIter).special->position += (*fakeKillIter).linVel;
                            (*fakeKillIter).special->rotation = glm::slerp((*fakeKillIter).special->rotation,glm::quat((*fakeKillIter).angVel),progress);
                            serverData->staticBricks.updateSpecialBrick((*fakeKillIter).special,serverData->world,0);
                        }
                        fakeKillIter++;
                    }
                }

                /*glFlush();
                glFinish();
                SDL_Delay(1);*/

                //Graphics:
                //Sun direction sundirection

                uniformsHolder *whichBrickUnis = serverData->env->useIBL ? brickUnis : brickDNCUnis;
                uniformsHolder *whichModelUnis = serverData->env->useIBL ? modelUnis : modelDNCUnis;

                serverData->env->calc(deltaT*10.0,serverData->playerCamera);

                bdrf->bind(brdf);

                std::string oldName = "";

                serverData->env->godRayPass->bind();
                //if(clientEnvironment.settings->godRayQuality != godRaysOff)
                //{
                    godPrePassSunUnis->use();

                        glUniform1i(godPrePassSunUnis->doingGodRayPass,true);
                        oldName = serverData->playerCamera->name;
                        serverData->playerCamera->name = "Shadow";
                        serverData->playerCamera->render(godPrePassSunUnis);
                        serverData->env->passUniforms(godPrePassSunUnis);
                        serverData->env->drawSun(godPrePassSunUnis);
                        glUniform1i(godPrePassSunUnis->doingGodRayPass,false);

                    godPrePassBrickUnis->use();

                        glUniform1i(godPrePassBrickUnis->doingGodRayPass,true);
                        serverData->playerCamera->render(godPrePassBrickUnis);
                        serverData->playerCamera->name = oldName;
                        serverData->staticBricks.renderEverything(godPrePassBrickUnis,true,0,SDL_GetTicks()/25);
                        for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                            serverData->livingBricks[a]->renderAlive(godPrePassBrickUnis,true,SDL_GetTicks()/25);
                        glUniform1i(godPrePassBrickUnis->doingGodRayPass,false);
                //}
                serverData->env->godRayPass->unbind();

                //Begin drawing to water textures

                waterFrame++;
                if(waterFrame >= 2)
                    waterFrame = 0;

                if(clientEnvironment.settings->waterQuality != waterStatic && waterRefraction)
                {
                    if(waterFrame == 0)
                    {
                        waterReflection->bind();

                            oldModelUnis->use();
                                glUniform1f(oldModelUnis->clipHeight,serverData->waterLevel);
                                serverData->playerCamera->renderReflection(oldModelUnis,serverData->waterLevel);
                                serverData->env->passUniforms(oldModelUnis);
                                clientEnvironment.settings->render(oldModelUnis);
                                serverData->env->drawSky(oldModelUnis);

                                glEnable(GL_CLIP_DISTANCE0);

                            whichModelUnis->use();
                                glUniform1f(whichModelUnis->clipHeight,serverData->waterLevel);
                                clientEnvironment.settings->render(whichModelUnis);
                                serverData->playerCamera->renderReflection(whichModelUnis,serverData->waterLevel);
                                serverData->env->passUniforms(whichModelUnis);
                                for(int a = 0; a<serverData->newDynamicTypes.size(); a++)
                                    serverData->newDynamicTypes[a]->renderInstanced(whichModelUnis);

                                clientEnvironment.newWheelModel->renderInstanced(whichModelUnis);


                                /*for(unsigned int a = 0; a<serverData->dynamics.size(); a++)
                                        serverData->dynamics[a]->render(&basic);*/

                             /*tessUnis->use();
                                glUniform1f(tess.clipHeight,waterLevel);
                                serverData->playerCamera->render(tess);
                                serverData->env->passUniforms(tess);
                                grass.use(tess);
                                theMap.render(tess);*/

                            whichBrickUnis->use();
                                glUniform1i(whichBrickUnis->target->getUniformLocation("debugMode"),debugMode);

                                glUniform1f(whichBrickUnis->clipHeight,serverData->waterLevel);
                                serverData->playerCamera->renderReflection(whichBrickUnis,serverData->waterLevel);
                                serverData->env->passUniforms(whichBrickUnis);
                                clientEnvironment.settings->render(whichBrickUnis);

                                glEnable(GL_BLEND);
                                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                                serverData->staticBricks.renderEverything(whichBrickUnis,false,&grass,SDL_GetTicks()/25);
                                for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                                    serverData->livingBricks[a]->renderAlive(whichBrickUnis,false,SDL_GetTicks()/25);
                                glDisable(GL_BLEND);

                        waterReflection->unbind();
                    }
                    else if(waterFrame == 1)
                    {
                        glEnable(GL_CLIP_DISTANCE0);
                        waterRefraction->bind();

                            whichModelUnis->use();
                                glUniform1f(whichModelUnis->clipHeight,-serverData->waterLevel);
                                clientEnvironment.settings->render(whichModelUnis);
                                serverData->playerCamera->render(whichModelUnis);
                                serverData->env->passUniforms(whichModelUnis);
                                for(int a = 0; a<serverData->newDynamicTypes.size(); a++)
                                    serverData->newDynamicTypes[a]->renderInstanced(whichModelUnis);

                                clientEnvironment.newWheelModel->renderInstanced(whichModelUnis);

                            oldModelUnis->use();
                                glUniform1f(oldModelUnis->clipHeight,-serverData->waterLevel);
                                serverData->playerCamera->render(oldModelUnis);
                                serverData->env->passUniforms(oldModelUnis);

                            /*tessUnis->use();
                                serverData->playerCamera->render(tess);
                                glUniform1f(tess.clipHeight,-waterLevel);
                                serverData->env->passUniforms(tess);
                                grass.use(tess);
                                theMap.render(tess);*/

                            whichBrickUnis->use();
                            glUniform1i(whichBrickUnis->target->getUniformLocation("debugMode"),debugMode);
                            serverData->env->passUniforms(whichBrickUnis);
                            clientEnvironment.settings->render(whichBrickUnis);
                                glUniform1f(whichBrickUnis->clipHeight,-serverData->waterLevel);
                                serverData->playerCamera->render(whichBrickUnis);
                                glEnable(GL_BLEND);
                                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                                serverData->staticBricks.renderEverything(whichBrickUnis,false,&grass,SDL_GetTicks()/25);
                                for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                                    serverData->livingBricks[a]->renderAlive(whichBrickUnis,false,SDL_GetTicks()/25);
                                glDisable(GL_BLEND);


                        waterRefraction->unbind();
                    }
                    glDisable(GL_CLIP_DISTANCE0);

                    /*waterDepth->bind();
                         /*tessUnis->use();
                            glUniform1f(tess.clipHeight,-waterLevel);
                            serverData->playerCamera->render(tess);
                            grass.use(tess); //TODO: Yes, at the moment, this is required for some reason...
                            theMap.render(tess);

                        brickUnis->use();
                            serverData->playerCamera->render(brickUnis);
                            serverData->env->passUniforms(brickUnis);
                            serverData->staticBricks.renderEverything(brickUnis,true,0,SDL_GetTicks()/25);
                            for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                                serverData->livingBricks[a]->renderAlive(brickUnis,true,SDL_GetTicks()/25);

                    waterDepth->unbind();*/
                }

                //End drawing to water textures

                if(clientEnvironment.settings->shadowResolution != shadowsOff)
                {
                    //Begin drawing shadows to shadow texture
                    glDisable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                    glEnable(GL_BLEND);

                    serverData->env->shadowBuffer->bind();

                        modelShadowUnis->use();
                            serverData->env->passLightMatricies(modelShadowUnis);
                            for(int a = 0; a<serverData->newDynamicTypes.size(); a++)
                                serverData->newDynamicTypes[a]->renderInstancedWithoutMaterials();


                        glDisable(GL_CULL_FACE);

                        shadowBrickUnis->use();
                                serverData->env->passLightMatricies(shadowBrickUnis);
                                serverData->staticBricks.renderEverything(shadowBrickUnis,true,0,SDL_GetTicks()/25);
                                for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                                    serverData->livingBricks[a]->renderAlive(shadowBrickUnis,true,SDL_GetTicks()/25);

                        glEnable(GL_CULL_FACE);

                    serverData->env->shadowBuffer->unbind();

                    glDisable(GL_BLEND);
                    glCullFace(GL_BACK);
                    glEnable(GL_CULL_FACE);
                    //End drawing shadows to shadow texture
                }

                //Begin drawing final scene
                context.clear(serverData->env->skyColor.r,serverData->env->skyColor.g,serverData->env->skyColor.b);
                context.select();

                    oldModelUnis->use();
                        clientEnvironment.settings->render(oldModelUnis);
                        serverData->playerCamera->render(oldModelUnis); //change
                        serverData->env->passUniforms(oldModelUnis);
                        renderLights(oldModelUnis,serverData->lights);

                        serverData->env->drawSky(oldModelUnis);

                        if(clientEnvironment.settings->waterQuality != waterStatic && waterRefraction)
                        {
                            waterUnis->use();
                                clientEnvironment.settings->render(waterUnis);
                                glUniform1f(waterUnis->deltaT,((float)SDL_GetTicks()) / 1000.0);     //why both of these...?
                                glUniform1f(waterUnis->waterDelta,((float)SDL_GetTicks())*0.0001);
                                glUniform1f(waterUnis->target->getUniformLocation("waterLevel"),serverData->waterLevel); //TODO: Don't string match this every frame
                                serverData->env->passUniforms(waterUnis,true);
                                serverData->playerCamera->render(waterUnis);

                                waterReflection->colorResult->bind(reflection);
                                waterRefraction->colorResult->bind(refraction);
                                //waterDepth->depthResult->bind(shadowNearMap);
                                dudvTexture->bind(normal);

                                water.render(waterUnis);

                                oldModelUnis->use();
                        }

                        //Render faces for players, pretty much:
                        for(unsigned int a = 0; a<serverData->newDynamicTypes.size(); a++)
                            serverData->newDynamicTypes[a]->renderNonInstanced(oldModelUnis);

                        //Start rendering item icons...
                        oldModelUnis->setModelMatrix(glm::mat4(1.0));

                        for(unsigned int a = 0; a<serverData->items.size(); a++)
                        {
                            //Skip updating transforms for items that we are currently holding, since we should use client physics transform for that instead of interpolated server snapshots...
                            if(!(!serverData->giveUpControlOfCurrentPlayer && serverData->currentPlayer && serverData->items[a]->heldBy == serverData->currentPlayer && !serverData->items[a]->hidden))
                                serverData->items[a]->updateTransform();
                        }

                        //Render paint can or not...
                        if(serverData->paintCan && serverData->fixedPaintCanItem && clientEnvironment.currentlyOpen == paintCan && serverData->cameraTarget)
                        {
                            serverData->fixedPaintCanItem->pitch = -1.57;
                            serverData->fixedPaintCanItem->hidden = false;
                            serverData->fixedPaintCanItem->heldBy = serverData->cameraTarget;
                            serverData->fixedPaintCanItem->updateTransform(true,serverData->playerCamera->getYaw());
                        }
                        else
                            serverData->fixedPaintCanItem->hidden = true;

                        //Item icons...
                        //Start all item icons as hidden by default...
                        /*for(int a = 0; a<serverData->itemIcons.size(); a++)
                            if(serverData->itemIcons[a]) //Paint can at a bare minimum will NOT have any icon!
                                serverData->itemIcons[a]->hidden = true;*/

                        if(clientEnvironment.currentlyOpen == inventory)
                        {
                            //For each inventory slot
                            for(unsigned int a = 0; a<inventorySize; a++)
                            {
                                heldItemType *type = serverData->inventory[a];
                                if(!type)
                                {
                                    hud->getChild("Inventory/ItemIcon" + std::to_string(a+1) + "/icon")->setProperty("Image","");
                                    continue;
                                }

                                if(type->uiName.length() < 1)
                                {
                                    hud->getChild("Inventory/ItemIcon" + std::to_string(a+1) + "/icon")->setProperty("Image","");
                                    continue;
                                }

                                hud->getChild("Inventory/ItemIcon" + std::to_string(a+1) + "/icon")->setProperty("Image",type->uiName);
                            }
                        }

                        //Preview texture:
                        if(showPreview)
                        {
                            glUniform1i(oldModelUnis->previewTexture,true);
                            serverData->env->godRayPass->colorResult->bind(normal);
                            glBindVertexArray(quadVAO);
                            glDrawArrays(GL_TRIANGLES,0,6);
                            glBindVertexArray(0);
                            glUniform1i(oldModelUnis->previewTexture,false);
                        }
                        //end preview texture

                        //drawDebugLocations(oldModelUnis,cubeVAO,serverData->debugLocations,serverData->debugColors);

        /*            tessUnis->use();
                        clientEnvironment.settings->render(tess);
                        serverData->playerCamera->render(tess); //change
                        serverData->env->passUniforms(tess);*/

        //                grass.use(tess);
         //               theMap.render(tess);

                        whichModelUnis->use();
                            clientEnvironment.settings->render(whichModelUnis);
                            serverData->playerCamera->render(whichModelUnis);
                            serverData->env->passUniforms(whichModelUnis);
                            renderLights(whichModelUnis,serverData->lights);
                            for(int a = 0; a<serverData->newDynamicTypes.size(); a++)
                                serverData->newDynamicTypes[a]->renderInstanced(whichModelUnis);

                        clientEnvironment.newWheelModel->renderInstanced(whichModelUnis);

                    whichBrickUnis->use();
                        serverData->playerCamera->render(whichBrickUnis);
                        serverData->env->passUniforms(whichBrickUnis);
                        clientEnvironment.settings->render(whichBrickUnis);
                        renderLights(whichBrickUnis,serverData->lights);
                        glUniform1i(whichBrickUnis->target->getUniformLocation("debugMode"),debugMode);

                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        serverData->staticBricks.renderEverything(whichBrickUnis,false,&grass,SDL_GetTicks()/25);
                        for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
                            serverData->livingBricks[a]->renderAlive(whichBrickUnis,false,SDL_GetTicks()/25);
                        if(serverData->ghostCar)
                            serverData->ghostCar->renderAlive(whichBrickUnis,false,SDL_GetTicks()/25);
                        glDisable(GL_BLEND);

                        glEnable(GL_BLEND);
                        glDisable(GL_CULL_FACE);
                        emitterUnis->use();
                            serverData->playerCamera->render(emitterUnis);
                            serverData->env->passUniforms(emitterUnis);
                            clientEnvironment.settings->render(emitterUnis);

                            for(unsigned int a = 0; a<serverData->particleTypes.size(); a++)
                                serverData->particleTypes[a]->render(emitterUnis);


                    serverData->bulletTrails->purge();
                    glDisable(GL_CULL_FACE);
                    bulletUnis->use();
                        serverData->playerCamera->render(bulletUnis);
                        serverData->bulletTrails->render(bulletUnis);
                    glEnable(GL_CULL_FACE);


                        glLineWidth(3.0);
                        boxEdgesUnis->use();
                            glUniform1i(boxEdgesUnis->target->getUniformLocation("drawingRopes"),true);
                            serverData->playerCamera->render(boxEdgesUnis);
                            for(unsigned int a = 0; a<serverData->ropes.size(); a++)
                                serverData->ropes[a]->render();
                            glUniform1i(boxEdgesUnis->target->getUniformLocation("drawingRopes"),false);

                    glDisable(GL_DEPTH_TEST);
                    glEnable(GL_BLEND);
                    glDisable(GL_CULL_FACE);
                    fontUnis->use();
                            serverData->playerCamera->render(fontUnis);
                            for(unsigned int a = 0; a<serverData->newDynamics.size(); a++)
                            {
                                if(!serverData->newDynamics[a])
                                    continue;
                                if(serverData->newDynamics[a]->shapeName.length() < 1)
                                    continue;
                                if(serverData->newDynamics[a] == serverData->cameraTarget && camMode == 0)
                                    continue;

                                glm::vec3 pos = serverData->newDynamics[a]->modelInterpolator.getPosition();
                                if(serverData->currentPlayer == serverData->newDynamics[a] && !serverData->giveUpControlOfCurrentPlayer && serverData->currentPlayer->body)
                                {
                                    btVector3 o = serverData->currentPlayer->body->getWorldTransform().getOrigin();
                                    pos = glm::vec3(o.x(),o.y(),o.z());
                                }
                                defaultFont.naiveRender(fontUnis,serverData->newDynamics[a]->shapeName,pos+serverData->newDynamics[a]->type->eyeOffset+glm::vec3(0,2,0),glm::length(serverData->newDynamics[a]->scale),serverData->newDynamics[a]->shapeNameColor);
                            }

                    //glEnable(GL_CULL_FACE);
                    glDisable(GL_BLEND);
                    glEnable(GL_DEPTH_TEST);

                    //Remember, 'god rays' actually includes the underwater texture too
                    screenOverlaysUnis->use();
                        glUniform1f(screenOverlaysUnis->target->getUniformLocation("waterLevel"),serverData->waterLevel); //TODO: Don't string match this every frame
                        glUniform1f(screenOverlaysUnis->target->getUniformLocation("vignetteStrength"),clientEnvironment.vignetteStrength);
                        glUniform3f(screenOverlaysUnis->target->getUniformLocation("vignetteColor"),clientEnvironment.vignetteColor.r,clientEnvironment.vignetteColor.g,clientEnvironment.vignetteColor.b);
                        if(clientEnvironment.vignetteStrength >= 0)
                            clientEnvironment.vignetteStrength -= deltaT * 0.001;
                        if(clientEnvironment.vignetteStrength < 0)
                            clientEnvironment.vignetteStrength = 0;

                        clientEnvironment.settings->render(screenOverlaysUnis);
                        serverData->playerCamera->render(screenOverlaysUnis);
                        serverData->env->passUniforms(screenOverlaysUnis);
                        serverData->env->renderGodRays(screenOverlaysUnis);

                    glEnable(GL_BLEND);

                    boxEdgesUnis->use();
                        serverData->playerCamera->render(boxEdgesUnis);
                        serverData->box->render(boxEdgesUnis);

                    glEnable(GL_CULL_FACE);

                    //Just for the transparent blue quad of crappy water, good water is rendered first thing
                    if(clientEnvironment.settings->waterQuality == waterStatic || !waterRefraction)
                    {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                        waterUnis->use();
                            clientEnvironment.settings->render(waterUnis);
                            serverData->env->passUniforms(waterUnis,true);
                            serverData->playerCamera->render(waterUnis);

                            water.render(waterUnis);

                        glDisable(GL_BLEND);
                    }
                //End drawing final scene

                clientEnvironment.bottomPrint.checkForTimeouts();
                clientEnvironment.palette->calcAnimation();
                float chatScroll = clientEnvironment.chat->getVertScrollbar()->getScrollPosition();
                chatScrollArrow->setVisible(chatScroll < (clientEnvironment.chat->getVertScrollbar()->getDocumentSize() - clientEnvironment.chat->getVertScrollbar()->getPageSize()));
                CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));
                glViewport(0,0,context.getResolution().x,context.getResolution().y);
                glDisable(GL_DEPTH_TEST);
                glActiveTexture(GL_TEXTURE0);
                CEGUI::System::getSingleton().renderAllGUIContexts();
                context.swap();
                glEnable(GL_DEPTH_TEST);

                break;
            }
            case STATE_CONNECTING:
            {
                clientEnvironment.ignoreGamePackets = false;

                //TODO: Multithread loading and have an actual loading screen...

                info("Trying to connect to server...");

                if(clientEnvironment.serverData)
                    delete clientEnvironment.serverData;
                clientEnvironment.serverData = new serverStuff;
                serverStuff *serverData = clientEnvironment.serverData;
                serverData->world = world;

                networkingPreferences netPrefs;
                netPrefs.timeoutMS = 1000;
                serverConnection = new client(netPrefs,clientEnvironment.wantedIP);
                hud->getChild("Chat/Editbox")->setUserData(serverConnection);
                serverConnection->userData = &clientEnvironment;
                serverData->connection = serverConnection;
                serverConnection->receiveHandle = recvHandle;
                serverConnection->kickHandle = gotKicked;
                syjError netErr = serverConnection->getLastError();
                if(netErr != syjError::noError)
                {
                    error(getErrorString(netErr));
                    error("Could not connect to server!");
                    joinServer->getChild("StatusText")->setText("Could not connect!");
                    clientEnvironment.waitingToPickServer = true;
                    currentState = STATE_MAINMENU;;
                    break;
                }
                else
                    connected = true;

                blocklandHolder = new blocklandCompatibility("assets/brick/types/test.cs","./assets/brick/types",clientEnvironment.brickSelector,true);

                serverData->staticBricks.allocateVertBuffer();
                serverData->staticBricks.allocatePerTexture(clientEnvironment.brickMat);
                serverData->staticBricks.allocatePerTexture(clientEnvironment.brickMatSide,true,true);
                serverData->staticBricks.allocatePerTexture(clientEnvironment.brickMatBottom,true);
                serverData->staticBricks.allocatePerTexture(clientEnvironment.brickMatRamp);
                serverData->staticBricks.blocklandTypes = blocklandHolder;

                packet requestName;
                requestName.writeUInt(clientPacketType_requestName,4);
                requestName.writeUInt(hardCodedNetworkVersion,32);
                requestName.writeBit(false);

                requestName.writeBit(clientEnvironment.loggedIn);
                if(clientEnvironment.loggedIn)
                {
                    requestName.writeString(clientEnvironment.loggedName);

                    unsigned char hash[32];

                    br_sha256_context shaContext;
                    br_sha256_init(&shaContext);
                    br_sha256_update(&shaContext,clientEnvironment.sessionToken.c_str(),clientEnvironment.sessionToken.length());
                    br_sha256_out(&shaContext,hash);

                    std::string hexStr = GetHexRepresentation(hash,32);

                    std::cout<<clientEnvironment.loggedName<<"\n";
                    std::cout<<clientEnvironment.sessionToken<<"\n";
                    std::cout<<hexStr<<"\n";

                    requestName.writeString(hexStr);
                }
                else
                    requestName.writeString(clientEnvironment.wantedName);

                requestName.writeFloat(clientEnvironment.wantedColor.r);
                requestName.writeFloat(clientEnvironment.wantedColor.g);
                requestName.writeFloat(clientEnvironment.wantedColor.b);
                serverConnection->send(&requestName,true);

                /*while(serverData->waitingForServerResponse)
                {
                    serverConnection->run();
                    if(serverData->kicked)
                    {
                        error("Kicked or server full or outdated client or malformed server response.");
                        joinServer->getChild("StatusText")->setText("Kicked by server!");
                        clientEnvironment.waitingToPickServer = true;
                        serverData->kicked = false;
                        currentState = STATE_MAINMENU;
                        break;
                    }
                }*/

                int shadowRes = 1024;
                switch(clientEnvironment.settings->shadowResolution)
                {
                    default: case shadowsOff: case shadow1k: shadowRes = 1024; break;
                    case shadow2k: shadowRes = 2048; break;
                    case shadow4k: shadowRes = 4096; break;
                }

                //glEnable(GL_DEPTH_TEST);
                //glEnable(GL_CULL_FACE);

                serverData->env = new environment(shadowRes,shadowRes);

                godRayDebug->getChild("Decay")->setText(std::to_string(serverData->env->godRayDecay));
                godRayDebug->getChild("Density")->setText(std::to_string(serverData->env->godRayDensity));
                godRayDebug->getChild("Exposure")->setText(std::to_string(serverData->env->godRayExposure));
                godRayDebug->getChild("Weight")->setText(std::to_string(serverData->env->godRayWeight));
                godRayDebug->getChild("Distance")->setText(std::to_string(serverData->env->sunDistance));

                serverData->env->loadDaySkyBox("assets/sky/bluecloud_");
                serverData->env->loadNightSkyBox("assets/sky/space_");
                serverData->env->loadSunModel("assets/sky/sun.txt");

                serverData->playerCamera = new perspectiveCamera;;
                serverData->playerCamera->setFieldOfVision(clientEnvironment.settings->fieldOfView);
                serverData->playerCamera->name = "Player";
                serverData->playerCamera->setDirection(glm::vec3(0.4,0,0.4));
                serverData->playerCamera->setPosition(glm::vec3(0,0,0));
                serverData->playerCamera->setAspectRatio(((double)context.getWidth())/((double)context.getHeight()));

                if(clientEnvironment.settings->waterQuality != waterStatic)
                {
                    perspectiveCamera waterCamera;
                    waterCamera.setFieldOfVision(serverData->playerCamera->getFieldOfVision());
                    waterCamera.name = "Water";
                    renderTarget::renderTargetSettings waterReflectionSettings;

                    switch(clientEnvironment.settings->waterQuality)
                    {
                        case waterQuarter:
                            waterReflectionSettings.resX = context.getResolution().x / 4.0;
                            waterReflectionSettings.resY = context.getResolution().y / 4.0;
                            break;

                        case waterHalf:
                            waterReflectionSettings.resX = context.getResolution().x / 2.0;
                            waterReflectionSettings.resY = context.getResolution().y / 2.0;
                            break;

                        default:
                        case waterFull:
                            waterReflectionSettings.resX = context.getResolution().x;
                            waterReflectionSettings.resY = context.getResolution().y;
                            break;
                    }

                    waterReflectionSettings.numColorChannels = 4;
                    waterReflectionSettings.clearColor = glm::vec4(0,0,0,0);
                    waterReflectionSettings.texWrapping = GL_REPEAT;
                    waterReflectionSettings.useColor = true;
                    waterReflection = new renderTarget(waterReflectionSettings);
                    waterRefraction = new renderTarget(waterReflectionSettings);
                    waterRefraction->settings.clearColor = glm::vec4(0.15,0.15,0.3,0.0);
                    renderTarget::renderTargetSettings waterDepthSettings;
                    waterDepthSettings.useDepth = true;
                    waterDepthSettings.resX = waterReflectionSettings.resX;
                    waterDepthSettings.resY = waterReflectionSettings.resY;
                    waterDepth = new renderTarget(waterDepthSettings);
                }

                serverData->box = new selectionBox(world,cubeVAO);

                //terrain theMap("assets/heightmap2.png",world);

                //bool play = false;

                serverData->bulletTrails = new bulletTrailsHolder;

                //TODO: remove this, it's for debugging client physics:
                //serverData->debugLocations.push_back(glm::vec3(0,0,0));
                //serverData->debugColors.push_back(glm::vec3(1,1,1));

                info("Connection established, requesting types...");

                currentState = STATE_LOADING;
                break;
            }
            case STATE_LOADING:
            {
                clientEnvironment.ignoreGamePackets = false;

                serverStuff *serverData = clientEnvironment.serverData;

                serverConnection->run(1);
                if(serverData->kicked)
                {
                    error("Kicked or server full or outdated client or malformed server response.");
                    joinServer->getChild("StatusText")->setText("Kicked by server!");
                    clientEnvironment.waitingToPickServer = true;
                    serverData->kicked = false;
                    currentState = STATE_MAINMENU;
                    break;
                }

                if(!serverData->waitingForServerResponse)
                {
                    for(unsigned int a = 0; a<clientEnvironment.prints->names.size(); a++)
                        serverData->staticBricks.allocatePerTexture(clientEnvironment.prints->textures[a],false,false,true);

                    serverData->ourTempBrick = new tempBrick(serverData->staticBricks);

                    info("Starting main game loop!");
                    currentState = STATE_PLAYING;
                    break;
                }

                const Uint8 *states = SDL_GetKeyboardState(NULL);
                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_QUIT)
                    {
                        currentState = STATE_QUITTING;
                        break;
                    }

                    processEventsCEGUI(event,states);

                    if(event.type == SDL_WINDOWEVENT)
                    {
                        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            context.setSize(event.window.data1,event.window.data2);
                        }
                    }
                }

                context.clear(0.5,0.5,1.0,1.0);
                context.select();
                CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));
                glViewport(0,0,context.getResolution().x,context.getResolution().y);
                glDisable(GL_DEPTH_TEST);
                glActiveTexture(GL_TEXTURE0);
                CEGUI::System::getSingleton().renderAllGUIContexts();
                context.swap();

                break;
            }
        }
    }


    info("Shutting down OpenAL");

    alcMakeContextCurrent(audioContext);
    alcDestroyContext(audioContext);
    alcCloseDevice(device);

    info("Shut down complete");
    return 0;
}
