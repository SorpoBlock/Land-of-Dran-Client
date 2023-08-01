#include "newModel.h"

namespace syj
{
    void newDynamic::createBoxBody(btDynamicsWorld *_world,btVector3 extents,btVector3 offset,btVector3 initPos)
    {
        world = _world;
        //TODO: Don't reuse motion state or shape for each dynamic!

        if(body)
            return;

        finalHalfExtents = extents;
        finalOffset = offset;

        //See dynamicType::dynamicType on LoD2 server player.cpp
        shape = new btCompoundShape();
        btTransform t;
        t.setIdentity();
        t.setOrigin(offset);
        btBoxShape *box = new btBoxShape(extents);
        shape->addChildShape(t,box);

        btScalar masses[1];
        masses[0] = 1.0;
        t.setIdentity(); //idk if needed
        shape->calculatePrincipalAxisTransform(masses,t,defaultInertia);

        btTransform startTrans = btTransform::getIdentity();
        startTrans.setOrigin(initPos);
        defaultMotionState = new btDefaultMotionState(startTrans);
        mass = 1.0;

        body = new btRigidBody(mass,defaultMotionState,shape,defaultInertia);
        body->setUserPointer(this);
        body->setUserIndex(userIndex_dynamic);
        world->addRigidBody(body);

        body->setActivationState(DISABLE_DEACTIVATION);
        body->setFriction(1);
        body->setRestitution(0.5);
        body->setAngularFactor(btVector3(0,0,0));
    }

    void newDynamic::setFixedRotation(std::string name,glm::mat4 rotation)
    {
        for(int a = 0; a<type->allMeshes.size(); a++)
        {
            if(type->allMeshes[a]->name == name)
            {
                meshFixedRotation[a] = rotation;
                meshFixedRotationUsed[a] = true;
                return;
            }
        }
        error("newDynamic::setFixedRotation - Mesh name " + name + " did not exist");
    }

    newDynamic::~newDynamic()
    {
        if(world && body)
        {
            world->removeRigidBody(body);
            delete defaultMotionState;
            delete shape;
        }

        for(int a = 0; a<type->allMeshes.size(); a++)
        {
            instancedMesh *mesh = (instancedMesh*)type->allMeshes[a];
            for(int b = 0; b<mesh->instances.size(); b++)
            {
                if(mesh->instances[b] == this)
                {
                    mesh->instances.erase(mesh->instances.begin() + b);
                    break;
                }
            }
            mesh->recompile();
        }
    }

    newDynamic::newDynamic(newModel *_type,glm::vec3 baseScale)
    {
        scale = baseScale;
        type = _type;
        for(unsigned int a = 0; a<type->allMeshes.size(); a++)
        {
            meshColors.push_back(glm::vec3(0,0,1));
            meshTransforms.push_back(glm::mat4(1.0));
            meshFixedRotation.push_back(glm::mat4(1.0));
            meshFixedRotationUsed.push_back(false);
            ((instancedMesh*)type->allMeshes[a])->instances.push_back(this);
        }

        for(int a = 0; a<type->animations.size(); a++)
        {
            animationProgress.push_back(0);
            animationFrame.push_back(0);
            animationSpeed.push_back(1.0);
            animationOn.push_back(false);
        }

        calculateMeshTransforms(0);
        type->compileAll();
    }

    void newDynamic::setNodeColor(std::string nodeName,glm::vec3 color)
    {
        for(int a = 0; a<meshColors.size(); a++)
        {
            if(type->allMeshes[a]->name == nodeName)
            {
                meshColors[a] = color;
                meshColorChanged = true;
                return;
            }
        }
    }

    //assimp to opengl conversion for 4x4 matrices
    void CopyaiMatRedundant(aiMatrix4x4 from, glm::mat4 &to)
    {
        to[0][0] = from.a1; to[1][0] = from.a2;
        to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2;
        to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2;
        to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2;
        to[2][3] = from.d3; to[3][3] = from.d4;
    }

    glm::vec3 lerpRedundant(glm::vec3 x, glm::vec3 y, float t)
    {
        return x * (1.f - t) + y * t;
    }

    void newDynamic::play(std::string name,bool reset,float speed)
    {
        int idx = -1;
        for(int a = 0; a<type->animations.size(); a++)
        {
            if(type->animations[a].name == name)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Couldn't find animation " + name);
            return;
        }

        if(reset)
            animationProgress[idx] = 0;
        animationOn[idx] = true;
        animationSpeed[idx] = speed;

        /*playingAnimation = idx;
        if(reset)
            animationProgress = 0;
        animationSpeed = speed;*/
    }

