#include "blocklandBrickType.h"

glm::vec4 readNextVec4(std::fstream &file)
{
    std::string line = "";
    lodGetLine(file,line);
    if(line == "")
        lodGetLine(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 4)
        syj::error("Not enough coords in brick color");
    return glm::vec4(atof(words[0].c_str()),atof(words[1].c_str()),atof(words[2].c_str()),atof(words[3].c_str()));
}

glm::vec3 readNextVec3(std::fstream &file)
{
    std::string line = "";
    lodGetLine(file,line);
    if(line == "")
        lodGetLine(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 3)
        syj::error("Not enough coords in brick normal");
    return glm::vec3(atof(words[0].c_str()),atof(words[1].c_str()),atof(words[2].c_str()));
}

glm::vec2 readNextVec2(std::fstream &file)
{
    std::string line = "";
    lodGetLine(file,line);
    if(line == "")
        lodGetLine(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 2)
        syj::error("Not enough coords in brick uv");
    return glm::vec2(atof(words[0].c_str()),atof(words[1].c_str()));
}

glm::vec3 readNextVec2Tex(std::fstream &file,syj::defaultBrickTextures tex)
{
    std::string line = "";
    lodGetLine(file,line);
    if(line == "")
        lodGetLine(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 2)
        syj::error("Not enough coords in brick uv");
    return glm::vec3(atof(words[0].c_str()),atof(words[1].c_str()),tex);
}

void sayVec3(glm::vec3 a){std::cout<<a.x<<","<<a.y<<","<<a.z<<"\n";}

namespace syj
{
    void specialBrickType::initModTerrain(std::vector<glm::vec3> &verts)
    {
        btConvexHullShape *modTerShape = new btConvexHullShape();

        for(int a = 0; a<verts.size(); a++)
        {
            if(isnanf(verts[a].x) || isnanf(verts[a].y) || isnanf(verts[a].z))
            {
                error("Invalid coordinate for mod terrain blb file ");
                continue;
            }

            btVector3 vec = btVector3(verts[a].x,verts[a].y,verts[a].z);

            /*if(vec.length() > 100)
            {
                error("Invalid coordinate for mod terrain blb file " + filePath + " | " + line);
                continue;
            }*/

            modTerShape->addPoint(vec,false);
        }

        modTerShape->recalcLocalAabb();
        shape = modTerShape;

        isModTerrain = true;
    }

    specialBrickType::~specialBrickType()
    {
        delete shape;
        if(buffers[customNormalBuffer])
            glDeleteBuffers(1,&buffers[customNormalBuffer]);
        if(buffers[customUVBuffer])
            glDeleteBuffers(1,&buffers[customUVBuffer]);
        if(buffers[customVertexColorBuffer])
            glDeleteBuffers(1,&buffers[customVertexColorBuffer]);
        if(buffers[customVertexBuffer])
            glDeleteBuffers(1,&buffers[customVertexBuffer]);
    }

