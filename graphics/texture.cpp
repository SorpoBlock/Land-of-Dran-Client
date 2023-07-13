#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "code/external/stb_image.h"

GLenum resolveChannels(int channels,bool hdr)
{
    switch(channels)
    {
        case 1: return hdr ? GL_R16F    : GL_RED;
        case 2: return hdr ? GL_RG16F   : GL_RG;
        case 3: return hdr ? GL_RGB16F  : GL_RGB;
        case 4: return hdr ? GL_RGBA16F : GL_RGBA;
    }

    return 0;
}

namespace syj
{
    void getImageDims(std::string filepath,int *x,int *y,int *c)
    {
        stbi_info(filepath.c_str(),x,y,c);
    }

    bool isHDR(std::string filename)
    {
        return stbi_is_hdr(filename.c_str());
    }

    float *loadImageF(std::string filename,int &tWidth,int &tHeight,int &tChannels,int desiredChannels)
    {
        return stbi_loadf(filename.c_str(),&tWidth,&tHeight,&tChannels,desiredChannels);
    }

    unsigned char *loadImageU(std::string filename,int &tWidth,int &tHeight,int &tChannels,int desiredChannels)
    {
        return stbi_load(filename.c_str(),&tWidth,&tHeight,&tChannels,desiredChannels);
    }

