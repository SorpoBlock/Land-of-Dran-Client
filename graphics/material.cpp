#include "material.h"

namespace syj
{
    void material::use(uniformsHolder *uniforms)
    {
        glUniform1i(uniforms->useAlbedo      ,albedoUsed);
        glUniform1i(uniforms->useNormal      ,normalUsed);
        glUniform1i(uniforms->useMetalness   ,metalnessUsed);
        glUniform1i(uniforms->useAO          ,occlusionUsed);
        glUniform1i(uniforms->useHeight      ,displacementUsed);
        glUniform1i(uniforms->useRoughness   ,roughnessUsed);

        if(albedoUsed)
            textures[albedo].bind(albedo);
        if(normalUsed)
            textures[normal].bind(normal);
        if(metalnessUsed || occlusionUsed || displacementUsed || roughnessUsed)
            textures[mohr].bind(mohr);
    }

    bool tryAddChannel(texture &toAdd,std::string filePath)
    {
        if(filePath != "")
        {
            toAdd.addChannel(filePath);
            return true;
        }
        else
        {
            toAdd.dummyChannel();
            return false;
        }
    }

    void material::useManuelOffset(brickShaderTexture albedoOffset,brickShaderTexture normalOffset,brickShaderTexture mohrOffset)
    {
        if(albedoUsed && albedoOffset != doNotUseBrickTexture)
            textures[albedo].bind((textureLocations)albedoOffset);
        if(normalUsed && normalOffset != doNotUseBrickTexture)
            textures[normal].bind((textureLocations)normalOffset);
        if((metalnessUsed || occlusionUsed || displacementUsed || roughnessUsed) && mohrOffset != doNotUseBrickTexture)
            textures[mohr].bind((textureLocations)mohrOffset);
    }

    std::string material::tryFile(std::string type,std::string name,preferenceFile &settings)
    {
        scope("material::tryFile");
        std::string folder = getFolderFromPath(settings.getFilePath());

        preference *color = settings.getPreference("Tex_" + name + "_" + type);
        if(!color)
            color = settings.getPreference("Tex_All_" + type);
        if(!color)
            color = settings.getPreference("All_" + type);
        if(!color)
            color = settings.getPreference(name + "_" + type);

        if(color)
        {
            std::string path = folder + color->toString();
            debug("Using " + path + " for " + type + " for " + name);
            return path;
        }
        return "";
    }

    void material::addOnlyAlbedo(std::string filename)
    {
        textures[0].setWrapping(GL_REPEAT);
        textures[0].setFilter(GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);
        textures[albedo].createFromFile(filename);
        albedoUsed = true;
    }

    material::material(std::string filePath)
    {
        scope("material::material");

        preferenceFile settings;
        if(!settings.importFromFile(filePath))
            error("Could not open " + filePath);

        preference *name = settings.getPreference("Name");
        if(!name)
            error("No name provided in preference file.");

        create(name->toString(),settings);
    }

    void material::create(std::string name,preferenceFile &settings)
    {
        textures[0].setWrapping(GL_REPEAT);
        textures[1].setWrapping(GL_REPEAT);
        textures[2].setWrapping(GL_REPEAT);
        textures[0].setFilter(GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);
        textures[1].setFilter(GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);
        textures[2].setFilter(GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);

        /*textures[0].setFilter(GL_LINEAR,GL_LINEAR)
        textures[1].setFilter(GL_NEAREST,GL_NEAREST_MIPMAP_NEAREST);
        textures[2].setFilter(GL_NEAREST,GL_NEAREST_MIPMAP_NEAREST);*/
        /*
        textures[0].setFilter(GL_NEAREST,GL_LINEAR_MIPMAP_LINEAR);
        textures[1].setFilter(GL_NEAREST,GL_LINEAR_MIPMAP_LINEAR);
        textures[2].setFilter(GL_NEAREST,GL_LINEAR_MIPMAP_LINEAR);*/


        std::string albedoPath = tryFile("Albedo",name,settings);
        if(albedoPath != "")
        {
            albedoUsed = true;
            textures[albedo].createFromFile(albedoPath);
        }

        std::string normalPath = tryFile("Normal",name,settings);
        if(normalPath != "")
        {
            normalUsed = true;
            textures[normal].createFromFile(normalPath);
        }

        debug("Mat name: " + name);

        std::string metalnessPath       = tryFile("Metalness",name,settings);
        std::string occlusionPath       = tryFile("AO",name,settings);
        std::string displacementPath    = tryFile("Height",name,settings);
        std::string roughnessPath       = tryFile("Roughness",name,settings);

        debug(albedoPath);
        debug(normalPath);
        debug(metalnessPath);
        debug(occlusionPath);
        debug(displacementPath);
        debug(roughnessPath);

        if(metalnessPath + occlusionPath + displacementPath + roughnessPath == "")
            return;

        bool isHdr;
        int width,height;
        int c; //don't use
        if(metalnessPath != "")
        {
            isHdr = isHDR(metalnessPath);
            getImageDims(metalnessPath,&width,&height,&c);
        }
        else if(occlusionPath != "")
        {
            isHdr = isHDR(occlusionPath);
            getImageDims(occlusionPath,&width,&height,&c);
        }
        else if(roughnessPath != "")
        {
            isHdr = isHDR(roughnessPath);
            getImageDims(roughnessPath,&width,&height,&c);
        }
        else if(displacementPath != "")
        {
            isHdr = isHDR(displacementPath);
            getImageDims(displacementPath,&width,&height,&c);
        }

        textures[mohr].allocate(isHdr,width,height,4);

        if(tryAddChannel(textures[mohr],metalnessPath))     metalnessUsed = true;
        if(tryAddChannel(textures[mohr],occlusionPath))     occlusionUsed = true;
        if(tryAddChannel(textures[mohr],displacementPath))  displacementUsed = true;
        if(tryAddChannel(textures[mohr],roughnessPath))     roughnessUsed = true;

        textures[mohr].finalize();
    }

    material::material(std::string name,preferenceFile &settings)
    {
        scope("material::material");

        create(name,settings);
    }

    material::~material()
    {

    }
}
