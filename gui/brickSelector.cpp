#include "brickSelector.h"

namespace syj
{
    bool closeBrickSelector(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *brickSelector = root->getChild("BrickSelector");
        brickSelector->setVisible(false);

        return true;
    }

    bool basicBrickTab(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *brickSelector = root->getChild("BrickSelector");
        brickSelector->getChild("BasicBricks")->setVisible(true);
        brickSelector->getChild("SpecialBricks")->setVisible(false);

        return true;
    }

    bool specialBrickTab(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *brickSelector = root->getChild("BrickSelector");
        brickSelector->getChild("BasicBricks")->setVisible(false);
        brickSelector->getChild("SpecialBricks")->setVisible(true);

        return true;
    }

    bool buyBrick(const CEGUI::EventArgs &e)
    {
        const CEGUI::WindowEventArgs& wea = static_cast<const CEGUI::WindowEventArgs&>(e);
        CEGUI::Window *button = wea.window;
        CEGUI::Window *brickSelector = button->getParent()->getParent()->getParent();
        CEGUI::Window *brickPopup = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("HUD")->getChild("BrickPopup");

        for(int a = 1; a<=9; a++)
        {
            CEGUI::Window *cart = brickSelector->getChild("Cart" + std::to_string(a));
            if(cart->getChild("BrickText")->getText() == "")
            {
                brickPopup->getChild("Cart" + std::to_string(a))->setProperty("Image",button->getChild("BrickImage")->getProperty("Image"));
                cart->getChild("BrickImage")->setProperty("Image",button->getChild("BrickImage")->getProperty("Image"));
                cart->getChild("BrickText")->setText(button->getChild("BrickText")->getText());
                cart->setUserData(button->getUserData());
                return true;
            }
        }

        for(int a = 9; a>=2; a--)
        {
            CEGUI::Window *cart = brickSelector->getChild("Cart" + std::to_string(a));
            CEGUI::Window *prevcart = brickSelector->getChild("Cart" + std::to_string(a-1));

            brickPopup->getChild("Cart" + std::to_string(a))->setProperty("Image",prevcart->getChild("BrickImage")->getProperty("Image"));

            cart->getChild("BrickImage")->setProperty("Image",prevcart->getChild("BrickImage")->getProperty("Image"));
            cart->getChild("BrickText")->setText(prevcart->getChild("BrickText")->getText());
            cart->setUserData(prevcart->getUserData());
        }

        CEGUI::Window *cart = brickSelector->getChild("Cart1");
        brickPopup->getChild("Cart1")->setProperty("Image",button->getChild("BrickImage")->getProperty("Image"));
        cart->getChild("BrickImage")->setProperty("Image",button->getChild("BrickImage")->getProperty("Image"));
        cart->getChild("BrickText")->setText(button->getChild("BrickText")->getText());
        cart->setUserData(button->getUserData());

        return true;
    }

    CEGUI::Window *loadBrickSelector()
    {
        CEGUI::Window *brickSelector = addGUIFromFile("brickSelector.layout");
        brickSelector->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeBrickSelector));
        brickSelector->getChild("BasicButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&basicBrickTab));
        brickSelector->getChild("SpecialButton")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&specialBrickTab));
        return brickSelector;
    }

    int addBasicBrickToSelector(CEGUI::Window *brickSelector,std::string imageFilePath,int width,int height,int length)
    {
        //If you set this to 6 it crashes SMH
        const int columns = 7;

        CEGUI::Window *button = brickSelector->getChild("BrickButton")->clone();
        CEGUI::ScrolledContainer *brickBox = (CEGUI::ScrolledContainer*)((CEGUI::ScrollablePane*)brickSelector->getChild("BasicBricks"))->getContentPane();

        int numBricks = brickBox->getChildCount();
        float column = numBricks % columns;
        float row = numBricks / columns;

        std::string text = "Basic " + std::to_string(width) +"-"+ std::to_string(height) +"-"+ std::to_string(length);
        CEGUI::ImageManager::getSingleton().addFromImageFile(text, imageFilePath,"/");
        button->getChild("BrickImage")->setProperty("Image",text);
        button->getChild("BrickText")->setText(text);
        button->setName(text);
        int *id = new int[3];
        id[0] = width;
        id[1] = height;
        id[2] = length;
        button->setUserData((void*)id);

        float xOffset = column / ((float)columns);
        float yOffset = row / 3.33333;
        float xSize = (1.0 / ((float)columns));
        button->setVisible(true);
        button->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&buyBrick));
        button->setArea(CEGUI::UDim(xOffset,0),CEGUI::UDim(yOffset,0),CEGUI::UDim(xSize,0),CEGUI::UDim(0.3,0));
        brickBox->addChild(button);

        return brickBox->getChildCount() - 1;
    }

    void addSpecialBrickToSelector(CEGUI::Window *brickSelector,std::string imageFilePath,std::string text,int pickingID)
    {
        if(getFileFromPath(text).length() > 2)
            text = getFileFromPath(text);

        //If you set this to 6 it crashes SMH
        const int columns = 7;

        CEGUI::Window *button = brickSelector->getChild("BrickButton")->clone();
        CEGUI::ScrolledContainer *brickBox = (CEGUI::ScrolledContainer*)((CEGUI::ScrollablePane*)brickSelector->getChild("SpecialBricks"))->getContentPane();

        int numBricks = brickBox->getChildCount();
        float column = numBricks % columns;
        float row = numBricks / columns;

        CEGUI::ImageManager::getSingleton().addFromImageFile(text, imageFilePath,"/");
        button->getChild("BrickImage")->setProperty("Image",text);
        button->getChild("BrickText")->setText(text);
        button->setName(text);
        int *id = new int;
        id[0] = pickingID;
        button->setUserData((void*)id);

        float xOffset = column / ((float)columns);
        float yOffset = row / 3.33333;
        float xSize = (1.0 / ((float)columns));
        button->setVisible(true);
        button->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&buyBrick));
        button->setArea(CEGUI::UDim(xOffset,0),CEGUI::UDim(yOffset,0),CEGUI::UDim(xSize,0),CEGUI::UDim(0.3,0));
        brickBox->addChild(button);
    }
}
