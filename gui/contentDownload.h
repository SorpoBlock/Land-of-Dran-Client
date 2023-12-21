#ifndef CONTENTDOWNLOAD_H_INCLUDED
#define CONTENTDOWNLOAD_H_INCLUDED

#include "code/utility/preference.h"
#include "code/graphics/uniformsBasic.h"
#include "code/utility/ceguiHelper.h"
#include "code/utility/inputMap.h"
#include "code/utility/clientStuff.h"

enum fileType
{
    unknownFile = 0,
    audioFile = 1,
    brickFile = 2,
    textureFile = 3,
    modelFile = 4
};

struct customFileDescriptor
{
    bool enabled = false;
    std::string name = "";
    std::string path = "";
    int size = 0;
    int bytesReceived = 0;
    bool doneDownloading = false;
    int checksum = 0;
    fileType type = unknownFile;
    int id = 0;
    bool selectable = false;
    CEGUI::ListboxTextItem *enabledBox = 0;

    void print()
    {
        std::cout<<id<<"|"<<name<<"|"<<path<<"|"<<size<<"|"<<(enabled?"enabled":"disabled")<<"|"<<(selectable?"selectable":"not selectable")<<"|"<<(doneDownloading?"done":"not done")<<"\n";
    }
};

void addCustomFileToGUI(std::string name,std::string path,unsigned int checksum,unsigned int size,unsigned char type,int id);
CEGUI::Window *loadContentMenu(clientStuff *clientEnvironment);

#endif // CONTENTDOWNLOAD_H_INCLUDED
