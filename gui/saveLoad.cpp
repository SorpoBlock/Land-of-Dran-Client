#include "saveLoad.h"

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

template <typename TP>
std::string dateString(TP tp)
{
    std::time_t tt = to_time_t(tp);
    std::tm *gmt = std::gmtime(&tt);
    std::stringstream buffer;
    buffer << std::put_time(gmt, "%d %B %Y");
    std::string formattedFileTime = buffer.str();
    return formattedFileTime;
}

bool closeSaveLoadWindow(const CEGUI::EventArgs &e)
{
    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    saveLoadWindow->setVisible(false);

    return true;
}

void refreshSaveLoad()
{
    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    CEGUI::MultiColumnList *saves = (CEGUI::MultiColumnList*)saveLoadWindow->getChild("SavesList");
    saves->resetList();

    std::filesystem::path pa;
    for(const auto& p: std::filesystem::recursive_directory_iterator("saves", std::filesystem::directory_options::skip_permission_denied))
    {
        if(!std::filesystem::exists(p))
            continue;

        pa = p.path();

        if (!is_directory(pa))
        {
            if(std::filesystem::is_regular_file(pa))
            {
                if(true || pa.extension() == ".bls" || pa.extension() == ".lds")
                {
                    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(pa);
                    std::string modified = dateString(ftime);
                    std::string stem = pa.stem().string();

                    int idx = saves->getRowCount();
                    saves->addRow(idx);

                    CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem(stem,idx);
                    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
                    cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
                    saves->setItem(cell,0,idx);

                    cell = new CEGUI::ListboxTextItem(modified,idx);
                    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
                    cell->setSelectionBrushImage("GWEN/Input.ListBox.EvenLineSelected");
                    saves->setItem(cell,1,idx);
                }
            }
        }
    }
}

bool refreshButton(const CEGUI::EventArgs &e)
{
    refreshSaveLoad();

    return true;
}

#define landOfDranCarMagic 24653491
#define landOfDranBuildMagic 16483534

bool saveBuild(const CEGUI::EventArgs &e)
{
    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    clientStuff *clientEnvironment = (clientStuff*)saveLoadWindow->getUserData();
    serverStuff *serverData = clientEnvironment->serverData;

    std::string fileName = saveLoadWindow->getChild("BuildFilePath")->getText().c_str();
    if(fileName.length() < 1)
    {
        error("Enter in a file name before saving a build!");
        return false;
    }
    fileName = "saves/" + fileName + ".lbs";

    std::ofstream build(fileName.c_str(),std::ios::binary);
    if(!build.is_open())
    {
        error("Could not open " + fileName + " for write!");
        return false;
    }

    float floatBuf = 0;
    unsigned char charBuf = 0;
    unsigned int uIntBuf = landOfDranBuildMagic+1;
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    uIntBuf = serverData->staticBricks.getBrickCount();
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    uIntBuf = serverData->staticBricks.blocklandTypes->specialBrickTypes.size();
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<serverData->staticBricks.blocklandTypes->specialBrickTypes.size(); a++)
    {
        std::string name = serverData->staticBricks.blocklandTypes->specialBrickTypes[a]->fileName;
        if(name.length() < 1 || name.length() > 255)
        {
            charBuf = 0;
            build.write((char*)&charBuf,sizeof(unsigned char));

            error("Name for a blockland brick type " + name + " had a filename length of " + std::to_string(name.length()));
            continue;
        }

        charBuf = name.length();
        build.write((char*)&charBuf,sizeof(unsigned char));
        build.write(name.c_str(),name.length());
    }

    uIntBuf = serverData->staticBricks.opaqueBasicBricks.size();
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<serverData->staticBricks.opaqueBasicBricks.size(); a++)
    {
        basicBrickRenderData *brick = serverData->staticBricks.opaqueBasicBricks[a];

        charBuf = brick->color.r * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.g * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.b * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.a * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        floatBuf = brick->position.x;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.y;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.z;
        build.write((char*)&floatBuf,sizeof(float));

        charBuf = brick->dimensions.x;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->dimensions.y;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->dimensions.z;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->dimensions.w;
        build.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = getAngleIDFromRot(brick->rotation);
        build.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = brick->material;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Client doesnt know brick ownership
        int intBuf = -1;
        build.write((char*)&intBuf,sizeof(int));

        //Client doesnt know brick names
        charBuf = 0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Don't feel like implementing print saving client side for now lol
        //Client also doens't know about attached music, or lights, or collision for now
        charBuf = 1;
        build.write((char*)&charBuf,sizeof(unsigned char));
    }

    uIntBuf = serverData->staticBricks.transparentBasicBricks.size();
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<serverData->staticBricks.transparentBasicBricks.size(); a++)
    {
        basicBrickRenderData *brick = serverData->staticBricks.transparentBasicBricks[a];

        charBuf = brick->color.r * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.g * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.b * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.a * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        floatBuf = brick->position.x;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.y;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.z;
        build.write((char*)&floatBuf,sizeof(float));

        charBuf = brick->dimensions.x;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->dimensions.y;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->dimensions.z;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->dimensions.w;
        build.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = getAngleIDFromRot(brick->rotation);
        build.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = brick->material;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Client doesnt know brick ownership
        int intBuf = -1;
        build.write((char*)&intBuf,sizeof(int));

        //Client doesnt know brick names
        charBuf = 0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Don't feel like implementing print saving client side for now lol
        //Client also doens't know about attached music, or lights, or collision for now
        charBuf = 1;
        build.write((char*)&charBuf,sizeof(unsigned char));
    }

    uIntBuf = serverData->staticBricks.opaqueSpecialBricks.size();
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<serverData->staticBricks.opaqueSpecialBricks.size(); a++)
    {
        specialBrickRenderData *brick = serverData->staticBricks.opaqueSpecialBricks[a];

        charBuf = brick->color.r * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.g * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.b * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.a * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        floatBuf = brick->position.x;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.y;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.z;
        build.write((char*)&floatBuf,sizeof(float));

        uIntBuf = brick->typeID;
        build.write((char*)&uIntBuf,sizeof(unsigned int));

        charBuf = getAngleIDFromRot(brick->rotation);
        build.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = brick->material;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Client doesnt know brick ownership
        int intBuf = -1;
        build.write((char*)&intBuf,sizeof(int));

        //Client doesnt know brick names
        charBuf = 0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Don't feel like implementing print saving client side for now lol
        //Client also doens't know about attached music, or lights, or collision for now
        charBuf = 1;
        build.write((char*)&charBuf,sizeof(unsigned char));
    }

    uIntBuf = serverData->staticBricks.transparentSpecialBricks.size();
    build.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<serverData->staticBricks.transparentSpecialBricks.size(); a++)
    {
        specialBrickRenderData *brick = serverData->staticBricks.transparentSpecialBricks[a];

        charBuf = brick->color.r * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.g * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.b * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = brick->color.a * 255.0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        floatBuf = brick->position.x;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.y;
        build.write((char*)&floatBuf,sizeof(float));
        floatBuf = brick->position.z;
        build.write((char*)&floatBuf,sizeof(float));

        uIntBuf = brick->typeID;
        build.write((char*)&uIntBuf,sizeof(unsigned int));

        charBuf = getAngleIDFromRot(brick->rotation);
        build.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = brick->material;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Client doesnt know brick ownership
        int intBuf = -1;
        build.write((char*)&intBuf,sizeof(int));

        //Client doesnt know brick names
        charBuf = 0;
        build.write((char*)&charBuf,sizeof(unsigned char));

        //Don't feel like implementing print saving client side for now lol
        //Client also doens't know about attached music, or lights, or collision for now
        charBuf = 1;
        build.write((char*)&charBuf,sizeof(unsigned char));
    }

    build.close();

    return true;
}

