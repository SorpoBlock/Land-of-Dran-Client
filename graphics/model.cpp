#include "model.h"

namespace syj
{
    //assimp to opengl conversion for 4x4 matrices
    void CopyaiMat(aiMatrix4x4 from, glm::mat4 &to)
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

    void mesh::copyMeshData(aiMesh *src,material *toUse)
    {
        scope("mesh::copyMeshData");

        materialToUse = toUse;

        name = src->mName.C_Str();
        debug("Loading mesh: " + name);

        debug("Mesh has " + std::to_string(src->mNumBones) + " bones.");

        if(src->HasPositions())
        {
            debug("Positions: " + std::to_string(src->mNumVertices));
            debug("First vertex: " +
                  std::to_string(src->mVertices[0].x) + "," +
                  std::to_string(src->mVertices[0].y) + "," +
                  std::to_string(src->mVertices[0].z));

            fillBuffer(positions,src->mVertices,3,src->mNumVertices * sizeof(aiVector3D));

            if(name.find("Collision") != std::string::npos)
            {
                float maxX = src->mVertices[0].x;
                float maxY = src->mVertices[0].y;
                float maxZ = src->mVertices[0].z;
                float minX = src->mVertices[0].x;
                float minY = src->mVertices[0].y;
                float minZ = src->mVertices[0].z;
                for(int a = 0; a<src->mNumVertices; a++)
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
                detectedCollisionMesh = true;
                hidden = true;
                detectedColMax = glm::vec3(maxX,maxY,maxZ);
                detectedColMin = glm::vec3(minX,minY,minZ);
            }
        }

        if(src->HasNormals())
        {
            debug("Has normals.");
            fillBuffer(normals,src->mNormals,3,src->mNumVertices * sizeof(aiVector3D));
        }

        if(src->HasTangentsAndBitangents())
        {
            debug("Has tangents and bitangents.");
            fillBuffer(tangents,src->mTangents,3,src->mNumVertices * sizeof(aiVector3D));
            fillBuffer(bitangents,src->mBitangents,3,src->mNumVertices * sizeof(aiVector3D));
        }

        if(src->GetNumUVChannels() > 0)
        {
            if(src->GetNumUVChannels() > 1)
                error("File has more than one UV channel which is not supported!");

            if(src->HasTextureCoords(0))
            {
                debug("Has UVs, components: " + std::to_string(src->mNumUVComponents[0]) + " Channels: " + std::to_string(src->GetNumUVChannels()));

                std::vector<glm::vec2> data;
                for(unsigned int b = 0; b<src->mNumVertices; b++)
                    data.push_back(glm::vec2(src->mTextureCoords[0][b].x,src->mTextureCoords[0][b].y));

                fillBuffer(uvs,&data[0][0],2,data.size() * sizeof(glm::vec2));
            }
        }

        if(src->GetNumColorChannels() > 0)
        {
            if(src->GetNumColorChannels() > 1)
                error("File has more than one color channel which is not supported!");

            if(src->HasVertexColors(0))
            {
                debug("Has colors.");
                fillBuffer(colors,src->mColors,4,src->mNumVertices * sizeof(glm::vec4));
            }
        }

        if(src->HasFaces())
        {
            debug("Faces: " + std::to_string(src->mNumFaces));
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
            renderSize = indices.size();
            fillBuffer(index,&indices[0],0,sizeof(unsigned int) * indices.size());
        }
        else
            error("Mesh does not have faces/indicies!");
    }

