#include "terrain.h"

namespace syj
{
    terrain::terrain(std::string heightMapFilePath,btDynamicsWorld *world) : tessellation(heightMapFilePath)
    {
        mTriMesh = new btTriangleMesh();
        std::vector<glm::vec3> verts;
        std::ifstream preloadattempt(std::string(heightMapFilePath + ".bin"),std::ios::binary);
        if(preloadattempt.is_open() && false)
        {
            int size = 0;
            preloadattempt.read((char*)&size,sizeof(int));
            debug("Loading "+std::to_string(size) + " verts of heightmap data from precompiled binary.");
            for(int a = 0; a<size; a++)
            {
                glm::vec3 v;
                preloadattempt.read((char*)&v,sizeof(glm::vec3));
                verts.push_back(v);
            }

            preloadattempt.close();
        }
        else
        {
            bool isHdr = isHDR(heightMapFilePath.c_str());
            int tWidth,tHeight,tChannels;
            unsigned char *data = 0;

            if(false)
            {
                /*debug("File " + heightMapFilePath + " is HDR");
                data = loadImageF(heightMapFilePath.c_str(),tWidth,tHeight,tChannels,0);
                error("I haven't actually implemented loading physics for HDR heightmaps yet!");*/
            }
            else
            {
                //unsigned char *cdata = 0;
                debug("File " + heightMapFilePath + " is not HDR");
                //data = loadImageU(heightMapFilePath.c_str(),tWidth,tHeight,tChannels,0);
                LDRtextureResourceDescriptor *res = loadImageU(heightMapFilePath.c_str(),0);
                data = res->data;
                tWidth = res->width;
                tHeight = res->height;
                tChannels = res->channels;
                /*data = new float[tWidth*tHeight*tChannels];
                for(int a = 0; a<tWidth*tHeight*tChannels; a++)
                    data[a] = cdata[a];
                delete cdata;*/
            }

            glm::vec2 offsets[6] = {
                glm::vec2(0,0),
                glm::vec2(1,1),
                glm::vec2(1,0),
                glm::vec2(0,0),
                glm::vec2(1,1),
                glm::vec2(0,1)
            };

            for(float x = 0; x<tWidth-1; x++)
            {
                for(float y = 0; y<tHeight-1; y++)
                {
                    for(int a = 0; a<6; a++)
                    {
                        glm::vec2 uvs = glm::vec2((x+offsets[a].x)/((float)tWidth),(y+offsets[a].y)/((float)tHeight));
                        glm::vec3 loc;
                        loc.x = uvs.x * ((float)prefs.numPatchesX) * prefs.patchSizeX;
                        loc.z = uvs.y * ((float)prefs.numPatchesZ) * prefs.patchSizeZ;
                        glm::vec2 texCoord = glm::vec2(uvs.x * ((float)tWidth),uvs.y * ((float)tHeight));
                        int idx = floor(texCoord.x + texCoord.y * tWidth);
                        float red = data[idx * tChannels];
                        float green = data[1 + (idx * tChannels)];
                        float height = red/256.0 + green;
                        height /= 256.0;
                        height *= prefs.heightMapYScale;
                        height += 2.0;
                        //height += prefs.yOffset;
                        height -= 300;
                        glm::vec3 vert = glm::vec3(loc.x,height,loc.z);
                        verts.push_back(vert);
                    }
                }
            }

            delete data;

            std::ofstream hmresults(heightMapFilePath + ".bin",std::ios::binary);
            if(hmresults.is_open())
            {
                int size = verts.size();
                hmresults.write((char*)&size,sizeof(int));
                debug("Saving " + std::to_string(size) + " verts of data to precompiled heightmap binary");
                for(int a = 0; a<verts.size(); a++)
                {
                    glm::vec3 v = verts[a];
                    hmresults.write((char*)&v,sizeof(glm::vec3));
                }
                hmresults.close();
            }
            else
                error("Could not open " + heightMapFilePath + ".bin for exporting heightmap!");
        }

        for(int a = 0; a<verts.size(); a+=3)
        {
            mTriMesh->addTriangle(
                                btVector3(verts[a].x,verts[a].y,verts[a].z),
                                btVector3(verts[a+1].x,verts[a+1].y,verts[a+1].z),
                                btVector3(verts[a+2].x,verts[a+2].y,verts[a+2].z)
            );
        }

        shape = new btBvhTriangleMeshShape(mTriMesh,true);
        btTransform startTrans = btTransform::getIdentity();
        btMotionState* ms = new btDefaultMotionState(startTrans);
        shape->setMargin(1);
        btRigidBody::btRigidBodyConstructionInfo info(0,ms,shape,btVector3(0,0,0));
        body = new btRigidBody(info);
        body->setUserIndex(17);
        world->addRigidBody(body);
        std::cout<<"Old friction: "<<body->getFriction()<<"\n";
        body->setFriction(0.5);
    }
}