    void texture::useForFramebuffer(GLenum attachmentNumber)
    {
        if(textureType == GL_TEXTURE_2D)
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentNumber, textureType, handle,0);
        else
            glFramebufferTexture(GL_FRAMEBUFFER,attachmentNumber,handle,0);
    }

    void texture::allocateDepth(unsigned int width,unsigned int height,GLenum internalFormat,GLenum format,GLenum type)
    {
        resX = width;
        resY = height;
        glBindTexture(textureType,handle);
        glTexImage2D(textureType, 0, internalFormat, width, height, 0, format, type, NULL);
        setParams(textureType,resX,resY);
        glBindTexture(textureType,0);
    }

    void texture::allocateDepth(unsigned int width,unsigned int height,unsigned int _layers,GLenum internalFormat,GLenum format,GLenum type)
    {
        layers = _layers;
        resX = width;
        resY = height;

        glBindTexture(textureType,handle);
        glTexImage3D(textureType, 0, internalFormat, width, height,layers, 0, format, type, NULL);
        setParams(textureType,resX,resY);
        glBindTexture(textureType,0);
    }

    void texture::allocate(bool hdr,unsigned int width,unsigned int height,unsigned int _layers,unsigned int channels)
    {
        scope("texture::allocate");
        if(created)
        {
            error("Texture already created.");
            return;
        }

        layers = _layers;
        isHdr = hdr;
        resX = width;
        resY = height;

        if(isHdr)
            hdrData = new float[resX*resY*channels*((layers==0)?1:layers)];
        else
            ldrData = new unsigned char[resX*resY*channels*((layers==0)?1:layers)];

        glBindTexture(textureType,handle);
        glTexImage3D(
                    textureType,
                    0,
                    resolveChannels(channels,isHdr),
                    resX,
                    resY,
                    layers,
                    0,
                    resolveChannels(channels,false),
                    isHdr ? GL_FLOAT : GL_UNSIGNED_BYTE,
                    isHdr ? (void*)hdrData : (void*)ldrData);
        setParams(textureType,resX,resY);
        glBindTexture(textureType,0);
    }

    void texture::allocate(bool hdr,unsigned int width,unsigned int height,unsigned int channels)
    {
        scope("texture::allocate");
        if(created)
        {
            error("Texture already created.");
            return;
        }

        isHdr = hdr;
        resX = width;
        resY = height;

        if(isHdr)
            hdrData = new float[resX*resY*channels];
        else
            ldrData = new unsigned char[resX*resY*channels];

        glBindTexture(textureType,handle);
        glTexImage2D(
                    textureType,
                    0,
                    resolveChannels(channels,isHdr),
                    resX,
                    resY,
                    0,
                    resolveChannels(channels,false),
                    isHdr ? GL_FLOAT : GL_UNSIGNED_BYTE,
                    isHdr ? (void*)hdrData : (void*)ldrData);
        setParams(textureType,resX,resY);
        glBindTexture(textureType,0);
    }

    void texture::addChannel(std::string fileName,unsigned int channels,channel fileChannel)
    {
        scope("texture::addChannel");

        int tWidth,tHeight,tChannels;

        if(resX == -1)
        {
            if(channels == 0)
            {
                error("Need to specify channels if calling addChannel before allocate");
                return;
            }
        }

        if(currentChannel > 0 && isHdr != stbi_is_hdr(fileName.c_str()))
            error("File " + fileName + " dynamic range does not match texture.");

        if(isHdr)
        {
            float *data = stbi_loadf(fileName.c_str(),&tWidth,&tHeight,&tChannels,0);
            if(tWidth == 0 || tHeight == 0)
            {
                error("Could not find " + fileName);
                return;
            }

            if(resX == -1)
                allocate(true,tWidth,tHeight,channels);

            if(tWidth != resX || tHeight != resY)
            {
                error("Resolution of " + fileName + " differed from texture.");
                return;
            }

            for(int a = 0; a<resX*resY; a++)
                hdrData[resX*resY*currentChannel + a] = data[a*tChannels + fileChannel];

            currentChannel++;
        }
        else
        {
            unsigned char *data = stbi_load(fileName.c_str(),&tWidth,&tHeight,&tChannels,0);
            if(tWidth == 0 || tHeight == 0)
            {
                error("Could not find " + fileName);
                return;
            }

            if(resX == -1)
                allocate(false,tWidth,tHeight,channels);

            if(tWidth != resX || tHeight != resY)
            {
                error("Resolution of " + fileName + " differed from texture.");
                return;
            }

            for(int a = 0; a<resX*resY; a++)
                ldrData[resX*resY*currentChannel + a] = data[a*tChannels + fileChannel];

            currentChannel++;
        }

    }

    void texture::dummyChannel()
    {
        if(isHdr)
            dummyChannel(0.0f);
        else
            dummyChannel((unsigned char)0);
    }

    void texture::dummyChannel(float value)
    {
        scope("texture::dummyChannel");
        if(!isHdr)
        {
            error("Trying to assign floats to ldr texture.");
            return;
        }
        if(created)
        {
            error("Texture already created!");
            return;
        }

        std::fill_n(hdrData + resX*resY*currentChannel,resX*resY,value);
        currentChannel++;
    }

    void texture::dummyChannel(unsigned char value)
    {
        scope("texture::dummyChannel");
        if(isHdr)
        {
            error("Trying to assign uChars to hdr texture.");
            return;
        }
        if(created)
        {
            error("Texture already created!");
            return;
        }

        std::fill_n(ldrData + resX*resY*currentChannel,resX*resY,value);
        currentChannel++;
    }

    void texture::finalize()
    {
        if(created)
        {
            error("Texture already created!");
            return;
        }

        scope("texture::finalize");

        setParams(textureType,resX,resY);
        glBindTexture(textureType,handle);

        if(isHdr)
        {
            float *rearranged = new float[resX*resY*currentChannel];

            for(int i = 0; i<resX*resY; i++)
                for(int c = 0; c<currentChannel; c++)
                    rearranged[i*currentChannel + c] = hdrData[resX*resY*c + i];

            glTexImage2D(
                        textureType,
                        0,
                        resolveChannels(currentChannel,isHdr),
                        resX,
                        resY,
                        0,
                        resolveChannels(currentChannel,false),
                        GL_FLOAT,
                        rearranged);

            delete rearranged;
            delete hdrData;
        }
        else
        {
            unsigned char *rearranged = new unsigned char[resX*resY*currentChannel];

            for(int i = 0; i<resX*resY; i++)
                for(int c = 0; c<currentChannel; c++)
                    rearranged[i*currentChannel + c] = ldrData[resX*resY*c + i];

            glTexImage2D(
                        textureType,
                        0,
                        resolveChannels(currentChannel,isHdr),
                        resX,
                        resY,
                        0,
                        resolveChannels(currentChannel,false),
                        GL_UNSIGNED_BYTE,
                        rearranged);

            delete rearranged;
            delete ldrData;
        }

        if(mipMaps)
            glGenerateMipmap(textureType);

        glBindTexture(textureType,0);

        created = true;
    }

    glm::vec2 texture::getResolution()
    {
        return glm::vec2(resX,resY);
    }

    void texture::useMipMaps(bool value)
    {
        scope("texture::useMipMaps");
        if(created)
            error("Call to texture::useMipMaps after creation!");
        mipMaps = value;
    }

    void texture::setFilter(GLenum _magFilter,GLenum _minFilter)
    {
        scope("texture::setFilter");
        if(created)
            error("Call to texture::setFilter after creation!");
        magFilter = _magFilter;
        minFilter = _minFilter;
    }

    void texture::setWrapping(GLenum _texWrapS,GLenum _texWrapT,GLenum _texWrapR)
    {
        scope("texture::setWrapping");
        if(created)
            error("Call to texture::setWrapping after creation!");
        texWrapS = _texWrapS;
        texWrapT = _texWrapT;
        texWrapR = _texWrapR;
    }

    void texture::setWrapping(GLenum texWrap)
    {
        setWrapping(texWrap,texWrap,texWrap);
    }

    void texture::bind(textureLocations loc)
    {
        glActiveTexture(GL_TEXTURE0 + loc);
        glBindTexture(textureType,handle);
    }

    void texture::bind(brickShaderTexture loc)
    {
        glActiveTexture(GL_TEXTURE0 + loc);
        glBindTexture(textureType,handle);
    }

    void texture::setParams(GLenum _textureType,int w,int h)
    {
        scope("texture::setParams");

        textureType = _textureType;
        resX = w;
        resY = h;

        if(minFilter != GL_NEAREST && minFilter != GL_LINEAR && !mipMaps)
        {
            error("No mipmaps for texture, disabling.");
            minFilter = GL_LINEAR;
        }

        glBindTexture(textureType,handle);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_S, texWrapS);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_T, texWrapT);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_R, texWrapR);
        glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, minFilter);
        glBindTexture(textureType,0);
    }

    bool texture::createFromFile(std::string filePath)
    {
        scope("texture::createFromFile");

        int tWidth,tHeight,tChannels;

        void *data;

        isHdr = stbi_is_hdr(filePath.c_str());

        if(isHdr)
        {
            debug("File " + filePath + " is HDR");
            data = stbi_loadf(filePath.c_str(),&tWidth,&tHeight,&tChannels,0);
        }
        else
        {
            debug("File " + filePath + " is not HDR");
            data = stbi_load(filePath.c_str(),&tWidth,&tHeight,&tChannels,0);

        }

        debug(filePath + " It had dimensions of " + std::to_string(tWidth) + "," + std::to_string(tHeight) + "," + std::to_string(tChannels));
        if(!data || tWidth == 0 || tHeight == 0 || tChannels == 0)
        {
            error("Could not load " + filePath);
            return false;
        }

        setParams(textureType,tWidth,tHeight);
        glBindTexture(textureType,handle);
        glTexImage2D(
                    textureType,
                    0,
                    resolveChannels(tChannels,isHdr),
                    tWidth,
                    tHeight,
                    0,
                    resolveChannels(tChannels,false),
                    isHdr ? GL_FLOAT : GL_UNSIGNED_BYTE,
                    data);

        if(mipMaps)
            glGenerateMipmap(textureType);



        glBindTexture(textureType,0);
        stbi_image_free(data);

        created = true;
        return true;
    }

    texture::texture(GLenum type) : textureType(type)
    {
        glGenTextures(1,&handle);
    }

    texture::~texture()
    {
        glDeleteTextures(1,&handle);
    }
}
