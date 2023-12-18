#include "contentDownload.h"

std::string typeToStr(fileType in)
{
    switch(in)
    {
        case unknownFile: return "Unknown!";
        case audioFile: return  "Audio";
        case brickFile: return "Brick";
        case textureFile: return "Texture";
        case modelFile: return "Model";
        default: return "Error";
    }
}

std::string fileSizeStr(unsigned int bytes)
{
    std::string ret = " Bytes";
    if(bytes >= 1024)
    {
        bytes /= 1024;
        ret = " Kb";
    }
    if(bytes >= 1024)
    {
        bytes /= 1024;
        ret = " Mb";
    }
    return std::to_string(bytes) + ret;
}

bool toggleCustomFile(const CEGUI::EventArgs &e)
{
    CEGUI::Window *contentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("CustomContent");
    CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)contentWindow->getChild("List");
    std::vector<customFileDescriptor*> *descriptorsList = (std::vector<customFileDescriptor*> *)contentWindow->getUserData();

    if(serverListGUI->getSelectedCount() < 1)
        return true;

    CEGUI::ListboxItem *first = serverListGUI->getFirstSelectedItem();
    if(!first)
        return true;

    customFileDescriptor *targetFile = (customFileDescriptor*)first->getUserData();
    targetFile->enabled = !targetFile->enabled;
    if(targetFile->enabledBox)
        targetFile->enabledBox->setText(targetFile->enabled ? "   X" : "   ");

    int totalSize = 0;
    int totalFiles = 0;

    for(int a = 0; a<descriptorsList->size(); a++)
    {
        customFileDescriptor *i = descriptorsList->at(a);
        if(i->enabled)
        {
            totalFiles++;
            totalSize += i->size;
        }
    }

    contentWindow->getChild("Number")->setText("Confirmed downloads: " + std::to_string(totalFiles));
    contentWindow->getChild("Size")->setText("Total Confirmed Size: " + fileSizeStr(totalSize));

    return true;
}

bool enableAllCustomFiles(const CEGUI::EventArgs &e)
{
    CEGUI::Window *contentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("CustomContent");
    CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)contentWindow->getChild("List");
    std::vector<customFileDescriptor*> *descriptorsList = (std::vector<customFileDescriptor*> *)contentWindow->getUserData();

    for(int a = 0; a<descriptorsList->size(); a++)
    {
        customFileDescriptor *targetFile = descriptorsList->at(a);
        targetFile->enabled = true;
        if(targetFile->enabledBox)
            targetFile->enabledBox->setText(targetFile->enabled ? "   X" : "   ");

        int totalSize = 0;
        int totalFiles = 0;

        for(int a = 0; a<descriptorsList->size(); a++)
        {
            customFileDescriptor *i = descriptorsList->at(a);
            if(i->enabled)
            {
                totalFiles++;
                totalSize += i->size;
            }
        }

        contentWindow->getChild("Number")->setText("Confirmed downloads: " + std::to_string(totalFiles));
        contentWindow->getChild("Size")->setText("Total Confirmed Size: " + fileSizeStr(totalSize));
    }

    serverListGUI->handleUpdatedItemData();
    return true;
}

bool disableAllCustomFiles(const CEGUI::EventArgs &e)
{
    CEGUI::Window *contentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("CustomContent");
    CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)contentWindow->getChild("List");
    std::vector<customFileDescriptor*> *descriptorsList = (std::vector<customFileDescriptor*> *)contentWindow->getUserData();

    for(int a = 0; a<descriptorsList->size(); a++)
    {
        customFileDescriptor *targetFile = descriptorsList->at(a);
        targetFile->enabled = false;
        if(targetFile->enabledBox)
            targetFile->enabledBox->setText(targetFile->enabled ? "   X" : "   ");

        int totalSize = 0;
        int totalFiles = 0;

        for(int a = 0; a<descriptorsList->size(); a++)
        {
            customFileDescriptor *i = descriptorsList->at(a);
            if(i->enabled)
            {
                totalFiles++;
                totalSize += i->size;
            }
        }

        contentWindow->getChild("Number")->setText("Confirmed downloads: " + std::to_string(totalFiles));
        contentWindow->getChild("Size")->setText("Total Confirmed Size: " + fileSizeStr(totalSize));
    }

    serverListGUI->handleUpdatedItemData();
    return true;
}

bool joinFromCustomContentWindow(const CEGUI::EventArgs &e)
{
    CEGUI::Window *contentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("CustomContent");
    CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)contentWindow->getChild("List");

    clientStuff *clientEnvironment = (clientStuff*)serverListGUI->getUserData();
    clientEnvironment->waitingOnContentList = false;
    return true;
}

