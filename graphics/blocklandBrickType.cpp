#include "blocklandBrickType.h"

glm::vec4 readNextVec4(std::fstream &file)
{
    std::string line = "";
    getline(file,line);
    if(line == "")
        getline(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 4)
        syj::error("Not enough coords in brick color");
    return glm::vec4(atof(words[0].c_str()),atof(words[1].c_str()),atof(words[2].c_str()),atof(words[3].c_str()));
}

glm::vec3 readNextVec3(std::fstream &file)
{
    std::string line = "";
    getline(file,line);
    if(line == "")
        getline(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 3)
        syj::error("Not enough coords in brick normal");
    return glm::vec3(atof(words[0].c_str()),atof(words[1].c_str()),atof(words[2].c_str()));
}

glm::vec2 readNextVec2(std::fstream &file)
{
    std::string line = "";
    getline(file,line);
    if(line == "")
        getline(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 2)
        syj::error("Not enough coords in brick uv");
    return glm::vec2(atof(words[0].c_str()),atof(words[1].c_str()));
}

glm::vec3 readNextVec2Tex(std::fstream &file,syj::defaultBrickTextures tex)
{
    std::string line = "";
    getline(file,line);
    if(line == "")
        getline(file,line);
    std::vector<std::string> words;
    split(line,words);
    if(words.size() < 2)
        syj::error("Not enough coords in brick uv");
    return glm::vec3(atof(words[0].c_str()),atof(words[1].c_str()),tex);
}

void sayVec3(glm::vec3 a){std::cout<<a.x<<","<<a.y<<","<<a.z<<"\n";}

namespace syj
{
    void specialBrickType::initModTerrain(std::string filePath)
    {
        std::ifstream file(filePath.c_str());
        if(!file.is_open())
        {
            error("Could not open mod terrain blb file " + filePath);
            return;
        }

        //btTriangleMesh *mesh = new btTriangleMesh(false,false);
        //std::vector<btVector3> verts;

        btConvexHullShape *modTerShape = new btConvexHullShape();

        btVector3 minDim(9999,9999,9999),maxDim(-9999,-9999,-9999);

        bool readingVerts = false;
        std::string line;
        std::vector<std::string> words;
        int vertsReadSincePositionLine = 0;
        while(!file.eof())
        {
            getline(file,line);

            if(!readingVerts)
            {
                if(line.find("POSITION:") != std::string::npos)
                    readingVerts = true;
            }
            else
            {
                if(line.find("POSITION:") != std::string::npos)
                {
                    error("Found POSITION: in mod terrain blb file before a full quad had been specified.");
                    continue;
                }

                /*if(verts.size() == 4)
                {
                    //mesh->addTriangle(verts[0],verts[1],verts[2]);
                    //mesh->addTriangle(verts[0],verts[2],verts[3]);
                    verts.clear();
                    readingVerts = false;
                    continue;
                }*/

                if(line.length() > 2)
                {
                    split(line,words);
                    if(words.size() == 3)
                    {
                        float x = atof(words[0].c_str());
                        float y = atof(words[1].c_str());
                        float z = atof(words[2].c_str());

                        std::swap(z,y);
                        y /= 2.5;

                        if(x > maxDim.x())
                            maxDim.setX(x);
                        if(y > maxDim.y())
                            maxDim.setY(y);
                        if(z > maxDim.z())
                            maxDim.setZ(z);

                        if(x < minDim.x())
                            minDim.setX(x);
                        if(y < minDim.y())
                            minDim.setY(y);
                        if(z < minDim.z())
                            minDim.setZ(z);

                        if(isnanf(x) || isnanf(y) || isnanf(z))
                        {
                            error("Invalid coordinate for mod terrain blb file " + filePath + " | " + line);
                            continue;
                        }

                        btVector3 vec = btVector3(x,y,z);

                        if(vec.length() > 100)
                        {
                            error("Invalid coordinate for mod terrain blb file " + filePath + " | " + line);
                            continue;
                        }

                        modTerShape->addPoint(vec,false);
                        //verts.push_back(vec);

                        vertsReadSincePositionLine++;
                        if(vertsReadSincePositionLine >= 4)
                        {
                            vertsReadSincePositionLine = 0;
                            readingVerts = false;
                        }
                    }
                    else
                    {
                        error(filePath + " line " + line + " was rejected as being malformed?");
                    }
                }
            }
        }

        modTerShape->recalcLocalAabb();
        shape = modTerShape;

        /*if(verts.size() != 0)
            error(blbFile + " finished blb file with incomplete quad!");*/

        //std::cout<<"Compiled mod ter brick type with "<<mesh->getNumTriangles()<<" triangles!\n";

        //modTerShape = new btBvhTriangleMeshShape(mesh,true);
        //modTerShape = new btConvexHullShape(&verts[0][0],verts.size());
        isModTerrain = true;

        btVector3 dim = maxDim - minDim;
        //std::cout<<"Dims: "<<dim.x()<<","<<dim.y()<<","<<dim.z()<<"\n";

        width = maxDim.x() - minDim.x();
        height = maxDim.y() - minDim.y();
        length = maxDim.z() - minDim.z();

        file.close();
    }

