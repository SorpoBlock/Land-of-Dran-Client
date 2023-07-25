#include "tempBrick.h"

namespace syj
{
    tempBrick::tempBrick(newBrickRenderer &renderer)
    {
        basic.material = tempBrickEffect;
        special.material = tempBrickEffect;

        basic.dimensions = glm::ivec4(0);
        basic.position = glm::vec3(-9999);
        special.position = glm::vec3(-9999);

        //Always make sure these stay in the transparent vectors:
        basic.color.a = 0.5;
        special.color.a = 0.5;

        renderer.addBasicBrick(&basic,0,0,0);
        renderer.addSpecialBrick(&special,0,0,0);
    }

    bool tempBrick::manipulate(inputMap &controls,CEGUI::Window *hud,CEGUI::Window *brickSelector,float yaw,audioPlayer *speaker,newBrickRenderer &renderer,bool &brickSlotChanged)
    {
        brickSlotChanged = false;
        int oldSelect = brickSlotSelected;

        for(int a = 0; a<9; a++)
        {
            if(controls.commandPressed((inputCommand)(useBrick1 + a)))
            {
                speaker->playSound("ClickChange",false,getX(),getY(),getZ());

                brickSlotChanged = true;
                brickSlotSelected = a+1;

                CEGUI::Window *brickPopup = hud->getChild("BrickPopup");
                CEGUI::Window *brickText = brickPopup->getChild("BrickText");
                cart = brickSelector->getChild("Cart" + std::to_string(a+1));
                brickText->setText(cart->getChild("BrickText")->getText());
                std::string name = cart->getChild("BrickText")->getText().c_str();
                if(name.length() < 1)
                {
                    cart = 0;
                    continue;
                }
                bool newIsBasic = name.substr(0,5) == "Basic";
                if(newIsBasic != isBasic)
                {
                    basicChanged = true;
                    specialChanged = true;
                }
                isBasic = newIsBasic;
                rotationID = 0;
                if(isBasic)
                {
                    int *dims = (int*)cart->getUserData();
                    basic.dimensions = glm::ivec4(dims[0],dims[1],dims[2],0);

                    yHalfPos = (basic.dimensions.y % 2);
                    if(rotationID % 2 == 0)
                    {
                        xHalfPos = (basic.dimensions.x % 2);
                        zHalfPos = (basic.dimensions.z % 2);
                    }
                    else
                    {
                        zHalfPos = (basic.dimensions.x % 2);
                        xHalfPos = (basic.dimensions.z % 2);
                    }
                }
                else
                {
                    int *preID = (int*)cart->getUserData();
                    if(preID)
                    {
                        int typeID = preID[0];
                        special.typeID = typeID;
                        special.type = renderer.transparentSpecialBrickTypes[typeID];

                        yHalfPos = (special.type->type->height % 2);
                        if(rotationID % 2 == 0)
                        {
                            xHalfPos = (special.type->type->width % 2);
                            zHalfPos = (special.type->type->length % 2);
                        }
                        else
                        {
                            zHalfPos = (special.type->type->width % 2);
                            xHalfPos = (special.type->type->length % 2);
                        }
                    }
                }
            }
        }

        if(controls.commandPressed(hideBricks))
        {
            brickSlotChanged = true;
            basicChanged = true;
            specialChanged = true;
            brickSlotSelected = -1;
            cart = 0;
        }

        if(controls.commandPressed(resizeToggle))
        {
            resize = !resize;
            hud->getChild("ResizeText")->setVisible(resize);
        }

        if(controls.commandPressed(superShift))
        {
            superShiftMode = !superShiftMode;
            hud->getChild("ShiftText")->setVisible(superShiftMode);
        }

        if(brickSlotSelected == -1 && !cart)
            return (oldSelect == -1) != (brickSlotSelected == -1);

        bool rotateCorrect = false;

        if(controls.commandPressed(rotateBrick))
        {
            rotateCorrect = true;

            rotationID++;
            if(rotationID > 3)
                rotationID = 0;
        }

        if(controls.commandPressed(rotateBrickBackwards))
        {
            rotateCorrect = true;

            rotationID--;
            if(rotationID < 0)
                rotationID = 3;
        }

        if(rotateCorrect)
        {
            speaker->playSound("ClickRotate",false,getX(),getY(),getZ());

            if(isBasic)
            {
                basicChanged = true;

                if(rotationID % 2 == 0)
                {
                    xHalfPos = (((int)basic.dimensions.x) % 2);
                    zHalfPos = (((int)basic.dimensions.z) % 2);
                }
                else
                {
                    zHalfPos = (((int)basic.dimensions.x) % 2);
                    xHalfPos = (((int)basic.dimensions.z) % 2);
                }
            }
            else
            {
                specialChanged = true;

                if(rotationID % 2 == 0)
                {
                    xHalfPos = (((int)special.type->type->width) % 2);
                    zHalfPos = (((int)special.type->type->length) % 2);
                }
                else
                {
                    zHalfPos = (((int)special.type->type->width) % 2);
                    xHalfPos = (((int)special.type->type->length) % 2);
                }
            }
            std::cout<<"Half poses after: "<<(xHalfPos?"t":"f")<<" and "<<(zHalfPos?"t":"f")<<"\n";
            return (oldSelect == -1) != (brickSlotSelected == -1);
        }

        //Start movements / resizing
        bool moveBrickSide = false;
        bool moveBrickAtAll = false;
        glm::vec3 brickMoveDir;

        if(firstPressTime + 500 < SDL_GetTicks() || firstPressTime == -1)
        {
            //if(controls.commandPressed(moveBrickForward))
            if(controls.commandKeyDown(moveBrickForward)&& lastBrickMove + brickMoveCooldown < SDL_GetTicks())
            {
                if(firstPressTime == -1)
                    firstPressTime = SDL_GetTicks();
                speaker->playSound("ClickMove",false,getX(),getY(),getZ());
                moveBrickSide = true;
                brickMoveDir = glm::vec3(0,0,-1);
            }
            //if(controls.commandPressed(moveBrickBackward))
            if(controls.commandKeyDown(moveBrickBackward) && lastBrickMove + brickMoveCooldown < SDL_GetTicks())
            {
                if(firstPressTime == -1)
                    firstPressTime = SDL_GetTicks();
                speaker->playSound("ClickMove",false,getX(),getY(),getZ());
                moveBrickSide = true;
                brickMoveDir = glm::vec3(0,0,1);
            }
            //if(controls.commandPressed(moveBrickLeft))
            if(controls.commandKeyDown(moveBrickLeft) && lastBrickMove + brickMoveCooldown < SDL_GetTicks())
            {
                if(firstPressTime == -1)
                    firstPressTime = SDL_GetTicks();
                speaker->playSound("ClickMove",false,getX(),getY(),getZ());
                moveBrickSide = true;
                brickMoveDir = glm::vec3(-1,0,0);
            }
            //if(controls.commandPressed(moveBrickRight))
            if(controls.commandKeyDown(moveBrickRight) && lastBrickMove + brickMoveCooldown < SDL_GetTicks())
            {
                if(firstPressTime == -1)
                    firstPressTime = SDL_GetTicks();
                speaker->playSound("ClickMove",false,getX(),getY(),getZ());
                moveBrickSide = true;
                brickMoveDir = glm::vec3(1,0,0);
            }
        }

        if(controls.commandKeyDown(moveBrickRight) || controls.commandKeyDown(moveBrickLeft) || controls.commandKeyDown(moveBrickForward) || controls.commandKeyDown(moveBrickBackward))
            moveBrickAtAll = true;

        int up = controls.commandKeyDown(moveBrickUp) ? 1 : 0;
        up += controls.commandKeyDown(moveBrickUpThree) ? 3 : 0;

        if(up)
        {
            moveBrickAtAll = true;
            if(lastBrickMove + brickMoveCooldown < SDL_GetTicks() && (firstPressTime + 500 < SDL_GetTicks() || firstPressTime == -1))
            {
                if(firstPressTime == -1)
                    firstPressTime = SDL_GetTicks();
                lastBrickMove = SDL_GetTicks();
                speaker->playSound("ClickMove",false,getX(),getY(),getZ());
                if(resize && isBasic)
                {
                    if(cart)
                    {
                        basic.dimensions.y++;
                        basicChanged = true;
                        if(yHalfPos)
                        {
                            uYPos++;
                            yHalfPos = false;
                        }
                        else
                        {
                            yHalfPos = true;
                        }
                        //position.y += (1.2 / 3.0) / 2.0;
                    }
                }
                else if(superShiftMode)
                {
                    if(isBasic)
                    {
                        uYPos += basic.dimensions.y;
                        //position.y += basic.dimensions.y * 1.2 / 3.0;
                        basicChanged = true;
                    }
                    else if(special.type)
                    {
                        uYPos += special.type->type->height;
                        //position.y += ((float)special.type->type->height) * 1.2 / 3.0;
                        specialChanged = true;
                    }
                }
                else
                {
                    if(isBasic)
                        basicChanged = true;
                    else
                        specialChanged = true;
                    uYPos += up;
                }
            }
        }

        int down = controls.commandKeyDown(moveBrickDown) ? 1 : 0;
        down += controls.commandKeyDown(moveBrickDownThree) ? 3 : 0;

        //if(controls.commandPressed(moveBrickDown))
        if(down)
        {
            moveBrickAtAll = true;
            if(lastBrickMove + brickMoveCooldown < SDL_GetTicks() && (firstPressTime + 500 < SDL_GetTicks() || firstPressTime == -1))
            {
                if(firstPressTime == -1)
                    firstPressTime = SDL_GetTicks();
                lastBrickMove = SDL_GetTicks();
                speaker->playSound("ClickMove",false,getX(),getY(),getZ());
                if(resize && isBasic)
                {
                    if(basic.dimensions.y > 1 && cart)
                    {
                        basic.dimensions.y--;
                        basicChanged = true;
                        //position.y -= (1.2 / 3.0) / 2.0;
                        if(yHalfPos)
                        {
                            yHalfPos = false;
                        }
                        else
                        {
                            uYPos--;
                            yHalfPos = true;
                        }
                    }
                }
                else if(superShiftMode)
                {
                    if(isBasic)
                    {
                        //position.y -= basic.dimensions.y * 1.2 / 3.0;
                        uYPos -= basic.dimensions.y;
                        basicChanged = true;
                    }
                    else if(special.type)
                    {
                        //position.y -= ((float)special.type->type->height) * 1.2 / 3.0;
                        uYPos -= special.type->type->height;
                        specialChanged = true;
                    }
                }
                else
                {
                    if(isBasic)
                        basicChanged = true;
                    else
                        specialChanged = true;
                    uYPos -= down;//position.y -= 1.2;
                }
            }
        }

        if(!moveBrickAtAll)
            firstPressTime = -1;

        int effectiveWidth = 1;
        int effectiveLength = 1;
        if(isBasic)
        {
            if(rotationID % 2 == 0)
            {
                effectiveWidth = basic.dimensions.x;
                effectiveLength = basic.dimensions.z;
            }
            else
            {
                effectiveWidth = basic.dimensions.z;
                effectiveLength = basic.dimensions.x;
            }
        }
        else
        {
            if(rotationID % 2 == 0)
            {
                effectiveWidth = special.type->type->width;
                effectiveLength = special.type->type->length;
            }
            else
            {
                effectiveWidth = special.type->type->length;
                effectiveLength = special.type->type->width;
            }
        }

        if(moveBrickSide && cart)
        {
            lastBrickMove = SDL_GetTicks();
            float tau = 6.28318530718;
            float playerFacing = yaw+3.141592;
            if(playerFacing > tau * (7.0/8.0) || playerFacing < tau * (1.0/8.0))
                playerFacing = 0.0;
            else if(playerFacing > tau * (5.0/8.0) && playerFacing < tau * (7.0/8.0))
                playerFacing = tau / 4.0;
            else if(playerFacing > tau * (3.0/8.0) && playerFacing < tau * (5.0/8.0))
                playerFacing = tau / 2.0;
            else
                playerFacing = tau * 3.0 / 4.0;

            float yawOfBrick = atan2(brickMoveDir.x,brickMoveDir.z);
            yawOfBrick -= playerFacing;
            if(yawOfBrick < 0)
                yawOfBrick += tau;
            brickMoveDir.x = sin(yawOfBrick);
            brickMoveDir.z = cos(yawOfBrick);

            if(superShiftMode && !(resize && isBasic))
            {
                if(isBasic)
                {
                    brickMoveDir *= glm::vec3(effectiveWidth,basic.dimensions.y,effectiveLength);
                    basicChanged = true;
                }
                else if(special.type)
                {
                    brickMoveDir *= glm::vec3(effectiveWidth,special.type->type->height,effectiveLength);
                    specialChanged = true;
                }
            }

            if(isBasic && resize)
            {
                if(fabs(brickMoveDir.x) > 0.01)
                    brickMoveDir.z = 0.0;

                std::cout<<"Resize move: "<<brickMoveDir.x<<","<<brickMoveDir.y<<","<<brickMoveDir.z<<" rot: "<<rotationID<<"\n";

                if(rotationID % 2)
                {
                    basic.dimensions.x += fabs(brickMoveDir.z);
                    basic.dimensions.z += fabs(brickMoveDir.x);

                    if(brickMoveDir.x > 0)
                    {
                        if(xHalfPos)
                        {
                            xHalfPos = false;
                            xPos++;
                        }
                        else
                            xHalfPos = true;
                    }
                    if(brickMoveDir.z > 0)
                    {
                        if(zHalfPos)
                        {
                            zHalfPos = false;
                            zPos++;
                        }
                        else
                            zHalfPos = true;
                    }
                    if(brickMoveDir.x < 0)
                    {
                        if(xHalfPos)
                        {
                            xHalfPos = false;
                        }
                        else
                        {
                            xHalfPos = true;
                            xPos--;
                        }
                    }
                    if(brickMoveDir.z < 0)
                    {
                        if(zHalfPos)
                        {
                            zHalfPos = false;
                        }
                        else
                        {
                            zHalfPos = true;
                            zPos--;
                        }
                    }
                }
                else
                {
                    glm::vec3 adjust = glm::vec3(0.5,0,0.5);
                    if(brickMoveDir.x < 0)
                    {
                        brickMoveDir.x = fabs(brickMoveDir.x);
                        adjust.x = -adjust.x;
                    }
                    if(brickMoveDir.y < 0)
                        brickMoveDir.x = fabs(brickMoveDir.y);
                    if(brickMoveDir.z < 0)
                    {
                        brickMoveDir.z = fabs(brickMoveDir.z);
                        adjust.z = - adjust.z;
                    }

                    if(basic.dimensions.x > 1 || brickMoveDir.x > 0)
                        basic.dimensions.x += brickMoveDir.x;
                    else
                        brickMoveDir.x = 0;
                    if(basic.dimensions.y > 1 || brickMoveDir.y > 0)
                        basic.dimensions.y += brickMoveDir.y;
                    else
                        brickMoveDir.y = 0;
                    if(basic.dimensions.z > 1 || brickMoveDir.z > 0)
                        basic.dimensions.z += brickMoveDir.z;
                    else
                        brickMoveDir.z = 0;

                    float moveX = brickMoveDir.x * adjust.x;
                    float moveZ = brickMoveDir.z * adjust.z;
                    if(moveX > 0.4)
                    {
                        if(xHalfPos)
                        {
                            xHalfPos = false;
                            xPos++;
                        }
                        else
                            xHalfPos = true;
                    }
                    if(moveX < -0.4)
                    {
                        if(xHalfPos)
                        {
                            xHalfPos = false;
                        }
                        else
                        {
                            xHalfPos = true;
                            xPos--;
                        }
                    }
                    if(moveZ > 0.4)
                    {
                        if(zHalfPos)
                        {
                            zHalfPos = false;
                            zPos++;
                        }
                        else
                            zHalfPos = true;
                    }
                    if(moveZ < -0.4)
                    {
                        if(zHalfPos)
                        {
                            zHalfPos = false;
                        }
                        else
                        {
                            zHalfPos = true;
                            zPos--;
                        }
                    }
                    basicChanged = true;
                }

                if(basic.dimensions.x > 255)
                    basic.dimensions.x = 255;
                if(basic.dimensions.y > 255)
                    basic.dimensions.y = 255;
                if(basic.dimensions.z > 255)
                    basic.dimensions.z = 255;

                return (oldSelect == -1) != (brickSlotSelected == -1);
            }
            else
            {
                if(isBasic)
                    basicChanged = true;
                else
                    specialChanged = true;
                //position += brickMoveDir;
                while(brickMoveDir.x > 0.5)
                {
                    brickMoveDir.x--;
                    xPos++;
                }
                while(brickMoveDir.x < -0.5)
                {
                    brickMoveDir.x++;
                    xPos--;
                }
                while(brickMoveDir.z > 0.5)
                {
                    brickMoveDir.z--;
                    zPos++;
                }
                while(brickMoveDir.z < -0.5)
                {
                    brickMoveDir.z++;
                    zPos--;
                }
            }
        }

        if(isBasic)
            basicChanged = true;
        else
            specialChanged = true;

        //If brick has an even length, such that it's x coordinate should end in .5
        /*if(effectiveWidth % 2 == 0)
        {
            //If it's x coordinate is a whole number
            if(fabs(position.x - std::round(position.x)) < 0.01)
                position.x += 0.5;
        }
        else if(fabs(position.x - std::round(position.x)) > 0.49)
        {
            //If it should end in a whole number but doesn't...
            position.x += 0.5;
        }

        if(effectiveLength % 2 == 0)
        {
            if(fabs(position.z - std::round(position.z)) < 0.01)
                position.z += 0.5;
        }
        else if(fabs(position.z - std::round(position.z)) > 0.49)
        {
            position.z += 0.5;
        }*/

        /*if(effectiveWidth % 2)
            position.x = floor(position.x);
        else
            position.x = floor(position.x+0.5)-0.5;

        if(effectiveLength % 2)
            position.z = floor(position.z);
        else
            position.z = floor(position.z+0.5)-0.5;*/

        return (oldSelect == -1) != (brickSlotSelected == -1);
    }