    specialBrickType::specialBrickType(std::string blbFile,bool customMesh)
    {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> uvsAndTex;
        std::vector<glm::vec3> norm;
        std::vector<glm::vec4> blbColor;
        float w,l,h;
        bool shouldBeModTerr = customMesh;//blbFile.find("ModTer") != std::string::npos;

        if(blbFile.find(".blb") == std::string::npos)
        {
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(blbFile,0);

            if(!scene)
            {
                error(blbFile + " was not a valid model for a brick!");
                return;
            }

            if(scene->mNumMeshes == 0)
            {
                error("Brick model file " + blbFile + " had no meshes!");
                return;
            }

            if(scene->mNumMeshes > 1)
                error("Brick model file " + blbFile + " had multiple meshes, we will only use the first for visuals!");

            aiVector3D aabbMaxAI,aabbMinAI;

            aiMesh *src = scene->mMeshes[0];
            std::string name = src->mName.C_Str();
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
            aabbMaxAI = aiVector3D(maxX,maxY,maxZ);
            aabbMinAI = aiVector3D(minX,minY,minZ);

            w = ceil(aabbMaxAI.x - aabbMinAI.x);
            h = ceil(aabbMaxAI.y - aabbMinAI.y)*2.5;
            l = ceil(aabbMaxAI.z - aabbMinAI.z);

            width = w;
            height = h;
            length = l;

            for(unsigned int f = 0; f<src->mNumFaces; f++)
            {
                if(src->mFaces[f].mNumIndices != 3)
                {
                    error("Brick model file " + blbFile + " has non-triangulated faces!");
                    return;
                }

                verts.push_back(glm::vec3(src->mVertices[src->mFaces[f].mIndices[0]].x,src->mVertices[src->mFaces[f].mIndices[0]].y,src->mVertices[src->mFaces[f].mIndices[0]].z));
                verts.push_back(glm::vec3(src->mVertices[src->mFaces[f].mIndices[1]].x,src->mVertices[src->mFaces[f].mIndices[1]].y,src->mVertices[src->mFaces[f].mIndices[1]].z));
                verts.push_back(glm::vec3(src->mVertices[src->mFaces[f].mIndices[2]].x,src->mVertices[src->mFaces[f].mIndices[2]].y,src->mVertices[src->mFaces[f].mIndices[2]].z));

                uvsAndTex.push_back(glm::vec3(src->mTextureCoords[0][src->mFaces[f].mIndices[0]].x,src->mTextureCoords[0][src->mFaces[f].mIndices[0]].y,BRICKTEX_STUDS));
                uvsAndTex.push_back(glm::vec3(src->mTextureCoords[0][src->mFaces[f].mIndices[1]].x,src->mTextureCoords[0][src->mFaces[f].mIndices[1]].y,BRICKTEX_STUDS));
                uvsAndTex.push_back(glm::vec3(src->mTextureCoords[0][src->mFaces[f].mIndices[2]].x,src->mTextureCoords[0][src->mFaces[f].mIndices[2]].y,BRICKTEX_STUDS));

                norm.push_back(glm::vec3(src->mNormals[src->mFaces[f].mIndices[0]].x,src->mNormals[src->mFaces[f].mIndices[0]].y,src->mNormals[src->mFaces[f].mIndices[0]].z));
                norm.push_back(glm::vec3(src->mNormals[src->mFaces[f].mIndices[1]].x,src->mNormals[src->mFaces[f].mIndices[1]].y,src->mNormals[src->mFaces[f].mIndices[1]].z));
                norm.push_back(glm::vec3(src->mNormals[src->mFaces[f].mIndices[2]].x,src->mNormals[src->mFaces[f].mIndices[2]].y,src->mNormals[src->mFaces[f].mIndices[2]].z));

                blbColor.push_back(glm::vec4(0,0,0,0));
                blbColor.push_back(glm::vec4(0,0,0,0));
                blbColor.push_back(glm::vec4(0,0,0,0));
            }

            for(int i  = 0; i<verts.size(); i++)
                verts[i].y -= (h/5);
        }
        else
        {
            std::fstream theFile(blbFile.c_str(),std::ios::in);
            if(!theFile.is_open())
            {
                error("Couldn't open " + blbFile);
                return;
            }

            fileName = lowercase(getFileFromPath(blbFile));

            std::string line = "";
            lodGetLine(theFile,line);
            std::vector<std::string> words;
            split(line,words);

            if(words.size() < 3)
            {
                error("Could not find dimensions of brick! " + blbFile);
                return;
            }

            width = atoi(words[0].c_str());
            height = atoi(words[2].c_str());
            length = atoi(words[1].c_str());

            w = width;
            l = length;
            h = height;

            while(!theFile.eof())
            {
                lodGetLine(theFile,line);
                std::transform(line.begin(), line.end(), line.begin(), ::tolower);

                if(line.find("quads") != std::string::npos)
                {
                    faceDirection side;
                    if(line.find("top") != std::string::npos)
                        side = FACE_UP;
                    if(line.find("bottom") != std::string::npos)
                        side = FACE_DOWN;
                    if(line.find("north") != std::string::npos)
                        side = FACE_NORTH;
                    if(line.find("south") != std::string::npos)
                        side = FACE_SOUTH;
                    if(line.find("east") != std::string::npos)
                        side = FACE_EAST;
                    if(line.find("west") != std::string::npos)
                        side = FACE_WEST;
                    if(line.find("omni") != std::string::npos)
                        side = FACE_OMNI;

                    lodGetLine(theFile,line);
                    unsigned int numQuads = atoi(line.c_str());
                    if(numQuads > 1000)
                    {
                        error("You probably didn't mean for "+blbFile+" to have over 1000 quads on one face!" + " " + blbFile);
                        continue;
                    }
                    else if(numQuads < 1)
                    {
                        continue;
                    }

                    defaultBrickTextures current = BRICKTEX_SIDES;

                    for(unsigned int a = 0; a<numQuads; a++)
                    {
                        lodGetLine(theFile,line);
                        while(line.find("TEX") == std::string::npos)
                            lodGetLine(theFile,line);

                        if(line == "TEX:RAMP")
                            current = BRICKTEX_RAMP;
                        else if(line == "TEX:SIDE")
                            current = BRICKTEX_SIDES;
                        else if(line.find("TEX:PRINT") != std::string::npos)
                        {
                            current = BRICKTEX_PRINT;
                            hasPrint = true;
                        }
                        else if(line == "TEX:BOTTOMLOOP")
                            current = BRICKTEX_BOTTOM;
                        else if(line == "TEX:BOTTOMEDGE")
                            current = BRICKTEX_BOTTOM;
                        else if(line == "TEX:TOP")
                            current = BRICKTEX_STUDS;
                        else
                            error("Expected texture for brick face, got instead: "+line + " " + blbFile);

                        lodGetLine(theFile,line);
                        while(line.find("POSITION") == std::string::npos)
                            lodGetLine(theFile,line);

                        if(line.find("POSITION:") == std::string::npos)
                            error("Expected position, got instead: "+line + " " + blbFile);

                        glm::vec3 posA = readNextVec3(theFile);
                        glm::vec3 posB = readNextVec3(theFile);
                        glm::vec3 posC = readNextVec3(theFile);
                        glm::vec3 posD = readNextVec3(theFile);

                        verts.push_back(posA);
                        verts.push_back(posB);
                        verts.push_back(posC);
                        verts.push_back(posA);
                        verts.push_back(posC);
                        verts.push_back(posD);

                        lodGetLine(theFile,line);
                        while(line.find("UV COORDS") == std::string::npos)
                            lodGetLine(theFile,line);
                        if(line.find("UV COORDS") == std::string::npos)
                            error("Expected uv, got instead: " + line + " " + blbFile);

                        glm::vec3 uvA = readNextVec2Tex(theFile,current);
                        glm::vec3 uvB = readNextVec2Tex(theFile,current);
                        glm::vec3 uvC = readNextVec2Tex(theFile,current);
                        glm::vec3 uvD = readNextVec2Tex(theFile,current);

                        /*if(blbFile.find("ModTer") != std::string::npos)
                        {
                            std::cout<<"Mod ter uv:\n";
                            sayVec3(uvA);
                            sayVec3(uvB);
                            sayVec3(uvC);
                            sayVec3(uvD);
                            std::cout<<"\n";
                        }*/

                        uvsAndTex.push_back(uvA);
                        uvsAndTex.push_back(uvB);
                        uvsAndTex.push_back(uvC);
                        uvsAndTex.push_back(uvA);
                        uvsAndTex.push_back(uvC);
                        uvsAndTex.push_back(uvD);

                        lodGetLine(theFile,line);
                        while(line.find("COLOR") == std::string::npos && line.find("NORMAL") == std::string::npos)
                            lodGetLine(theFile,line);

                        if(line == "COLORS:")
                        {
                            glm::vec4 colorA = readNextVec4(theFile);
                            glm::vec4 colorB = readNextVec4(theFile);
                            glm::vec4 colorC = readNextVec4(theFile);
                            glm::vec4 colorD = readNextVec4(theFile);

                            blbColor.push_back(colorA);
                            blbColor.push_back(colorB);
                            blbColor.push_back(colorC);
                            blbColor.push_back(colorA);
                            blbColor.push_back(colorC);
                            blbColor.push_back(colorD);

                            lodGetLine(theFile,line);
                            if(line != "NORMALS:")
                            {
                                error("Expected normals for brick but no NORMALS: line after colors " + blbFile);
                            }
                        }
                        else if(line.find("NORMALS:") == std::string::npos)
                        {
                            error("Expected normals for brick but no NORMALS: line after uvs " + blbFile);
                            for(int a = 0; a<6; a++)
                                blbColor.push_back(glm::vec4(0,0,0,0));
                        }
                        else
                        {
                            for(int a = 0; a<6; a++)
                                blbColor.push_back(glm::vec4(0,0,0,0));
                        }

                        glm::vec3 normA = readNextVec3(theFile);
                        glm::vec3 normB = readNextVec3(theFile);
                        glm::vec3 normC = readNextVec3(theFile);
                        glm::vec3 normD = readNextVec3(theFile);

                        norm.push_back(normA);
                        norm.push_back(normB);
                        norm.push_back(normC);
                        norm.push_back(normA);
                        norm.push_back(normC);
                        norm.push_back(normD);
                    }
                }
            }

            for(unsigned int a = 0; a<verts.size(); a++)
            {
                std::swap(verts[a].z,verts[a].y);
                std::swap(norm[a].z,norm[a].y);

                verts[a].y /= 2.5;
            }

            theFile.close();
        }

        for(int a = 0; a<blbColor.size(); a++)
        {
            if(blbColor[a].a > 0.01 && blbColor[a].a < 0.99)
            {
                hasTransparency = true;
                break;
            }
        }

        initFromVectors(verts,norm,uvsAndTex,blbColor);

        if(!shouldBeModTerr)
            shape = new btBoxShape(btVector3(((float)w)/2.0,((float)h)/(2.0*2.5),((float)l)/2.0));
        else
            initModTerrain(verts);
    }

