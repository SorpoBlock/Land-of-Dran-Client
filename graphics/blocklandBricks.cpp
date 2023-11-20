#include "blocklandBricks.h"

namespace syj
{
    void blocklandCompatibility::addSpecialBrickType(std::string filePath,CEGUI::Window *brickSelector,int serverID)
    {
        std::string name = filePath.substr(0,filePath.length()-4);
        std::string pngFile = filePath.substr(0,filePath.length()-3) + "png";

        specialBrickType *tmp = new specialBrickType(filePath);
        specialBrickTypes.push_back(tmp);
        tmp->serverID = serverID;

        if(doesFileExist(pngFile))
            addSpecialBrickToSelector(brickSelector,pngFile,name,specialBrickTypes.size() - 1);
        else
        {
            replaceAll(pngFile,"Bricks/","BrickIcons/");
            replaceAll(pngFile,"Bricks\\","BrickIcons\\");
            if(doesFileExist(pngFile))
                addSpecialBrickToSelector(brickSelector,pngFile,name,specialBrickTypes.size() - 1);
            else
                addSpecialBrickToSelector(brickSelector,"assets/brick/types/Unknown.png",name,specialBrickTypes.size() - 1);
        }
    }

    blocklandCompatibility::~blocklandCompatibility()
    {
        for(int a = 0; a<specialBrickTypes.size(); a++)
            delete specialBrickTypes[a];
        for(int a = 0; a<printTypes.size(); a++)
            delete printTypes[a];
    }

    specialBrickTypeInstanceHolder::~specialBrickTypeInstanceHolder()
    {
        glDeleteVertexArrays(1,&vao);
        glDeleteBuffers(1,&buffers[positionMatBuffer]);
        glDeleteBuffers(1,&buffers[rotationBuffer]);
        glDeleteBuffers(1,&buffers[paintColorBuffer]);
    }

