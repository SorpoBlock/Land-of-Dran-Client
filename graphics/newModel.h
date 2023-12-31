#ifndef NEWMODEL_H_INCLUDED
#define NEWMODEL_H_INCLUDED

#include "code/physics/bulletIncludes.h"
#include "code/networking/interpolator.h"
#include "code/utility/preference.h"
#include "code/graphics/texture.h"
#include "code/graphics/uniformsBasic.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <map>
#include <glm/gtx/transform.hpp>
#include "code/graphics/material.h"

#define meshFlag_usePickingColors 256
#define meshFlag_skipCameraMatrix 512

namespace syj
{
    void printMatScale(glm::mat4 in,std::string name);

    struct newNode;
    struct newModel;

    struct newAnimation
    {
        int startFrame;
        int endFrame;
        float speedDefault = 0.04;
        int serverID = -1;
        std::string name;
    };

    struct newDynamic
    {
        //Only used for players at the moment, as result of packetType_clientPhysicsData
        int flingPreventionStartTime = 0;
        btRigidBody *body = 0;
        btCompoundShape *shape = 0;
        btBoxShape *boxShape = 0;
        void createBoxBody(btDynamicsWorld *_world,btVector3 extents,btVector3 offset,btVector3 initPos);
        btVector3 finalOffset;
        btVector3 finalHalfExtents;
        btMotionState *defaultMotionState = 0;
        float mass = 1.0;
        btVector3 defaultInertia;
        btDynamicsWorld *world = 0;
        unsigned int lastPlayerControl = 0;
        bool walking = false;
        float crouchProgress = 0.0;
        bool control(float yaw,bool forward,bool backward,bool left,bool right,bool jump,bool crouch,bool isJetting,bool allowTurning=true,bool relativeSpeed = false);

        //Networking stuff:
        int serverID = -1;

        //Bare minimum graphics stuff:
        newModel *type = 0;
        interpolator modelInterpolator;
        glm::vec3 scale;

        //Alternative to just using the modelInterpolator:
        glm::mat4 globalTransform = glm::mat4(1.0);
        bool useGlobalTransform = false;

        //Instancing related stuff:
        bool meshColorChanged = true;   //needs to be true at least once to allocate buffer
        std::vector<glm::vec3> meshColors;
        std::vector<glm::mat4> meshTransforms;
        std::vector<int> meshFlags;
        int bufferOffset = -1;

        //For head tilt and other specific hand-coded rotations:
        std::vector<glm::mat4> meshFixedRotation;
        std::vector<bool> meshFixedRotationUsed;

        void setFixedRotation(std::string name,glm::mat4 rotation);

        //Optional animation stuff:
        /*int playingAnimation = -1; //-1 means no animation
        float animationSpeed = 1;
        float animationProgress = 0;
        float frame = 0;*/
        std::vector<float> animationProgress;
        std::vector<float> animationFrame;
        std::vector<float> animationSpeed;
        std::vector<bool>  animationOn;
        std::vector<bool>  animationLooping;

        //Other option graphics stuff:
        std::string shapeName = "";
        glm::vec3 shapeNameColor = glm::vec3(1,1,1);
        texture *decal = 0;

        //Hides entire model and all meshes:
        bool hidden = false;

        void setNodeColor(std::string nodeName,glm::vec3 color);
        void setNodeFlag(std::string nodeName,int flag);
        void setAllFlag(int flag);
        void bufferSubData();
        void play(std::string name,bool reset = false,float speed = 1.0,bool loop = false);
        void play(int animID,bool reset = false,float speed = 1.0,bool loop = false);
        void stop(int animID);
        void stop(std::string name);
        void stopAll();
        void calculateMeshTransforms(float deltaMS,newNode *node = 0,glm::mat4 transform = glm::mat4(1.0));

        newDynamic(newModel *_type,glm::vec3 baseScale = glm::vec3(0.02,0.02,0.02));
        ~newDynamic();
        newDynamic(const newDynamic&) = delete; //Disable copy constructor
    };

    enum instancedLayout
    {
        newPositions = 0,
        newUVs = 1,
        newNormals = 2,
        newTangents = 3,
        newBitangents = 4,
        newIndex = 5,
        perMeshFlags = 6,
        perMeshColor = 7,
        perMeshTransformA = 8,  //Each of these contains one row of a 4x4 glm::mat4
        perMeshTransformB = 9,  //But only A actually pointers to a buffer
        perMeshTransformC = 10,  //There's only 9 buffers allocated so be careful when indexing
        perMeshTransformD = 11
    };