    void newDynamic::play(int animID,bool reset,float speed)
    {
        int idx = -1;
        for(int a = 0; a<type->animations.size(); a++)
        {
            if(type->animations[a].serverID == animID)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Couldn't find animation " + std::to_string(animID));
            return;
        }

        if(reset)
            animationProgress[idx] = 0;
        animationOn[idx] = true;
        animationSpeed[idx] = speed;

        /*playingAnimation = idx;
        if(reset)
            animationProgress = 0;
        animationSpeed = speed;*/
    }

    void newDynamic::stop(std::string name)
    {
        //TODO: CRASH HERE
        int idx = -1;
        for(int a = 0; a<type->animations.size(); a++)
        {
            if(type->animations[a].name == name)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Couldn't find animation " + name);
            return;
        }

        animationOn[idx] = false;
        //playingAnimation = -1;
    }

    void newDynamic::stop(int animID)
    {
        int idx = -1;
        for(int a = 0; a<type->animations.size(); a++)
        {
            if(type->animations[a].serverID == animID)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Couldn't find animation " + std::to_string(animID));
            return;
        }

        animationOn[idx] = false;
        //playingAnimation = -1;
    }

    void newDynamic::stopAll()
    {
        for(int a = 0; a<type->animations.size(); a++)
            animationOn[a] = false;
    }