bool saveCar(const CEGUI::EventArgs &e)
{
    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    clientStuff *clientEnvironment = (clientStuff*)saveLoadWindow->getUserData();
    serverStuff *serverData = clientEnvironment->serverData;
    if(!clientEnvironment)
        return false;

    std::string fileName = saveLoadWindow->getChild("CarFilePath")->getText().c_str();
    if(fileName.length() < 1)
    {
        error("Enter in a file name before saving a car!");
        return false;
    }
    fileName = "saves/" + fileName + ".lds";

    if(serverData->drivenCarId == -1)
    {
        error("You are not driving a car!");
        return false;
    }

    livingBrick *brickCar = 0;
    for(unsigned int a = 0; a<serverData->livingBricks.size(); a++)
    {
        if(serverData->livingBricks[a]->serverID == serverData->drivenCarId)
        {
            brickCar = serverData->livingBricks[a];
            break;
        }
    }

    if(!brickCar)
    {
        error("Could not find the car you are on!");
        return false;
    }

    std::ofstream car(fileName.c_str(),std::ios::binary);
    if(!car.is_open())
    {
        error("Could not open " + fileName + " for write!");
        return false;
    }

    int intBuf = 0;
    float floatBuf = 0;

    intBuf = landOfDranCarMagic;
    car.write((char*)&intBuf,sizeof(int));

    std::vector<light*> carLights;
    for(int a = 0; a<serverData->lights.size(); a++)
        if(serverData->lights[a]->attachedCar && serverData->lights[a]->attachedCar == brickCar)
            carLights.push_back(serverData->lights[a]);

    intBuf = carLights.size();
    car.write((char*)&intBuf,sizeof(int));

    for(int a = 0; a<carLights.size(); a++)
    {
        floatBuf = carLights[a]->color.r;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = carLights[a]->color.g;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = carLights[a]->color.b;
        car.write((char*)&floatBuf,sizeof(float));

        floatBuf = carLights[a]->attachOffset.x;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = carLights[a]->attachOffset.y;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = carLights[a]->attachOffset.z;
        car.write((char*)&floatBuf,sizeof(float));

        intBuf = carLights[a]->isSpotlight ? 1 : 0;
        car.write((char*)&intBuf,sizeof(int));
        if(carLights[a]->isSpotlight)
        {
            floatBuf = carLights[a]->direction.x;
            car.write((char*)&floatBuf,sizeof(float));
            floatBuf = carLights[a]->direction.y;
            car.write((char*)&floatBuf,sizeof(float));
            floatBuf = carLights[a]->direction.z;
            car.write((char*)&floatBuf,sizeof(float));
            floatBuf = carLights[a]->direction.w;
            car.write((char*)&floatBuf,sizeof(float));
        }
    }

    intBuf = brickCar->wheelBrickData.size();
    car.write((char*)&intBuf,sizeof(int));

    for(unsigned int a = 0; a<brickCar->wheelBrickData.size(); a++)
    {
        floatBuf = brickCar->wheelBrickData[a].wheelScale;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].carX;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].carY;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].carZ;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].breakForce;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].steerAngle;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].engineForce;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].suspensionLength;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].suspensionStiffness;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].dampingCompression;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].dampingRelaxation;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].frictionSlip;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->wheelBrickData[a].rollInfluence;
        car.write((char*)&floatBuf,sizeof(float));

        specialBrickType *wheelType = 0;
        for(unsigned int z = 0; z<brickCar->blocklandTypes->specialBrickTypes.size(); z++)
        {
            if(brickCar->blocklandTypes->specialBrickTypes[z]->serverID == brickCar->wheelBrickData[a].typeID)
            {
                wheelType = brickCar->blocklandTypes->specialBrickTypes[z];
                break;
            }
        }

        if(!wheelType)
        {
            error("Could not find special brick type " + std::to_string(brickCar->wheelBrickData[a].typeID) + " for wheel!");
            intBuf = 0;
            car.write((char*)&intBuf,sizeof(int));
        }
        else
        {
            intBuf = wheelType->fileName.length();
            car.write((char*)&intBuf,sizeof(int));
            car.write(wheelType->fileName.c_str(),wheelType->fileName.length());
        }
        /*intBuf = brickCar->opaqueSpecialBricks[a]->type->type->fileName.length();
        car.write((char*)&intBuf,sizeof(int));
        car.write(brickCar->opaqueSpecialBricks[a]->type->type->fileName.c_str(),brickCar->opaqueSpecialBricks[a]->type->type->fileName.length());*/
    }

    intBuf = brickCar->opaqueBasicBricks.size() + brickCar->transparentBasicBricks.size();
    car.write((char*)&intBuf,sizeof(int));

    for(unsigned int a = 0; a<brickCar->opaqueBasicBricks.size(); a++)
    {
        floatBuf = brickCar->opaqueBasicBricks[a]->position.x;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueBasicBricks[a]->position.y;
        car.write((char*)&floatBuf,sizeof(float));
        intBuf = brickCar->opaqueBasicBricks[a]->carPlatesUp;
        car.write((char*)&intBuf,sizeof(int));
        unsigned char charBuf = brickCar->opaqueBasicBricks[a]->yHalfPos ? 1 : 0;
        car.write((char*)&charBuf,sizeof(unsigned char));
        floatBuf = brickCar->opaqueBasicBricks[a]->position.z;
        car.write((char*)&floatBuf,sizeof(float));

        intBuf = brickCar->opaqueBasicBricks[a]->dimensions.x;
        car.write((char*)&intBuf,sizeof(int));
        intBuf = brickCar->opaqueBasicBricks[a]->dimensions.y;
        car.write((char*)&intBuf,sizeof(int));
        intBuf = brickCar->opaqueBasicBricks[a]->dimensions.z;
        car.write((char*)&intBuf,sizeof(int));

        unsigned char rot = getAngleIDFromRot(brickCar->opaqueBasicBricks[a]->rotation);
        car.write((char*)&rot,sizeof(unsigned char));

        intBuf = brickCar->opaqueBasicBricks[a]->material;
        car.write((char*)&intBuf,sizeof(int));

        floatBuf = brickCar->opaqueBasicBricks[a]->color.r;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueBasicBricks[a]->color.g;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueBasicBricks[a]->color.b;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueBasicBricks[a]->color.a;
        car.write((char*)&floatBuf,sizeof(float));
    }

    for(unsigned int a = 0; a<brickCar->transparentBasicBricks.size(); a++)
    {
        floatBuf = brickCar->transparentBasicBricks[a]->position.x;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentBasicBricks[a]->position.y;
        car.write((char*)&floatBuf,sizeof(float));
        intBuf = brickCar->transparentBasicBricks[a]->carPlatesUp;
        car.write((char*)&intBuf,sizeof(int));
        unsigned char charBuf = brickCar->transparentBasicBricks[a]->yHalfPos ? 1 : 0;
        car.write((char*)&charBuf,sizeof(unsigned char));
        floatBuf = brickCar->transparentBasicBricks[a]->position.z;
        car.write((char*)&floatBuf,sizeof(float));

        intBuf = brickCar->transparentBasicBricks[a]->dimensions.x;
        car.write((char*)&intBuf,sizeof(int));
        intBuf = brickCar->transparentBasicBricks[a]->dimensions.y;
        car.write((char*)&intBuf,sizeof(int));
        intBuf = brickCar->transparentBasicBricks[a]->dimensions.z;
        car.write((char*)&intBuf,sizeof(int));

        unsigned char rot = getAngleIDFromRot(brickCar->transparentBasicBricks[a]->rotation);
        car.write((char*)&rot,sizeof(unsigned char));

        intBuf = brickCar->transparentBasicBricks[a]->material;
        car.write((char*)&intBuf,sizeof(int));

        floatBuf = brickCar->transparentBasicBricks[a]->color.r;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentBasicBricks[a]->color.g;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentBasicBricks[a]->color.b;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentBasicBricks[a]->color.a;
        car.write((char*)&floatBuf,sizeof(float));
    }

    intBuf = brickCar->opaqueSpecialBricks.size() + brickCar->transparentSpecialBricks.size();
    car.write((char*)&intBuf,sizeof(int));

    for(unsigned int a = 0; a<brickCar->opaqueSpecialBricks.size(); a++)
    {
        floatBuf = brickCar->opaqueSpecialBricks[a]->position.x;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueSpecialBricks[a]->position.y;
        car.write((char*)&floatBuf,sizeof(float));
        intBuf = brickCar->opaqueSpecialBricks[a]->carPlatesUp;
        car.write((char*)&intBuf,sizeof(int));
        unsigned char charBuf = brickCar->opaqueSpecialBricks[a]->yHalfPos ? 1 : 0;
        car.write((char*)&charBuf,sizeof(unsigned char));
        floatBuf = brickCar->opaqueSpecialBricks[a]->position.z;
        car.write((char*)&floatBuf,sizeof(float));

        intBuf = brickCar->opaqueSpecialBricks[a]->type->type->fileName.length();
        car.write((char*)&intBuf,sizeof(int));
        car.write(brickCar->opaqueSpecialBricks[a]->type->type->fileName.c_str(),brickCar->opaqueSpecialBricks[a]->type->type->fileName.length());

        unsigned char rot = getAngleIDFromRot(brickCar->opaqueSpecialBricks[a]->rotation);
        car.write((char*)&rot,sizeof(unsigned char));

        intBuf = brickCar->opaqueSpecialBricks[a]->material;
        car.write((char*)&intBuf,sizeof(int));

        floatBuf = brickCar->opaqueSpecialBricks[a]->color.r;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueSpecialBricks[a]->color.g;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueSpecialBricks[a]->color.b;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->opaqueSpecialBricks[a]->color.a;
        car.write((char*)&floatBuf,sizeof(float));
    }

    for(unsigned int a = 0; a<brickCar->transparentSpecialBricks.size(); a++)
    {
        floatBuf = brickCar->transparentSpecialBricks[a]->position.x;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentSpecialBricks[a]->position.y;
        car.write((char*)&floatBuf,sizeof(float));
        intBuf = brickCar->transparentSpecialBricks[a]->carPlatesUp;
        car.write((char*)&intBuf,sizeof(int));
        unsigned char charBuf = brickCar->transparentSpecialBricks[a]->yHalfPos ? 1 : 0;
        car.write((char*)&charBuf,sizeof(unsigned char));
        floatBuf = brickCar->transparentSpecialBricks[a]->position.z;
        car.write((char*)&floatBuf,sizeof(float));

        intBuf = brickCar->transparentSpecialBricks[a]->type->type->fileName.length();
        car.write((char*)&intBuf,sizeof(int));
        car.write(brickCar->transparentSpecialBricks[a]->type->type->fileName.c_str(),brickCar->transparentSpecialBricks[a]->type->type->fileName.length());

        unsigned char rot = getAngleIDFromRot(brickCar->transparentSpecialBricks[a]->rotation);
        car.write((char*)&rot,sizeof(unsigned char));

        intBuf = brickCar->transparentSpecialBricks[a]->material;
        car.write((char*)&intBuf,sizeof(int));

        floatBuf = brickCar->transparentSpecialBricks[a]->color.r;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentSpecialBricks[a]->color.g;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentSpecialBricks[a]->color.b;
        car.write((char*)&floatBuf,sizeof(float));
        floatBuf = brickCar->transparentSpecialBricks[a]->color.a;
        car.write((char*)&floatBuf,sizeof(float));
    }

    car.close();

    refreshSaveLoad();

    return true;
}