    void mesh::render(uniformsHolder *graphics,bool skipMats,texture *decal)
    {
        if(!skipMats)
        {
            glUniform1i(graphics->target->getUniformLocation("currentMesh"),pickingID);
            materialToUse->use(graphics);
            //Redundant texture bind...
            if(name == "Face1" && decal)
            {
                glUniform1i(graphics->useAlbedo      ,1);
                decal->bind(albedo);
            }
        }

        if(!hidden)
        {
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES,renderSize,GL_UNSIGNED_INT,0);
            glBindVertexArray(0);
        }
    }

    void mesh::fillBuffer(layout dest,void *data,int elements,int size)
    {
        scope("mesh::fillBuffer");

        GLenum err = glGetError();

        bufferUsed[dest] = true;
        glBindVertexArray(vao);

        if(dest == index)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffers[dest]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,size,data,GL_STATIC_DRAW);
            glBindVertexArray(0);

            err = glGetError();
            if(err != GL_NO_ERROR)
                error("(idx) OpenGL error: " + std::to_string(err));

            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER,buffers[dest]);
        glEnableVertexAttribArray(dest);
        glBufferData(GL_ARRAY_BUFFER,size,data,GL_STATIC_DRAW);
        glVertexAttribPointer(dest,elements,GL_FLOAT,GL_FALSE,0,0);
        glBindVertexArray(0);

        err = glGetError();
        if(err != GL_NO_ERROR)
            error("OpenGL error: " + std::to_string(err));
    }

    mesh::mesh()
    {
        glGenVertexArrays(1,&vao);
        glGenBuffers(9,buffers);
    }

    mesh::~mesh()
    {
        glDeleteVertexArrays(1,&vao);
        glDeleteBuffers(9,buffers);
    }

    std::map<std::string,int> aiProcessMap = {
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
    model::model()
    {

    }

    model::model(std::string filename)
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
        loadModel(modelPath,pFlags,&modelLoadFile);

        rootNode->transformCollisionMeshes(totalColMin,totalColMax);
    }

    std::string v3str(glm::vec3 in)
    {
        return std::to_string(in.x) + "," + std::to_string(in.y) + "," + std::to_string(in.z);
    }

    std::string q4str(glm::quat in)
    {
        return std::to_string(in.x) + "," + std::to_string(in.y) + "," + std::to_string(in.z) + ", w:" + std::to_string(in.w);
    }

    node::node(aiNode *source,model *_modelParent,int debugIndent)
    {
        _modelParent->nodes.push_back(this);
        modelType = _modelParent;
        name = source->mName.C_Str();
        CopyaiMat(source->mTransformation,transform);

        std::string indents = "";
        for(int a = 0; a<debugIndent; a++)
            indents += "\t";
        debug(indents + "Node " + name);

        //Debug only:
        glm::vec3 scale,skew,trans;
        glm::vec4 perspective;
        glm::quat rot;
        glm::decompose(transform,scale,rot,trans,skew,perspective);
        debug(indents + "Pos:  " + v3str(trans));
        debug(indents + "Quat: " + q4str(rot));

        for(int a = 0; a<source->mNumMeshes; a++)
        {
            debug(indents + "Mesh: " + modelType->meshes[source->mMeshes[a]]->name + " has " + std::to_string(_modelParent->meshes[source->mMeshes[a]]->renderSize) + " verts ");
            meshes.push_back(modelType->meshes[source->mMeshes[a]]);
        }

        for(int a = 0; a<source->mNumChildren; a++)
            children.push_back(new node(source->mChildren[a],_modelParent,debugIndent+1));
    }

    node::~node()
    {
        for(unsigned int a = 0; a<children.size(); a++)
            if(children[a])
                delete children[a];
    }

    node *model::findNode(std::string name,node *toSearch)
    {
        if(!toSearch)
            toSearch = rootNode;
        if(toSearch->name == name)
            return toSearch;

        node *ret = 0;
        for(unsigned int a = 0; a<toSearch->children.size(); a++)
        {
            if(toSearch->children[a])
                ret = findNode(name,toSearch->children[a]);
            if(ret)
                break;
        }
        return ret;
    }

    void model::loadModel(std::string filePath,unsigned int pFlags,preferenceFile *settings)
    {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(filePath,pFlags);

        if(!scene)
        {
            error("Problem loaded model file " + filePath);
            error(importer.GetErrorString());
            return;
        }

        loadModel(scene,filePath,pFlags,settings);
    }

    void model::loadModel(const aiScene *scene,std::string filePath,unsigned int pFlags,preferenceFile *settings)
    {
        debug(std::to_string(scene->mNumMaterials) + " materials found to load.");
        for(unsigned int a = 0; a<scene->mNumMaterials; a++)
        {
            aiMaterial *src = scene->mMaterials[a];
            debug("Loading material: " + std::string(src->GetName().C_Str()));

//            material *dest = new material(src,settings,getFolderFromPath(filePath));
            std::string name = src->GetName().C_Str();
            material *dest = new material(name,*settings);
            materials.push_back(dest);
        }

        debug(std::to_string(scene->mNumMeshes) + " meshes found to load.");
        for(unsigned int a = 0; a<scene->mNumMeshes; a++)
        {
            aiMesh *src = scene->mMeshes[a];
            mesh *dest = new mesh;
            dest->copyMeshData(src,materials[src->mMaterialIndex]);
            meshes.push_back(dest);
            dest->pickingID = numPickingIDs;
            numPickingIDs++;

            std::cout<<dest->name<<" has id: "<<numPickingIDs-1<<"\n";
        }

        //Recursive function
        rootNode = new node(scene->mRootNode,this);

        if(scene->HasCameras())
        {
            if(scene->mNumCameras > 0)
            {
                hasCamera = true;
                cameraPosition = glm::vec3(scene->mCameras[0]->mPosition.x,scene->mCameras[0]->mPosition.y,scene->mCameras[0]->mPosition.z);
                std::cout<<"Camera position: "<<cameraPosition.x<<","<<cameraPosition.y<<","<<cameraPosition.z<<"\n";
            }
        }
    }

    model::~model()
    {
        for(unsigned int a = 0; a<meshes.size(); a++)
            delete meshes[a];
        for(unsigned int a = 0; a<materials.size(); a++)
            delete materials[a];
        if(rootNode)
            delete rootNode;
    }

    glm::vec4 postovec4(glm::vec3 a){return glm::vec4(a.x,a.y,a.z,1.0);}

    glm::mat4 node::transformCollisionMeshes(glm::vec3 &totalColMin,glm::vec3 &totalColMax,glm::mat4 startTransform)
    {
        startTransform = startTransform * transform;

        for(int a = 0; a<meshes.size(); a++)
        {
            if(meshes[a]->detectedCollisionMesh)
            {
                glm::vec3 s = meshes[a]->detectedColMax - meshes[a]->detectedColMin;
                meshes[a]->detectedColMin = startTransform * postovec4(meshes[a]->detectedColMin);
                meshes[a]->detectedColMax = startTransform * postovec4(meshes[a]->detectedColMax);

                if(meshes[a]->detectedColMin.x > meshes[a]->detectedColMax.x)
                    std::swap(meshes[a]->detectedColMin.x,meshes[a]->detectedColMax.x);
                if(meshes[a]->detectedColMin.y > meshes[a]->detectedColMax.y)
                    std::swap(meshes[a]->detectedColMin.y,meshes[a]->detectedColMax.y);
                if(meshes[a]->detectedColMin.z > meshes[a]->detectedColMax.z)
                    std::swap(meshes[a]->detectedColMin.z,meshes[a]->detectedColMax.z);

                if(meshes[a]->detectedColMin.x < totalColMin.x)
                    totalColMin.x = meshes[a]->detectedColMin.x;
                if(meshes[a]->detectedColMin.y < totalColMin.y)
                    totalColMin.y = meshes[a]->detectedColMin.y;
                if(meshes[a]->detectedColMin.z < totalColMin.z)
                    totalColMin.z = meshes[a]->detectedColMin.z;

                if(meshes[a]->detectedColMax.x > totalColMax.x)
                    totalColMax.x = meshes[a]->detectedColMax.x;
                if(meshes[a]->detectedColMax.y > totalColMax.y)
                    totalColMax.y = meshes[a]->detectedColMax.y;
                if(meshes[a]->detectedColMax.z > totalColMax.z)
                    totalColMax.z = meshes[a]->detectedColMax.z;

                glm::vec3 scale; glm::quat rot; glm::vec3 trans; glm::vec4 per; glm::vec3 skew;
                glm::decompose(startTransform,scale,rot,trans,skew,per);
             }
        }

        for(int a = 0; a<children.size(); a++)
        {
            int numChildren = children.size();
            if(a >= numChildren)
                break;
            node *child = children[a];
            if(child)
                child->transformCollisionMeshes(totalColMin,totalColMax,startTransform);
        }
    }

    void node::render(uniformsHolder *graphics,glm::mat4 startTransform,bool skipMats,std::vector<glm::vec3> *nodeColors,texture *decal)
    {
        glUniform1i(graphics->isAnimated,0);
        startTransform = startTransform * transform;

        for(int a = 0; a<meshes.size(); a++)
        {
            graphics->setModelMatrix(startTransform);
            if(nodeColors)
            {
                if(meshes[a]->pickingID <= nodeColors->size())
                {
                    glm::vec3 color = nodeColors->at(meshes[a]->pickingID);
                    glUniform3f(graphics->target->getUniformLocation("nodeColorOld"),color.r,color.g,color.b);
                }
            }
            meshes[a]->render(graphics,skipMats,decal);
        }

        for(int a = 0; a<children.size(); a++)
            children[a]->render(graphics,startTransform,skipMats,nodeColors,decal);
    }

    void node::renderForPicking(uniformsHolder *graphics,glm::mat4 startTransform)
    {
        glUniform1i(graphics->isAnimated,0);
        startTransform = startTransform * transform;

        for(int a = 0; a<meshes.size(); a++)
        {
            graphics->setModelMatrix(startTransform);
            glUniform1i(graphics->target->getUniformLocation("pickingColor"),meshes[a]->pickingID);
            meshes[a]->render(graphics,true);
            /*meshes[a]->pickingID = modelType->numPickingIDs;
            modelType->numPickingIDs++;*/
        }

        for(int a = 0; a<children.size(); a++)
            children[a]->renderForPicking(graphics,startTransform);
    }

    void model::render(uniformsHolder *graphics,glm::mat4 startTransform,bool skipMats,std::vector<glm::vec3> *nodeColors,texture *decal)
    {
        if(nodeColors)
            glUniform1i(graphics->target->getUniformLocation("useNodeColor"),1);
        rootNode->render(graphics,startTransform,skipMats,nodeColors,decal);
        glUniform1i(graphics->target->getUniformLocation("useNodeColor"),0);
    }

    void model::renderForPicking(uniformsHolder *graphics,glm::mat4 startTransform)
    {
        //numPickingIDs = 0;
        glDisable(GL_BLEND);
        glUniform1i(graphics->target->getUniformLocation("usePickingColor"),1);
        rootNode->renderForPicking(graphics,startTransform);
        glUniform1i(graphics->target->getUniformLocation("usePickingColor"),0);
    }

}