    void newDynamic::calculateMeshTransforms(float deltaMS,newNode *node,glm::mat4 transform)
    {
        if(!node)
            modelInterpolator.advance(deltaMS);

        if(hidden)
        {
            for(int a = 0; a<meshTransforms.size(); a++)
                meshTransforms[a] = glm::mat4(nan(""));
            return;
        }

        if(!node)
        {
            node = type->rootNode;

            for(int a = 0; a<type->animations.size(); a++)
            {
                if(!animationOn[a])
                    continue;

                animationFrame[a] = type->defaultFrame;
                animationProgress[a] += deltaMS * type->animations[a].speedDefault * animationSpeed[a];
                if(animationProgress[a] >= type->animations[a].endFrame)
                    animationProgress[a] -= type->animations[a].endFrame - type->animations[a].startFrame;
                animationFrame[a] = animationProgress[a] + type->animations[a].startFrame;

                /*if(type->animations.size() > 0 && playingAnimation != -1)
                {
                    animationProgress += deltaMS * type->animations[playingAnimation].speedDefault * animationSpeed;
                    if(animationProgress >= type->animations[playingAnimation].endFrame)
                        animationProgress -= type->animations[playingAnimation].endFrame - type->animations[playingAnimation].startFrame;
                    frame = animationProgress + type->animations[playingAnimation].startFrame;
                }*/
            }

            /*frame = type->defaultFrame;
            if(type->animations.size() > 0 && playingAnimation != -1)
            {
                animationProgress += deltaMS * type->animations[playingAnimation].speedDefault * animationSpeed;
                if(animationProgress >= type->animations[playingAnimation].endFrame)
                    animationProgress -= type->animations[playingAnimation].endFrame - type->animations[playingAnimation].startFrame;
                frame = animationProgress + type->animations[playingAnimation].startFrame;
            }*/
        }

        glm::mat4 newTrans = glm::mat4(1.0);
        if(type->animations.size() > 0)
        {
            if(node->rotFrames.size() == 0 && node->posFrames.size() == 0)
                newTrans = node->defaultTransform;
            else
            {
                glm::vec3 pos = glm::vec3(0,0,0);//node->posFrames[0];
                glm::mat4 rot = glm::mat4(1.0); //glm::toMat4(node->rotFrames[0]);

                for(int i = 0; i<type->animations.size(); i++)
                {
                    if(!animationOn[i])
                        continue;

                    for(unsigned int a = 0; a<node->posFrames.size(); a++)
                    {
                        if(node->posTimes[a] >= animationFrame[i])
                        {
                            float nextTime = node->posTimes[a];
                            float prevTime = node->posTimes[node->posTimes.size() - 1];
                            glm::vec3 nextPos = node->posFrames[a];
                            glm::vec3 prevPos = node->posFrames[node->posTimes.size() - 1];
                            if(a > 0)
                            {
                                prevTime = node->posTimes[a-1];
                                prevPos = node->posFrames[a-1];
                            }

                            float timeAheadPrev = animationFrame[i] - prevTime;
                            float timeBetween = nextTime - prevTime;
                            float progress = timeAheadPrev / timeBetween;

                            pos += lerpRedundant(prevPos,nextPos,progress);

                            break;
                        }
                    }

                    for(unsigned int a = 1; a<node->rotFrames.size(); a++)
                    {
                        if(node->rotTimes[a] >= animationFrame[i])
                        {
                            float nextTime = node->rotTimes[a];
                            float prevTime = node->rotTimes[node->rotTimes.size() - 1];
                            glm::quat nextRot = node->rotFrames[a];
                            glm::quat prevRot = node->rotFrames[node->rotTimes.size() - 1];
                            if(a > 0)
                            {
                                prevTime = node->rotTimes[a-1];
                                prevRot = node->rotFrames[a-1];
                            }

                            float timeAheadPrev = animationFrame[i] - prevTime;
                            float timeBetween = nextTime - prevTime;
                            float progress = timeAheadPrev / timeBetween;

                            rot = glm::toMat4(glm::slerp(prevRot,nextRot,progress)) * rot;

                            break;
                        }
                    }
                }
                /*glm::vec3 pos = node->posFrames[0];
                for(unsigned int a = 0; a<node->posFrames.size(); a++)
                {
                    if(node->posTimes[a] >= frame)
                    {
                        float nextTime = node->posTimes[a];
                        float prevTime = node->posTimes[node->posTimes.size() - 1];
                        glm::vec3 nextPos = node->posFrames[a];
                        glm::vec3 prevPos = node->posFrames[node->posTimes.size() - 1];
                        if(a > 0)
                        {
                            prevTime = node->posTimes[a-1];
                            prevPos = node->posFrames[a-1];
                        }

                        float timeAheadPrev = frame - prevTime;
                        float timeBetween = nextTime - prevTime;
                        float progress = timeAheadPrev / timeBetween;

                        pos = lerpRedundant(prevPos,nextPos,progress);

                        break;
                    }
                }

                glm::quat rot = node->rotFrames[0];
                for(unsigned int a = 1; a<node->rotFrames.size(); a++)
                {
                    if(node->rotTimes[a] >= frame)
                    {
                        float nextTime = node->rotTimes[a];
                        float prevTime = node->rotTimes[node->rotTimes.size() - 1];
                        glm::quat nextRot = node->rotFrames[a];
                        glm::quat prevRot = node->rotFrames[node->rotTimes.size() - 1];
                        if(a > 0)
                        {
                            prevTime = node->rotTimes[a-1];
                            prevRot = node->rotFrames[a-1];
                        }

                        float timeAheadPrev = frame - prevTime;
                        float timeBetween = nextTime - prevTime;
                        float progress = timeAheadPrev / timeBetween;

                        rot = glm::slerp(prevRot,nextRot,progress);

                        break;
                    }
                }*/

                //glm::vec3 pos = glm::vec3(0,0,0);
                //glm::quat rot = glm::quat(1,0,0,0);
                newTrans = glm::translate(node->rotationPivot) * glm::mat4(rot) * glm::translate(-node->rotationPivot) * glm::translate(pos);
            }
        }
        else
            newTrans = node->defaultTransform;

        for(unsigned int a = 0; a<node->instancedMeshes.size(); a++)
        {
            glm::mat4 finalTransform;
            int meshId = node->instancedMeshes[a]->meshIndex;
            if(meshFixedRotationUsed[meshId])
                finalTransform = transform * meshFixedRotation[meshId] * newTrans;
            else
                finalTransform = transform * newTrans;

            if(useGlobalTransform)
                meshTransforms[node->instancedMeshes[a]->meshIndex] = globalTransform * glm::scale(scale) * finalTransform;
            else
                meshTransforms[node->instancedMeshes[a]->meshIndex] = glm::translate(modelInterpolator.getPosition()) * glm::toMat4(modelInterpolator.getRotation()) * glm::scale(scale) * finalTransform;
        }

        transform = transform * newTrans;

        for(unsigned int a = 0; a<node->children.size(); a++)
            calculateMeshTransforms(deltaMS,node->children[a],transform);
    }

    std::string stripSillyAssimpNodeNames(std::string in)
    {
        if(in.find("_$AssimpFbx$_") != std::string::npos)
            return in.substr(0,in.find("_$AssimpFbx$_"));
        else
            return in;
    }