bool loadCar()
{
    scope("loadCar");

    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    clientStuff *clientEnvironment = (clientStuff*)saveLoadWindow->getUserData();
    serverStuff *serverData = clientEnvironment->serverData;
    if(!clientEnvironment)
        return false;

    CEGUI::MultiColumnList *saves = (CEGUI::MultiColumnList*)saveLoadWindow->getChild("SavesList");

    if(!saves->getFirstSelectedItem())
    {
        error("No item selected!");
        return false;
    }

    std::string file = saves->getFirstSelectedItem()->getText().c_str();
    file = "saves/" + file + ".lds";

    std::ifstream carFile(file.c_str(),std::ios::binary);
    if(!carFile.is_open())
    {
        error("Could not open car file " + file + " to load!");
        return false;
    }

    if(serverData->ghostCar)
    {
        error("Ghost car already existed, deleting.");
        delete serverData->ghostCar;
        serverData->ghostCar = 0;
    }

    serverData->ghostCar = new livingBrick;
    serverData->ghostCar->allocateVertBuffer();
    serverData->ghostCar->allocatePerTexture(clientEnvironment->brickMat);
    serverData->ghostCar->allocatePerTexture(clientEnvironment->brickMatSide,true,true);
    serverData->ghostCar->allocatePerTexture(clientEnvironment->brickMatBottom,true);
    serverData->ghostCar->allocatePerTexture(clientEnvironment->brickMatRamp);
    serverData->ghostCar->addBlocklandCompatibility(serverData->staticBricks.blocklandTypes);

    int intBuf;
    float floatBuf;

    carFile.read((char*)&intBuf,sizeof(int));
    if(intBuf != landOfDranCarMagic)
    {
        error("Invalid or outdated save file version!");
        carFile.close();
        return false;
    }

    if((!carFile.is_open()) || carFile.eof())
    {
        error("(1) Reached end of car file prematurely!");
        delete serverData->ghostCar;
        serverData->ghostCar = 0;
        return false;
    }

    serverData->ghostCarLights.clear();
    carFile.read((char*)&intBuf,sizeof(int));
    int numLights = intBuf;
    if(numLights < 0 || numLights >= 100)
    {
        error("Save file had over 100 lights, is this file valid?");
        carFile.close();
        return false;
    }

    for(int a = 0; a<numLights; a++)
    {
        light *tmp = new light;
        serverData->ghostCarLights.push_back(tmp);

        carFile.read((char*)&floatBuf,sizeof(float));
        tmp->color.r = floatBuf;
        carFile.read((char*)&floatBuf,sizeof(float));
        tmp->color.g = floatBuf;
        carFile.read((char*)&floatBuf,sizeof(float));
        tmp->color.b = floatBuf;

        carFile.read((char*)&floatBuf,sizeof(float));
        tmp->attachOffset.x = floatBuf;
        carFile.read((char*)&floatBuf,sizeof(float));
        tmp->attachOffset.y = floatBuf;
        carFile.read((char*)&floatBuf,sizeof(float));
        tmp->attachOffset.z = floatBuf;

        carFile.read((char*)&intBuf,sizeof(int));
        if(intBuf == 1)
        {
            tmp->isSpotlight = true;
            carFile.read((char*)&floatBuf,sizeof(float));
            tmp->direction.x = floatBuf;
            carFile.read((char*)&floatBuf,sizeof(float));
            tmp->direction.y = floatBuf;
            carFile.read((char*)&floatBuf,sizeof(float));
            tmp->direction.z = floatBuf;
            carFile.read((char*)&floatBuf,sizeof(float));
            tmp->direction.w = floatBuf;
        }
        else if(intBuf != 0)
        {
            error("isSpotlight was neither 1 or 0, is this file valid?");
            carFile.close();
            return false;
        }
    }

    carFile.read((char*)&intBuf,sizeof(int));
    int numWheels = intBuf;

    if(numWheels > 24 || numWheels < 0)
    {
        error("Save file had over 24 wheels, is this file valid?");
        carFile.close();
        return false;
    }

    if((!carFile.is_open()) || carFile.eof())
    {
        error("(2) Reached end of car file prematurely!");
        delete serverData->ghostCar;
        serverData->ghostCar = 0;
        return false;
    }

    for(int a = 0; a<numWheels; a++)
    {
        extraWheelData wheel;

        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.wheelScale = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(3) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.carX = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(4) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.carY = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(5) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.carZ = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(6) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.breakForce = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(7) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.steerAngle = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(8) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.engineForce = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(9) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.suspensionLength = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(10) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.suspensionStiffness = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(11) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.dampingCompression = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(12) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.dampingRelaxation = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(13) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.frictionSlip = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(14) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        wheel.rollInfluence = floatBuf;
        if((!carFile.is_open()) || carFile.eof())
        {
            error("(15) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        if(intBuf > 0 && intBuf < 200)
        {
            if((!carFile.is_open()) || carFile.eof())
            {
                error("(16) Reached end of car file prematurely!");
                delete serverData->ghostCar;
                serverData->ghostCar = 0;
                return false;
            }

            bool foundBrickType = false;

            char *brickTypeName = new char[intBuf+1];
            carFile.read(brickTypeName,intBuf);

            if(!brickTypeName)
            {
                error("Invalid brick type name string read from file!");
                delete serverData->ghostCar;
                serverData->ghostCar = 0;
                return false;
            }

            brickTypeName[intBuf] = 0;
            for(unsigned int z = 0; z<serverData->staticBricks.blocklandTypes->specialBrickTypes.size(); z++)
            {
                if(serverData->staticBricks.blocklandTypes->specialBrickTypes[z]->fileName == std::string(brickTypeName))
                {
                    foundBrickType = true;
                    wheel.typeID = z;
                    break;
                }
            }
            delete brickTypeName;

            if((!carFile.is_open()) || carFile.eof())
            {
                error("(17) Reached end of car file prematurely!");
                delete serverData->ghostCar;
                serverData->ghostCar = 0;
                return false;
            }

            if(!foundBrickType)
            {
                error("Server does not have brick type " + std::string(brickTypeName) + " for wheel!");
                delete serverData->ghostCar;
                serverData->ghostCar = 0;
                return false;
            }
            else
            {
                specialBrickRenderData *brick = new specialBrickRenderData;
                brick->position.x = wheel.carX;
                brick->position.y = wheel.carY;
                brick->position.z = wheel.carZ;
                brick->color = glm::vec4(1,1,1,1);
                brick->figuredAngleID = 0;
                brick->rotation = serverData->ghostCar->rotations[0];
                serverData->ghostCar->addSpecialBrick(brick,serverData->world,wheel.typeID,brick->figuredAngleID,false);
            }
        }
        else
            error("No special brick type for wheel brick saved!");

        serverData->ghostCar->wheelBrickData.push_back(wheel);
    }

    carFile.read((char*)&intBuf,sizeof(int));
    int numBasicBricks = intBuf;

    if((!carFile.is_open()) || carFile.eof())
    {
        error("(18) Reached end of car file prematurely!");
        delete serverData->ghostCar;
        serverData->ghostCar = 0;
        return false;
    }

    if(numBasicBricks > 7000 || numBasicBricks < 0)
    {
        error("Save file for car had over 70 basic bricks, is this file valid?");
        carFile.close();
        return false;
    }

    for(int a = 0; a<numBasicBricks; a++)
    {
        basicBrickRenderData *brick = new basicBrickRenderData;

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->position.x = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(19) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->position.y = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(20) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->carPlatesUp = intBuf;
        unsigned char charBuf;
        carFile.read((char*)&charBuf,sizeof(unsigned char));
        brick->yHalfPos = charBuf == 1;

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->position.z = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(21) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->dimensions.x = intBuf;

        if(brick->dimensions.x < 0 || brick->dimensions.x > 255)
        {
            error("Brick X dimension was " + std::to_string(brick->dimensions.x) + " this is an error.");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(22) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->dimensions.y = intBuf;

        if(brick->dimensions.y < 0 || brick->dimensions.y > 255)
        {
            error("Brick Y dimension was " + std::to_string(brick->dimensions.y) + " this is an error.");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(23) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->dimensions.z = intBuf;

        if(brick->dimensions.z < 0 || brick->dimensions.z > 255)
        {
            error("Brick Z dimension was " + std::to_string(brick->dimensions.z) + " this is an error.");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(24) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        unsigned char rot = 0;
        carFile.read((char*)&rot,sizeof(unsigned char));
        if(rot > 3)
        {
            error("Rot was " + std::to_string((unsigned int)rot) + " very bad no.");
            rot = 0;
        }
        brick->figuredAngleID = rot;
        brick->rotation = serverData->ghostCar->rotations[rot];

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(25) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->material = intBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(26) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.r = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(27) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.g = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(28) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.b = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(29) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.a = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(30) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        serverData->ghostCar->addBasicBrick(brick,brick->figuredAngleID,0,serverData->world,false);
    }

    carFile.read((char*)&intBuf,sizeof(int));
    int numSpecialBricks = intBuf;

    if((!carFile.is_open()) || carFile.eof())
    {
        error("(31) Reached end of car file prematurely!");
        delete serverData->ghostCar;
        serverData->ghostCar = 0;
        return false;
    }

    if(numSpecialBricks > 70 || numSpecialBricks < 0)
    {
        error("Save file for car had over 70 special bricks, is this file valid?");
        carFile.close();
        return false;
    }

    for(int a = 0; a<numSpecialBricks; a++)
    {
        specialBrickRenderData *brick = new specialBrickRenderData;

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->position.x = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(32) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->position.y = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(33) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->carPlatesUp = intBuf;
        unsigned char charBuf;
        carFile.read((char*)&charBuf,sizeof(unsigned char));
        brick->yHalfPos = charBuf == 1;

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->position.z = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(34) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));

        if(intBuf < 1 || intBuf > 200)
        {
            error("Special brick type file name appears to be " + std::to_string(intBuf) + " characters long.");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        char *fileName = new char[intBuf+1];
        carFile.read(fileName,intBuf);
        if(!fileName)
        {
            error("Could not read special brick type file name!");
            carFile.close();
            return false;
        }
        fileName[intBuf] = 0;
        std::string name = fileName;
        delete fileName;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(35) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        int typeIdx = -1;
        for(unsigned int b = 0; b<serverData->ghostCar->blocklandTypes->specialBrickTypes.size(); b++)
        {
            specialBrickType *tmp = serverData->ghostCar->blocklandTypes->specialBrickTypes[b];
            if(tmp->fileName == name)
            {
                typeIdx = b;
                break;
            }
        }

        unsigned char rot = 0;
        carFile.read((char*)&rot,sizeof(unsigned char));
        if(rot > 3)
        {
            error("Rot was " + std::to_string((unsigned int)rot) + " very bad no.");
            rot = 0;
        }
        brick->figuredAngleID = rot;
        brick->rotation = serverData->ghostCar->rotations[rot];

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(36) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&intBuf,sizeof(int));
        brick->material = intBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(37) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.r = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(38) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.g = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(39) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.b = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(40) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }
        carFile.read((char*)&floatBuf,sizeof(float));
        brick->color.a = floatBuf;

        if((!carFile.is_open()) || carFile.eof())
        {
            error("(41) Reached end of car file prematurely!");
            delete serverData->ghostCar;
            serverData->ghostCar = 0;
            return false;
        }

        if(typeIdx == -1)
        {
            error("Could not find special brick type " + name);
            continue;
        }

        serverData->ghostCar->addSpecialBrick(brick,serverData->world,typeIdx,brick->figuredAngleID,false);
    }

    serverData->ghostCar->recompileEverything();
    serverData->ghostCar->compiled = true;

    serverData->ghostCar->carTransform.addTransform(0,glm::vec3(0,0,0),glm::quat(1,0,0,0));

    carFile.close();

    return true;
}

bool loadCarAsCar(const CEGUI::EventArgs &e)
{
    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    clientStuff *clientEnvironment = (clientStuff*)saveLoadWindow->getUserData();
    clientEnvironment->serverData->loadCarAsCar = true;
    return loadCar();
}

bool loadCarAsBricks(const CEGUI::EventArgs &e)
{
    CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::Window *saveLoadWindow = root->getChild("SaveLoad");
    clientStuff *clientEnvironment = (clientStuff*)saveLoadWindow->getUserData();
    clientEnvironment->serverData->loadCarAsCar = false;
    return loadCar();
}

namespace syj
{
    void sendBrickCarToServer(clientStuff *clientEnvironment,livingBrick *car,glm::vec3 origin)
    {
        serverStuff *serverData = clientEnvironment->serverData;

        std::vector<basicBrickRenderData*> allBasicBricks;
        for(int a = 0; a<car->opaqueBasicBricks.size(); a++)
            allBasicBricks.push_back(car->opaqueBasicBricks[a]);
        for(int a = 0; a<car->transparentBasicBricks.size(); a++)
            allBasicBricks.push_back(car->transparentBasicBricks[a]);

        std::vector<specialBrickRenderData*> allSpecialBricks;
        for(int a = 0; a<car->opaqueSpecialBricks.size(); a++)
            allSpecialBricks.push_back(car->opaqueSpecialBricks[a]);
        for(int a = 0; a<car->transparentSpecialBricks.size(); a++)
            allSpecialBricks.push_back(car->transparentSpecialBricks[a]);

        //A car needs 5 things now
        //Amounts and settings packet (lights, wheels, bricks, special bricks)
        //Lights packet
        //Wheels packet
        //One or more basic brick packets
        //One or more special brick packets

        packet amountPacket;
        amountPacket.writeUInt(11,4); //11 = carPlantRequest
        amountPacket.writeUInt(0,3);  //0 = amounts packet
        amountPacket.writeUInt(allBasicBricks.size(),10);
        amountPacket.writeUInt(allSpecialBricks.size(),10);
        amountPacket.writeUInt(serverData->ghostCarLights.size(),6);
        amountPacket.writeUInt(car->wheelBrickData.size(),6);
        amountPacket.writeBit(serverData->loadCarAsCar); //If false, just load the bricks from it
        amountPacket.writeFloat(origin.x);  //Where we want to load the car
        amountPacket.writeFloat(origin.y);
        amountPacket.writeFloat(origin.z);
        serverData->connection->send(&amountPacket,true);

        packet lightPacket;
        lightPacket.writeUInt(11,4); //11 = carPlantRequest
        lightPacket.writeUInt(1,3);  //1  = lights packet
        lightPacket.writeUInt(serverData->ghostCarLights.size(),8);
        for(int a = 0; a<serverData->ghostCarLights.size(); a++)
        {
            lightPacket.writeFloat(serverData->ghostCarLights[a]->color.r);
            lightPacket.writeFloat(serverData->ghostCarLights[a]->color.g);
            lightPacket.writeFloat(serverData->ghostCarLights[a]->color.b);

            lightPacket.writeFloat(serverData->ghostCarLights[a]->attachOffset.x);
            lightPacket.writeFloat(serverData->ghostCarLights[a]->attachOffset.y);
            lightPacket.writeFloat(serverData->ghostCarLights[a]->attachOffset.z);

            lightPacket.writeBit(serverData->ghostCarLights[a]->isSpotlight);
            if(serverData->ghostCarLights[a]->isSpotlight)
            {
                lightPacket.writeFloat(serverData->ghostCarLights[a]->direction.x);
                lightPacket.writeFloat(serverData->ghostCarLights[a]->direction.y);
                lightPacket.writeFloat(serverData->ghostCarLights[a]->direction.z);
                lightPacket.writeFloat(serverData->ghostCarLights[a]->direction.w);
            }
        }
        serverData->connection->send(&lightPacket,true);

        packet wheelPacket;
        wheelPacket.writeUInt(11,4); //11 = carPlantRequest
        wheelPacket.writeUInt(2,3);  //2  = wheels packet
        wheelPacket.writeUInt(car->wheelBrickData.size(),5);
        for(unsigned int a = 0; a<car->wheelBrickData.size(); a++)
        {
            wheelPacket.writeFloat(car->wheelBrickData[a].wheelScale);
            wheelPacket.writeFloat(car->wheelBrickData[a].carX);
            wheelPacket.writeFloat(car->wheelBrickData[a].carY);
            wheelPacket.writeFloat(car->wheelBrickData[a].carZ);
            wheelPacket.writeFloat(car->wheelBrickData[a].breakForce);
            wheelPacket.writeFloat(car->wheelBrickData[a].steerAngle);
            wheelPacket.writeFloat(car->wheelBrickData[a].engineForce);
            wheelPacket.writeFloat(car->wheelBrickData[a].suspensionLength);
            wheelPacket.writeFloat(car->wheelBrickData[a].suspensionStiffness);
            wheelPacket.writeFloat(car->wheelBrickData[a].dampingCompression);
            wheelPacket.writeFloat(car->wheelBrickData[a].dampingRelaxation);
            wheelPacket.writeFloat(car->wheelBrickData[a].frictionSlip);
            wheelPacket.writeFloat(car->wheelBrickData[a].rollInfluence);
            int typeID = car->wheelBrickData[a].typeID;
            if(typeID < 0 || typeID >= serverData->staticBricks.blocklandTypes->specialBrickTypes.size())
                wheelPacket.writeUInt(0,10);
            else
            {
                typeID = serverData->staticBricks.blocklandTypes->specialBrickTypes[typeID]->serverID;
                wheelPacket.writeUInt(typeID,10);
            }
        }
        serverData->connection->send(&wheelPacket,true);

        int bricksLeftToSend = allBasicBricks.size();
        int bricksSent = 0;

        while(bricksLeftToSend > 0)
        {
            int bricksSentThisPacket = bricksLeftToSend > 50 ? 50 : bricksLeftToSend;

            packet basicBrickPacket;
            basicBrickPacket.writeUInt(11,4); //11 = carPlantRequest
            basicBrickPacket.writeUInt(3,3);  //3  = basic bricks
            basicBrickPacket.writeUInt(bricksSentThisPacket,6);

            for(int a = bricksSent; a < bricksSent + bricksSentThisPacket; a++)
            {
                basicBrickRenderData *brick = allBasicBricks[a];

                basicBrickPacket.writeFloat(brick->position.x);
                basicBrickPacket.writeFloat(brick->position.y);
                basicBrickPacket.writeUInt(brick->carPlatesUp,10);
                basicBrickPacket.writeBit(brick->yHalfPos);
                basicBrickPacket.writeFloat(brick->position.z);

                basicBrickPacket.writeUInt(brick->dimensions.x,8);
                basicBrickPacket.writeUInt(brick->dimensions.y,8);
                basicBrickPacket.writeUInt(brick->dimensions.z,8);

                basicBrickPacket.writeUInt(brick->figuredAngleID,2);

                int tmpMat = brick->material;
                int shapeFx = 0;
                if(tmpMat >= bob)
                {
                    tmpMat -= bob;
                    shapeFx += 2;
                }
                else if(tmpMat >= undulo)
                {
                    tmpMat -= undulo;
                    shapeFx += 1;
                }
                basicBrickPacket.writeUInt(shapeFx,4);
                basicBrickPacket.writeUInt(tmpMat,4);

                basicBrickPacket.writeUInt(brick->color.r*255,8);
                basicBrickPacket.writeUInt(brick->color.g*255,8);
                basicBrickPacket.writeUInt(brick->color.b*255,8);
                basicBrickPacket.writeUInt(brick->color.a*255,8);
            }

            bricksLeftToSend -= bricksSentThisPacket;
            bricksSent += bricksSentThisPacket;

            serverData->connection->send(&basicBrickPacket,true);
        }

        bricksLeftToSend = allSpecialBricks.size();
        bricksSent = 0;

        while(bricksLeftToSend > 0)
        {
            int bricksSentThisPacket = bricksLeftToSend > 50 ? 50 : bricksLeftToSend;

            packet specialBrickPacket;
            specialBrickPacket.writeUInt(11,4); //11 = carPlantRequest
            specialBrickPacket.writeUInt(4,3);  //4  = special bricks
            specialBrickPacket.writeUInt(bricksSentThisPacket,6);

            for(int a = bricksSent; a < bricksSent + bricksSentThisPacket; a++)
            {
                specialBrickRenderData *brick = allSpecialBricks[a];

                specialBrickPacket.writeUInt(brick->type->type->serverID,10);

                specialBrickPacket.writeFloat(brick->position.x);
                specialBrickPacket.writeFloat(brick->position.y);
                specialBrickPacket.writeUInt(brick->carPlatesUp,10);
                specialBrickPacket.writeBit(brick->yHalfPos);
                specialBrickPacket.writeFloat(brick->position.z);

                specialBrickPacket.writeUInt(brick->figuredAngleID,2);

                int tmpMat = brick->material;
                int shapeFx = 0;
                if(tmpMat >= bob)
                {
                    tmpMat -= bob;
                    shapeFx += 2;
                }
                else if(tmpMat >= undulo)
                {
                    tmpMat -= undulo;
                    shapeFx += 1;
                }
                specialBrickPacket.writeUInt(shapeFx,4);
                specialBrickPacket.writeUInt(tmpMat,4);

                specialBrickPacket.writeUInt(brick->color.r*255,8);
                specialBrickPacket.writeUInt(brick->color.g*255,8);
                specialBrickPacket.writeUInt(brick->color.b*255,8);
                specialBrickPacket.writeUInt(brick->color.a*255,8);
            }

            bricksLeftToSend -= bricksSentThisPacket;
            bricksSent += bricksSentThisPacket;

            serverData->connection->send(&specialBrickPacket,true);
        }
    }

    CEGUI::Window *loadSaveLoadWindow(clientStuff *clientEnvironment)
    {
        CEGUI::Window *saveLoadWindow = addGUIFromFile("saveLoad.layout");
        saveLoadWindow->setUserData(clientEnvironment);
        saveLoadWindow->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeSaveLoadWindow));

        saveLoadWindow->getChild("Refresh")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&refreshButton));
        saveLoadWindow->getChild("SaveCar")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&saveCar));
        saveLoadWindow->getChild("SaveBuild")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&saveBuild));
        saveLoadWindow->getChild("LoadVehicle")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&loadCarAsCar));
        saveLoadWindow->getChild("LoadBricks")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&loadCarAsBricks));
        CEGUI::MultiColumnList *saves = (CEGUI::MultiColumnList*)saveLoadWindow->getChild("SavesList");
        saves->addColumn("Save Name",0,CEGUI::UDim(0.5,0));
        saves->addColumn("Date Modified",1,CEGUI::UDim(0.4,0.4));

        refreshSaveLoad();
        return saveLoadWindow;
    }
}










