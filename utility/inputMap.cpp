#include "inputMap.h"

namespace syj
{

    std::string inputCommandToString(inputCommand command)
    {
        switch(command)
        {
            case nothingBound: return "No command";
            case toggleMouseLock: return "Toggle-Mouse-Lock";
            case walkForward: return "Walk-Forward";
            case walkBackward: return "Walk-Backward";
            case walkRight: return "Walk-Right";
            case walkLeft: return "Walk-Left";
            case jump: return "Jump";
            case zoom: return "Zoom";
            case dropPlayerAtCamera: return "DropPlayerAtCamera";
            case openOptions: return "OpenOptions";
            case changeCameraMode: return "ChangeCameraMode";
            case openBrickSelector: return "OpenBrickSelector";
            case changePaintColumn: return "Change Paint Column";
            case useBrick1: return "Use Brick 1";
            case useBrick2: return "Use Brick 2";
            case useBrick3: return "Use Brick 3";
            case useBrick4: return "Use Brick 4";
            case useBrick5: return "Use Brick 5";
            case useBrick6: return "Use Brick 6";
            case useBrick7: return "Use Brick 7";
            case useBrick8: return "Use Brick 8";
            case useBrick9: return "Use Brick 9";
            case hideBricks: return "Put Bricks Away";
            case moveBrickForward: return "Brick Forward";
            case moveBrickBackward: return "Brick Backward";
            case moveBrickLeft: return "Brick Left";
            case moveBrickRight: return "Brick Right";
            case moveBrickUp: return "Move 1 Plate Up";
            case moveBrickDown: return "Move 1 Plate Down";
            case plantBrick: return "Plant Brick";
            case resizeToggle: return "Resize/Move toggle";
            case superShift: return "Toggle Super Shift";
            case rotateBrick: return "Rotate Brick";
            case changeMaterial: return "Change Material";
            case startSelection: return "Start Selection";
            case openEvalWindow: return "Open Eval Console";
            case openInventory: return "Open/Close Inventory";
            case rotateBrickBackwards: return "Rotate Brick Counter-Clockwise";
            case moveBrickUpThree: return "Move 1 Brick Up";
            case moveBrickDownThree: return "Move 1 Brick Down";
            case endOfCommandEnum: return "Error 1";
            default: return "Error 2";
        }
    }

    void inputMap::toPrefs(preferenceFile *file)
    {
        for(unsigned int a = 1; a<inputCommand::endOfCommandEnum; a++)
        {
            file->addIntegerPreference(inputCommandToString((inputCommand)a),keyForCommand[a]);
            file->set(inputCommandToString((inputCommand)a),keyForCommand[a]);
        }
    }

    void inputMap::fromPrefs(preferenceFile *file)
    {
        for(unsigned int a = 1; a<inputCommand::endOfCommandEnum; a++)
            if(preference *pref = file->getPreference(inputCommandToString((inputCommand)a)))
                keyForCommand[a] = (SDL_Scancode)pref->toInteger();
    }

