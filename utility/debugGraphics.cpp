#include "code/utility/debugGraphics.h"

namespace syj
{
    glm::vec3 multVec3ByMat(glm::mat4 rotation,glm::vec3 point)
    {
        glm::vec4 homo;
        homo.x = point.x;
        homo.y = point.y;
        homo.z = point.z;
        homo.w = 1;
        homo = rotation * homo;
        return glm::vec3(homo.x,homo.y,homo.z);
    }

    glm::vec3 multVec3ByMat(glm::quat rotation,glm::vec3 point)
    {
        return multVec3ByMat(glm::toMat4(rotation),point);
    }

    RgbColor HsvToRgb(HsvColor hsv)
    {
        RgbColor rgb;
        unsigned char region, remainder, p, q, t;

        if (hsv.s == 0)
        {
            rgb.r = hsv.v;
            rgb.g = hsv.v;
            rgb.b = hsv.v;
            return rgb;
        }

        region = hsv.h / 43;
        remainder = (hsv.h - (region * 43)) * 6;

        p = (hsv.v * (255 - hsv.s)) >> 8;
        q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
        t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

        switch (region)
        {
            case 0:
                rgb.r = hsv.v; rgb.g = t; rgb.b = p;
                break;
            case 1:
                rgb.r = q; rgb.g = hsv.v; rgb.b = p;
                break;
            case 2:
                rgb.r = p; rgb.g = hsv.v; rgb.b = t;
                break;
            case 3:
                rgb.r = p; rgb.g = q; rgb.b = hsv.v;
                break;
            case 4:
                rgb.r = t; rgb.g = p; rgb.b = hsv.v;
                break;
            default:
                rgb.r = hsv.v; rgb.g = p; rgb.b = q;
                break;
        }

        return rgb;
    }

    HsvColor RgbToHsv(RgbColor rgb)
    {
        HsvColor hsv;
        unsigned char rgbMin, rgbMax;

        rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
        rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

        hsv.v = rgbMax;
        if (hsv.v == 0)
        {
            hsv.h = 0;
            hsv.s = 0;
            return hsv;
        }

        hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
        if (hsv.s == 0)
        {
            hsv.h = 0;
            return hsv;
        }

        if (rgbMax == rgb.r)
            hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
        else if (rgbMax == rgb.g)
            hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
        else
            hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

        return hsv;
    }

    void sayVec3(btVector3 a)
    {
        std::cout<<a.x()<<","<<a.y()<<","<<a.z()<<"\n";
    }

    glm::vec3 BtToGlm(btVector3 in)
    {
        return glm::vec3(in.x(),in.y(),in.z());
    }

    btVector3 glmToBt(glm::vec3 in)
    {
        return btVector3(in.x,in.y,in.z);
    }

    GLuint createBoxEdgesVAO()
    {
        std::vector<glm::vec3> verts;
        //Upper side
        verts.push_back(glm::vec3(0,1,0));
        verts.push_back(glm::vec3(1,1,0));

        verts.push_back(glm::vec3(1,1,0));
        verts.push_back(glm::vec3(1,1,1));

        verts.push_back(glm::vec3(1,1,1));
        verts.push_back(glm::vec3(0,1,1));

        verts.push_back(glm::vec3(0,1,1));
        verts.push_back(glm::vec3(0,1,0));

        //Lower side
        verts.push_back(glm::vec3(0,0,0));
        verts.push_back(glm::vec3(1,0,0));

        verts.push_back(glm::vec3(1,0,0));
        verts.push_back(glm::vec3(1,0,1));

        verts.push_back(glm::vec3(1,0,1));
        verts.push_back(glm::vec3(0,0,1));

        verts.push_back(glm::vec3(0,0,1));
        verts.push_back(glm::vec3(0,0,0));

        //The sides:
        verts.push_back(glm::vec3(0,0,0));
        verts.push_back(glm::vec3(0,1,0));

        verts.push_back(glm::vec3(1,0,0));
        verts.push_back(glm::vec3(1,1,0));

        verts.push_back(glm::vec3(0,0,1));
        verts.push_back(glm::vec3(0,1,1));

        verts.push_back(glm::vec3(1,0,1));
        verts.push_back(glm::vec3(1,1,1));

        GLuint vao;
        glGenVertexArrays(1,&vao);

        GLuint buffer;
        glGenBuffers(1,&buffer);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,buffer);

