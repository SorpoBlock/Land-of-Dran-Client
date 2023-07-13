#ifndef BLOCKLANDBRICKS_H_INCLUDED
#define BLOCKLANDBRICKS_H_INCLUDED

#include "code/graphics/blocklandBrickType.h"
#include "code/gui/brickSelector.h"
#include <filesystem>

using namespace std::filesystem;

namespace syj
{
    struct printAlias
    {
        int width = -1,length = -1,height = -1;
        std::string dbName = "";
        std::string uiName = "";
        int faceMask = 0;
        void addFace(faceDirection toAdd)
        {
            faceMask |= 1 << toAdd;
        }
    };

    //Not used for rendering bricks or loading builds, but for keeping track of icons from Blockland and minimizing btCollisionShapes:
    struct basicBrickType
    {
        int buttonSelectorChildIndex = -1;
        btCollisionShape *shape = 0;
        int width = 0;
        int height = 0;
        int length = 0;

        void init(int w,int h,int l,int selectorIndex = -1)
        {
            if(shape)
                return;

            shape = new btBoxShape(btVector3(((float)w)/2.0,((float)h)/(2.0*2.5),((float)l)/2.0));
            width = w;
            height = h;
            length = l;
            buttonSelectorChildIndex = selectorIndex;
        }
    };

    struct specialBrickTypeInstanceHolder;
    //Special bricks have their own custom model and per-vertex coloring:
    struct specialBrickRenderData : brickRenderData
    {
        //This one should only be modified in the add or update methods
        specialBrickTypeInstanceHolder *oldType = 0;

        //Literally just because we don't know if it's a transparent type or not...
        //So far only used in the update method
        int typeID = -1;
        specialBrickTypeInstanceHolder *type = 0;
        int brickOffsets = -1;

        int printID = -1;

        specialBrickRenderData(){}
        specialBrickRenderData(const specialBrickRenderData &source)
        {
            position = source.position;
            rotation = source.rotation;
            color = source.color;
            material = source.material;
            type = source.type;
            oldType = source.oldType;
            typeID = source.typeID;
            printID = source.printID;
        }
    };

    struct specialBrickTypeInstanceHolder
    {
        specialBrickType *type = 0;

        faceDirection whichFace;
        GLuint buffers[9];
        GLuint vao;
        specialBrickTypeInstanceHolder(specialBrickType *based);
        void update(specialBrickRenderData *theBrick);

        std::vector<specialBrickRenderData*> instances;
        int numCompiledInstances = 0;
        void recompileInstances();
    };

    struct blocklandCompatibility
    {
        //For basic bricks from Blockland:
        std::vector<std::string> blocklandUINames;
        std::vector<std::string> blocklandDatablockNames;
        std::vector<basicBrickType> basicTypes;
        std::vector<printAlias*> printTypes;
        /*std::vector<glm::vec3>   blocklandDimensions;
        std::vector<btCollisionShape*> collisionShapes;*/

        //For Blockland special bricks:
        std::vector<specialBrickType*> specialBrickTypes;

        //Associate blockland ui names with brick dimensions
        blocklandCompatibility(std::string aliasFile,std::string searchPath,CEGUI::Window *brickSelector = 0,bool onlyBasic = false);

        void addSpecialBrickType(std::string filePath,CEGUI::Window *brickSelector,int serverID);
    };
}

#endif // BLOCKLANDBRICKS_H_INCLUDED