    std::map<std::string,int> aiProcessMapRedundant = {
                                        {"aiProcess_CalcTangentSpace",aiProcess_CalcTangentSpace},
                                        {"aiProcess_JoinIdenticalVertices",aiProcess_JoinIdenticalVertices},
                                        {"aiProcess_MakeLeftHanded",aiProcess_MakeLeftHanded},
                                        {"aiProcess_Triangulate",aiProcess_Triangulate},
                                        {"aiProcess_RemoveComponent",aiProcess_RemoveComponent},
                                        {"aiProcess_GenNormals",aiProcess_GenNormals},
                                        {"aiProcess_GenSmoothNormals",aiProcess_GenSmoothNormals},
                                        {"aiProcess_SplitLargeMeshes",aiProcess_SplitLargeMeshes},
                                        {"aiProcess_PreTransformVertices",aiProcess_PreTransformVertices},
                                        {"aiProcess_LimitBoneWeights",aiProcess_LimitBoneWeights},
                                        {"aiProcess_ValidateDataStructure",aiProcess_ValidateDataStructure},
                                        {"aiProcess_ImproveCacheLocality",aiProcess_ImproveCacheLocality},
                                        {"aiProcess_RemoveRedundantMaterials",aiProcess_RemoveRedundantMaterials},
                                        {"aiProcess_FixInfacingNormals",aiProcess_FixInfacingNormals},
                                        {"aiProcess_SortByPType",aiProcess_SortByPType},
                                        {"aiProcess_FindDegenerates",aiProcess_FindDegenerates},
                                        {"aiProcess_FindInvalidData",aiProcess_FindInvalidData},
                                        {"aiProcess_GenUVCoords",aiProcess_GenUVCoords},
                                        {"aiProcess_TransformUVCoords",aiProcess_TransformUVCoords},
                                        {"aiProcess_FindInstances",aiProcess_FindInstances},
                                        {"aiProcess_OptimizeMeshes",aiProcess_OptimizeMeshes},
                                        {"aiProcess_OptimizeGraph",aiProcess_OptimizeGraph},
                                        {"aiProcess_FlipUVs",aiProcess_FlipUVs},
                                        {"aiProcess_FlipWindingOrder",aiProcess_FlipWindingOrder},
                                        {"aiProcess_SplitByBoneCount",aiProcess_SplitByBoneCount},
                                        {"aiProcess_Debone",aiProcess_Debone},
                                        {"aiProcess_GlobalScale",aiProcess_GlobalScale},
                                        {"aiProcess_EmbedTextures",aiProcess_EmbedTextures},
                                        {"aiProcess_ForceGenNormals",aiProcess_ForceGenNormals},
                                        {"aiProcess_DropNormals",aiProcess_DropNormals},
                                        {"aiProcess_GenBoundingBoxes",aiProcess_GenBoundingBoxes},
                                        };