        glEnableVertexAttribArray(positions);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), &verts[0], GL_STATIC_DRAW);
        glVertexAttribPointer(
                positions,                  // attribute, 0 = verticies
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        glBindVertexArray(0);
        return vao;
    }

    void storeTransform(glm::vec3 pos,glm::vec3 dir)
    {
        std::ofstream out("debugTransform.txt");
        out<<pos.x<<"\n";
        out<<pos.y<<"\n";
        out<<pos.z<<"\n";
        out<<dir.x<<"\n";
        out<<dir.y<<"\n";
        out<<dir.z<<"\n";
        out.close();
    }

    void loadTransform(glm::vec3 &pos,glm::vec3 &dir)
    {
        std::ifstream in("debugTransform.txt");
        std::string line = "";
        lodGetLine(in,line);
        pos.x = atof(line.c_str());
        lodGetLine(in,line);
        pos.y = atof(line.c_str());
        lodGetLine(in,line);
        pos.z = atof(line.c_str());
        lodGetLine(in,line);
        dir.x = atof(line.c_str());
        lodGetLine(in,line);
        dir.y = atof(line.c_str());
        lodGetLine(in,line);
        dir.z = atof(line.c_str());
        in.close();
    }

    void drawDebugLocations(uniformsHolder &unis,GLuint cubeVAO,std::vector<glm::vec3> debugLocations,std::vector<glm::vec3> debugColors)
    {
        if(debugLocations.size() < 1)
            return;

        glDisable(GL_CULL_FACE);
        glUniform1i(unis.target->getUniformLocation("drawingDebugLocations"),true);
        glBindVertexArray(cubeVAO);
        for(int a = 0; a<debugLocations.size(); a++)
        {
            glUniform3vec(unis.target->getUniformLocation("debugLoc"),debugLocations[a]);
            glUniform3vec(unis.target->getUniformLocation("debugColor"),debugColors[a]);
            glDrawArrays(GL_TRIANGLES,0,36);
        }
        glBindVertexArray(0);
        glUniform1i(unis.target->getUniformLocation("drawingDebugLocations"),false);
        glEnable(GL_CULL_FACE);
    }

    double drand(double min,double max)
    {
        double ret = rand() % RAND_MAX;
        ret /= ((double)RAND_MAX);
        ret *= max - min;
        ret += min;
        return ret;
    }

    glm::vec3 computeFaceNormal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
    {
        // Uses p2 as a new origin for p1,p3
        auto a = p3 - p2;
        auto b = p1 - p2;
        // Compute the cross product a X b to get the face normal
        return glm::normalize(glm::cross(a, b));
    }

    std::vector<glm::vec3> calculateNormals(std::vector<glm::vec3> &verts)
    {
        std::vector<glm::vec3> ret;

        scope("calculateNormals");
        if(verts.size() % 3 != 0)
        {
            error("Verts not divisible by 3");
            return ret;
        }

        for(unsigned int a = 0; a<verts.size(); a+=3)
        {
            glm::vec3 norm = computeFaceNormal(verts[a],verts[a+1],verts[a+2]);
            ret.push_back(norm);
            ret.push_back(norm);
            ret.push_back(norm);
        }

        return ret;
    }

    void printComputerStats()
    {
        scope("PrintComputerStats");

        debug("Number of displays: " + std::to_string(SDL_GetNumVideoDisplays()));
        for(int i = 0; i<SDL_GetNumVideoDisplays(); i++)
        {
            SDL_DisplayMode cur;
            if(SDL_GetCurrentDisplayMode(i,&cur) != 0)
                debug("SDL_GetCurrentDisplayMode failed: " + std::string(SDL_GetError()));
            else
                debug("Display " + std::to_string(i) + " Resolution: " +
                    std::to_string(cur.w) + "," + std::to_string(cur.h) + " Refresh: " + std::to_string(cur.refresh_rate) +
                    " Format: " + std::to_string(cur.format));
        }

        debug("GPU information:");
        debug((char*)glGetString(GL_VENDOR));
        debug((char*)glGetString(GL_RENDERER));
        debug((char*)glGetString(GL_VERSION));
        int extensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
        debug(std::to_string(extensions) + " available extensions:");
        extensions = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,&extensions);
        debug(std::to_string(extensions) + " maximum textures per stage.");
        extensions = 0;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,&extensions);
        debug(std::to_string(extensions) + " maximum textures.");


        debug(std::to_string(SDL_GetSystemRAM()) + "MB RAM installed.");


        debug("CPU Information:");
        debug("Cache size: " + std::to_string(SDL_GetCPUCacheLineSize()));
        debug("Number of cores: " + std::to_string(SDL_GetCPUCount()));
    }

    GLuint createQuadVAO()
    {
        GLuint debugTriangleVAO;

        glGenVertexArrays(1,&debugTriangleVAO);
        glBindVertexArray(debugTriangleVAO);

        std::vector<glm::vec3> verts;
        verts.clear();
        verts.push_back(glm::vec3(-1,-1,0));
        verts.push_back(glm::vec3(1,-1,0));
        verts.push_back(glm::vec3(1,1,0));

        verts.push_back(glm::vec3(1,1,0));
        verts.push_back(glm::vec3(-1,1,0));
        verts.push_back(glm::vec3(-1,-1,0));

        std::vector<glm::vec2> uv;
        uv.clear();
        uv.push_back(glm::vec2(0,0));
        uv.push_back(glm::vec2(1,0));
        uv.push_back(glm::vec2(1,1));

        uv.push_back(glm::vec2(1,1));
        uv.push_back(glm::vec2(0,1));
        uv.push_back(glm::vec2(0,0));

        GLuint vertexBuffer;
        glGenBuffers(1,&vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,verts.size() * sizeof(glm::vec3),&verts[0],GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        GLuint uvBuffer;
        glGenBuffers(1,&uvBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glEnableVertexAttribArray(uvs);
        glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(glm::vec2), &uv[0], GL_STATIC_DRAW);
        glVertexAttribPointer(
                uvs,                  // attribute, 0 = verticies
                2,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );


        glBindVertexArray(0);

        return debugTriangleVAO;
    }

    GLuint createCubeVAO()
    {
        scope("createCubeVAO");

        GLuint cubeVAO;

        //Just to purge the error buffer
        glGetError();

        glGenVertexArrays(1,&cubeVAO);
        glBindVertexArray(cubeVAO);

        std::vector<glm::vec3> cubeVerts;

        //Size of cube
        float adjust = 0.5;

        //Top
        cubeVerts.push_back(glm::vec3(-adjust,adjust,-adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,-adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(-adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(-adjust,adjust,-adjust));

        //Right
        cubeVerts.push_back(glm::vec3(adjust,-adjust,-adjust));
        cubeVerts.push_back(glm::vec3(adjust,-adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,-adjust));
        cubeVerts.push_back(glm::vec3(adjust,-adjust,-adjust));

        //Left
        cubeVerts.push_back(glm::vec3(-adjust, adjust,   adjust));
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,   adjust));
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,   -adjust));
        cubeVerts.push_back(glm::vec3(-adjust, adjust,   adjust));
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,   -adjust));
        cubeVerts.push_back(glm::vec3(-adjust, adjust,   -adjust));

        //Front
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,adjust));
        cubeVerts.push_back(glm::vec3(-adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,-adjust,adjust));
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,adjust));

        //Back
        cubeVerts.push_back(glm::vec3(adjust, adjust,   -adjust));
        cubeVerts.push_back(glm::vec3(-adjust, adjust,  -adjust));
        cubeVerts.push_back(glm::vec3(-adjust,  -adjust,   -adjust));
        cubeVerts.push_back(glm::vec3(adjust, adjust,    -adjust));
        cubeVerts.push_back(glm::vec3(-adjust, -adjust,  -adjust));
        cubeVerts.push_back(glm::vec3(adjust,  -adjust,  -adjust));

        //Bottom
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,-adjust));
        cubeVerts.push_back(glm::vec3(adjust,-adjust,-adjust));
        cubeVerts.push_back(glm::vec3(adjust,-adjust,adjust));
        cubeVerts.push_back(glm::vec3(adjust,-adjust,adjust));
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,adjust));
        cubeVerts.push_back(glm::vec3(-adjust,-adjust,-adjust));


        GLuint cubeVertBuffer;
        glEnableVertexAttribArray(positions);
        glGenBuffers(1,&cubeVertBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVertBuffer);
        glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), &cubeVerts[0], GL_STATIC_DRAW);
        glVertexAttribPointer(
                positions,                  // attribute, 0 = verticies
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        std::vector<glm::vec2> uv;
        for(unsigned int a = 0; a<cubeVerts.size(); a++)
        {
            uv.push_back(glm::vec2(cubeVerts[a].x+0.5,cubeVerts[a].z+0.5));
        }

        GLuint cubeUVBuffer;
        glEnableVertexAttribArray(uvs);
        glGenBuffers(1,&cubeUVBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, cubeUVBuffer);
        glBufferData(GL_ARRAY_BUFFER, 36 * 2 * sizeof(float), &uv[0], GL_STATIC_DRAW);
        glVertexAttribPointer(
                uvs,                  // attribute, 0 = verticies
                2,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        std::vector<glm::vec3> tnormals;
        for(int a = 0; a<36; a++)
            tnormals.push_back(glm::vec3(0,1,0));

        GLuint cubeNormBuffer;
        glEnableVertexAttribArray(normals);
        glGenBuffers(1,&cubeNormBuffer);
        glBindBuffer(GL_ARRAY_BUFFER,cubeNormBuffer);
        glBufferData(GL_ARRAY_BUFFER,tnormals.size() * sizeof(glm::vec3),&tnormals[0],GL_STATIC_DRAW);
        glVertexAttribPointer(normals,3,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindVertexArray(0);

        return cubeVAO;
    }

}
