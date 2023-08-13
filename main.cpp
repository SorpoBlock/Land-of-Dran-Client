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
#include "code/utility/rectToCube.h"
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
#include <CURL/curl.h>
#include "code/gui/updater.h"
//#include <openssl/sha.h>
#include <bearssl/bearssl_hash.h>

#define hardCodedNetworkVersion 10011

#define cammode_firstPerson 0
#define cammode_thirdPerson 1
#define cammode_adminCam 2

using namespace syj;
using namespace std::filesystem;
using namespace std::chrono;

void gotKicked(client *theClient,unsigned int reason,void *userData)
{
    serverStuff *ohWow = (serverStuff*)userData;
    ohWow->kicked = true;
    ohWow->fatalNotify("Disconnected!","Connection with server lost, reason: " + std::to_string(reason) + ".","Exit");
}

std::string GetHexRepresentation(const unsigned char *Bytes, size_t Length) {
    std::ostringstream os;
    os.fill('0');
    os<<std::hex;
    for(const unsigned char *ptr = Bytes; ptr < Bytes+Length; ++ptr) {
        os<<std::setw(2)<<(unsigned int)*ptr;
    }
    return os.str();
}

bool godRayButton(const CEGUI::EventArgs &e)
{
    CEGUI::Window *godRayWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("GodRays");
    serverStuff *ohWow = (serverStuff*)godRayWindow->getUserData();

    ohWow->env->godRayDecay = atof(godRayWindow->getChild("Decay")->getText().c_str());
    ohWow->env->godRayDensity = atof(godRayWindow->getChild("Density")->getText().c_str());
    ohWow->env->godRayExposure = atof(godRayWindow->getChild("Exposure")->getText().c_str());
    ohWow->env->godRayWeight = atof(godRayWindow->getChild("Weight")->getText().c_str());
}