    newModel::newModel(std::string textFilePath)
    {
        std::string modelPath = "";
        unsigned int pFlags = 0;

        preferenceFile modelLoadFile;
        if(!modelLoadFile.importFromFile(textFilePath))
        {
            error("Could not open " + textFilePath);
            return;
        }

        preference *tmp = modelLoadFile.getPreference("FilePath");
        if(tmp)
            modelPath = getFolderFromPath(textFilePath) + tmp->toString();

        //This goes through the entire file for each possible flag
        //Not really a great way to do it, but I doubt many of these files will be more than 2 lines anyway
        for (const auto& [key, value] : aiProcessMapRedundant)
        {
            tmp = modelLoadFile.getPreference(key);
            if(tmp)
            {
                if(tmp->toBool())
                    pFlags += value;
            }
        }

        if(modelPath == "")
        {
            error("No Filepath pref set in model loading settings file " + textFilePath);
            return;
        }

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(modelPath,pFlags);

        if(!scene)
        {
            error("Problem loaded model file " + modelPath);
            error(importer.GetErrorString());
            return;
        }

        for(unsigned int a = 0; a<scene->mNumMaterials; a++)
        {
            aiMaterial *src = scene->mMaterials[a];

            std::string name = src->GetName().C_Str();
            material *dest = new material(name,modelLoadFile);
            allMaterials.push_back(dest);
        }

        for(unsigned int a = 0; a<scene->mNumMeshes; a++)
        {
            aiMesh *src = scene->mMeshes[a];
            instancedMesh *dest = new instancedMesh(src);
            dest->materialToUse = allMaterials[src->mMaterialIndex];
            dest->meshIndex = allMeshes.size();
            allMeshes.push_back(dest);
        }

        //Recursive function
        rootNode = new newNode(scene->mRootNode,this);

        if(scene->mNumAnimations == 1)
        {
            aiAnimation *anim = scene->mAnimations[0];
            for(unsigned int a = 0; a<anim->mNumChannels; a++)
            {
                aiNodeAnim *nodeAnim = anim->mChannels[a];
                newNode *target = 0;
                std::string animName = stripSillyAssimpNodeNames(nodeAnim->mNodeName.C_Str());
                for(unsigned int b = 0; b<allNodes.size(); b++)
                {
                    if(allNodes[b]->name == animName)
                    {
                        target = allNodes[b];
                        break;
                    }
                }

                if(!target)
                {
                    error("Could not find node " + std::string(nodeAnim->mNodeName.C_Str()) + " for animation!");
                    continue;
                }

                for(unsigned int b = 0; b<nodeAnim->mNumPositionKeys; b++)
                {
                    glm::vec3 pos(nodeAnim->mPositionKeys[b].mValue.x,nodeAnim->mPositionKeys[b].mValue.y,nodeAnim->mPositionKeys[b].mValue.z);
                    target->posFrames.push_back(pos);
                    target->posTimes.push_back(nodeAnim->mPositionKeys[b].mTime);
                }

                for(unsigned int b = 0; b<nodeAnim->mNumRotationKeys; b++)
                {
                    glm::quat rot(nodeAnim->mRotationKeys[b].mValue.w,nodeAnim->mRotationKeys[b].mValue.x,nodeAnim->mRotationKeys[b].mValue.y,nodeAnim->mRotationKeys[b].mValue.z);
                    target->rotFrames.push_back(rot);
                    target->rotTimes.push_back(nodeAnim->mRotationKeys[b].mTime);
                }

            }
        }
        else if(scene->mNumAnimations > 1)
            error("More than one set of animations in model!");

        if(scene->HasCameras())
        {
            if(scene->mNumCameras > 0)
            {
                hasCamera = true;
                cameraPosition = glm::vec3(scene->mCameras[0]->mPosition.x,scene->mCameras[0]->mPosition.y,scene->mCameras[0]->mPosition.z);
            }
        }

        calculateTotalCollisionExtents(rootNode,glm::mat4(1.0));
    }

    newNode::newNode(aiNode *source,newModel *parent)
    {
        parent->allNodes.push_back(this);
        name = source->mName.C_Str();

        if(name.find("_RotationPivot") != std::string::npos)
        {
            glm::vec3 scale,skew,trans;
            glm::vec4 perspective;
            glm::quat rot;
            glm::mat4 to;
            CopyaiMatRedundant(source->mTransformation,to);
            glm::decompose(to,scale,rot,trans,skew,perspective);
            rotationPivot = trans;
        }

        name = stripSillyAssimpNodeNames(name);

        glm::mat4 to;
        CopyaiMatRedundant(source->mTransformation,to);
        defaultTransform = to * defaultTransform;

        for(unsigned int a = 0; a<source->mNumMeshes; a++)
        {
            newMesh *toPutBack = parent->allMeshes[source->mMeshes[a]];
            if(toPutBack->isInstanced)
                instancedMeshes.push_back((instancedMesh*)toPutBack);
        }

        for(unsigned int a = 0; a<source->mNumChildren; a++)
        {
            if(stripSillyAssimpNodeNames(source->mChildren[a]->mName.C_Str()) == name)
                foldNodeInto(source->mChildren[a],parent);
            else
                children.push_back(new newNode(source->mChildren[a],parent));
        }
    }

    void newNode::foldNodeInto(aiNode *source,newModel *parent)
    {
        std::string tmpname = source->mName.C_Str();
        if(tmpname.find("_RotationPivot") != std::string::npos)
        {
            glm::vec3 scale,skew,trans;
            glm::vec4 perspective;
            glm::quat rot;
            glm::mat4 to;
            CopyaiMatRedundant(source->mTransformation,to);
            glm::decompose(to,scale,rot,trans,skew,perspective);
            rotationPivot = trans;
        }

        for(unsigned int a = 0; a<source->mNumMeshes; a++)
        {
            newMesh *toPutBack = parent->allMeshes[source->mMeshes[a]];
            if(toPutBack->isInstanced)
                instancedMeshes.push_back((instancedMesh*)toPutBack);
        }

        for(unsigned int a = 0; a<source->mNumChildren; a++)
        {
            if(stripSillyAssimpNodeNames(source->mChildren[a]->mName.C_Str()) == name)
                foldNodeInto(source->mChildren[a],parent);
            else
                children.push_back(new newNode(source->mChildren[a],parent));
        }
    }

