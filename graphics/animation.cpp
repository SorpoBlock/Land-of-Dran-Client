#include "animation.h"

namespace syj
{

    glm::vec3 lerp(glm::vec3 x, glm::vec3 y, float t)
    {
      return x * (1.f - t) + y * t;
    }

    glm::mat4 bone::getTransform(float frame)
    {
        glm::vec3 pos = posKeys[0];
        for(unsigned int a = 0; a<posKeys.size(); a++)
        {
            if(posTimes[a] >= frame)
            {
                float nextTime = posTimes[a];
                float prevTime = posTimes[posTimes.size() - 1];
                glm::vec3 nextPos = posKeys[a];
                glm::vec3 prevPos = posKeys[posTimes.size() - 1];
                if(a > 0)
                {
                    prevTime = posTimes[a-1];
                    prevPos = posKeys[a-1];
                }

                float timeAheadPrev = frame - prevTime;
                float timeBetween = nextTime - prevTime;
                float progress = timeAheadPrev / timeBetween;

                pos = lerp(prevPos,nextPos,progress);

                break;
            }
        }

        glm::vec3 scale = scaleKeys[0];
        for(unsigned int a = 1; a<scaleKeys.size(); a++)
        {
            if(scaleTimes[a] >= frame)
            {
                float nextTime = scaleTimes[a];
                float prevTime = scaleTimes[scaleTimes.size() - 1];
                glm::vec3 nextScale = scaleKeys[a];
                glm::vec3 prevScale = scaleKeys[scaleTimes.size() - 1];
                if(a > 0)
                {
                    prevTime = scaleTimes[a-1];
                    prevScale = scaleKeys[a-1];
                }

                float timeAheadPrev = frame - prevTime;
                float timeBetween = nextTime - prevTime;
                float progress = timeAheadPrev / timeBetween;

                scale = lerp(prevScale,nextScale,progress);

                break;
            }
        }

        glm::quat rot = rotKeys[0];
        for(unsigned int a = 1; a<rotKeys.size(); a++)
        {
            if(rotTimes[a] >= frame)
            {
                float nextTime = rotTimes[a];
                float prevTime = rotTimes[rotTimes.size() - 1];
                glm::quat nextRot = rotKeys[a];
                glm::quat prevRot = rotKeys[rotTimes.size() - 1];
                if(a > 0)
                {
                    prevTime = rotTimes[a-1];
                    prevRot = rotKeys[a-1];
                }

                float timeAheadPrev = frame - prevTime;
                float timeBetween = nextTime - prevTime;
                float progress = timeAheadPrev / timeBetween;

                rot = glm::slerp(prevRot,nextRot,progress);

                break;
            }
        }

        return glm::translate(pos) * glm::mat4(rot) * glm::scale(scale);
    }

    meshAnimationData::meshAnimationData(mesh *baseMesh,aiMesh *source,animatedModel *parent)
    {
        boneIDs.resize(baseMesh->renderSize,glm::ivec4(-1,-1,-1,-1));
        weights.resize(baseMesh->renderSize,glm::vec4(0,0,0,0));

        debug("Mesh had " + std::to_string(source->mNumBones) + " bones!");
        for(unsigned int a = 0; a<source->mNumBones; a++)
        {
            aiBone *srcBone = source->mBones[a];
            bone *tmp = parent->getBone(srcBone->mName.C_Str());
            if(!tmp)
            {
                //Seems to be trivial???
                error("Bone " + std::string(srcBone->mName.C_Str()) + " not found!");
                //log<<LOG_ERROR<<"Bone " + std::string(srcBone->mName.C_Str()) + " not found";
                continue;
            }

            unsigned int idToUse = tmp->id;
            CopyaiMat(srcBone->mOffsetMatrix,tmp->offset);

            std::cout<<"Bone had "<<srcBone->mNumWeights<<" weights\n";
            for(unsigned int b = 0; b<srcBone->mNumWeights; b++)
            {
                aiVertexWeight data = srcBone->mWeights[b];
                if(data.mVertexId >= baseMesh->renderSize)
                {
                    error("Bone vertex id out of range.");
                    continue;
                }

                if(boneIDs[data.mVertexId].r == -1)
                {
                    boneIDs[data.mVertexId].r = idToUse;
                    weights[data.mVertexId].r = data.mWeight;
                }
                else if(boneIDs[data.mVertexId].g == -1)
                {
                    boneIDs[data.mVertexId].g = idToUse;
                    weights[data.mVertexId].g = data.mWeight;
                }
                else if(boneIDs[data.mVertexId].b == -1)
                {
                    boneIDs[data.mVertexId].b = idToUse;
                    weights[data.mVertexId].b = data.mWeight;
                }
                else if(boneIDs[data.mVertexId].a == -1)
                {
                    boneIDs[data.mVertexId].a = idToUse;
                    weights[data.mVertexId].a = data.mWeight;
                }
            }
        }

        glCreateBuffers(2,buffer);

        glBindVertexArray(baseMesh->vao);

        debug("Adding " + std::to_string(boneIDs.size()) + " bone ids and " + std::to_string(weights.size()) + " weight arrays to mesh " + baseMesh->name);

        glBindBuffer(GL_ARRAY_BUFFER,buffer[0]);
        glEnableVertexAttribArray(6);
        glBufferData(GL_ARRAY_BUFFER,boneIDs.size() * sizeof(glm::ivec4),&boneIDs[0][0],GL_STATIC_DRAW);
        glVertexAttribIPointer(6,4,GL_INT,GL_FALSE,0);

        glBindBuffer(GL_ARRAY_BUFFER,buffer[1]);
        glEnableVertexAttribArray(7);
        glBufferData(GL_ARRAY_BUFFER,weights.size() * sizeof(glm::vec4),&weights[0][0],GL_STATIC_DRAW);
        glVertexAttribPointer(7,4,GL_FLOAT,GL_FALSE,0,0);

        glBindVertexArray(0);
    }