    specialBrickTypeInstanceHolder::specialBrickTypeInstanceHolder(specialBrickType *based) : type(based)
    {
        glGenVertexArrays(1,&vao);

        //Per brick buffers:
        glGenBuffers(1,&buffers[positionMatBuffer]);
        glGenBuffers(1,&buffers[rotationBuffer]);
        glGenBuffers(1,&buffers[paintColorBuffer]);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER,type->buffers[customVertexBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_VERTS);
        glVertexAttribDivisor(BRICKLAYOUT_VERTS,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_VERTS,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindBuffer(GL_ARRAY_BUFFER,type->buffers[customUVBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_UV);
        glVertexAttribDivisor(BRICKLAYOUT_UV,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_UV,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindBuffer(GL_ARRAY_BUFFER,type->buffers[customNormalBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_NORMAL);
        glVertexAttribDivisor(BRICKLAYOUT_NORMAL,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_NORMAL,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindBuffer(GL_ARRAY_BUFFER,type->buffers[customVertexColorBuffer]);
        glEnableVertexAttribArray(BRICKLAYOUT_VERTEXCOLOR);
        glVertexAttribDivisor(BRICKLAYOUT_VERTEXCOLOR,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_VERTEXCOLOR,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttrib4f(BRICKLAYOUT_VERTEXCOLOR,0,0,0,0);              //Sets default color if we don't have any

        //Per-brick position and material:
        glEnableVertexAttribArray(BRICKLAYOUT_POSITIONMAT);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[positionMatBuffer]);
        glVertexAttribPointer(BRICKLAYOUT_POSITIONMAT,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_POSITIONMAT,1);                     //1 == different for each brick

        //Per-brick rotation:
        glEnableVertexAttribArray(BRICKLAYOUT_ROTATION);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[rotationBuffer]);
        glVertexAttribPointer(BRICKLAYOUT_ROTATION,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_ROTATION,1);                     //1 == different for each brick

        //Per-brick color:
        glEnableVertexAttribArray(BRICKLAYOUT_PAINTCOLOR);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[paintColorBuffer]);
        glVertexAttribPointer(BRICKLAYOUT_PAINTCOLOR,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttribDivisor(BRICKLAYOUT_PAINTCOLOR,1);                     //1 == different for each brick

        glBindVertexArray(0);
    }

    void specialBrickTypeInstanceHolder::update(specialBrickRenderData *theBrick)
    {
        int offset = theBrick->brickOffsets;
        if(offset == -1 || offset >= instances.size())
        {
            std::cout<<"Invalid instance to update for special brick type, offset: "<<offset<<" size of vector: "<<instances.size()<<"\n";
            return;
        }
        glm::vec4 posAndMat = glm::vec4(theBrick->position.x,theBrick->position.y,theBrick->position.z,theBrick->material);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[positionMatBuffer]);
        glBufferSubData(GL_ARRAY_BUFFER,offset * sizeof(glm::vec4),sizeof(glm::vec4),&posAndMat);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[rotationBuffer]);
        glBufferSubData(GL_ARRAY_BUFFER,offset * sizeof(glm::quat),sizeof(glm::quat),&theBrick->rotation);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[paintColorBuffer]);
        glBufferSubData(GL_ARRAY_BUFFER,offset * sizeof(glm::vec4),sizeof(glm::vec4),&theBrick->color);
    }

    void specialBrickTypeInstanceHolder::recompileInstances()
    {
        glBindVertexArray(vao);

        std::vector<glm::vec4> positionAndMat;
        std::vector<glm::quat> rotation;
        std::vector<glm::vec4> paintColor;

        for(unsigned int a = 0; a<instances.size(); a++)
        {
            specialBrickRenderData *tmp = instances[a];

            tmp->brickOffsets = a;
            positionAndMat.push_back(glm::vec4(tmp->position.x,tmp->position.y,tmp->position.z,tmp->material));
            rotation.push_back(tmp->rotation);
            paintColor.push_back(tmp->color);
        }

        glBindBuffer(GL_ARRAY_BUFFER,buffers[positionMatBuffer]);
        glBufferData(GL_ARRAY_BUFFER,positionAndMat.size() * sizeof(glm::vec4),&positionAndMat[0],GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[rotationBuffer]);
        glBufferData(GL_ARRAY_BUFFER,rotation.size() * sizeof(glm::quat),&rotation[0],GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[paintColorBuffer]);
        glBufferData(GL_ARRAY_BUFFER,paintColor.size() * sizeof(glm::vec4),&paintColor[0],GL_STREAM_DRAW);

        numCompiledInstances = positionAndMat.size();

        glBindVertexArray(0);
    }

    blocklandCompatibility::blocklandCompatibility(std::string aliasFile,std::string searchPath,CEGUI::Window *brickSelector,bool onlyBasic)
    {
        int startTime = SDL_GetTicks();

        printAlias *print1x1 = new printAlias;
        print1x1->dbName = "1x1print";
        print1x1->uiName = "1x1 print";
        print1x1->width = 1;
        print1x1->height = 3;
        print1x1->length = 1;
        print1x1->addFace(FACE_SOUTH);
        printTypes.push_back(print1x1);

        printAlias *print1x4x4 = new printAlias;
        print1x4x4->dbName = "1x4x4print";
        print1x4x4->uiName = "1x4x4 print";
        print1x4x4->width = 4;
        print1x4x4->height = 12;
        print1x4x4->length = 1;
        print1x4x4->addFace(FACE_SOUTH);
        printTypes.push_back(print1x4x4);

        printAlias *print2x2f = new printAlias;
        print2x2f->dbName = "2x2f print";
        print2x2f->uiName = "2x2fprint";
        print2x2f->width = 2;
        print2x2f->height = 1;
        print2x2f->length = 2;
        print2x2f->addFace(FACE_UP);
        printTypes.push_back(print2x2f);

        printAlias *print1x2f = new printAlias;
        print1x2f->dbName = "1x2f print";
        print1x2f->uiName = "1x2fprint";
        print1x2f->width = 1;
        print1x2f->height = 1;
        print1x2f->length = 2;
        print1x2f->addFace(FACE_UP);
        printTypes.push_back(print1x2f);

        printAlias *print1x1f = new printAlias;
        print1x1f->dbName = "1x1f print";
        print1x1f->uiName = "1x1fprint";
        print1x1f->width = 1;
        print1x1f->height = 1;
        print1x1f->length = 1;
        print1x1f->addFace(FACE_UP);
        printTypes.push_back(print1x1f);

        std::ifstream f(aliasFile.c_str());

        if(!f.is_open())
        {
            error("Could not open aliases!");
            return;
        }

        std::string line = "";
        while(!f.eof())
        {
            getline(f,line);
            int barPos = line.find("|");
            if(barPos == std::string::npos)
                continue;
            std::string key = line.substr(0,barPos);
            std::string val = line.substr(barPos+1,line.length() - (barPos+1));
            blocklandDatablockNames.push_back(lowercase(key));
            blocklandUINames.push_back(lowercase(val));
            /*blocklandDimensions.push_back(glm::vec3(0,0,0));
            collisionShapes.push_back(0);*/
        }
        basicTypes.resize(blocklandUINames.size());
        f.close();

        for (recursive_directory_iterator i(searchPath.c_str()), end; i != end; ++i)
        {
            if (!is_directory(i->path()))
            {
                if(i->path().extension() == ".blb")
                {
                    std::ifstream blb(i->path());
                    if(!blb.is_open())
                    {
                        error("Cannot open " + i->path().string());
                        continue;
                    }
                    std::string dims = "";
                    getline(blb,dims);
                    std::vector<std::string> words;
                    split(dims,words);
                    getline(blb,dims);
                    blb.close();

                    std::string fname = lowercase(i->path().stem().string());

                    float x = atoi(words[0].c_str());
                    float y = atoi(words[2].c_str());
                    float z = atoi(words[1].c_str());

                    if(dims == "BRICK")
                    {
                        int selectorIdx = -1;

                        if(brickSelector)
                        {
                            std::string blbName = i->path().string();
                            blbName = blbName.substr(0,blbName.length() - 3) + "png";
                            if(doesFileExist(blbName))
                                selectorIdx = addBasicBrickToSelector(brickSelector,blbName,x,y,z);
                            else
                                selectorIdx = addBasicBrickToSelector(brickSelector,"assets/brick/types/Unknown.png",x,y,z);
                        }


                        bool needSearchDB = true;
                        for(int a = 0; a<blocklandUINames.size(); a++)
                        {
                            if(blocklandUINames[a] == fname)
                            {
                                basicTypes[a].init(x,y,z);
                                needSearchDB = false;
                                break;
                            }
                        }
                        if(needSearchDB)
                        {
                            for(int a = 0; a<blocklandDatablockNames.size(); a++)
                            {
                                if(blocklandDatablockNames[a] == fname)
                                {
                                    basicTypes[a].init(x,y,z,selectorIdx);
                                    break;
                                }
                            }
                        }
                    }
                    else if(dims == "SPECIAL" || dims == "SPECIALBRICK")
                    {
                        if(onlyBasic)
                            continue;

                        specialBrickType *tmp = new specialBrickType(i->path().string());
                        //tmp->fileName = fname;
                        /*for(int a = 0; a<blocklandUINames.size(); a++)
                        {
                            std::cout<<"DB: "<<blocklandDatablockNames[a]<<" UIName: "<<blocklandUINames[a]<<"\n";
                        }*/

                        specialBrickTypes.push_back(tmp);

                        if(brickSelector)
                        {
                            std::string blbName = i->path().string();
                            blbName = blbName.substr(0,blbName.length() - 3) + "png";
                            if(doesFileExist(blbName))
                            {
                                addSpecialBrickToSelector(brickSelector,blbName,i->path().stem().string(),specialBrickTypes.size() - 1);
                            }
                            else
                                addSpecialBrickToSelector(brickSelector,"assets/brick/types/Unknown.png",i->path().stem().string(),specialBrickTypes.size() - 1);
                        }
                        continue;
                    }
                    else
                        error("blb file neither BRICK nor SPECIAL: " + dims + " File: " + i->path().string());
                }
            }
        }

        info("Loaded " + std::to_string(specialBrickTypes.size()) + " special types and " + std::to_string(basicTypes.size()) + " normal types in " + std::to_string(SDL_GetTicks()-startTime) + "ms");
        std::cout<<&specialBrickTypes<<" vector pointer! this: "<<this<<"\n";
    }
}