    struct newMesh
    {
        //Lets us know how we can down-cast it
        virtual bool isInstanced() = 0;

        //Essential to rendering it:
        GLuint vao = 0;

        //Used to identify the mesh:
        std::string name = "";
        int meshIndex = 0;  //Used for updating instance transforms, and also for the picking render pass
        int numVerts = 0; //Actually the number of indicies for verts, i.e. after vertex duplication

        //Might not actually be raw
        glm::vec3 rawMinExtents,rawMaxExtents;
        bool hidden = false;
        bool isCollisionMesh = false;

        //Essential to rendering it:
        GLuint buffers[9];
        //instanced uses all 9, noninstanced uses 6

        //Called upon loading the given model file at start-up:
        void fillBuffer(instancedLayout dest,void *data,int size,int elements,bool elementBuffer = false);

        virtual void render(uniformsHolder *graphics) = 0;
        virtual void renderWithoutMaterial() = 0;
    };

    struct instancedMesh : newMesh
    {
        bool isInstanced(){return true;}

        //non-instanced meshes can change material per instance:
        material *materialToUse = 0;

        //Where to render it and how often:
        std::vector<newDynamic*> instances;

        //Should be called after an update to instances:
        void recompile();

        //Called each frame:
        void render(uniformsHolder *graphics);
        void renderWithoutMaterial();

        instancedMesh(aiMesh *src);
        ~instancedMesh();

        instancedMesh(const instancedMesh&) = delete; //Disable copy constructor
    };

    struct nonInstancedMesh : newMesh
    {
        instancedMesh *counterPart = 0;
        newModel *parent = 0;
        bool isInstanced(){return false;}
        std::vector<newDynamic*> instances;
        void render(uniformsHolder *graphics);
        void renderWithoutMaterial();

        nonInstancedMesh(aiMesh *src);
        ~nonInstancedMesh();

        nonInstancedMesh(const nonInstancedMesh&) = delete; //Disable copy constructor
    };

    struct newNode
    {
        //Can have instancedMeshes (for most things)
        std::vector<instancedMesh*> instancedMeshes;

        //Or non-instanced meshes for things like player faces that may need different textures...
        //someday

        newNode *parent = 0;
        std::string name = "";
        std::vector<newNode*> children;

        glm::mat4 defaultTransform = glm::mat4(1.0);
        glm::vec3 rotationPivot = glm::vec3(0,0,0);

        std::vector<glm::vec3> posFrames;
        std::vector<float> posTimes;

        std::vector<glm::quat> rotFrames;
        std::vector<float> rotTimes;

        void foldNodeInto(aiNode *source,newModel *parent);
        newNode(aiNode *source,newModel *parent);
        ~newNode();

        newNode(const newNode&) = delete; //Disable copy constructor
    };

    struct newModel
    {
        //All dependency on this needs to be removed as a priority:
        //void *oldModelType = 0;

        glm::vec3 networkScale = glm::vec3(1,1,1);

        bool rotationPivotFix = false;
        bool oneMaterial = false;

        //Network stuff:
        int serverID = -1;

        //Rendering data:
        std::vector<newNode*> allNodes;
        std::vector<instancedMesh*> instancedMeshes;
        std::vector<nonInstancedMesh*> nonInstancedMeshes;
        std::vector<material*> allMaterials;
        newNode *rootNode = 0;

        //Eye position for player models:
        bool hasCamera = false;
        glm::vec3 eyeOffset;        //TODO: Only use one of these

        //Used for icon size adjustment and I guess collision?
        glm::vec3 totalColMin,totalColMax;

        //Keeps track of where to get meshTransforms and per mesh colors from in big array, excludes non instanced meshes
        int instancedMeshCount = 0;

        //Animations:
        std::vector<newAnimation> animations;
        int defaultFrame = 0;

        newModel(std::string textFilePath);
        ~newModel();

        void calculateTotalCollisionExtents(newNode *current,glm::mat4 transform);
        void renderInstanced(uniformsHolder *graphics);
        void renderInstancedWithoutMaterials();
        void renderNonInstanced(uniformsHolder *graphics);
        void compileAll();

        newModel(const newModel&) = delete; //Disable copy constructor
    };
}

#endif // NEWMODEL_H_INCLUDED