    bone *animatedModel::getBone(std::string name)
    {
        for(unsigned int a = 0; a<bones.size(); a++)
            if(bones[a]->name == name)
                return bones[a];
        return 0;
    }

    animatedModel::~animatedModel()
    {
        for(unsigned int a = 0; a<weightData.size(); a++)
            if(weightData[a])
                delete weightData[a];
        for(unsigned int a = 0; a<bones.size(); a++)
            if(bones[a])
                delete bones[a];
        if(finalBoneMatrices)
            delete finalBoneMatrices;
    }

    void animatedModel::render(uniformsHolder *unis,float frame,glm::mat4 startTransform,std::vector<glm::vec3> *nodeColors,texture *decal,std::vector<nodeTransform> *additionalTransforms)
    {
        updateBoneTransforms(rootNode,startTransform,frame,additionalTransforms);
        glUniform1i(unis->isAnimated,1);
        glUniformMatArray("boneTransforms",unis->target,finalBoneMatrices,bones.size());
        if(nodeColors)
            glUniform1i(unis->target->getUniformLocation("useNodeColor"),1);
        renderAnimated(unis,frame,startTransform,0,nodeColors,decal,additionalTransforms);
        glUniform1i(unis->isAnimated,0);
        glUniform1i(unis->target->getUniformLocation("useNodeColor"),0);
    }

    bone::bone(animatedModel *parent,aiNodeAnim *source)
    {
        name = source->mNodeName.C_Str();
        target = parent->findNode(name);
        if(!target)
            error("Unfound node for bone: " + name);

        for(unsigned int a = 0; a<source->mNumPositionKeys; a++)
        {
            posKeys.push_back(glm::vec3(source->mPositionKeys[a].mValue.x,source->mPositionKeys[a].mValue.y,source->mPositionKeys[a].mValue.z));
            posTimes.push_back(source->mPositionKeys[a].mTime);
        }
        for(unsigned int a = 0; a<source->mNumScalingKeys; a++)
        {
            scaleKeys.push_back(glm::vec3(source->mScalingKeys[a].mValue.x,source->mScalingKeys[a].mValue.y,source->mScalingKeys[a].mValue.z));
            scaleTimes.push_back(source->mScalingKeys[a].mTime);
        }
        for(unsigned int a = 0; a<source->mNumRotationKeys; a++)
        {
            rotKeys.push_back(glm::quat(source->mRotationKeys[a].mValue.w,source->mRotationKeys[a].mValue.x,source->mRotationKeys[a].mValue.y,source->mRotationKeys[a].mValue.z));
            rotTimes.push_back(source->mRotationKeys[a].mTime);
        }
    }