bool cancelFromCustomContentWindow(const CEGUI::EventArgs &e)
{
    CEGUI::Window *contentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("CustomContent");
    CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)contentWindow->getChild("List");

    clientStuff *clientEnvironment = (clientStuff*)serverListGUI->getUserData();
    clientEnvironment->cancelCustomContent = true;
    contentWindow->setVisible(false);
    return true;
}

void addCustomFileToGUI(std::string name,std::string path,unsigned int checksum,unsigned int size,unsigned char type,int id)
{
    fileType fType = (fileType)type;

    CEGUI::Window *contentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("CustomContent");
    CEGUI::MultiColumnList *serverListGUI = (CEGUI::MultiColumnList*)contentWindow->getChild("List");
    std::vector<customFileDescriptor*> *descriptorsList = (std::vector<customFileDescriptor*> *)contentWindow->getUserData();

    contentWindow->setVisible(true);

    customFileDescriptor *tmp = new customFileDescriptor;
    tmp->checksum = checksum;
    tmp->size = size;
    tmp->enabled = false;
    tmp->name = name;
    tmp->path = path;
    tmp->id = id;
    tmp->type = fType;
    tmp->selectable = false;
    descriptorsList->push_back(tmp);

    if(!okayFilePath(path))
    {
        syj::error("Custom file from server has invalid path: " + path);
        return;
    }

    if(size < 1 || size > 2000000)
    {
        syj::error("Custom file size must be between 1 and 2 million bytes!");
        return;
    }

    if(name.length() < 1 || name.length() > 48)
    {
        syj::error("Custom file name must be between 1 and 48 characters!");
        return;
    }

    if(std::filesystem::exists("add-ons/" + path))
    {
        if(size == std::filesystem::file_size("add-ons/" + path))
        {
            if(getFileChecksum(std::string("add-ons/" + path).c_str()) == checksum)
            {
                syj::info("We already have an identical add-ons/" + path + " skipping!");
                return;
            }
        }
    }

    tmp->selectable = true;

    int idx = serverListGUI->getRowCount();
    serverListGUI->addRow(idx);

    std::string brushImage = (idx & 1) ? "GWEN/Input.ListBox.EvenLineSelected" : "GWEN/Input.ListBox.OddLineSelected";

    CEGUI::ListboxTextItem *cell = new CEGUI::ListboxTextItem("   ",idx);
    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
    cell->setSelectionBrushImage(brushImage);
    cell->setID(idx);
    cell->setUserData(tmp);
    serverListGUI->setItem(cell,0,idx);
    tmp->enabledBox = cell;

    cell = new CEGUI::ListboxTextItem(typeToStr(fType),idx);
    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
    cell->setSelectionBrushImage(brushImage);
    cell->setID(idx);
    cell->setUserData(tmp);
    serverListGUI->setItem(cell,1,idx);

    cell = new CEGUI::ListboxTextItem(name,idx);
    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
    cell->setSelectionBrushImage(brushImage);
    cell->setID(idx);
    cell->setUserData(tmp);
    serverListGUI->setItem(cell,2,idx);

    cell = new CEGUI::ListboxTextItem(path,idx);
    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
    cell->setSelectionBrushImage(brushImage);
    cell->setID(idx);
    cell->setUserData(tmp);
    serverListGUI->setItem(cell,3,idx);

    cell = new CEGUI::ListboxTextItem(fileSizeStr(size),idx);
    cell->setTextColours(CEGUI::Colour(0,0,0,1.0));
    cell->setSelectionBrushImage(brushImage);
    cell->setID(idx);
    cell->setUserData(tmp);
    serverListGUI->setItem(cell,4,idx);
}

CEGUI::Window *loadContentMenu(clientStuff *clientEnvironment)
{
    CEGUI::Window *ret = addGUIFromFile("content.layout");
    CEGUI::MultiColumnList *list = (CEGUI::MultiColumnList*)ret->getChild("List");

    list->setUserData(clientEnvironment);

    list->addColumn("Enb.",0,CEGUI::UDim(0.1,0));                //0.10
    list->addColumn("Type",1,CEGUI::UDim(0.1,0));                //0.20
    list->addColumn("Name",2,CEGUI::UDim(0.2,0));                //0.40
    list->addColumn("Relative Path",3,CEGUI::UDim(0.45,0));      //0.85
    list->addColumn("Size",4,CEGUI::UDim(0.14,0));               //0.99

    list->subscribeEvent(CEGUI::MultiColumnList::EventSelectionChanged,CEGUI::Event::Subscriber(&toggleCustomFile));
    ret->getChild("All")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&enableAllCustomFiles));
    ret->getChild("None")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&disableAllCustomFiles));
    ret->getChild("Cancel")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&cancelFromCustomContentWindow));
    ret->getChild("Join")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&joinFromCustomContentWindow));

    std::vector<customFileDescriptor*> *contentList = new std::vector<customFileDescriptor*>;
    ret->setUserData(contentList);

    return ret;
}

