#ifndef ANIMATION_H_INCLUDED
#define ANIMATION_H_INCLUDED

#include "model.h"
#include "code/networking/interpolator.h"

namespace syj
{

    class animatedModel;

    struct bone
    {
        //This is loaded not in the bone constructor, but in meshAnimationData
        glm::mat4 offset = glm::mat4(1.0);
        node *target = 0;
        std::string name;
        unsigned int id;
        std::vector<glm::vec3> posKeys;
        std::vector<float> posTimes;
        std::vector<glm::vec3> scaleKeys;
        std::vector<float> scaleTimes;
        std::vector<glm::quat> rotKeys;
        std::vector<float> rotTimes;
        glm::mat4 getTransform(float frame);
        bone(animatedModel *parent,aiNodeAnim *source);
        bone(){};
    };

    class meshAnimationData
    {
        public:

        std::vector<glm::ivec4> boneIDs;
        std::vector<glm::vec4> weights;
        //Contains boneId = 0, vertexWeight = 1, buffer data for 2 layout positions
        GLuint buffer[2];

        meshAnimationData(mesh *baseMesh,aiMesh *source,animatedModel *parent);
    };

    struct animation
    {
        int startFrame;
        int endFrame;
        float speedDefault;
        int serverID = -1;
        std::string name;
    };

    struct nodeTransform
    {
        std::string nodeName = "";
        node *target = 0;
        glm::mat4 transform = glm::mat4(1.0);
    };

    class animatedModel : public model
    {
        public:

        std::vector<animation> animations;
        int defaultFrame = 0;
        glm::vec3 eyeOffset = glm::vec3(0,0,0);
        int serverID = 0;

        float duration = 1.0;
        float ticksPerSecond = 1.0;
        std::vector<bone*> bones;
        std::vector<meshAnimationData*> weightData;
        bone *getBone(std::string name);
        bone *getBone(node *target);
        void loadAnimations(const aiScene *scene);
        animatedModel(std::string filename);
        ~animatedModel();
        void renderAnimated(uniformsHolder *unis,float frame,glm::mat4 startTransform = glm::mat4(1.0),node *toRender = 0,std::vector<glm::vec3> *nodeColors = 0,texture *decal = 0,std::vector<nodeTransform> *additionalTransforms = 0);
        void render(uniformsHolder *unis,float frame,glm::mat4 startTransform = glm::mat4(1.0),std::vector<glm::vec3> *nodeColors = 0,texture *decal = 0,std::vector<nodeTransform> *additionalTransforms = 0);
        glm::mat4 *finalBoneMatrices = 0;
        void updateBoneTransforms(node *startNode,glm::mat4 transform,float frame,std::vector<nodeTransform> *additionalTransforms = 0);
    };

    struct animatedInstance
    {
        texture *decal = 0;
        std::vector<glm::vec3> nodeColors;
        std::vector<nodeTransform> additionalTransforms;
        void addExtraTransform(std::string node)
        {
            scope("addExtraTransform");
            for(int a= 0; a<type->nodes.size(); a++)
            {
                if(type->nodes[a]->name == node)
                {
                    nodeTransform tmp;
                    tmp.nodeName = node;
                    tmp.target = type->nodes[a];
                    additionalTransforms.push_back(tmp);
                    return;
                }
            }
            error("Could not find node by name " + node);
        }
        void setExtraTransform(std::string node,glm::mat4 matrix)
        {
            for(int a = 0; a<additionalTransforms.size(); a++)
            {
                if(additionalTransforms[a].nodeName == node)
                {
                    additionalTransforms[a].transform = matrix;
                    return;
                }
            }
            error("Could not find node by name " + node);
        }

        animatedModel *type = 0;
        /*glm::vec3 position;
        glm::quat rotation;*/
        std::string shapeName = "";
        glm::vec3 shapeNameColor = glm::vec3(1,1,1);
        interpolator modelInterpolator;

        glm::vec3 scale = glm::vec3(1,1,1);
        int playingAnimation = -1;
        float animationSpeed;
        float animationProgress;

        void play(std::string name,bool reset = false,float speed = 1.0);
        void play(int animID,bool reset = false,float speed = 1.0);
        void stop();
        void renderInst(uniformsHolder *unis,float deltaMS = 0,bool stopInterpolation = false,bool skipMats = false);
        void advance(float deltaMS);
    };
}

#endif // ANIMATION_H_INCLUDED