    void animatedModel::loadAnimations(const aiScene *scene)
    {
        debug("Animations: " + std::to_string(scene->mNumAnimations));
        //for(unsigned int a = 0; a<scene->mNumAnimations; a++)
        //{
        if(scene->mAnimations)
        {
            int a = 0;
            aiAnimation *anim = scene->mAnimations[a];
            if(anim)
            {
                //todo: multiple durations:
                duration = anim->mDuration;
                ticksPerSecond = anim->mTicksPerSecond;

                debug("Num animNodes: " + std::to_string(anim->mNumChannels));
                for(unsigned int b = 0; b<anim->mNumChannels; b++)
                {
                    aiNodeAnim *channel = anim->mChannels[b];
                    debug("Channel: " + std::string(channel->mNodeName.C_Str()));
                    bones.push_back(new bone(this,channel));
                    bones[bones.size() - 1]->id = bones.size() - 1;
                }
            }
        }
        //}

        for(unsigned int a = 0; a<scene->mNumMeshes; a++)
        {
            aiMesh *mesh = scene->mMeshes[a];
            for(unsigned int b = 0; b<mesh->mNumBones; b++)
            {
                aiBone *tmp = mesh->mBones[b];
                if(!getBone(tmp->mName.C_Str()))
                {
                    if(tmp->mNumWeights > 0)
                    {
                        error("Bone " + std::string(tmp->mName.C_Str()) + " not found! Bone is non-trivial with " + std::to_string(tmp->mNumWeights) + " verts.");
                        /*bone *fixBone = new bone;
                        fixBone->name = tmp->mName.C_Str();
                        fixBone->target = findNode(fixBone->name);
                        if(!fixBone->target)
                            log<<LOG_ERROR<<"Fix bone does not have valid node target!";
                        CopyaiMat(tmp->mOffsetMatrix,fixBone->offset);
                        fixBone->id = bones.size();
                        fixBone->posKeys.push_back(glm::vec3(0,0,0));
                        fixBone->posTimes.push_back(0);
                        fixBone->posKeys.push_back(glm::vec3(0,0,0));
                        fixBone->posTimes.push_back(duration);

                        fixBone->scaleKeys.push_back(glm::vec3(1,1,1));
                        fixBone->scaleTimes.push_back(0);
                        fixBone->scaleKeys.push_back(glm::vec3(1,1,1));
                        fixBone->scaleTimes.push_back(duration);

                        fixBone->rotKeys.push_back(glm::quat(glm::vec3(0,0,0)));
                        fixBone->rotTimes.push_back(0);
                        fixBone->rotKeys.push_back(glm::quat(glm::vec3(0,0,0)));
                        fixBone->rotTimes.push_back(duration);
                        bones.push_back(fixBone);*/
                    }
                }
            }
        }

        for(unsigned int a = 0; a<scene->mNumMeshes; a++)
        {
            aiMesh *src = scene->mMeshes[a];
            weightData.push_back(new meshAnimationData(meshes[a],src,this));
        }

        finalBoneMatrices = new glm::mat4[bones.size()];
    }

    void animatedInstance::advance(float deltaMS)
    {
        if(playingAnimation != -1)
        {
            animationProgress += deltaMS * type->animations[playingAnimation].speedDefault * animationSpeed;
            if(animationProgress >= type->animations[playingAnimation].endFrame)
                animationProgress -= type->animations[playingAnimation].endFrame - type->animations[playingAnimation].startFrame;
        }
    }

    void animatedInstance::renderInst(uniformsHolder *unis,float deltaMS,bool stopInterpolation,bool skipMats)
    {
        glm::mat4 translate;
        if(stopInterpolation)
            translate = glm::translate(modelInterpolator.keyFrames[0].position);
        else
            translate = glm::translate(modelInterpolator.getPosition());
        glm::mat4 rotate = glm::toMat4(modelInterpolator.getRotation());
        glm::mat4 scaling = glm::scale(scale);

        if(playingAnimation != -1)
        {
            animationProgress += deltaMS * type->animations[playingAnimation].speedDefault * animationSpeed;
            if(animationProgress >= type->animations[playingAnimation].endFrame)
                animationProgress -= type->animations[playingAnimation].endFrame - type->animations[playingAnimation].startFrame;
            type->render(unis,animationProgress + type->animations[playingAnimation].startFrame,translate * rotate * scaling,nodeColors.size() > 0 ? &nodeColors : 0,decal,additionalTransforms.size() > 0 ? &additionalTransforms : 0);
        }
        else
        {
            type->render(unis,type->defaultFrame,translate * rotate * scaling,nodeColors.size() > 0 ? &nodeColors : 0,decal,additionalTransforms.size() > 0 ? &additionalTransforms : 0);
        }
    }