    void tempBrick::teleport(glm::vec3 pos)
    {
        if(!cart)
            return;

        if(cart->getChild("BrickText")->getText().length() < 1)
            return;

        xPos = pos.x;
        uYPos = std::round(pos.y/0.4);
        zPos = pos.z;

        if(isBasic)
        {
            uYPos += basic.dimensions.y/2;

            yHalfPos = (((int)basic.dimensions.y) % 2);
            if(rotationID % 2 == 0)
            {
                xHalfPos = (((int)basic.dimensions.x) % 2);
                zHalfPos = (((int)basic.dimensions.z) % 2);
            }
            else
            {
                zHalfPos = (((int)basic.dimensions.x) % 2);
                xHalfPos = (((int)basic.dimensions.z) % 2);
            }
            basicChanged = true;
        }
        else
        {
            uYPos += special.type->type->height/2;

            yHalfPos = (((int)special.type->type->height) % 2);
            if(rotationID % 2 == 0)
            {
                xHalfPos = (((int)special.type->type->width) % 2);
                zHalfPos = (((int)special.type->type->length) % 2);
            }
            else
            {
                zHalfPos = (((int)special.type->type->width) % 2);
                xHalfPos = (((int)special.type->type->length) % 2);
            }
            specialChanged = true;
        }

        /*position = pos;

        //TODO: Remove
        rotationID = 0;

        int width = 0;
        int height = 0;
        int length = 0;

        if(isBasic)
        {
            position.y += ((float)basic.dimensions.y) / 5.0;

            position.x = round(position.x);
            position.z = round(position.z);

            position.x += ((float)(((int)~basic.dimensions.x) & 1)) / 2.0;
            position.z += ((float)(((int)~basic.dimensions.z) & 1)) / 2.0;

            basicChanged = true;

            width = basic.dimensions.x;
            height = basic.dimensions.y;
            length = basic.dimensions.z;
        }
        else
        {
            position.y += ((float)special.type->type->height) / 5.0;

            position.x = round(position.x);
            position.z = round(position.z);

            position.x += ((float)(~special.type->type->width & 1)) / 2.0;
            position.z += ((float)(~special.type->type->length & 1)) / 2.0;

            specialChanged = true;

            width = special.type->type->width;
            height = special.type->type->height;
            length = special.type->type->length;
        }

        int effectiveWidth = 1;
        int effectiveLength = 1;
        if(rotationID % 2 == 0)
        {
            effectiveWidth = width;
            effectiveLength = length;
        }
        else
        {
            effectiveWidth = width;
            effectiveLength = length;
        }

        if(effectiveWidth % 2)
            position.x = floor(position.x);
        else
            position.x = floor(position.x+0.5)-0.5;

        if(effectiveLength % 2)
            position.z = floor(position.z);
        else
            position.z = floor(position.z+0.5)-0.5;

        double multi = 2.5;
        if(height % 2)
        {
            position.y = 0.2+floorl(position.y*multi)/multi;
        }
        else
        {
            position.y = floorl((0.2+position.y)*multi)/multi;
        }

        //position.y = floor(position.y*2.5)/2.5;*/
    }