    inputMap::inputMap(std::string filePath)
    {
        bindKey(toggleMouseLock,SDL_SCANCODE_M);
        bindKey(walkForward,SDL_SCANCODE_W);
        bindKey(walkBackward,SDL_SCANCODE_S);
        bindKey(walkRight,SDL_SCANCODE_A);
        bindKey(walkLeft,SDL_SCANCODE_D);
        bindKey(jump,SDL_SCANCODE_SPACE);
        bindKey(zoom,SDL_SCANCODE_F);
        bindKey(dropPlayerAtCamera,SDL_SCANCODE_F7);
        bindKey(openOptions,SDL_SCANCODE_O);
        bindKey(changeCameraMode,SDL_SCANCODE_TAB);
        bindKey(openBrickSelector,SDL_SCANCODE_B);
        bindKey(changePaintColumn,SDL_SCANCODE_E);
        bindKey(useBrick1,SDL_SCANCODE_1);
        bindKey(useBrick2,SDL_SCANCODE_2);
        bindKey(useBrick3,SDL_SCANCODE_3);
        bindKey(useBrick4,SDL_SCANCODE_4);
        bindKey(useBrick5,SDL_SCANCODE_5);
        bindKey(useBrick6,SDL_SCANCODE_6);
        bindKey(useBrick7,SDL_SCANCODE_7);
        bindKey(useBrick8,SDL_SCANCODE_8);
        bindKey(useBrick9,SDL_SCANCODE_9);
        bindKey(hideBricks,SDL_SCANCODE_0);
        bindKey(moveBrickForward,SDL_SCANCODE_I);
        bindKey(moveBrickBackward,SDL_SCANCODE_K);
        bindKey(moveBrickLeft,SDL_SCANCODE_J);
        bindKey(moveBrickRight,SDL_SCANCODE_L);
        bindKey(moveBrickUp,SDL_SCANCODE_PERIOD);
        bindKey(moveBrickDown,SDL_SCANCODE_COMMA);
        bindKey(plantBrick,SDL_SCANCODE_RETURN);
        bindKey(resizeToggle,SDL_SCANCODE_RSHIFT);
        bindKey(superShift,SDL_SCANCODE_LALT);
        bindKey(rotateBrick,SDL_SCANCODE_U);
        bindKey(changeMaterial,SDL_SCANCODE_EQUALS);
        bindKey(startSelection,SDL_SCANCODE_INSERT);
        bindKey(openInventory,SDL_SCANCODE_Q);
        bindKey(openEvalWindow,SDL_SCANCODE_F12);
        bindKey(rotateBrickBackwards,SDL_SCANCODE_KP_7);
        bindKey(moveBrickUpThree,SDL_SCANCODE_P);
        bindKey(moveBrickDownThree,SDL_SCANCODE_SEMICOLON);

        if(filePath == "")
            return;

        preferenceFile bindFile;
        if(bindFile.importFromFile(filePath))
            fromPrefs(&bindFile);

        preferenceFile outFile;
        toPrefs(&outFile);
        outFile.exportToFile(filePath);

        for(int a = 0; a<inputCommand::endOfCommandEnum; a++)
        {
            keyPolled[a] = false;
            keyPressed[a] = false;
            keyToProcess[a] = false;
        }
    }


    void inputMap::bindKey(inputCommand command,SDL_Scancode key)
    {
        if(command == nothingBound || command == inputCommand::endOfCommandEnum)
            return;
        keyForCommand[command] = key;
    }

    void inputMap::handleInput(SDL_Event &event)
    {
        if(event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
            return;

        if(supressed)
            return;

        for(unsigned int a = 1; a<inputCommand::endOfCommandEnum; a++)
        {
            if(keyForCommand[a] == event.key.keysym.scancode)
            {
                if(event.type == SDL_KEYDOWN)
                {
                    keyPressed[a] = true;
                    if(!keyToProcess[a])
                    {
                        keyToProcess[a] = true;
                        keyPolled[a] = false;
                    }
                }
                else
                {
                    keyPressed[a] = false;
                    if(keyPolled[a])
                    {
                        keyToProcess[a] = false;
                        keyPolled[a] = false;
                    }
                }
            }
        }
    }

    bool inputMap::commandPressed(inputCommand command)
    {
        if(supressed)
            return false;

        if(command == inputCommand::endOfCommandEnum)
            return false;

        if(keyToProcess[command])
        {
            if(!keyPolled[command])
            {
                if(!keyPressed[command])
                {
                    keyPolled[command] = false;
                    keyToProcess[command] = false;
                    keyPressed[command] = false;
                }
                else
                    keyPolled[command] = true;
                return true;
            }
        }

        return false;
    }

    bool inputMap::commandKeyDown(inputCommand command)
    {
        if(supressed)
            return false;

        scope("inputMap::commandKeyDown");
        if(keystates == nullptr)
        {
            error("Need to call getKeyStates first");
            return false;
        }

        if(command == inputCommand::endOfCommandEnum || command == inputCommand::nothingBound)
            return false;

        return keystates[keyForCommand[command]];
    }

}