int main(int argc, char *argv[])
{
    serverStuff ohWow;

    preferenceFile prefs;
    ohWow.prefs = &prefs;
    ohWow.settings = new options;
    std::string ip = "127.0.0.1";

    ohWow.settings->prefs = &prefs;
    ohWow.settings->setDefaults(&prefs);
    prefs.importFromFile("config.txt");
    ohWow.settings->loadFromFile(&prefs);
    prefs.exportToFile("config.txt");
    if(prefs.getPreference("IP"))
        ip = prefs.getPreference("IP")->toString();

    logger::setErrorFile("logs/error.txt");
    logger::setInfoFile("logs/log.txt");
    syj::log().setDebug(false);
    scope("Main");
    info("Starting up!");
    if(argc > 0)
        info(argv[0]);

    create_directory("logs");
    create_directory("saves");
    create_directory("assets");
    create_directory("assets/brick");
    create_directory("assets/brick/types");
    create_directory("assets/brick/prints");
    remove("oldlandofdran.exe");
    int revisionVersion = -1;
    int networkVersion = -1;
    info("Retrieving version info from master server...");
    getVersions(revisionVersion,networkVersion);
    ohWow.masterRevision = revisionVersion;
    ohWow.masterNetwork = networkVersion;
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
    renderOptions.startingResX = ohWow.settings->resolutionX;
    renderOptions.startingResY = ohWow.settings->resolutionY;
    renderOptions.useVSync = ohWow.settings->vsync;
    renderOptions.useFullscreen = ohWow.settings->fullscreen;
    switch(ohWow.settings->antiAliasing)
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
    ohWow.context = &context;

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

    ohWow.speaker = new audioPlayer;

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

    CEGUI::Window *optionsWindow = loadOptionsGUI(ohWow.settings,prefs,playerInput);
    ohWow.brickSelector = loadBrickSelector();
    CEGUI::Window *hud = addGUIFromFile("hud.layout");
    CEGUI::Window *brickHighlighter = hud->getChild("BrickPopup/Selector");
    CEGUI::Window *chat = hud->getChild("Chat");
    CEGUI::Window *updater = addUpdater(&ohWow);
    chat->moveToBack();
    ohWow.playerList = hud->getChild("PlayerList");
    ohWow.chat = (CEGUI::Listbox*)hud->getChild("Chat/Listbox");
    ohWow.whosTyping = hud->getChild("WhosTyping");
    CEGUI::Window *chatEditbox = hud->getChild("Chat/Editbox");
    CEGUI::Window *chatScrollArrow = hud->getChild("Chat/DidScroll");
    ohWow.inventoryBox = hud->getChild("Inventory");
    setUpWrenchDialogs(hud,&ohWow);
    ohWow.wrench = hud->getChild("Wrench");
    ohWow.wheelWrench = hud->getChild("WheelWrench");
    ohWow.steeringWrench = hud->getChild("SteeringWrench");
    CEGUI::Window *stats = hud->getChild("Stats");
    CEGUI::Window *crossHair = hud->getChild("Crosshair");
    CEGUI::Window *evalWindow = configureEvalWindow(hud,&ohWow);
    CEGUI::Window *joinServer = loadJoinServer(&ohWow);
    CEGUI::Window *godRayDebug = addGUIFromFile("godRayDebug.layout");
    godRayDebug->setUserData(&ohWow);
    godRayDebug->getChild("Button")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&godRayButton));
    ohWow.evalWindow = evalWindow;
    ohWow.messageBox = initHud(hud);
    ohWow.palette = new paletteGUI(hud);
    ohWow.bottomPrint.textBar = hud->getChild("BottomPrint");
    CEGUI::Window *saveLoadWindow = loadSaveLoadWindow(&ohWow);

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


    if(SDLNet_Init())
        error("Could not initalize SDLNet");

    bool shaderFailedToCompile = false;

    program basicProgram;
    shader basicVertex("shaders/basicVertex.glsl",GL_VERTEX_SHADER);
    shader basicFragment("shaders/basicFragment.glsl",GL_FRAGMENT_SHADER);
    basicProgram.bindShader(basicVertex);
    basicProgram.bindShader(basicFragment);
    basicProgram.compile();
    uniformsHolder basic(basicProgram);
    basic.name = "basic";
    if(!basicProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Basic shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    CEGUI::Window *escapeMenu = addEscapeMenu(&ohWow,&basic);

    program newModelProgram;
    shader newModelVertex("shaders/newModelVertex.glsl",GL_VERTEX_SHADER);
    newModelProgram.bindShader(newModelVertex);
    newModelProgram.bindShader(basicFragment);
    newModelProgram.compile();
    uniformsHolder newModelUnis(newModelProgram);
    newModelUnis.name = "newModel";
    if(!newModelProgram.isCompiled())
    {
        notify("Shader Failed to Compile","New model shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program shadowProgram;
    shader shadowVertex("shaders/shadowVertex.glsl",GL_VERTEX_SHADER);
    shader shadowGeometry("shaders/shadowGeometry.glsl",GL_GEOMETRY_SHADER);
    shader shadowFragment("shaders/shadowFragment.glsl",GL_FRAGMENT_SHADER);
    shadowProgram.bindShader(shadowVertex);
    shadowProgram.bindShader(shadowGeometry);
    shadowProgram.bindShader(shadowFragment);
    shadowProgram.compile();
    uniformsHolder shadow(shadowProgram);
    shadow.name = "shadow";
    if(!shadowProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Shadow shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program newModelShadowProgram;
    shader newModelShadowVertex("shaders/newModelShadow.glsl",GL_VERTEX_SHADER);
    newModelShadowProgram.bindShader(newModelShadowVertex);
    newModelShadowProgram.bindShader(shadowGeometry);
    newModelShadowProgram.bindShader(shadowFragment);
    newModelShadowProgram.compile();
    uniformsHolder newModelShadowUnis(newModelShadowProgram);
    newModelShadowUnis.name = "newModelShadow";
    if(!newModelShadowProgram.isCompiled())
    {
        notify("Shader Failed to Compile","New model shadow shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program shadowProgramNoGeo;
    shadowProgramNoGeo.bindShader(shadowVertex);
    shadowProgramNoGeo.bindShader(shadowFragment);
    shadowProgramNoGeo.compile();
    uniformsHolder shadowNoGeoUnis(shadowProgramNoGeo);
    shadowNoGeoUnis.name = "shadowNoGeo";
    if(!shadowProgramNoGeo.isCompiled())
    {
        notify("Shader Failed to Compile","ShadowNoGeo shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program emitterProgram;
    shader emitterVertex("shaders/emitterVertex.glsl",GL_VERTEX_SHADER);
    shader emitterFragment("shaders/emitterFragment.glsl",GL_FRAGMENT_SHADER);
    emitterProgram.bindShader(emitterVertex);
    emitterProgram.bindShader(emitterFragment);
    emitterProgram.compile();
    uniformsHolder emitterUnis(emitterProgram);
    emitterUnis.name = "emitter";
    if(!emitterProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Emitter shadow shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    //program tessProgram;
    shader tessVertex("shaders/tessVertex.glsl",GL_VERTEX_SHADER);
    /*shader tessControl("shaders/tessControl.glsl",GL_TESS_CONTROL_SHADER);
    shader tessEval("shaders/tessEval.glsl",GL_TESS_EVALUATION_SHADER);
    tessProgram.bindShader(tessVertex);
    tessProgram.bindShader(tessControl);
    tessProgram.bindShader(tessEval);
    tessProgram.bindShader(basicFragment);
    tessProgram.compile();
    uniformsHolder tess(tessProgram);
    tess.name = "tess";
    if(!tessProgram.isCompiled())
        return 0;*/

    /*program spriteProgram;
    shader spriteVertex("shaders/spriteVertex.glsl",GL_VERTEX_SHADER);
    shader spriteFragment("shaders/spriteFragment.glsl",GL_FRAGMENT_SHADER);
    spriteProgram.bindShader(spriteVertex);
    spriteProgram.bindShader(spriteFragment);
    spriteProgram.compile();
    uniformsHolder spriteUnis(spriteProgram);
    spriteUnis.name = "sprite";
    if(!spriteProgram.isCompiled())
        return 0;*/

    /*program shadowTessProgram;
    shader shadowTessEval("shaders/shadowTessEval.glsl",GL_TESS_EVALUATION_SHADER);
    shadowTessProgram.bindShader(tessVertex);
    shadowTessProgram.bindShader(tessControl);
    shadowTessProgram.bindShader(shadowGeometry);
    shadowTessProgram.bindShader(shadowTessEval);
    shadowTessProgram.bindShader(shadowFragment);
    shadowTessProgram.compile();
    uniformsHolder shadowTessUnis(shadowTessProgram);
    shadowTessUnis.name = "shadowTess";
    if(!shadowTessProgram.isCompiled())
        return 0;

    program shadowSpriteProgram;
    shader shadowSpriteVertex("shaders/shadowSpriteVertex.glsl",GL_VERTEX_SHADER);
    shadowSpriteProgram.bindShader(shadowSpriteVertex);
    shadowSpriteProgram.bindShader(shadowGeometry);
    shadowSpriteProgram.bindShader(shadowFragment);
    shadowSpriteProgram.compile();
    uniformsHolder shadowSpriteUnis(shadowSpriteProgram);
    shadowSpriteUnis.name = "shadowSprite";
    if(!shadowSpriteProgram.isCompiled())
        return 0;*/

    program waterProgram;
    shader waterControl("shaders/waterControl.glsl",GL_TESS_CONTROL_SHADER);
    shader waterEval("shaders/waterEval.glsl",GL_TESS_EVALUATION_SHADER);
    shader waterFragment("shaders/waterFragment.glsl",GL_FRAGMENT_SHADER);
    waterProgram.bindShader(tessVertex);
    waterProgram.bindShader(waterControl);
    waterProgram.bindShader(waterEval);
    waterProgram.bindShader(waterFragment);
    waterProgram.compile();
    uniformsHolder waterUnis(waterProgram);
    waterUnis.name = "water";
    if(!waterProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Water shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program brickProgram;
    shader brickVertex("shaders/brickVertex.glsl",GL_VERTEX_SHADER);
    shader brickFragment("shaders/brickFragment.glsl",GL_FRAGMENT_SHADER);
    brickProgram.bindShader(brickVertex);
    brickProgram.bindShader(brickFragment);
    brickProgram.compile();
    uniformsHolder brickUnis(brickProgram);
    brickUnis.name = "brick";
    if(!brickProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Brick shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program brickSimpleProgram;
    shader brickSimpleVertex("shaders/simpleBrickVertex.glsl",GL_VERTEX_SHADER);
    shader brickSimpleFragment("shaders/simpleBrickFragment.glsl",GL_FRAGMENT_SHADER);
    brickSimpleProgram.bindShader(brickSimpleVertex);
    brickSimpleProgram.bindShader(brickSimpleFragment);
    brickSimpleProgram.compile();
    uniformsHolder brickSimpleUnis(brickSimpleProgram);
    brickSimpleUnis.name = "brickSimple";
    if(!brickSimpleProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Brick shader (simple) failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program shadowBrickProgram;
    shader shadowBrickVertex("shaders/shadowBrickVertex.glsl",GL_VERTEX_SHADER);
    shadowBrickProgram.bindShader(shadowBrickVertex);
    shadowBrickProgram.bindShader(shadowGeometry);
    shadowBrickProgram.bindShader(shadowFragment);
    shadowBrickProgram.compile();
    uniformsHolder shadowBrickUnis(shadowBrickProgram);
    shadowBrickUnis.name = "shadowBrick";
    if(!shadowBrickProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Brick shadow shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program shadowBrickProgramNoGeo;
    shadowBrickProgramNoGeo.bindShader(shadowBrickVertex);
    shadowBrickProgramNoGeo.bindShader(shadowFragment);
    shadowBrickProgramNoGeo.compile();
    uniformsHolder shadowBrickNoGeoUnis(shadowBrickProgramNoGeo);
    shadowBrickNoGeoUnis.name = "shadowBrick";
    if(!shadowBrickProgramNoGeo.isCompiled())
    {
        notify("Shader Failed to Compile","Brick shadow no geo shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program boxEdgesProgram;
    shader boxEdgesVertex("shaders/boxEdgesVertex.glsl",GL_VERTEX_SHADER);
    shader boxEdgesFragment("shaders/boxEdgesFragment.glsl",GL_FRAGMENT_SHADER);
    boxEdgesProgram.bindShader(boxEdgesVertex);
    boxEdgesProgram.bindShader(boxEdgesFragment);
    boxEdgesProgram.compile();
    uniformsHolder boxEdgesUnis(boxEdgesProgram);
    boxEdgesUnis.name = "basic";
    if(!boxEdgesProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Box edges shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program rectToCubeProgram;
    shader rectToCubeVert("shaders/rectToCubeVert.glsl",GL_VERTEX_SHADER);
    shader rectToCubeFrag("shaders/rectToCubeFrag.glsl",GL_FRAGMENT_SHADER);
    rectToCubeProgram.bindShader(rectToCubeVert);
    rectToCubeProgram.bindShader(rectToCubeFrag);
    rectToCubeProgram.compile();
    if(!rectToCubeProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Rect-to-cube shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }

    program fontProgram;
    shader fontVert("shaders/fontVertex.glsl",GL_VERTEX_SHADER);
    shader fontFrag("shaders/fontFragment.glsl",GL_FRAGMENT_SHADER);
    fontProgram.bindShader(fontVert);
    fontProgram.bindShader(fontFrag);
    fontProgram.compile();
    uniformsHolder fontUnis(fontProgram);
    fontUnis.name = "font";
    if(!fontProgram.isCompiled())
    {
        notify("Shader Failed to Compile","Font shader failed to compile. Check logs folder. This will cause severe graphics issues.","Close");
        shaderFailedToCompile = true;
    }
    /*std::vector<glm::vec3> shape = defaultSquareShape();
    std::vector<glm::vec3> norms = calculateNormals(shape);
    std::vector<glm::vec2> uvsAndDims = defaultSquareUVs();
    std::vector<glm::vec4> offsets;
    std::vector<glm::vec4> allOffsets;
    int spriteDensity = 0;
    switch(ohWow.settings->spriteDensity)
    {
        case spritesOff: spriteDensity = 0; break;
        case sprites500: spriteDensity = 500; break;
        case sprites1000: spriteDensity = 1000; break;
        default: case sprites2000: spriteDensity = 2000; break;
        case sprites4000: spriteDensity = 4000; break;
    }
    for(unsigned int a = 0; a<spriteDensity; a++)
        offsets.push_back(glm::vec4(drand(0,50),drand(-100,100),drand(0,50),(double)(rand() % 4)));
    for(int x = 0; x<12; x++)
        for(int z = 0; z<12; z++)
            for(int a = 0; a<offsets.size(); a++)
                allOffsets.push_back(glm::vec4(offsets[a].x+x*50,offsets[a].y,offsets[a].z+z*50,offsets[a].w));
    sprite bush(shape,uvsAndDims,norms,allOffsets.size());

    bush.updateOffsets(allOffsets);
    texture plant1;
    plant1.createFromFile("assets/spritesheet.png");
    texture plant1n;
    plant1n.createFromFile("assets/spritesheet_n.png");*/

    //Start connect screen

    ohWow.picker = new avatarPicker(context.getResolution().x,context.getResolution().y);

    ohWow.brickMat = new material("assets/brick/otherBrickMat.txt");
    ohWow.brickMatSide = new material("assets/brick/sideBrickMat.txt");
    ohWow.brickMatRamp = new material("assets/brick/rampBrickMat.txt");
    ohWow.brickMatBottom = new material("assets/brick/bottomBrickMat.txt");

    coolLabel:

    hud->setVisible(false);
    context.setMouseLock(false);
    joinServer->setVisible(true);

    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *bounceText = addGUIFromFile("justText.layout");
    bounceText->setVisible(true);
    bounceText->moveToBack();

    float hue = 0;
    float lastTick = SDL_GetTicks();
    float deltaT = 0;
    float horBounceDir = 1;
    float vertBounceDir = 1;

    while(ohWow.waitingToPickServer)
    {
        const Uint8 *states = SDL_GetKeyboardState(NULL);

        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                return 0;

            processEventsCEGUI(e,states);
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

        SDL_Delay(1);
    }

    context.setMouseLock(true);
    hud->setVisible(true);
    joinServer->setVisible(false);

    root->removeChild("Bouncer");
    updater->setVisible(false);

    //End connect screen

    info("Trying to connect to server...");

    bool connected = false;
    networkingPreferences netPrefs;
    netPrefs.timeoutMS = 1000;
    client serverConnection(netPrefs,ohWow.wantedIP);
    hud->getChild("Chat/Editbox")->setUserData(&serverConnection);
    serverConnection.userData = &ohWow;
    ohWow.connection = &serverConnection;
    serverConnection.receiveHandle = recvHandle;
    serverConnection.kickHandle = gotKicked;
    syjError netErr = serverConnection.getLastError();
    if(netErr != syjError::noError)
    {
        error(getErrorString(netErr));
        error("Could not connect to server!");
        joinServer->getChild("StatusText")->setText("Could not connect!");
        ohWow.waitingToPickServer = true;
        goto coolLabel;
        return 0;
    }
    else
        connected = true;

    blocklandCompatibility blocklandHolder("assets/brick/types/test.cs",".\\assets\\brick\\types",ohWow.brickSelector,true);
    ohWow.staticBricks.allocateVertBuffer();
    ohWow.staticBricks.allocatePerTexture(ohWow.brickMat);
    ohWow.staticBricks.allocatePerTexture(ohWow.brickMatSide,true,true);
    ohWow.staticBricks.allocatePerTexture(ohWow.brickMatBottom,true);
    ohWow.staticBricks.allocatePerTexture(ohWow.brickMatRamp);
    ohWow.staticBricks.blocklandTypes = &blocklandHolder;

    packet requestName;
    requestName.writeUInt(clientPacketType_requestName,4);
    requestName.writeUInt(hardCodedNetworkVersion,32);

    requestName.writeBit(ohWow.loggedIn);
    if(ohWow.loggedIn)
    {
        requestName.writeString(ohWow.loggedName);

        unsigned char hash[32];

        br_sha256_context shaContext;
        br_sha256_init(&shaContext);
        br_sha256_update(&shaContext,ohWow.sessionToken.c_str(),ohWow.sessionToken.length());
        br_sha256_out(&shaContext,hash);

        std::string hexStr = GetHexRepresentation(hash,32);
        requestName.writeString(hexStr);
    }
    else
        requestName.writeString(ohWow.wantedName);

    requestName.writeFloat(ohWow.wantedColor.r);
    requestName.writeFloat(ohWow.wantedColor.g);
    requestName.writeFloat(ohWow.wantedColor.b);
    serverConnection.send(&requestName,true);

    info("Connection established, requesting types...");

    while(ohWow.waitingForServerResponse)
    {
        serverConnection.run();
        if(ohWow.kicked)
        {
            error("Kicked or server full or outdated client or malformed server response.");
            joinServer->getChild("StatusText")->setText("Kicked by server!");
            ohWow.waitingToPickServer = true;
            ohWow.kicked = false;
            goto coolLabel;
            return 0;
        }
    }

    int shadowRes = 1024;
    switch(ohWow.settings->shadowResolution)
    {
        default: case shadowsOff: case shadow1k: shadowRes = 1024; break;
        case shadow2k: shadowRes = 2048; break;
        case shadow4k: shadowRes = 4096; break;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    ohWow.env = new environment(shadowRes,shadowRes);

    godRayDebug->getChild("Decay")->setText(std::to_string(ohWow.env->godRayDecay));
    godRayDebug->getChild("Density")->setText(std::to_string(ohWow.env->godRayDensity));
    godRayDebug->getChild("Exposure")->setText(std::to_string(ohWow.env->godRayExposure));
    godRayDebug->getChild("Weight")->setText(std::to_string(ohWow.env->godRayWeight));

    /*ohWow.env->loadDaySkyBox("assets/sky/bluecloud_");
    ohWow.env->loadNightSkyBox("assets/sky/space_");*/
    ohWow.env->loadSunModel("assets/sky/sun.txt");

    ohWow.playerCamera = new perspectiveCamera;;
    ohWow.playerCamera->setFieldOfVision(ohWow.settings->fieldOfView);
    ohWow.playerCamera->name = "Player";
    ohWow.playerCamera->setDirection(glm::vec3(0.4,0,0.4));
    ohWow.playerCamera->setPosition(glm::vec3(0,0,0));
    ohWow.playerCamera->setAspectRatio(((double)context.getWidth())/((double)context.getHeight()));

    GLuint quadVAO = createQuadVAO();
    GLuint cubeVAO = createCubeVAO();
    //GLuint boxEdgesVAO = createBoxEdgesVAO();

    texture *bdrf = generateBDRF(quadVAO);
    //std::string iblName = "mountain";
    std::string iblName = "MoonlessGolf";
    //std::string iblName = "sunset";
    //std::string iblName = "shanghai";
    //std::string iblName = "alexs_apartment";
    //std::string iblName = "field";
    //std::string iblName = "Zollhoff";
    GLuint IBL = processEquirectangularMap(rectToCubeProgram,cubeVAO,"assets/"+iblName+"/main.hdr");
    GLuint IBLRad = processEquirectangularMap(rectToCubeProgram,cubeVAO,"assets/"+iblName+"/mainRad.hdr",true);
    GLuint IBLIrr = processEquirectangularMap(rectToCubeProgram,cubeVAO,"assets/"+iblName+"/mainIrr.hdr",true);

    renderTarget *waterReflection = 0;
    renderTarget *waterRefraction = 0;
    renderTarget *waterDepth = 0;
    texture *dudvTexture = 0;
    tessellation water(0);

    if(ohWow.settings->waterQuality != waterStatic)
    {
        perspectiveCamera waterCamera;
        waterCamera.setFieldOfVision(ohWow.playerCamera->getFieldOfVision());
        waterCamera.name = "Water";
        renderTarget::renderTargetSettings waterReflectionSettings;

        switch(ohWow.settings->waterQuality)
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
        dudvTexture = new texture;
        dudvTexture->setWrapping(GL_REPEAT);
        dudvTexture->createFromFile("assets/dudv.png");
    }

    btDefaultCollisionConfiguration *collisionConfiguration = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);
    btDynamicsWorld *world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    btVector3 gravity = btVector3(0,-70,0);
    world->setGravity(gravity);
    world->setForceUpdateAllAabbs(false);
    ohWow.world = world;

    btCollisionShape *plane = new btStaticPlaneShape(btVector3(0,1,0),0);
    btDefaultMotionState* planeState = new btDefaultMotionState();
    btRigidBody::btRigidBodyConstructionInfo planeCon(0,planeState,plane);
    btRigidBody *groundPlane = new btRigidBody(planeCon);
    groundPlane->setFriction(1.0);
    //groundPlane->setUserIndex(bodyUserIndex_plane);
    world->addRigidBody(groundPlane);

    selectionBox *box = new selectionBox(world,cubeVAO);

    material grass("assets/ground/grass1/grass.txt");
    //terrain theMap("assets/heightmap2.png",world);

    ohWow.prints = new printLoader(".\\assets\\brick\\prints");

    for(unsigned int a = 0; a<ohWow.prints->names.size(); a++)
        ohWow.staticBricks.allocatePerTexture(ohWow.prints->textures[a],false,false,true);

    model wheelModel("assets/ball/ball.txt");

    unsigned int last10Secs = SDL_GetTicks();
    unsigned int frames = 0;

    //bool play = false;

    int waterFrame = 0;

    glm::vec3 lastCamPos = ohWow.playerCamera->getPosition();

    int camMode = cammode_firstPerson;
    float desiredFov = ohWow.settings->fieldOfView;
    float currentZoom = ohWow.settings->fieldOfView;

    double totalSteps = 0;

    float bottomBarOpen = 0;
    float bottomBarClose = 0.165;
    float bottomBarOpenTime = 500;
    float bottomBarLastAct = SDL_GetTicks();
    bool bottomBarShouldBeOpen = false;

    tempBrick myTempBrick(ohWow.staticBricks);

    int stageOfSelection = -1;
    bool drawDebugLines = false;
    glm::vec3 start;
    glm::vec3 end;

    glm::vec3 lastPlayerDir = glm::vec3(0,0,0);
    int lastPlayerControlMask = 0;
    int transSendInterval = 30;
    int lastSentTransData = 0;
    bool doJump = false;
    //ohWow.inventoryOpen = hud->getChild("Inventory")->isVisible();

    info("Starting main game loop!");

    bool hitDebug = true;
    bool justTurnOnChat = false;
    float lastPhysicsStep = 0.0;

    //TODO: remove this, it's for debugging client physics:
    //ohWow.debugLocations.push_back(glm::vec3(0,0,0));
    //ohWow.debugColors.push_back(glm::vec3(1,1,1));

    bool cont = true;
    while(cont)
    {
        if(ohWow.exitToWindows)
            cont = false;

        if(ohWow.fatalNotifyStarted)
        {
            int *status = (int*)ohWow.messageBox->getUserData();
            if(status)
            {
                if(status[0] == 0)
                {
                    cont = false;
                    break;
                }
            }
        }

        //Options:

        hud->getChild("Inventory")->setVisible(ohWow.currentlyOpen == inventory);
        if(ohWow.currentlyOpen != paintCan)
            ohWow.palette->close();

        //ohWow.playerCamera->setFieldOfVision(ohWow.settings->fieldOfView);
        ohWow.brickSelector->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        escapeMenu->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        evalWindow->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        saveLoadWindow->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        ohWow.wrench->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        ohWow.wheelWrench->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        ohWow.steeringWrench->setAlpha(((float)ohWow.settings->hudOpacity) / 100.0);
        ohWow.playerList->setAlpha(((float)ohWow.settings->hudOpacity)/100.0);
        godRayDebug->setAlpha(((float)ohWow.settings->hudOpacity)/100.0);
        //chat->moveToBack();

        float gain = ohWow.settings->masterVolume;
        gain /= 100.0;
        float musicGain = ohWow.settings->musicVolume;
        musicGain /= 100.0;
        ohWow.speaker->setVolumes(gain,musicGain);

        //Input:

        GLenum error = glGetError();
        if(error != GL_NO_ERROR)
        {
            std::cout<<"Error: "<<error<<"\n";
        }

        lastCamPos = ohWow.playerCamera->getPosition();

        frames++;
        if(last10Secs < SDL_GetTicks())
        {
            double fps = frames;
            totalSteps /= ((double)frames);
            if(ohWow.cameraTarget)
                stats->setText("Sent/recv: " + std::to_string(serverConnection.numPacketsSent) + "/" + std::to_string(serverConnection.numPacketsReceived) + " FPS: " + std::to_string(fps)  + " Player Keyframes: " + std::to_string(ohWow.cameraTarget->modelInterpolator.keyFrames.size()));
            last10Secs = SDL_GetTicks() + 1000;
            frames = 0;
            totalSteps = 0;
        }

        deltaT = SDL_GetTicks() - lastTick;
        lastTick = SDL_GetTicks();

        for(unsigned int a = 0; a<32; a++)
        {
            if(ohWow.speaker->carToTrack[a])
            {
                glm::vec3 pos = ohWow.speaker->carToTrack[a]->carTransform.getPosition();
                glm::vec3 vel = ohWow.speaker->carToTrack[a]->carTransform.guessVelocity() * glm::vec3(0.4);
                alSource3f(ohWow.speaker->loopSources[a],AL_POSITION,pos.x,pos.y,pos.z);
                alSource3f(ohWow.speaker->loopSources[a],AL_VELOCITY,vel.x,vel.y,vel.z);
            }
        }

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
            if(ohWow.currentPlayer)
            {
                if(rightDown && ohWow.canJet)
                {
                    ohWow.currentPlayer->body->setGravity(btVector3(0,20,0));
                }
                else
                {
                    btTransform t = ohWow.currentPlayer->body->getWorldTransform();
                    btVector3 o = t.getOrigin();

                    if(o.y() < ohWow.waterLevel-2.0)
                    {
                        ohWow.currentPlayer->body->setDamping(0.4,0.0);
                        ohWow.currentPlayer->body->setGravity(btVector3(0,-0.5,0) * world->getGravity());
                    }
                    else if(o.y() < ohWow.waterLevel)
                    {
                        ohWow.currentPlayer->body->setDamping(0.4,0.0);
                        ohWow.currentPlayer->body->setGravity(btVector3(0,0,0));
                    }
                    else
                    {
                        ohWow.currentPlayer->body->setDamping(0.0,0.0);
                        ohWow.currentPlayer->body->setGravity(world->getGravity());
                    }
                }
            }

            //if(SDL_GetTicks() - lastPhysicsStep >= 15)
            //{
                float physicsDeltaT = SDL_GetTicks() - lastPhysicsStep;
                world->stepSimulation(physicsDeltaT / 1000.0);
                lastPhysicsStep = SDL_GetTicks();
            //}
        }

        const Uint8 *states = SDL_GetKeyboardState(NULL);
        playerInput.getKeyStates();

        box->performRaycast(ohWow.playerCamera->getPosition(),ohWow.playerCamera->getDirection(),0);

        //Clicking ray cast
        btVector3 raystart = glmToBt(ohWow.playerCamera->getPosition());
        btVector3 rayend = glmToBt(ohWow.playerCamera->getPosition() + ohWow.playerCamera->getDirection() * glm::vec3(30.0));
        btCollisionWorld::AllHitsRayResultCallback res(raystart,rayend);
        world->rayTest(raystart,rayend,res);

        int idx = -1;
        float dist = 9999999;

        if(ohWow.cameraTarget)
        {
            for(int a = 0; a<res.m_collisionObjects.size(); a++)
            {
                //if(res.m_collisionObjects[a] != ohWow.cameraTarget->body)
                //{
                    if(fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-ohWow.playerCamera->getPosition())) < dist)
                    {
                        dist = fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-ohWow.playerCamera->getPosition()));
                        idx = a;
                    }
                //}
            }
        }
        else if(camMode == cammode_firstPerson)
            camMode = cammode_thirdPerson;

        if(ohWow.ghostCar)
        {
            ohWow.ghostCar->carTransform.keyFrames.clear();
            ohWow.ghostCar->carTransform.highestProcessed = 0;
            if(idx != -1)
                ohWow.ghostCar->carTransform.addTransform(0,BtToGlm(res.m_hitPointWorld[idx]) + glm::vec3(0,5,0),glm::quat(1,0,0,0));
            else
                ohWow.ghostCar->carTransform.addTransform(0,BtToGlm(rayend) + glm::vec3(0,5,0),glm::quat(1,0,0,0));
        }

        /*if(stageOfSelection == -1)
        {
            start = glm::vec3(0,0,0);
            end = glm::vec3(0,0,0);
        }

        if(idx != -1)
        {
            if(stageOfSelection == 1)
                end = BtToGlm(res.m_hitPointWorld[idx]);
        }*/

        bool supress = evalWindow->isActive() && evalWindow->isVisible();
        supress |= ohWow.wheelWrench->isActive() && ohWow.wheelWrench->isVisible();
        supress |= ohWow.wrench->isActive() && ohWow.wrench->isVisible();
        supress |= saveLoadWindow->getChild("CarFilePath")->isActive();
        supress |= saveLoadWindow->getChild("BuildFilePath")->isActive();

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            playerInput.handleInput(event);

            if(event.type == SDL_QUIT)
            {
                cont = false;
                break;
            }

            processEventsCEGUI(event,states);

            if(event.type == SDL_KEYDOWN)
            {
                if(event.key.keysym.sym == SDLK_PAGEUP)
                {
                    ohWow.chat->getVertScrollbar()->setStepSize(36.3222);
                    //float maxScroll = chat->getVertScrollbar()->getDocumentSize() - chat->getVertScrollbar()->getPageSize();
                    if(ohWow.chat->getVertScrollbar()->getScrollPosition() >= 36.3222)
                        ohWow.chat->getVertScrollbar()->setScrollPosition(ohWow.chat->getVertScrollbar()->getScrollPosition() - 36.3222);
                }
                if(event.key.keysym.sym == SDLK_PAGEDOWN)
                {
                    ohWow.chat->getVertScrollbar()->setStepSize(36.3222);
                    //float maxScroll = chat->getVertScrollbar()->getDocumentSize() - chat->getVertScrollbar()->getPageSize();
                    ohWow.chat->getVertScrollbar()->setScrollPosition(ohWow.chat->getVertScrollbar()->getScrollPosition() + 36.3222);
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
                        if(ohWow.brickSelector->isVisible())
                           anyVisible = true;
                        if(evalWindow->isVisible())
                           anyVisible = true;
                        if(ohWow.wrench->isVisible())
                           anyVisible = true;
                        if(ohWow.wheelWrench->isVisible())
                           anyVisible = true;
                        if(ohWow.steeringWrench->isVisible())
                           anyVisible = true;
                        if(saveLoadWindow->isVisible())
                            anyVisible = true;
                        if(ohWow.playerList->isVisible())
                            anyVisible = true;
                        if(godRayDebug->isVisible())
                            anyVisible = true;


                        if(anyVisible)
                        {
                            context.setMouseLock(true);
                            escapeMenu->setVisible(false);
                            ohWow.brickSelector->setVisible(false);
                            optionsWindow->setVisible(false);
                            evalWindow->setVisible(false);
                            ohWow.wrench->setVisible(false);
                            ohWow.wheelWrench->setVisible(false);
                            ohWow.steeringWrench->setVisible(false);
                            saveLoadWindow->setVisible(false);
                            ohWow.playerList->setVisible(false);
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

                if(event.key.keysym.sym == SDLK_F1)
                {
                    if(ohWow.currentPlayer)
                    {
                        btTransform t = ohWow.currentPlayer->body->getWorldTransform();
                        btVector3 v = t.getOrigin();
                        std::cout<<"Physics pos: "<<v.x()<<","<<v.y()<<","<<v.z()<<"\n";
                    }

                    std::cout<<"Position: "<<ohWow.playerCamera->getPosition().x<<","<<ohWow.playerCamera->getPosition().y<<","<<ohWow.playerCamera->getPosition().z<<"\n";
                    std::cout<<"Direction: "<<ohWow.playerCamera->getDirection().x<<","<<ohWow.playerCamera->getDirection().y<<","<<ohWow.playerCamera->getDirection().z<<"\n";
                }

                if(event.key.keysym.sym == SDLK_F5)
                {
                    godRayDebug->setVisible(true);
                }

                if(event.key.keysym.sym == SDLK_F2)
                {
                    if(ohWow.currentPlayer)
                    {
                        btTransform t;
                        t.setIdentity();
                        t.setOrigin(btVector3(ohWow.playerCamera->getPosition().x,ohWow.playerCamera->getPosition().y,ohWow.playerCamera->getPosition().z));
                        ohWow.currentPlayer->body->setWorldTransform(t);
                        ohWow.currentPlayer->body->setLinearVelocity(btVector3(0,0,0));
                        ohWow.currentPlayer->body->setAngularVelocity(btVector3(0,0,0));
                    }

                    for(unsigned int a = 0; a<ohWow.newDynamics.size(); a++)
                    {
                        ohWow.newDynamics[a]->modelInterpolator.keyFrames.clear();
                    }
                    for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                    {
                        ohWow.livingBricks[a]->carTransform.keyFrames.clear();
                    }
                }

                if(event.key.keysym.sym == SDLK_F4)
                {
                    if(ohWow.cameraTarget)
                    {
                        unsigned int currentServerTime = (getServerTime() - interpolator::clientTimePoint) + interpolator::serverTimePoint;
                        std::cout<<"Server time: "<<currentServerTime<<"\n";
                        for(int a = 0; a<ohWow.cameraTarget->modelInterpolator.keyFrames.size(); a++)
                            std::cout<<a<<": "<<ohWow.cameraTarget->modelInterpolator.keyFrames[a].packetTime<<"ms - "<<ohWow.cameraTarget->modelInterpolator.keyFrames[a].position.x<<"\n";
                    }
                    interpolator::serverTimePoint -= 50;
                }

                if(event.key.keysym.sym == SDLK_F3)
                {
                    ohWow.playerList->setVisible(true);
                    ohWow.playerList->moveToFront();
                }

                if(event.key.keysym.sym == SDLK_F8)
                {
                    packet data;
                    data.writeUInt(14,4);
                    data.writeBit(true);
                    ohWow.connection->send(&data,true);

                    //camMode = cammode_adminCam;
                }

                if(event.key.keysym.sym == SDLK_F7)
                {
                    packet data;
                    data.writeUInt(14,4);
                    data.writeBit(false);
                    data.writeFloat(ohWow.playerCamera->getPosition().x);
                    data.writeFloat(ohWow.playerCamera->getPosition().y);
                    data.writeFloat(ohWow.playerCamera->getPosition().z);
                    ohWow.connection->send(&data,true);

                    //camMode = cammode_firstPerson;
                }

                if(states[SDL_SCANCODE_LCTRL] && event.key.keysym.sym == SDLK_z)
                {
                    packet undoPacket;
                    undoPacket.writeUInt(4,4);
                    ohWow.connection->send(&undoPacket,true);
                }

                if(states[SDL_SCANCODE_LCTRL] && event.key.keysym.sym == SDLK_w)
                {
                    packet data;
                    data.writeUInt(9,4);
                    if(ohWow.currentlyOpen == inventory)
                        data.writeUInt(ohWow.selectedSlot,3);
                    else
                        data.writeUInt(7,3);
                    ohWow.connection->send(&data,true);
                }
            }

            if(event.type == SDL_MOUSEMOTION)
            {
                if(context.getMouseLocked() && (ohWow.freeLook || ohWow.boundToObject))
                {
                    glm::vec2 res = context.getResolution();
                    float x = (((float)event.motion.xrel)/res.x);
                    float y = (((float)event.motion.yrel)/res.y);
                    x *= ((float)ohWow.settings->mouseSensitivity) / 100.0;
                    y *= ((float)ohWow.settings->mouseSensitivity) / 100.0;
                    ohWow.playerCamera->turn(ohWow.settings->invertMouseY ? y : -y,-x);

                    box->drag(ohWow.playerCamera->getPosition(),ohWow.playerCamera->getDirection());
                }
            }

            if(event.type == SDL_MOUSEWHEEL)
            {
                //if(!ohWow.brickSelector->isMouseContainedInArea() && !evalWindow->isMouseContainedInArea())
                if(context.getMouseLocked())
                {
                    if(ohWow.currentlyOpen == inventory)
                    {
                        if(event.wheel.y > 0)
                        {
                            ohWow.selectedSlot--;
                            if(ohWow.selectedSlot < 0 || ohWow.selectedSlot >= inventorySize)
                                ohWow.selectedSlot = inventorySize - 1;
                        }
                        else if(event.wheel.y < 0)
                        {
                            ohWow.selectedSlot++;
                            if(ohWow.selectedSlot >= inventorySize)
                                ohWow.selectedSlot = 0;
                        }
                    }
                    else if(ohWow.currentlyOpen == brickBar)
                    {
                        myTempBrick.scroll(hud,ohWow.brickSelector,ohWow.staticBricks,event.wheel.y);
                    }
                    else
                    {
                        ohWow.palette->scroll(event.wheel.y);
                        myTempBrick.basicChanged = myTempBrick.basicChanged || myTempBrick.isBasic;
                        myTempBrick.specialChanged = myTempBrick.specialChanged || !myTempBrick.isBasic;
                    }
                }
            }

            if(event.type == SDL_MOUSEBUTTONUP)
            {
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    if(box->currentPhase == selectionBox::selectionPhase::stretching)
                        box->currentPhase = selectionBox::selectionPhase::selecting;
                }
            }

            if(event.type == SDL_MOUSEBUTTONDOWN)
            {
                if(context.getMouseLocked())
                {
                    packet data;
                    data.writeUInt(clientPacketType_clicked,4);
                    data.writeBit(event.button.button == SDL_BUTTON_LEFT);
                    data.writeFloat(ohWow.playerCamera->getPosition().x);
                    data.writeFloat(ohWow.playerCamera->getPosition().y);
                    data.writeFloat(ohWow.playerCamera->getPosition().z);
                    data.writeFloat(ohWow.playerCamera->getDirection().x);
                    data.writeFloat(ohWow.playerCamera->getDirection().y);
                    data.writeFloat(ohWow.playerCamera->getDirection().z);
                    if(ohWow.currentlyOpen == inventory)
                        data.writeUInt(ohWow.selectedSlot,3);
                    else
                        data.writeUInt(7,3);
                    ohWow.connection->send(&data,true);

                    if(event.button.button == SDL_BUTTON_LEFT && ohWow.currentPlayer)
                    {
                        ohWow.currentPlayer->stop("grab");
                        ohWow.currentPlayer->play("grab",true);
                    }
                }

                if(event.button.button == SDL_BUTTON_RIGHT)
                {
                    if(ohWow.ghostCar)
                    {
                        delete ohWow.ghostCar;
                        ohWow.ghostCar = 0;
                    }

                    box->currentPhase = selectionBox::selectionPhase::idle;
                }

                if(event.button.button == SDL_BUTTON_LEFT && context.getMouseLocked())
                {
                    if(ohWow.ghostCar)
                    {
                        if(idx != -1)
                            sendBrickCarToServer(&ohWow,ohWow.ghostCar,BtToGlm(res.m_hitPointWorld[idx]));
                        else
                            sendBrickCarToServer(&ohWow,ohWow.ghostCar,BtToGlm(rayend) + glm::vec3(0,5,0));
                        delete ohWow.ghostCar;
                        ohWow.ghostCar = 0;
                    }

                    //We actually clicked on something in the world...
                    if(idx != -1 && ohWow.cameraTarget)
                    {
                        if(ohWow.currentlyOpen == brickBar)
                            myTempBrick.teleport(BtToGlm(res.m_hitPointWorld[idx]));

                        if(box->currentPhase == selectionBox::selectionPhase::waitingForClick)
                        {
                            box->minExtents = res.m_hitPointWorld[idx];
                            box->maxExtents = res.m_hitPointWorld[idx];
                            box->currentPhase = selectionBox::selectionPhase::selecting;
                            box->movePulls();
                        }
                        else if(box->currentPhase == selectionBox::selectionPhase::selecting)
                        {
                            box->currentPhase = selectionBox::selectionPhase::stretching;
                        }

                        /*if(stageOfSelection == 0)
                        {
                            drawDebugLines = true;
                            start = BtToGlm(res.m_hitPointWorld[idx]);
                            end = BtToGlm(res.m_hitPointWorld[idx]);
                            stageOfSelection = 1;
                        }
                        else if(stageOfSelection == 1)
                        {
                            stageOfSelection = -1;
                            drawDebugLines = false;

                            if(start.x > end.x)
                                std::swap(start.x,end.x);
                            if(start.y > end.y)
                                std::swap(start.y,end.y);
                            if(start.z > end.z)
                                std::swap(start.z,end.z);

                            packet data;
                            data.writeUInt(5,4);
                            data.writeFloat(start.x);
                            data.writeFloat(start.y);
                            data.writeFloat(start.z);
                            data.writeFloat(end.x);
                            data.writeFloat(end.y);
                            data.writeFloat(end.z);
                            ohWow.connection->send(&data,true);
                        }*/
                    }
                }
            }
        }

        if(justTurnOnChat)
        {
            justTurnOnChat = false;
            chatEditbox->setText("");
        }
        supress |= chatEditbox->isActive();

        playerInput.supress(supress);

        if(playerInput.commandPressed(changeMaterial))
        {
            switch(myTempBrick.mat)
            {
                case brickMaterial::none: default: myTempBrick.mat = undulo; break;
                case brickMaterial::undulo: myTempBrick.mat = bob; break;
                case brickMaterial::bob: myTempBrick.mat = peral; break;
                case brickMaterial::peral: myTempBrick.mat = chrome; break;
                case brickMaterial::chrome: myTempBrick.mat = glow; break;
                case brickMaterial::glow: myTempBrick.mat = blink; break;
                case brickMaterial::blink: myTempBrick.mat = swirl; break;
                case brickMaterial::swirl: myTempBrick.mat = slippery; break;
                case brickMaterial::slippery: myTempBrick.mat = none; break;
            }

            ohWow.palette->window->getChild("PaintName")->setText(getBrickMatName(myTempBrick.mat));

            ohWow.palette->open();
        }
        if(playerInput.commandPressed(startSelection))
            box->currentPhase = selectionBox::selectionPhase::waitingForClick;
        if(playerInput.commandPressed(dropPlayerAtCamera) && ohWow.cameraTarget)
        {
            //ohWow.cameraTarget->setPosition(btVector3(ohWow.playerCamera->getPosition().x,ohWow.playerCamera->getPosition().y,ohWow.playerCamera->getPosition().z));
            //ohWow.cameraTarget->setLinearVelocity(glm::vec3(0,0,0));
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
            ohWow.brickSelector->setVisible(true);
            ohWow.brickSelector->moveToFront();
        }
        if(playerInput.commandPressed(openInventory))
        {
            if(ohWow.currentlyOpen == inventory)
                ohWow.currentlyOpen = allClosed;
            else
                ohWow.currentlyOpen = inventory;
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
            ohWow.currentlyOpen = paintCan;
            glm::vec3 pos = ohWow.playerCamera->getPosition();
            ohWow.speaker->playSound("Rattle",false,pos.x,pos.y,pos.z);
            ohWow.palette->advanceColumn();
            myTempBrick.basicChanged = myTempBrick.basicChanged || myTempBrick.isBasic;
            myTempBrick.specialChanged = myTempBrick.specialChanged || !myTempBrick.isBasic;
        }
        CEGUI::Window *brickPopup = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("HUD")->getChild("BrickPopup");

        bottomBarShouldBeOpen = ohWow.currentlyOpen == brickBar; //myTempBrick.brickSlotSelected != -1;
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

        if(myTempBrick.brickSlotSelected != -1)
        {
            CEGUI::UDim dx = brickHighlighter->getPosition().d_x;
            CEGUI::UDim dy = brickHighlighter->getPosition().d_y;
            dx.d_scale = 0.0020202 + (myTempBrick.brickSlotSelected-1) * 0.111;
            brickHighlighter->setPosition(CEGUI::UVector2(dx,dy));
        }

        bool guiOpened=false,guiClosed=false;
        myTempBrick.selectBrick(playerInput,hud,ohWow.brickSelector,ohWow.staticBricks,guiOpened,guiClosed);
        if(guiOpened && !guiClosed)
        {
            if(ohWow.currentlyOpen != brickBar)
                bottomBarLastAct = SDL_GetTicks();
            ohWow.currentlyOpen = brickBar;

        }
        if(guiClosed && !guiOpened)
        {
            if(ohWow.currentlyOpen == brickBar)
                bottomBarLastAct = SDL_GetTicks();
            ohWow.currentlyOpen = allClosed;
        }

        if(myTempBrick.brickSlotSelected != -1)
            myTempBrick.manipulate(playerInput,hud,ohWow.brickSelector,ohWow.playerCamera->getYaw(),ohWow.speaker,ohWow.staticBricks);

        if(playerInput.commandPressed(plantBrick))
        {
            if(box->currentPhase == selectionBox::selectionPhase::selecting)
            {
                packet data;
                data.writeUInt(5,4);
                data.writeFloat(box->minExtents.x());
                data.writeFloat(box->minExtents.y());
                data.writeFloat(box->minExtents.z());
                data.writeFloat(box->maxExtents.x());
                data.writeFloat(box->maxExtents.y());
                data.writeFloat(box->maxExtents.z());
                ohWow.connection->send(&data,true);

                box->currentPhase = selectionBox::selectionPhase::idle;
            }
            else
                myTempBrick.plant(ohWow.staticBricks,world,&ohWow);
        }

        myTempBrick.update(ohWow.staticBricks,ohWow.palette);

        if(ohWow.adminCam && camMode != cammode_adminCam)
            camMode = cammode_adminCam;
        else if(!ohWow.adminCam && camMode == cammode_adminCam)
            camMode = cammode_firstPerson;

        if(camMode == cammode_adminCam) //admin cam
        {
            crossHair->setVisible(false);
            ohWow.playerCamera->thirdPerson = false;
            float cameraSpeed = 0.035;
            if(states[SDL_SCANCODE_LCTRL] == SDL_PRESSED)
                cameraSpeed = 0.2;
            if(playerInput.commandKeyDown(walkForward))
                ohWow.playerCamera->walkForward(deltaT * cameraSpeed);
            if(playerInput.commandKeyDown(walkBackward))
                ohWow.playerCamera->walkForward(-deltaT * cameraSpeed);
            if(playerInput.commandKeyDown(walkLeft))
                ohWow.playerCamera->walkRight(-deltaT * cameraSpeed);
            if(playerInput.commandKeyDown(walkRight))
                ohWow.playerCamera->walkRight(deltaT * cameraSpeed);
        }
        else
        {
            if(ohWow.boundToObject && ohWow.cameraTarget)
            {
                if(camMode == cammode_firstPerson) //1st person
                {
                    crossHair->setVisible(true);
                    ohWow.playerCamera->thirdPerson = false;

                    glm::vec3 pos;
                    if(ohWow.currentPlayer && !ohWow.giveUpControlOfCurrentPlayer)
                    {
                        btTransform t = ohWow.currentPlayer->body->getWorldTransform();
                        btVector3 v = t.getOrigin();
                        pos.x = v.x();
                        pos.y = v.y();
                        pos.z = v.z();
                    }
                    else
                    {
                        if(ohWow.currentPlayer)
                            ohWow.currentPlayer->useGlobalTransform = false;
                        pos = ohWow.cameraTarget->modelInterpolator.getPosition();
                    }

                    pos += ohWow.cameraTarget->type->eyeOffset;
                    ohWow.playerCamera->setPosition( pos );
                }
                else //3rd person
                {
                    crossHair->setVisible(false);
                    ohWow.playerCamera->thirdPerson = true;

                    //3rd person camera follow distance backwards raycast:
                    btVector3 v;
                    if(ohWow.currentPlayer && ohWow.currentPlayer->body)
                    {
                        btTransform t = ohWow.currentPlayer->body->getWorldTransform();
                        v = t.getOrigin();
                    }
                    else
                    {
                        v = glmToBt(ohWow.cameraTarget->modelInterpolator.getPosition());
                    }

                    v += glmToBt(ohWow.cameraTarget->type->eyeOffset / glm::vec3(2.0,2.0,2.0));

                    btVector3 raystart = glmToBt(glm::vec3(v.x(),v.y(),v.z()));
                    btVector3 rayend = glmToBt(glm::vec3(v.x(),v.y(),v.z()) - ohWow.playerCamera->getDirection() * glm::vec3(30.0));
                    btCollisionWorld::ClosestRayResultCallback res(raystart,rayend);
                    world->rayTest(raystart,rayend,res);

                    /*int idx = -1;
                    float dist = 9999999;

                    if(ohWow.cameraTarget)
                    {
                        for(int a = 0; a<res.m_collisionObjects.size(); a++)
                        {
                            if(res.m_collisionObjects[a] != ohWow.cameraTarget->body)
                            {
                                if(fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-ohWow.playerCamera->getPosition())) < dist)
                                {
                                    dist = fabs(glm::length(BtToGlm(res.m_hitPointWorld[a])-ohWow.playerCamera->getPosition()));
                                    idx = a;
                                }
                            }
                        }
                    }*/

                    if(ohWow.currentPlayer && ohWow.currentPlayer->body && !ohWow.giveUpControlOfCurrentPlayer)
                    {
                        btTransform t = ohWow.currentPlayer->body->getWorldTransform();
                        v = t.getOrigin();
                        ohWow.playerCamera->thirdPersonTarget = glm::vec3(v.x(),v.y(),v.z()) + ohWow.cameraTarget->type->eyeOffset / glm::vec3(2.0,2.0,2.0);
                        ohWow.playerCamera->thirdPersonDistance = 30 * res.m_closestHitFraction * 0.98;
                        ohWow.playerCamera->setPosition(glm::vec3(v.x(),v.y(),v.z()));
                        ohWow.playerCamera->turn(0,0);
                        ohWow.currentPlayer->useGlobalTransform = true;
                        btQuaternion bulletRot = t.getRotation();
                        glm::quat rot = glm::quat(bulletRot.w(),bulletRot.x(),bulletRot.y(),bulletRot.z());
                        ohWow.currentPlayer->globalTransform = glm::translate(glm::vec3(v.x(),v.y(),v.z())) * glm::toMat4(rot);
                    }
                    else
                    {
                        if(ohWow.currentPlayer)
                            ohWow.currentPlayer->useGlobalTransform = false;

                        ohWow.playerCamera->thirdPersonTarget = ohWow.cameraTarget->modelInterpolator.getPosition() + ohWow.cameraTarget->type->eyeOffset / glm::vec3(2.0,2.0,2.0);
                        ohWow.playerCamera->thirdPersonDistance = 30 * res.m_closestHitFraction * 0.98;
                        ohWow.playerCamera->setPosition(ohWow.cameraTarget->modelInterpolator.getPosition());
                        ohWow.playerCamera->turn(0,0);
                    }
                }
                //TODO: Should just store which item is the current held not-hidden item, loop is bad
                for(int a = 0; a<ohWow.items.size(); a++)
                {
                    if(ohWow.currentPlayer && ohWow.items[a]->heldBy == ohWow.currentPlayer && !ohWow.items[a]->hidden)
                    {
                        if(ohWow.giveUpControlOfCurrentPlayer)
                            ohWow.items[a]->updateTransform(false,ohWow.playerCamera->getYaw());
                        else
                            ohWow.items[a]->updateTransform(true,ohWow.playerCamera->getYaw());
                        ohWow.items[a]->calculateMeshTransforms(0);
                    }
                }
            }
            else //static cam
            {
                ohWow.playerCamera->thirdPerson = false;
                ohWow.playerCamera->setPosition(ohWow.cameraPos);
                if(!ohWow.freeLook)
                {
                    ohWow.playerCamera->setDirection(ohWow.cameraDir);
                    ohWow.playerCamera->turn(0,0);
                }
            }
        }

        if(playerInput.commandKeyDown(zoom))
            desiredFov = 15;
        else
            desiredFov = ohWow.settings->fieldOfView;

        currentZoom += (desiredFov - currentZoom) * deltaT * 0.001;
        ohWow.playerCamera->setFieldOfVision(currentZoom);

        glm::vec3 microphoneDir = glm::vec3(sin(ohWow.playerCamera->getYaw()),0,cos(ohWow.playerCamera->getYaw()));
        ohWow.speaker->microphone(ohWow.playerCamera->getPosition()+glm::vec3(0.05,0.05,0.05),microphoneDir);

        if(playerInput.commandKeyDown(jump))
            doJump = true;

        for(unsigned int a = 0; a<ohWow.items.size(); a++)
        {
            heldItemType *type = ohWow.items[a]->itemType;
            if(type->useDefaultSwing)
            {
                if(ohWow.items[a]->heldBy == ohWow.cameraTarget)
                    ohWow.items[a]->advance(context.getMouseLocked() && mouseState & SDL_BUTTON_LEFT,deltaT);
                else
                    ohWow.items[a]->advance(false,deltaT);
            }
            else
                ohWow.items[a]->advance(false,deltaT);
            ohWow.items[a]->bufferSubData();
        }

        for(int a = 0; a<ohWow.newDynamics.size(); a++)
        {
            ohWow.newDynamics[a]->hidden = (ohWow.newDynamics[a] == ohWow.cameraTarget) && (camMode == cammode_firstPerson);
            ohWow.newDynamics[a]->calculateMeshTransforms(deltaT);
            ohWow.newDynamics[a]->bufferSubData();
        }
        for(int a = 0; a<ohWow.items.size(); a++)
            if(!(ohWow.currentPlayer && ohWow.items[a]->heldBy == ohWow.currentPlayer && !ohWow.items[a]->hidden))
                ohWow.items[a]->calculateMeshTransforms(0);

        auto emitterIter = ohWow.emitters.begin();
        while(emitterIter != ohWow.emitters.end())
        {
            emitter *e = *emitterIter;
            if(e->type->lifetimeMS != 0 && !e->attachedToCar && !e->attachedToBasicBrick && !e->attachedToSpecialBrick)
            {
                if(e->creationTime + e->type->lifetimeMS < SDL_GetTicks())
                {
                    delete e;
                    emitterIter = ohWow.emitters.erase(emitterIter);
                    continue;
                }
            }
            bool usePhysicsPos = ohWow.currentPlayer && ohWow.currentPlayer == e->attachedToModel && !ohWow.giveUpControlOfCurrentPlayer;
            e->update(ohWow.playerCamera->position,ohWow.playerCamera->direction,usePhysicsPos);
            ++emitterIter;
        }

        for(unsigned int a = 0; a<ohWow.particleTypes.size(); a++)
            ohWow.particleTypes[a]->deadParticlesCheck(ohWow.playerCamera->getPosition(),deltaT);

        sortLights(ohWow.lights,ohWow.playerCamera->getPosition(),deltaT);

        //Networking:

        ohWow.tryApplyHeldPackets();

        checkForCameraToBind(&ohWow);

        if(connected)
        {
            glm::vec3 up = glm::vec3(0,1,0);
            ohWow.playerCamera->nominalUp = up;

            if(ohWow.cameraLean && camMode == 0 && ohWow.cameraTarget)
            {
                ohWow.totalLean += playerInput.commandKeyDown(walkLeft) ? deltaT * 0.001 : 0;
                ohWow.totalLean -= playerInput.commandKeyDown(walkRight) ? deltaT * 0.001 : 0;
                if(!(playerInput.commandKeyDown(walkRight) || playerInput.commandKeyDown(walkLeft)))
                    ohWow.totalLean *= 0.9;

                if(ohWow.totalLean > 0.17)
                    ohWow.totalLean = 0.17;
                if(ohWow.totalLean < -0.17)
                    ohWow.totalLean = -0.17;

                glm::vec3 playerForward = glm::vec3(0,0,1);//ohWow.playerCamera->getDirection();
                glm::vec3 playerRight = glm::normalize(glm::cross(up,playerForward));

                glm::vec3 dir = glm::normalize(glm::vec3(ohWow.totalLean * playerRight.x,1,ohWow.totalLean * playerRight.z));
                glm::vec4 afterDir = glm::toMat4(ohWow.cameraTarget->modelInterpolator.getRotation()) * glm::vec4(dir.x,dir.y,dir.z,0);
                ohWow.playerCamera->nominalUp = glm::vec3(afterDir.x,afterDir.y,afterDir.z);
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
            if(ohWow.currentPlayer && !ohWow.giveUpControlOfCurrentPlayer && camMode != cammode_adminCam)
            {
                if(SDL_GetTicks() - ohWow.currentPlayer->flingPreventionStartTime > 100)
                {
                    if(ohWow.currentPlayer->control(atan2(ohWow.playerCamera->getDirection().x,ohWow.playerCamera->getDirection().z),netControlState & 1,netControlState & 2,netControlState & 4,netControlState & 8,netControlState &16,ohWow.canJet && rightDown))
                        didJump = true;
                    if(didJump)
                        ohWow.speaker->playSound("Jump",false,ohWow.playerCamera->getPosition().x,ohWow.playerCamera->getPosition().y-1.0,ohWow.playerCamera->getPosition().z);
                }
            }

            int fullControlState  = netControlState + (leftDown << 5);
            fullControlState |= (ohWow.currentlyOpen == inventory) ? 0b1000000 : 0;
            fullControlState |= ohWow.selectedSlot << 7;
            fullControlState += rightDown << 11;
            fullControlState |= (ohWow.currentlyOpen == paintCan) ? (1 << 12) : 0;
            fullControlState |= chatEditbox->isActive() ? (1 << 13) : 0;

            glm::vec3 playerDirDiff = ohWow.playerCamera->getDirection() - lastPlayerDir;
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
                    transPacket.writeBit(ohWow.currentlyOpen == paintCan);
                    if(ohWow.currentlyOpen == paintCan)
                    {
                        glm::vec4 paintColor = ohWow.palette->getColor();
                        transPacket.writeFloat(paintColor.r);
                        transPacket.writeFloat(paintColor.g);
                        transPacket.writeFloat(paintColor.b);
                        transPacket.writeFloat(paintColor.a);
                    }

                    doJump = false;

                    lastPlayerDir = ohWow.playerCamera->getDirection();
                    transPacket.writeFloat(ohWow.playerCamera->getDirection().x);
                    transPacket.writeFloat(ohWow.playerCamera->getDirection().y);
                    transPacket.writeFloat(ohWow.playerCamera->getDirection().z);
                    if(ohWow.currentlyOpen == inventory)
                        transPacket.writeUInt(ohWow.selectedSlot,3);
                    else
                        transPacket.writeUInt(7,3);

                    transPacket.writeBit(ohWow.adminCam);
                    if(ohWow.adminCam)
                    {
                        transPacket.writeFloat(ohWow.playerCamera->getPosition().x);
                        transPacket.writeFloat(ohWow.playerCamera->getPosition().y);
                        transPacket.writeFloat(ohWow.playerCamera->getPosition().z);
                    }
                    transPacket.writeBit(chatEditbox->isActive());

                    bool actuallyUseClientPhysics = useClientPhysics;
                    if(!ohWow.currentPlayer)
                        actuallyUseClientPhysics = false;
                    if(ohWow.giveUpControlOfCurrentPlayer)
                        actuallyUseClientPhysics = false;

                    transPacket.writeBit(actuallyUseClientPhysics);
                    if(actuallyUseClientPhysics)
                    {
                        btTransform t = ohWow.currentPlayer->body->getWorldTransform();
                        btVector3 o = t.getOrigin();
                        transPacket.writeFloat(o.x());
                        transPacket.writeFloat(o.y());
                        transPacket.writeFloat(o.z());
                        btVector3 v = ohWow.currentPlayer->body->getLinearVelocity();
                        transPacket.writeFloat(v.x());
                        transPacket.writeFloat(v.y());
                        transPacket.writeFloat(v.z());
                    }

                    serverConnection.send(&transPacket,false);

                    lastSentTransData = SDL_GetTicks();
                }
            }

            float progress = ((float)ohWow.staticBricks.getBrickCount()) / ((float)ohWow.skippingCompileNextBricks);
            setBrickLoadProgress(progress,ohWow.staticBricks.getBrickCount());

            serverConnection.run();
        }

        for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
        {
            //std::cout<<"Advancing: "<<deltaT<<"\n";
            ohWow.livingBricks[a]->carTransform.advance(deltaT);
            //std::cout<<"\n";
            for(int wheel = 0; wheel<ohWow.livingBricks[a]->wheels.size(); wheel++)
                ohWow.livingBricks[a]->wheels[wheel]->advance(deltaT);
        }

        //std::cout<<ohWow.staticBricks.opaqueBasicBricks.size()<<" bricks\n";

        auto fakeKillIter = ohWow.fakeKills.begin();
        while(fakeKillIter != ohWow.fakeKills.end())
        {
            if((*fakeKillIter).endTime < SDL_GetTicks())
            {
                if((*fakeKillIter).basic)
                    ohWow.staticBricks.removeBasicBrick((*fakeKillIter).basic,ohWow.world);
                if((*fakeKillIter).special)
                    ohWow.staticBricks.removeSpecialBrick((*fakeKillIter).special,ohWow.world);
                fakeKillIter = ohWow.fakeKills.erase(fakeKillIter);
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
                    ohWow.staticBricks.updateBasicBrick((*fakeKillIter).basic,ohWow.world);
                }
                if((*fakeKillIter).special)
                {
                    (*fakeKillIter).special->position += (*fakeKillIter).linVel;
                    (*fakeKillIter).special->rotation = glm::slerp((*fakeKillIter).special->rotation,glm::quat((*fakeKillIter).angVel),progress);
                    ohWow.staticBricks.updateSpecialBrick((*fakeKillIter).special,ohWow.world,0);
                }
                fakeKillIter++;
            }
        }

        //Graphics:
        ohWow.env->iblShadowsCalc(ohWow.playerCamera,glm::vec3(0.496595,0.50,0.856871));

        glActiveTexture(GL_TEXTURE0 + cubeMapRadiance);
        glBindTexture(GL_TEXTURE_CUBE_MAP,IBLRad);
        glActiveTexture(GL_TEXTURE0 + cubeMapIrradiance);
        glBindTexture(GL_TEXTURE_CUBE_MAP,IBLIrr);
        bdrf->bind(brdf);

        std::string oldName = "";

        ohWow.env->godRayPass->bind();
        if(ohWow.settings->godRayQuality != godRaysOff)
        {
            shadowProgramNoGeo.use();

                glUniform1i(shadowNoGeoUnis.doingGodRayPass,true);
                oldName = ohWow.playerCamera->name;
                ohWow.playerCamera->name = "Shadow";
                ohWow.playerCamera->render(shadowNoGeoUnis);
                ohWow.env->passUniforms(shadowNoGeoUnis);
                ohWow.env->drawSun(shadowNoGeoUnis);
                glUniform1i(shadowNoGeoUnis.doingGodRayPass,false);

            shadowBrickProgramNoGeo.use();

                glUniform1i(shadowBrickNoGeoUnis.doingGodRayPass,true);
                ohWow.playerCamera->render(shadowBrickNoGeoUnis);
                ohWow.playerCamera->name = oldName;
                ohWow.staticBricks.renderEverything(shadowBrickNoGeoUnis,true,0,SDL_GetTicks()/25);
                for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                    ohWow.livingBricks[a]->renderAlive(shadowBrickNoGeoUnis,true,SDL_GetTicks()/25);
                glUniform1i(shadowBrickNoGeoUnis.doingGodRayPass,false);
        }
        ohWow.env->godRayPass->unbind();

        //Begin drawing to water textures

        waterFrame++;
        if(waterFrame >= 2)
            waterFrame = 0;

        if(ohWow.settings->waterQuality != waterStatic && waterRefraction)
        {
            if(waterFrame == 0)
            {
                waterReflection->bind();

                    basicProgram.use();
                        glUniform1f(basic.clipHeight,ohWow.waterLevel);
                        ohWow.playerCamera->renderReflection(basic,ohWow.waterLevel);
                        ohWow.env->passUniforms(basic);
                        ohWow.settings->render(basic);
                        glActiveTexture(GL_TEXTURE0 + cubeMapEnvironment);
                        glBindTexture(GL_TEXTURE_CUBE_MAP,IBL);
                        ohWow.env->drawSky(basic);

                        glEnable(GL_CLIP_DISTANCE0);

                        for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                        {
                            for(unsigned int wheel = 0; wheel<ohWow.livingBricks[a]->wheels.size(); wheel++)
                            {
                                wheelModel.render(&basic,
                                                  glm::translate(ohWow.livingBricks[a]->wheels[wheel]->getPosition()) *
                                                  glm::toMat4(ohWow.livingBricks[a]->wheels[wheel]->getRotation()) *
                                                  glm::scale(glm::vec3(0.06)) *
                                                  glm::scale(ohWow.livingBricks[a]->wheels[wheel]->scale)
                                                  );
                            }
                        }


                    newModelProgram.use();
                        glUniform1f(newModelUnis.clipHeight,ohWow.waterLevel);
                        ohWow.settings->render(newModelUnis);
                        ohWow.playerCamera->renderReflection(newModelUnis,ohWow.waterLevel);
                        ohWow.env->passUniforms(newModelUnis);
                        for(int a = 0; a<ohWow.newDynamicTypes.size(); a++)
                            ohWow.newDynamicTypes[a]->renderInstanced(&newModelUnis);


                        /*for(unsigned int a = 0; a<ohWow.dynamics.size(); a++)
                                ohWow.dynamics[a]->render(&basic);*/

                     /*tessProgram.use();
                        glUniform1f(tess.clipHeight,waterLevel);
                        ohWow.playerCamera->render(tess);
                        ohWow.env->passUniforms(tess);
                        grass.use(tess);
                        theMap.render(tess);*/

                    brickSimpleProgram.use();

                        glUniform1f(brickSimpleUnis.clipHeight,ohWow.waterLevel);
                        ohWow.playerCamera->renderReflection(brickSimpleUnis,ohWow.waterLevel);
                        ohWow.env->passUniforms(brickSimpleUnis);
                        ohWow.settings->render(brickSimpleUnis);

                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        ohWow.staticBricks.renderEverything(brickSimpleUnis,false,&grass,SDL_GetTicks()/25);
                        for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                            ohWow.livingBricks[a]->renderAlive(brickSimpleUnis,false,SDL_GetTicks()/25);
                        glDisable(GL_BLEND);

                waterReflection->unbind();
            }
            else if(waterFrame == 1)
            {
                glEnable(GL_CLIP_DISTANCE0);
                waterRefraction->bind();

                    newModelProgram.use();
                        glUniform1f(newModelUnis.clipHeight,-ohWow.waterLevel);
                        ohWow.settings->render(newModelUnis);
                        ohWow.playerCamera->render(newModelUnis);
                        ohWow.env->passUniforms(newModelUnis);
                        for(int a = 0; a<ohWow.newDynamicTypes.size(); a++)
                            ohWow.newDynamicTypes[a]->renderInstanced(&newModelUnis);

                    basicProgram.use();
                        glUniform1f(basic.clipHeight,-ohWow.waterLevel);
                        ohWow.playerCamera->render(basic);
                        ohWow.env->passUniforms(basic);

                        for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                        {
                            for(unsigned int wheel = 0; wheel<ohWow.livingBricks[a]->wheels.size(); wheel++)
                            {
                                wheelModel.render(&basic,
                                                  glm::translate(ohWow.livingBricks[a]->wheels[wheel]->getPosition()) *
                                                  glm::toMat4(ohWow.livingBricks[a]->wheels[wheel]->getRotation()) *
                                                  glm::scale(glm::vec3(0.06)) *
                                                  glm::scale(ohWow.livingBricks[a]->wheels[wheel]->scale)
                                                  );
                            }
                        }
                        //ohWow.env->drawSky(basic);
                        /*(for(unsigned int a = 0; a<ohWow.dynamics.size(); a++)
                                ohWow.dynamics[a]->render(&basic);*/

                    /*tessProgram.use();
                        ohWow.playerCamera->render(tess);
                        glUniform1f(tess.clipHeight,-waterLevel);
                        ohWow.env->passUniforms(tess);
                        grass.use(tess);
                        theMap.render(tess);*/

                    brickSimpleProgram.use();
                    ohWow.env->passUniforms(brickSimpleUnis);
                    ohWow.settings->render(brickSimpleUnis);
                        glUniform1f(brickSimpleUnis.clipHeight,-ohWow.waterLevel);
                        ohWow.playerCamera->render(brickSimpleUnis);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        ohWow.staticBricks.renderEverything(brickSimpleUnis,false,&grass,SDL_GetTicks()/25);
                        for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                            ohWow.livingBricks[a]->renderAlive(brickSimpleUnis,false,SDL_GetTicks()/25);
                        glDisable(GL_BLEND);


                waterRefraction->unbind();
            }
            glDisable(GL_CLIP_DISTANCE0);

            waterDepth->bind();
                 /*tessProgram.use();
                    glUniform1f(tess.clipHeight,-waterLevel);
                    ohWow.playerCamera->render(tess);
                    grass.use(tess); //TODO: Yes, at the moment, this is required for some reason...
                    theMap.render(tess);*/

                brickProgram.use();
                    ohWow.playerCamera->render(brickUnis);
                    ohWow.env->passUniforms(brickUnis);
                    ohWow.staticBricks.renderEverything(brickUnis,true,0,SDL_GetTicks()/25);
                    for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                        ohWow.livingBricks[a]->renderAlive(brickUnis,true,SDL_GetTicks()/25);

            waterDepth->unbind();
        }


        //End drawing to water textures

        if(ohWow.settings->shadowResolution != shadowsOff)
        {
            //Begin drawing shadows to shadow texture
            glDisable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glEnable(GL_BLEND);

            ohWow.env->shadowBuffer->bind();

                shadowProgram.use();
                    ohWow.env->passLightMatricies(shadow);

                    for(unsigned int a = 0; a<ohWow.items.size(); a++)
                    {
                        /*if(ohWow.items[a]->heldBy == ohWow.cameraTarget)
                            ohWow.items[a]->render(shadow,true,ohWow.playerCamera->getYaw());*/
                        if(!(!ohWow.giveUpControlOfCurrentPlayer && ohWow.currentPlayer && ohWow.items[a]->heldBy == ohWow.currentPlayer && !ohWow.items[a]->hidden))
                            ohWow.items[a]->updateTransform();
                    }

                    for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                    {
                        for(unsigned int wheel = 0; wheel<ohWow.livingBricks[a]->wheels.size(); wheel++)
                        {
                            wheelModel.render(&shadow,
                                              glm::translate(ohWow.livingBricks[a]->wheels[wheel]->getPosition()) *
                                              glm::toMat4(ohWow.livingBricks[a]->wheels[wheel]->getRotation()) *
                                              glm::scale(glm::vec3(0.06)) *
                                              glm::scale(ohWow.livingBricks[a]->wheels[wheel]->scale),true
                                              );
                        }
                    }

                    /*for(unsigned int a = 0; a<ohWow.dynamics.size(); a++)
                        ohWow.dynamics[a]->render(&shadow);*/

                newModelShadowProgram.use();
                    ohWow.env->passLightMatricies(newModelShadowUnis);
                    for(int a = 0; a<ohWow.newDynamicTypes.size(); a++)
                        ohWow.newDynamicTypes[a]->renderInstancedWithoutMaterials();


                shadowBrickProgram.use();
                        ohWow.env->passLightMatricies(shadowBrickUnis);
                        ohWow.staticBricks.renderEverything(shadowBrickUnis,true,0,SDL_GetTicks()/25);
                        for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                            ohWow.livingBricks[a]->renderAlive(shadowBrickUnis,true,SDL_GetTicks()/25);

            ohWow.env->shadowBuffer->unbind();

            glDisable(GL_BLEND);
            glCullFace(GL_BACK);
            glEnable(GL_CULL_FACE);
            //End drawing shadows to shadow texture
        }

        //Begin drawing final scene
        context.clear(ohWow.env->skyColor.r,ohWow.env->skyColor.g,ohWow.env->skyColor.b);
        context.select();

            basicProgram.use();
                ohWow.settings->render(basic);
                ohWow.playerCamera->render(basic); //change
                ohWow.env->passUniforms(basic);
                renderLights(basic,ohWow.lights);

                glActiveTexture(GL_TEXTURE0 + cubeMapEnvironment);
                glBindTexture(GL_TEXTURE_CUBE_MAP,IBL);

                ohWow.env->drawSky(basic);



                if(ohWow.settings->waterQuality != waterStatic && waterRefraction)
                {
                    waterProgram.use();
                        ohWow.settings->render(waterUnis);
                        glUniform1f(waterUnis.deltaT,((float)SDL_GetTicks()) / 1000.0);     //why both of these...?
                        glUniform1f(waterUnis.waterDelta,((float)SDL_GetTicks())*0.0001);
                        glUniform1f(waterUnis.target->getUniformLocation("waterLevel"),ohWow.waterLevel); //TODO: Don't string match this every frame
                        ohWow.env->passUniforms(waterUnis,true);
                        ohWow.playerCamera->render(waterUnis);

                        waterReflection->colorResult->bind(reflection);
                        waterRefraction->colorResult->bind(refraction);
                        waterDepth->depthResult->bind(shadowNearMap);
                        dudvTexture->bind(normal);

                        water.render(waterUnis);

                        basicProgram.use();
                }


                /*for(unsigned int a = 0; a<ohWow.dynamics.size(); a++)
                {
                    //if((ohWow.dynamics[a] != ohWow.cameraTarget) || (camMode != cammode_firstPerson))
                    if(ohWow.dynamics[a] != ohWow.cameraTarget)
                    {
                        ohWow.dynamics[a]->render(&basic);
                    }
                    else if(ohWow.dynamics[a] == ohWow.cameraTarget && camMode != cammode_firstPerson)
                    {
                        /*glm::mat4 final = glm::translate(glm::vec3(0,176.897,0)) * glm::rotate(glm::mat4(1.0),ohWow.playerCamera->getPitch(),glm::vec3(1.0,0.0,0.0)) * glm::translate(glm::vec3(0,-176.897,0));
                        ohWow.dynamics[a]->setExtraTransform("Head",final);
                        ohWow.dynamics[a]->setExtraTransform("Face1",final);
                        ohWow.dynamics[a]->render(&basic);
                    }
                }*/

                renderLights(basic,ohWow.lights);
                for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                {
                    for(unsigned int wheel = 0; wheel<ohWow.livingBricks[a]->wheels.size(); wheel++)
                    {
                        wheelModel.render(&basic,
                                          glm::translate(ohWow.livingBricks[a]->wheels[wheel]->getPosition()) *
                                          glm::toMat4(ohWow.livingBricks[a]->wheels[wheel]->getRotation()) *
                                          glm::scale(glm::vec3(0.06)) *
                                          glm::scale(ohWow.livingBricks[a]->wheels[wheel]->scale)
                                          );
                    }
                }

                for(unsigned int a = 0; a<ohWow.newDynamicTypes.size(); a++)
                    ohWow.newDynamicTypes[a]->renderNonInstanced(&basic);
                basic.setModelMatrix(glm::mat4(1.0));

                for(unsigned int a = 0; a<ohWow.items.size(); a++)
                {
                    /*if(ohWow.items[a]->heldBy == ohWow.cameraTarget)
                    {
                        if(!ohWow.usingPaint)
                            ohWow.items[a]->render(basic,true,ohWow.playerCamera->getYaw());
                    }
                    else*/
                    if(!(!ohWow.giveUpControlOfCurrentPlayer && ohWow.currentPlayer && ohWow.items[a]->heldBy == ohWow.currentPlayer && !ohWow.items[a]->hidden))
                        ohWow.items[a]->updateTransform();
                }

                if(ohWow.paintCan && ohWow.fixedPaintCanItem && ohWow.currentlyOpen == paintCan && ohWow.cameraTarget)
                {
                    ohWow.fixedPaintCanItem->pitch = -1.57;
                    ohWow.fixedPaintCanItem->hidden = false;
                    ohWow.fixedPaintCanItem->heldBy = ohWow.cameraTarget;
                    ohWow.fixedPaintCanItem->updateTransform(true,ohWow.playerCamera->getYaw());
                }
                else
                    ohWow.fixedPaintCanItem->hidden = true;

                if(ohWow.currentlyOpen == inventory)
                {
                    for(unsigned int a = 0; a<inventorySize; a++)
                    {
                        heldItemType *type = ohWow.inventory[a];
                        if(!type)
                            continue;

                        glm::vec3 startSize = type->type->totalColMax - type->type->totalColMin;

                        float highestDim = startSize.x;
                        if(startSize.y > highestDim)
                            highestDim = startSize.y;
                        if(startSize.z > highestDim)
                            highestDim = startSize.z;

                        startSize.x = 1.0 / highestDim;
                        startSize.y = 1.0 / highestDim;
                        startSize.z = 1.0 / highestDim;

                        glm::vec3 trans = startSize * type->type->totalColMin;

                        glm::mat4 tot;
                        if(ohWow.selectedSlot == a)
                            tot = glm::scale(startSize) * glm::translate(-type->type->totalColMin) * glm::toMat4(glm::quat(glm::vec3(0,fmod(((float)SDL_GetTicks())*0.0033,6.28),0)));
                        else
                            tot = glm::scale(startSize) * glm::translate(-type->type->totalColMin) * glm::toMat4(glm::quat(glm::vec3(0,1.57,0)));
                        tot = glm::scale(glm::vec3(2,2,1)) * tot;
                        tot = glm::translate(glm::vec3(-1,-1,0)) * tot;

                        float boxSize = 0.21;//0.15372;
                        tot = glm::scale(glm::vec3(0.15)) * tot;
                        tot = glm::translate(glm::vec3(0.87+0.09,1 - (0.04875+boxSize+(boxSize*2*a)),0)) * tot;

                        glUniform1i(basic.target->getUniformLocation("skipCamera"),1);
                        ((animatedModel*)type->type->oldModelType)->render(&basic,0,tot);
                        glUniform1i(basic.target->getUniformLocation("skipCamera"),0);
                    }
                }


                /*hammer.render(&basic,glm::translate(glm::vec3(0,200,0)) * glm::scale(glm::vec3(0.1,0.1,0.1)));
                wrench.render(&basic,glm::translate(glm::vec3(10,200,0)) * glm::scale(glm::vec3(0.1,0.1,0.1)));
                spraycan.render(&basic,glm::translate(glm::vec3(20,200,0)) * glm::scale(glm::vec3(0.1,0.1,0.1)));*/

                //for(int a = 0; a<ohWow.livingBricks.size(); a++)
                  //  ohWow.livingBricks[a]->renderWheels(&basic,&sphere);

                //Preview texture:
                /*glUniform1i(basic.previewTexture,true);
                //waterRefraction->colorResult->bind(normal);
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES,0,6);
                glBindVertexArray(0);
                glUniform1i(basic.previewTexture,false);*/
                //end preview texture

                drawDebugLocations(basic,cubeVAO,ohWow.debugLocations,ohWow.debugColors);

/*            tessProgram.use();
                ohWow.settings->render(tess);
                ohWow.playerCamera->render(tess); //change
                ohWow.env->passUniforms(tess);*/

//                grass.use(tess);
 //               theMap.render(tess);

                newModelProgram.use();
                    ohWow.settings->render(newModelUnis);
                    ohWow.playerCamera->render(newModelUnis);
                    ohWow.env->passUniforms(newModelUnis);
                    renderLights(newModelUnis,ohWow.lights);
                    for(int a = 0; a<ohWow.newDynamicTypes.size(); a++)
                        ohWow.newDynamicTypes[a]->renderInstanced(&newModelUnis);

            brickProgram.use();
                ohWow.playerCamera->render(brickUnis);
                ohWow.env->passUniforms(brickUnis);
                ohWow.settings->render(brickUnis);
                renderLights(brickUnis,ohWow.lights);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                ohWow.staticBricks.renderEverything(brickUnis,false,&grass,SDL_GetTicks()/25);
                for(unsigned int a = 0; a<ohWow.livingBricks.size(); a++)
                    ohWow.livingBricks[a]->renderAlive(brickUnis,false,SDL_GetTicks()/25);
                if(ohWow.ghostCar)
                    ohWow.ghostCar->renderAlive(brickUnis,false,SDL_GetTicks()/25);
                glDisable(GL_BLEND);

                glEnable(GL_BLEND);
                glDisable(GL_CULL_FACE);
                emitterProgram.use();
                    ohWow.playerCamera->render(emitterUnis);
                    ohWow.env->passUniforms(emitterUnis);
                    ohWow.settings->render(emitterUnis);

                    for(unsigned int a = 0; a<ohWow.particleTypes.size(); a++)
                        ohWow.particleTypes[a]->render(emitterUnis);

                glEnable(GL_CULL_FACE);

                glLineWidth(3.0);
                boxEdgesProgram.use();
                    glUniform1i(boxEdgesProgram.getUniformLocation("drawingRopes"),true);
                    ohWow.playerCamera->render(boxEdgesUnis);
                    for(unsigned int a = 0; a<ohWow.ropes.size(); a++)
                        ohWow.ropes[a]->render();
                    glUniform1i(boxEdgesProgram.getUniformLocation("drawingRopes"),false);

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glDisable(GL_CULL_FACE);
            fontProgram.use();
                    ohWow.playerCamera->render(fontUnis);
                    for(unsigned int a = 0; a<ohWow.newDynamics.size(); a++)
                    {
                        if(!ohWow.newDynamics[a])
                            continue;
                        if(ohWow.newDynamics[a]->shapeName.length() < 1)
                            continue;
                        if(ohWow.newDynamics[a] == ohWow.cameraTarget && camMode == 0)
                            continue;

                        glm::vec3 pos = ohWow.newDynamics[a]->modelInterpolator.getPosition();
                        if(ohWow.currentPlayer == ohWow.newDynamics[a] && !ohWow.giveUpControlOfCurrentPlayer && ohWow.currentPlayer->body)
                        {
                            btVector3 o = ohWow.currentPlayer->body->getWorldTransform().getOrigin();
                            pos = glm::vec3(o.x(),o.y(),o.z());
                        }
                        defaultFont.naiveRender(fontUnis,ohWow.newDynamics[a]->shapeName,pos+ohWow.newDynamics[a]->type->eyeOffset+glm::vec3(0,2,0),glm::length(ohWow.newDynamics[a]->scale),ohWow.newDynamics[a]->shapeNameColor);
                    }
            //glEnable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);

            //Remember, 'god rays' actually includes the underwater texture too
            basicProgram.use();
                glUniform1f(basicProgram.getUniformLocation("waterLevel"),ohWow.waterLevel); //TODO: Don't string match this every frame
                ohWow.settings->render(basic);
                ohWow.playerCamera->render(basic);
                ohWow.env->passUniforms(basic);
                ohWow.env->renderGodRays(basic);

            /*spriteProgram.use();
                glEnable(GL_CLIP_DISTANCE0);
                glUniform1f(spriteUnis.clipHeight,waterLevel); //TODO: Only cull plants underwater if camera above water
                ohWow.playerCamera->render(spriteUnis); //change
                ohWow.env->passUniforms(spriteUnis);

                cumT += deltaT;
                glUniform1f(spriteUnis.deltaT,SDL_GetTicks());
                glUniform1i(spriteUnis.useNormal,true);
                glUniform1i(spriteUnis.useAlbedo,true);
                glUniform1i(spriteUnis.calcTBN,true);
                plant1.bind(albedo);
                plant1n.bind(normal);
//                theMap.render(spriteUnis,true);
                glDisable(GL_CULL_FACE);
                bush.render();
                glEnable(GL_CULL_FACE);
                glDisable(GL_CLIP_DISTANCE0);*/

            glEnable(GL_BLEND);

            boxEdgesProgram.use();
                ohWow.playerCamera->render(boxEdgesUnis);
                box->render(boxEdgesUnis);

            glEnable(GL_CULL_FACE);

            //Just for the transparent blue quad of crappy water, good water is rendered first thing
            if(ohWow.settings->waterQuality == waterStatic || !waterRefraction)
            {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                waterProgram.use();
                    ohWow.settings->render(waterUnis);
                    ohWow.env->passUniforms(waterUnis,true);
                    ohWow.playerCamera->render(waterUnis);

                    water.render(waterUnis);

                glDisable(GL_BLEND);
            }
        //End drawing final scene

        ohWow.bottomPrint.checkForTimeouts();
        ohWow.palette->calcAnimation();
        float chatScroll = ohWow.chat->getVertScrollbar()->getScrollPosition();
        chatScrollArrow->setVisible(chatScroll < (ohWow.chat->getVertScrollbar()->getDocumentSize() - ohWow.chat->getVertScrollbar()->getPageSize()));
        CEGUI::System::getSingleton().getRenderer()->setDisplaySize(CEGUI::Size<float>(context.getResolution().x,context.getResolution().y));
        glViewport(0,0,context.getResolution().x,context.getResolution().y);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        CEGUI::System::getSingleton().renderAllGUIContexts();
        context.swap();
        glEnable(GL_DEPTH_TEST);
    }

    info("Shut down complete");
    return 0;
}