    void tempBrick::plant(newBrickRenderer &renderer,btDynamicsWorld *world,serverStuff *serverConnection)
    {
        if(!cart)
            return;

        packet data;
        data.writeUInt(3,4);
        if(isBasic)
        {
            /*data.writeFloat(basic.position.x);
            data.writeFloat(basic.position.y);
            data.writeFloat(basic.position.z);*/

            data.writeUInt(xPos+8192,14);
            data.writeBit(xHalfPos);
            data.writeUInt(uYPos,16);
            data.writeBit(yHalfPos);
            data.writeUInt(zPos+8192,14);
            data.writeBit(zHalfPos);

            data.writeFloat(basic.color.r);
            data.writeFloat(basic.color.g);
            data.writeFloat(basic.color.b);
            data.writeFloat(basic.color.a);
            data.writeUInt(rotationID,2);
            data.writeUInt(mat,16);
            data.writeBit(isBasic);
            data.writeUInt(basic.dimensions.x,8);
            data.writeUInt(basic.dimensions.y,8);
            data.writeUInt(basic.dimensions.z,8);
        }
        else
        {
            int *preID = (int*)cart->getUserData();
            if(preID)
            {
                if(renderer.blocklandTypes->specialBrickTypes[*preID])
                {
                    data.writeUInt(xPos+8192,14);
                    data.writeBit(xHalfPos);
                    data.writeUInt(uYPos,16);
                    data.writeBit(yHalfPos);
                    data.writeUInt(zPos+8192,14);
                    data.writeBit(zHalfPos);

                    data.writeFloat(special.color.r);
                    data.writeFloat(special.color.g);
                    data.writeFloat(special.color.b);
                    data.writeFloat(special.color.a);
                    data.writeUInt(rotationID,2);
                    data.writeUInt(mat,16);
                    data.writeBit(isBasic);
                    data.writeUInt(renderer.blocklandTypes->specialBrickTypes[*preID]->serverID,10);
                }
                else
                {
                    error("Special brick type not found!");
                    return;
                }
            }
            else
            {
                error("Unknown brick selected!");
                return;
            }
        }

        serverConnection->connection->send(&data,true);
    }