    specialBrickType::~specialBrickType()
    {
        if(customNormalBuffer)
            glDeleteBuffers(1,&buffers[customNormalBuffer]);
        if(customUVBuffer)
            glDeleteBuffers(1,&buffers[customUVBuffer]);
        if(customVertexColorBuffer)
            glDeleteBuffers(1,&buffers[customVertexColorBuffer]);
        if(customVertexBuffer)
            glDeleteBuffers(1,&buffers[customVertexBuffer]);
    }

    specialBrickType::specialBrickType(std::string blbFile)
    {
        bool shouldBeModTerr = blbFile.find("ModTer") != std::string::npos;

        std::fstream theFile(blbFile.c_str(),std::ios::in);
        if(!theFile.is_open())
        {
            error("Couldn't open " + blbFile);
            return;
        }

        fileName = lowercase(getFileFromPath(blbFile));

        std::string line = "";
        getline(theFile,line);
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

        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> uvsAndTex;
        std::vector<glm::vec3> norm;
        std::vector<glm::vec4> blbColor;

        float w = width;
        float l = length;
        float h = height;

        while(!theFile.eof())
        {
            getline(theFile,line);
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

                getline(theFile,line);
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
                    getline(theFile,line);
                    while(line.find("TEX") == std::string::npos)
                        getline(theFile,line);

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

                    getline(theFile,line);
                    while(line.find("POSITION") == std::string::npos)
                        getline(theFile,line);

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

                    getline(theFile,line);
                    while(line.find("UV COORDS") == std::string::npos)
                        getline(theFile,line);
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

                    getline(theFile,line);
                    while(line.find("COLOR") == std::string::npos && line.find("NORMAL") == std::string::npos)
                        getline(theFile,line);

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

                        getline(theFile,line);
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

        for(int a = 0; a<blbColor.size(); a++)
        {
            if(blbColor[a].a > 0.01 && blbColor[a].a < 0.99)
            {
                hasTransparency = true;
                break;
            }
        }

        initFromVectors(verts,norm,uvsAndTex,blbColor);

        theFile.close();

        if(!shouldBeModTerr)
            shape = new btBoxShape(btVector3(((float)w)/2.0,((float)h)/(2.0*2.5),((float)l)/2.0));
        else
            initModTerrain(blbFile);
    }

    void specialBrickType::initFromVectors(
                                         std::vector<glm::vec3> &vertPositions,
                                         std::vector<glm::vec3> &normals,
                                         std::vector<glm::vec3> &uvsAndTex,
                                         std::vector<glm::vec4> &vertColors)
    {
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
    }
}