    newNode::~newNode()
    {
        for(unsigned int a = 0; a<children.size(); a++)
            if(children[a])
                delete children[a];
    }

    void instancedMesh::render(uniformsHolder *graphics)
    {
        if(instances.size() < 1)
            return;

        if(hidden)
            return;

        if(materialToUse)
            materialToUse->use(*graphics);

        glBindVertexArray(vao);

        glDrawElementsInstanced(GL_TRIANGLES,numVerts,GL_UNSIGNED_INT,(void*)0,instances.size());

        glBindVertexArray(0);
    }

    void instancedMesh::renderWithoutMaterial()
    {
        if(instances.size() < 1)
            return;

        if(hidden)
            return;

        glBindVertexArray(vao);

        glDrawElementsInstanced(GL_TRIANGLES,numVerts,GL_UNSIGNED_INT,(void*)0,instances.size());

        glBindVertexArray(0);
    }

    void instancedMesh::fillBuffer(instancedLayout dest,void *data,int size,int elements)
    {
        scope("instancedMesh::fillBuffer");

        GLenum err = glGetError();

        glBindVertexArray(vao);

        if(dest == newIndex)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffers[dest]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,size,data,GL_STATIC_DRAW);
            glBindVertexArray(0);

            err = glGetError();
            if(err != GL_NO_ERROR)
                error("dest == index OpenGL error: " + std::to_string(err));

            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER,buffers[dest]);
        glEnableVertexAttribArray(dest);
        glBufferData(GL_ARRAY_BUFFER,size,data,GL_STATIC_DRAW);
        glVertexAttribDivisor(dest,0);
        glVertexAttribPointer(dest,elements,GL_FLOAT,GL_FALSE,0,0);
        glBindVertexArray(0);

        err = glGetError();
        if(err != GL_NO_ERROR)
            error("OpenGL error: " + std::to_string(err) + " size: " + std::to_string(size) + " elements: " + std::to_string(elements) + " layout: " + std::to_string(dest) + " VAO: " + std::to_string(vao));
    }

    void newDynamic::bufferSubData()
    {
        for(int a = 0; a<type->allMeshes.size(); a++)
        {
            glBindVertexArray(type->allMeshes[a]->vao);

            if(meshColorChanged)
            {
                glBindBuffer(GL_ARRAY_BUFFER,((instancedMesh*)type->allMeshes[a])->buffers[perMeshColor]);
                glBufferSubData(GL_ARRAY_BUFFER,sizeof(glm::vec3) * bufferOffset,sizeof(glm::vec3),&meshColors[a][0]);
            }

            glBindBuffer(GL_ARRAY_BUFFER,((instancedMesh*)type->allMeshes[a])->buffers[perMeshTransformA]);
            glBufferSubData(GL_ARRAY_BUFFER,sizeof(glm::mat4) * bufferOffset,sizeof(glm::mat4),&meshTransforms[a][0][0]);

            glBindVertexArray(0);
        }

        meshColorChanged = false;
    }

    //TODO: Colors probably don't need to be recompiled constantly
    void instancedMesh::recompile()
    {
        scope("instancedMesh::recompile");

        std::vector<glm::mat4> transforms;
        std::vector<glm::vec3> colors;

        for(unsigned int a = 0; a<instances.size(); a++)
        {
            instances[a]->bufferOffset = a;

            transforms.push_back(instances[a]->meshTransforms[meshIndex]);
            colors.push_back(instances[a]->meshColors[meshIndex]);
            instances[a]->meshColorChanged = false;
        }

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER,buffers[perMeshTransformA]);
        glBufferData(GL_ARRAY_BUFFER,sizeof(glm::mat4) * transforms.size(),&transforms[0][0][0],GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,buffers[perMeshColor]);
        glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec3) * colors.size(),&colors[0][0],GL_STREAM_DRAW);

        glBindVertexArray(0);

        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
            error("OpenGL error: " + std::to_string(err));
    }

    instancedMesh::instancedMesh(aiMesh *src)
    {
        scope("instancedMesh::instancedMesh");

        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
            error("Before anything OpenGL error: " + std::to_string(err));

        glGenVertexArrays(1,&vao);
        glGenBuffers(8,buffers);

        glBindVertexArray(vao);

        err = glGetError();
        if(err != GL_NO_ERROR)
            error("After allocation OpenGL error: " + std::to_string(err));

        glBindBuffer(GL_ARRAY_BUFFER,buffers[perMeshColor]);
        glEnableVertexAttribArray(perMeshColor);
        glVertexAttribPointer(perMeshColor,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(perMeshColor,1);

        glBindBuffer(GL_ARRAY_BUFFER,buffers[perMeshTransformA]);
        for(int i = 0; i<4; i++)
        {
            glEnableVertexAttribArray(perMeshTransformA + i);
            glVertexAttribPointer(perMeshTransformA + i,4,GL_FLOAT,GL_FALSE,sizeof(glm::mat4),(void*)(4* sizeof(float) * i));
            glVertexAttribDivisor(perMeshTransformA + i,1);
        }

        err = glGetError();
        if(err != GL_NO_ERROR)
            error("After attribs set OpenGL error: " + std::to_string(err));

        glBindVertexArray(0);

        name = src->mName.C_Str();

        if(src->HasPositions())
        {
            fillBuffer(newPositions,src->mVertices,src->mNumVertices * sizeof(aiVector3D),3);

            float maxX = src->mVertices[0].x;
            float maxY = src->mVertices[0].y;
            float maxZ = src->mVertices[0].z;
            float minX = src->mVertices[0].x;
            float minY = src->mVertices[0].y;
            float minZ = src->mVertices[0].z;
            for(unsigned int a = 0; a<src->mNumVertices; a++)
            {
                aiVector3D v = src->mVertices[a];
                if(v.x > maxX)
                    maxX = v.x;
                if(v.y > maxY)
                    maxY = v.y;
                if(v.z > maxZ)
                    maxZ = v.z;
                if(v.x < minX)
                    minX = v.x;
                if(v.y < minY)
                    minY = v.y;
                if(v.z < minZ)
                    minZ = v.z;
            }
            rawMaxExtents = glm::vec3(maxX,maxY,maxZ);
            rawMinExtents = glm::vec3(minX,minY,minZ);

            if(name.find("Collision") != std::string::npos)
            {
                hidden = true;
                isCollisionMesh = true;
            }
        }
        else
            error("Model did not have position data!");

        if(src->HasNormals())
            fillBuffer(newNormals,src->mNormals,src->mNumVertices * sizeof(aiVector3D),3);

        if(src->HasTangentsAndBitangents())
        {
            fillBuffer(newTangents,src->mTangents,src->mNumVertices * sizeof(aiVector3D),3);
            fillBuffer(newBitangents,src->mBitangents,src->mNumVertices * sizeof(aiVector3D),3);
        }

        if(src->GetNumUVChannels() == 1)
        {
            std::vector<glm::vec2> data;
            for(unsigned int b = 0; b<src->mNumVertices; b++)
                data.push_back(glm::vec2(src->mTextureCoords[0][b].x,src->mTextureCoords[0][b].y));

            fillBuffer(newUVs,&data[0][0],data.size() * sizeof(glm::vec2),2);
        }
        else if(src->GetNumUVChannels() > 1)
            error("File has more than one UV channel which is not supported!");

        if(src->HasFaces())
        {
            std::vector<unsigned int> indices;
            for(unsigned int b = 0; b<src->mNumFaces; b++)
            {
                if(src->mFaces[b].mNumIndices != 3)
                    error("Mesh has non-triangle face, ignoring. Did you know you can specify aiProcess_Triangulate to fix this?");
                else
                {
                    indices.push_back(src->mFaces[b].mIndices[0]);
                    indices.push_back(src->mFaces[b].mIndices[1]);
                    indices.push_back(src->mFaces[b].mIndices[2]);
                }
            }
            numVerts = indices.size();
            fillBuffer(newIndex,&indices[0],sizeof(unsigned int) * indices.size(),0);
        }
        else
            error("Mesh did not have faces / indicies");
    }

    instancedMesh::~instancedMesh()
    {
        scope("instancedMesh::~instancedMesh");

        glDeleteVertexArrays(1,&vao);
        glDeleteBuffers(8,buffers);
    }

    glm::vec4 postovec4Redundant(glm::vec3 a){return glm::vec4(a.x,a.y,a.z,1.0);}

    void newModel::calculateTotalCollisionExtents(newNode *current,glm::mat4 transform)
    {
        transform = transform * current->defaultTransform;

        for(int a = 0; a<current->instancedMeshes.size(); a++)
        {
            if(current->instancedMeshes[a]->isCollisionMesh)
            {
                glm::vec3 s = current->instancedMeshes[a]->rawMaxExtents - current->instancedMeshes[a]->rawMinExtents;
                /*std::cout<<current->instancedMeshes[a]->name<<" name\n";
                std::cout<<"Min: "<<current->instancedMeshes[a]->rawMinExtents.x<<","<<current->instancedMeshes[a]->rawMinExtents.y<<","<<current->instancedMeshes[a]->rawMinExtents.z<<"\n";
                std::cout<<"Max: "<<current->instancedMeshes[a]->rawMaxExtents.x<<","<<current->instancedMeshes[a]->rawMaxExtents.y<<","<<current->instancedMeshes[a]->rawMaxExtents.z<<"\n";*/
                current->instancedMeshes[a]->rawMinExtents = transform * postovec4Redundant(current->instancedMeshes[a]->rawMinExtents);
                current->instancedMeshes[a]->rawMaxExtents = transform * postovec4Redundant(current->instancedMeshes[a]->rawMaxExtents);
                /*std::cout<<"Min after: "<<current->instancedMeshes[a]->rawMinExtents.x<<","<<current->instancedMeshes[a]->rawMinExtents.y<<","<<current->instancedMeshes[a]->rawMinExtents.z<<"\n";
                std::cout<<"Max after: "<<current->instancedMeshes[a]->rawMaxExtents.x<<","<<current->instancedMeshes[a]->rawMaxExtents.y<<","<<current->instancedMeshes[a]->rawMaxExtents.z<<"\n";*/

                if(current->instancedMeshes[a]->rawMinExtents.x > current->instancedMeshes[a]->rawMaxExtents.x)
                    std::swap(current->instancedMeshes[a]->rawMinExtents.x,current->instancedMeshes[a]->rawMaxExtents.x);
                if(current->instancedMeshes[a]->rawMinExtents.y > current->instancedMeshes[a]->rawMaxExtents.y)
                    std::swap(current->instancedMeshes[a]->rawMinExtents.y,current->instancedMeshes[a]->rawMaxExtents.y);
                if(current->instancedMeshes[a]->rawMinExtents.z > current->instancedMeshes[a]->rawMaxExtents.z)
                    std::swap(current->instancedMeshes[a]->rawMinExtents.z,current->instancedMeshes[a]->rawMaxExtents.z);

                if(current->instancedMeshes[a]->rawMinExtents.x < totalColMin.x)
                    totalColMin.x = current->instancedMeshes[a]->rawMinExtents.x;
                if(current->instancedMeshes[a]->rawMinExtents.y < totalColMin.y)
                    totalColMin.y = current->instancedMeshes[a]->rawMinExtents.y;
                if(current->instancedMeshes[a]->rawMinExtents.z < totalColMin.z)
                    totalColMin.z = current->instancedMeshes[a]->rawMinExtents.z;

                if(current->instancedMeshes[a]->rawMaxExtents.x > totalColMax.x)
                    totalColMax.x = current->instancedMeshes[a]->rawMaxExtents.x;
                if(current->instancedMeshes[a]->rawMaxExtents.y > totalColMax.y)
                    totalColMax.y = current->instancedMeshes[a]->rawMaxExtents.y;
                if(current->instancedMeshes[a]->rawMaxExtents.z > totalColMax.z)
                    totalColMax.z = current->instancedMeshes[a]->rawMaxExtents.z;
             }
        }

        for(int a = 0; a<current->children.size(); a++)
            calculateTotalCollisionExtents(current->children[a],transform);
    }

    void newModel::render(uniformsHolder *graphics)
    {
        for(unsigned int a = 0; a<allMeshes.size(); a++)
            allMeshes[a]->render(graphics);
    }

    void newModel::renderWithoutMaterials()
    {
        for(unsigned int a = 0; a<allMeshes.size(); a++)
            allMeshes[a]->renderWithoutMaterial();
    }

    void newModel::compileAll()
    {
        for(int a = 0; a<allMeshes.size(); a++)
            ((instancedMesh*)allMeshes[a])->recompile();
    }
}