    void tempBrick::update(newBrickRenderer &renderer,paletteGUI *palette)
    {
        if(!specialChanged && !basicChanged)
            return;

        if(brickSlotSelected == -1)
        {
            basic.dimensions = glm::ivec4(0);
            basic.position = glm::vec3(-9999);
            special.position = glm::vec3(-9999);
            special.color.a = 0;
            basic.color.a = 0;
            renderer.updateBasicBrick(&basic,0);
            renderer.updateSpecialBrick(&special,0,rotationID);
            specialChanged = false;
            basicChanged = false;
            return;
        }

        if(!cart)
        {
            //std::cout<<"No cart\n";
            return;
        }
        if(cart->getChild("BrickText")->getText().length() < 1)
        {
            //std::cout<<"No brick text\n";
            return;
        }

        if(isBasic)
        {
            special.position = glm::vec3(-9999);

            basic.color = palette->getColor();
            basic.position = glm::vec3(getX(),getY(),getZ());
            basic.rotation = renderer.rotations[rotationID];
        }
        else
        {
            basic.dimensions = glm::ivec4(0);
            basic.position = glm::vec3(-9999);

            special.color = palette->getColor();
            special.position = glm::vec3(getX(),getY(),getZ());
            special.rotation = renderer.rotations[rotationID];

            int *preID = (int*)cart->getUserData();
            if(preID)
            {
                int typeID = preID[0];
                special.typeID = typeID;
                special.type = renderer.transparentSpecialBrickTypes[typeID];
            }
        }

        if(basicChanged)
        {
            basic.color.a -= 0.015;
            renderer.updateBasicBrick(&basic,0);
            basic.color.a += 0.015;
            basicChanged = false;
        }
        if(specialChanged)
        {
            special.color.a -= 0.015;
            renderer.updateSpecialBrick(&special,0,rotationID);
            special.color.a += 0.015;
            specialChanged = false;
        }
    }
}