    void animatedInstance::play(std::string name,bool reset,float speed)
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
        playingAnimation = idx;
        if(reset)
            animationProgress = 0;
        animationSpeed = speed;
    }

    void animatedInstance::play(int animID,bool reset,float speed)
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
        playingAnimation = idx;
        if(reset)
            animationProgress = 0;
        animationSpeed = speed;
    }

    void animatedInstance::stop()
    {
        playingAnimation = -1;
    }

    void animatedModel::updateBoneTransforms(node *startNode,glm::mat4 transform,float frame,std::vector<nodeTransform> *additionalTransforms)
    {
        if(!startNode)
            startNode = rootNode;

        glm::mat4 nodeTransform = startNode->transform;
        bone *curBone = getBone(startNode->name);
        if(curBone)
        {
            //todo: Update bone anim here
            nodeTransform = curBone->getTransform(frame);
        }

        glm::mat4 additionalTransform = glm::mat4(1.0);
        if(additionalTransforms)
        {
            for(int a = 0; a<additionalTransforms->size(); a++)
            {
                if(additionalTransforms->at(a).target == startNode)
                {
                    additionalTransform = additionalTransforms->at(a).transform;

                    break;
                }
            }
        }

        transform = transform * additionalTransform *  nodeTransform;

        if(curBone)
            finalBoneMatrices[curBone->id] = transform * additionalTransform * curBone->offset;

        for(unsigned int a = 0; a<startNode->children.size(); a++)
            updateBoneTransforms(startNode->children[a],transform,frame,additionalTransforms);
    }

    void animatedModel::renderAnimated(uniformsHolder *unis,float frame,glm::mat4 startTransform,node *toRender,std::vector<glm::vec3> *nodeColors,texture *decal,std::vector<nodeTransform> *additionalTransforms)
    {
        if(!toRender)
            toRender = rootNode;

        glm::mat4 nodeTransform = toRender->transform;
        bone *curBone = getBone(toRender->name);
        if(curBone)
        {
            //todo: Update bone anim here
            nodeTransform = curBone->getTransform(frame);
        }

        glm::mat4 additionalTransform = glm::mat4(1.0);
        if(additionalTransforms)
        {
            for(int a = 0; a<additionalTransforms->size(); a++)
            {
                if(additionalTransforms->at(a).target == toRender)
                {
                    additionalTransform = additionalTransforms->at(a).transform;
                    break;
                }
            }
        }

        startTransform = startTransform * nodeTransform;

        for(int a = 0; a<toRender->meshes.size(); a++)
        {
            if(toRender->meshes[a]->hidden)
                continue;

            if(nodeColors)
            {
                glm::vec3 color = glm::vec3(1,1,1);
                if(toRender->meshes[a]->pickingID >= 0 && toRender->meshes[a]->pickingID < nodeColors->size())
                    color = nodeColors->at(toRender->meshes[a]->pickingID);
                glUniform3f(unis->target->getUniformLocation("nodeColor"),color.r,color.g,color.b);
            }

            //glUniformMat(unis->target->getUniformLocation("additionalMatrix"),additionalTransform);
            unis->setModelMatrix(startTransform);
            toRender->meshes[a]->render(unis,false,decal);
        }

        for(unsigned int a = 0; a<toRender->children.size(); a++)
            renderAnimated(unis,frame,startTransform,toRender->children[a],nodeColors,decal,additionalTransforms);

    }




















    animatedModel::animatedModel(std::string filename)
    {
        std::string modelPath = "";
        unsigned int pFlags = 0;

        preferenceFile modelLoadFile;
        if(!modelLoadFile.importFromFile(filename))
        {
            error("Could not open " + filename);
            return;
        }

        preference *tmp = modelLoadFile.getPreference("FilePath");
        if(tmp)
            modelPath = getFolderFromPath(filename) + tmp->toString();

        //This goes through the entire file for each possible flag
        //Not really a great way to do it, but I doubt many of these files will be more than 2 lines anyway
        for (const auto& [key, value] : aiProcessMap)
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
            error("No Filepath pref set in model loading settings file " + filename);
            return;
        }

        debug("Pflags: " + std::to_string(pFlags));

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(modelPath,pFlags);

        if(!scene)
        {
            error("Problem loaded model file " + modelPath);
            error(importer.GetErrorString());
            return;
        }

        loadModel(scene,modelPath,pFlags,&modelLoadFile);
        loadAnimations(scene);

        rootNode->transformCollisionMeshes(totalColMin,totalColMax);
    }
}
