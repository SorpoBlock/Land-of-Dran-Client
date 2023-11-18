#ifndef NEWBRICKTYPE_H_INCLUDED
#define NEWBRICKTYPE_H_INCLUDED

#include "code/graphics/material.h"
#include "code/gui/palette.h"
#include "code/graphics/blocklandBricks.h"
#include "code/external/octree/octree.h"
#include "code/graphics/printLoader.h"

namespace syj
{
    std::string getBrickMatName(brickMaterial mat);
    int getAngleIDFromRot(glm::quat in);

    btRigidBody *addBrickToWorld(glm::vec3 pos,int rotation,int w,int h,int l,btCollisionShape *shape,btDynamicsWorld *world,brickMaterial mat,bool isModTer);
    //btRigidBody *addBrickToWorld(brickRenderData *theBrick,btDynamicsWorld *world);

    //One *instance* of a brick
    //Basic bricks can have dynamically changing dimensions and specified print faces:
    struct basicBrickRenderData : brickRenderData
    {
        glm::uvec4 dimensions;

        int bufferOffset = -1;

        basicBrickRenderData(){}

        basicBrickRenderData(const basicBrickRenderData &toCopy)
        {
            position = toCopy.position;
            material = toCopy.material;
            rotation = toCopy.rotation;
            color = toCopy.color;
            dimensions = toCopy.dimensions;
            printID = toCopy.printID;
            //printFaces = toCopy.printFaces;
            printMask = toCopy.printMask;
            hasPrint = toCopy.hasPrint;
        }

        int printBufferOffset = -1;
        int printID = 0;
        bool hasPrint = false;
        //Only to be used by updateBasicBrick
        int oldPrintID = -1;
        unsigned char printMask = 0;
        //printAlias *printFaces = 0;
    };

    struct newBrickRenderer
    {
        public:
        //Per-brick data for most bricks, but not print bricks or bricks with special models:
        std::vector<int> opaqueBasicAlloc;
        std::vector<basicBrickRenderData*> opaqueBasicBricks;
        std::vector<int> transparentBasicAlloc;
        std::vector<basicBrickRenderData*> transparentBasicBricks;

        public:

        brickRenderData *getBrick(int serverID,bool &isSpecial)
        {
            for(unsigned int a = 0; a<opaqueBasicBricks.size(); a++)
            {
                if(opaqueBasicBricks[a]->serverID == serverID)
                {
                    isSpecial = false;
                    return opaqueBasicBricks[a];
                }
            }
            for(unsigned int a = 0; a<transparentBasicBricks.size(); a++)
            {
                if(transparentBasicBricks[a]->serverID == serverID)
                {
                    isSpecial = false;
                    return transparentBasicBricks[a];
                }
            }
            for(unsigned int a = 0; a<opaqueSpecialBricks.size(); a++)
            {
                if(opaqueSpecialBricks[a]->serverID == serverID)
                {
                    isSpecial = true;
                    return opaqueSpecialBricks[a];
                }
            }
            for(unsigned int a = 0; a<transparentSpecialBricks.size(); a++)
            {
                if(transparentSpecialBricks[a]->serverID == serverID)
                {
                    isSpecial = true;
                    return transparentSpecialBricks[a];
                }
            }
            return 0;
        }

        int getBrickCount()
        {
            return opaqueBasicBricks.size() + transparentBasicBricks.size() + opaqueSpecialBricks.size() + transparentSpecialBricks.size();
        }

        Octree<basicBrickType*> *collisionShapes = 0;

        glm::quat rotations[4] = {

        glm::quat(0,0,1,0),
        glm::quat(4.7122,0,1,0),
        glm::quat(3.1415,0,1,0),
        glm::quat(1.5708,0,1,0)};

        blocklandCompatibility *blocklandTypes = 0;
        void addBlocklandCompatibility(blocklandCompatibility *source);

        //For Blockland special bricks:
        std::vector<specialBrickTypeInstanceHolder*> specialBrickTypes;
        std::vector<specialBrickRenderData*> opaqueSpecialBricks;

        std::vector<specialBrickTypeInstanceHolder*> transparentSpecialBrickTypes;
        std::vector<specialBrickRenderData*> transparentSpecialBricks;

        int currentOpaqueBasicBufferOffset = 0;
        int currentTransparentBasicBufferOffset = 0;

        //For all non-special faces, period:
        GLuint vertBuffer;

        //For basic and print bricks only:
        //For all faces of a given texture:
        std::vector<material*>  theTextures;
        std::vector<GLuint>     perTextureVAO;
        std::vector<GLuint>     perTexturePositionMatBuffer;
        std::vector<GLuint>     perTextureRotationBuffer;
        std::vector<GLuint>     perTexturePaintColorBuffer;
        std::vector<GLuint>     perTextureDimensionBuffer;
        std::vector<GLuint>     perTextureDirectionBuffer;
        std::vector<int>        perTextureInstances;

        std::vector<GLuint>     transparentPerTextureVAO;
        std::vector<GLuint>     transparentPerTexturePositionMatBuffer;
        std::vector<GLuint>     transparentPerTextureRotationBuffer;
        std::vector<GLuint>     transparentPerTexturePaintColorBuffer;
        std::vector<GLuint>     transparentPerTextureDimensionBuffer;
        std::vector<GLuint>     transparentPerTextureDirectionBuffer;
        std::vector<int>        transparentPerTextureInstances;

        //Be sure to call loadBlocklandCompatibility first!
        void loadBlocklandSave(std::string filePath,btDynamicsWorld *world,printLoader &prints,paletteGUI *pallete = 0);

        //Done once on start-up:
        void allocateVertBuffer();

        //Done once on start-up for each possible texture.
        //Once for the defaultBrickTextures, and once per print:
        int allocatePerTexture(material* mat,bool sideOrBottomTexture = false,bool sideTexture = false,bool printTexture = false);

        //Totally reset all VBOs and recompile from a list of all loaded bricks:
        void recompileTransparentSpecialBricks();
        void recompileOpaqueSpecialBricks();
        void recompileOpaqueBasicBricks(int extraAlloc = 1000,int extraAllocPrint = 50);
        void recompileOpaquePrints(int extraAllocPrint);
        void recompileTransparentPrints(int extraAllocPrint);
        void recompileTransparentBasicBricks(int extraAlloc = 1000,int extraAllocPrint = 50);
        void recompileEverything();

        //Render all bricks completely with all materials
        void renderEverything(uniformsHolder *unis,bool skipMats = false,material *specialPrintBrickMaterial=0,float deltaT = 0);
        void renderTransparent(uniformsHolder *unis,bool skipMats = false,float deltaT = 0);

        void updateBasicBrick(basicBrickRenderData *theBrick,btDynamicsWorld *world,bool changedAlpha=false);
        void updateSpecialBrick(specialBrickRenderData *theBrick,btDynamicsWorld *world,int rotationID);

        void addSpecialBrick(specialBrickRenderData *theBrick,btDynamicsWorld *world,int typeIdx,int rotationID,bool doNotCompile = false);
        void addBasicBrick(basicBrickRenderData *theBrick,int rotationID,btCollisionShape *shape=0,btDynamicsWorld *world=0,bool doNotCompile = false);

        void removeBasicBrick(basicBrickRenderData *theBrick,btDynamicsWorld *world);
        bool removeSpecialBrick(specialBrickRenderData *theBrick,btDynamicsWorld *world,bool noDelete = false);
    };
}

#endif // NEWBRICKTYPE_H_INCLUDED
