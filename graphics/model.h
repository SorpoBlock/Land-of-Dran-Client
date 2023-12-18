#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

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

namespace syj
{
    void CopyaiMat(aiMatrix4x4 from, glm::mat4 &to);
    extern std::map<std::string,int> aiProcessMap;

    /*class material
    {
        private:
            texture textures[3];
            bool texturesUsed[6];
        public:
            void render(uniformsHolder *renderer);
            material(aiMaterial *tex,preferenceFile *settings,std::string folder);
            ~material();
    };*/

    class mesh
    {
        public:
        bool detectedCollisionMesh = false;
        glm::vec3 detectedColMax;
        glm::vec3 detectedColMin;

        GLuint vao;
        GLuint buffers[9];
        bool bufferUsed[9];
        int renderSize = 0;
        void fillBuffer(layout dest,void *data,int size,int elements);
        material *materialToUse = 0;

        public:
            unsigned int pickingID = 0;
            bool hidden = false;
            //Make private
            std::string name = "";
            void copyMeshData(aiMesh *src,material *toUse);
            void render(uniformsHolder *graphics,bool skipMats = false,texture *decal = 0);
            mesh();
            ~mesh();
    };

    class model;

    struct node
    {
        //This reference probably is not needed, but will be populated for now
        model *modelType = 0;

        node *parent = 0;
        std::vector<node*> children;

        std::string name = "";
        std::vector<mesh*> meshes;
        glm::mat4 transform = glm::mat4(1.0);

        void transformCollisionMeshes(glm::vec3 &totalColMin,glm::vec3 &totalColMax,glm::mat4 startTransform = glm::mat4(1.0));
        void render(uniformsHolder *graphics,glm::mat4 startTransform,bool skipMats = false,std::vector<glm::vec3> *nodeColors = 0,texture *decal = 0);
        void renderForPicking(uniformsHolder *graphics,glm::mat4 startTransform);
        node(aiNode *source,model *_modelParent,int debugIndent = 0);
        ~node();
    };

    class model
    {
        friend class node;

        private:
        public:
            glm::vec3 totalColMax = glm::vec3(-9999,-9999,-9999);
            glm::vec3 totalColMin = glm::vec3(9999,9999,9999);

            bool hasCamera = false;
            glm::vec3 cameraPosition;

            std::vector<node*> nodes;
            std::vector<mesh*> meshes;
            std::vector<material*> materials;
            bool isValid = false;
            void loadModel(std::string modelpath,unsigned int pFlags,preferenceFile *settings);
            void loadModel(const aiScene *scene,std::string modelpath,unsigned int pFlags,preferenceFile *settings);
            node *rootNode = 0;
        public:
            //You pass a path to a text file that includes settings for loading the model
            unsigned int numPickingIDs = 0;
            node *findNode(std::string name,node *toSearch = 0);
            void render(uniformsHolder *graphics,glm::mat4 startTransform = glm::mat4(1.0),bool skipMats = false,std::vector<glm::vec3> *nodeColors = 0,texture *decal = 0);
            void renderForPicking(uniformsHolder *graphics,glm::mat4 startTransform = glm::mat4(1.0));
            model(std::string filepath);
            model();
            ~model();
    };
}

#endif // MODEL_H_INCLUDED