    void specialBrickType::initFromVectors(
                                         std::vector<glm::vec3> &vertPositions,
                                         std::vector<glm::vec3> &normals,
                                         std::vector<glm::vec3> &uvsAndTex,
                                         std::vector<glm::vec4> &vertColors)
    {
        glBindVertexArray(0);

        //Dimension and direction are used for drawing basic bricks dynamically, they are not relevant here
        //Per vertex buffers:
        glGenBuffers(1,&buffers[customNormalBuffer]);
        glGenBuffers(1,&buffers[customUVBuffer]);
        glGenBuffers(1,&buffers[customVertexColorBuffer]);
        glGenBuffers(1,&buffers[customVertexBuffer]);

        //Per vertex positions:
        glBindBuffer(GL_ARRAY_BUFFER,buffers[customVertexBuffer]);
        glBufferData(GL_ARRAY_BUFFER,vertPositions.size() * sizeof(glm::vec3),&vertPositions[0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(BRICKLAYOUT_VERTS);
        glVertexAttribDivisor(BRICKLAYOUT_VERTS,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_VERTS,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        //Per vertex uvs:
        glBindBuffer(GL_ARRAY_BUFFER,buffers[customUVBuffer]);
        glBufferData(GL_ARRAY_BUFFER,uvsAndTex.size() * sizeof(glm::vec3),&uvsAndTex[0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(BRICKLAYOUT_UV);
        glVertexAttribDivisor(BRICKLAYOUT_UV,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_UV,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        //Per vertex normals:
        glBindBuffer(GL_ARRAY_BUFFER,buffers[customNormalBuffer]);
        glBufferData(GL_ARRAY_BUFFER,normals.size() * sizeof(glm::vec3),&normals[0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(BRICKLAYOUT_NORMAL);
        glVertexAttribDivisor(BRICKLAYOUT_NORMAL,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_NORMAL,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        //Per vertex color:
        glBindBuffer(GL_ARRAY_BUFFER,buffers[customVertexColorBuffer]);
        glBufferData(GL_ARRAY_BUFFER,vertColors.size() * sizeof(glm::vec4),&vertColors[0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(BRICKLAYOUT_VERTEXCOLOR);
        glVertexAttribDivisor(BRICKLAYOUT_VERTEXCOLOR,0);                //0 = same for every brick
        glVertexAttribPointer(BRICKLAYOUT_VERTEXCOLOR,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glVertexAttrib4f(BRICKLAYOUT_VERTEXCOLOR,0,0,0,0);              //Sets default color if we don't have any

        vertexCount = vertPositions.size();

        glBindBuffer(GL_ARRAY_BUFFER,0);
    }
}
