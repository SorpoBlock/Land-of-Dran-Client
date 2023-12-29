#ifndef INPUTMAP_H_INCLUDED
#define INPUTMAP_H_INCLUDED

#include "code/utility/preference.h"
#include "code/utility/logger.h"
#include <SDL2/SDL.h>

namespace syj
{
    //A list of things to which one may bind keys:
    enum inputCommand
    {
        nothingBound = 0,           //Indicates no command is bound to a particular key
        toggleMouseLock = 1,        //Frees the mouse or locks it for looking around with the mouse
        walkForward = 2,            //Walk or fly relative to direction camera is looking:
        walkBackward = 3,
        walkRight = 4,
        walkLeft = 5,
        jump = 6,
        zoom = 7,
        dropPlayerAtCamera = 8,
        openOptions = 9,
        changeCameraMode = 10,
        openBrickSelector = 11,
        changePaintColumn = 12,
        useBrick1 = 13,
        useBrick2 = 14,
        useBrick3 = 15,
        useBrick4 = 16,
        useBrick5 = 17,
        useBrick6 = 18,
        useBrick7 = 19,
        useBrick8 = 20,
        useBrick9 = 21,
        hideBricks = 22,
        moveBrickForward = 23,
        moveBrickBackward = 24,
        moveBrickLeft = 25,
        moveBrickRight = 26,
        moveBrickUp = 27,
        moveBrickDown = 28,
        plantBrick = 29,
        resizeToggle = 30,
        superShift = 31,
        rotateBrick = 32,
        changeMaterial = 33,
        startSelection = 34,
        openEvalWindow = 35,
        openInventory = 36,
        rotateBrickBackwards = 37,
        moveBrickUpThree = 38,
        moveBrickDownThree = 39,
        toggleGUI = 40,
        dropCameraAtPlayer = 41,
        playersListButton = 42,
        debugInfo = 43,
        debugMode = 44,
        endOfCommandEnum = 45        //inputCommands::endOfEnum gives the size of the enum
    };

    std::string inputCommandToString(inputCommand command);

    //Basically this is an intermediate layer that allows the player to make custom key binds
    //It uses the SDL event polling frame work, filters it through a preference file, and provides its own event polling framework
    struct inputMap
    {
        protected:

        //Data directly from SDL_GetKeyboardState
        const Uint8 *keystates = nullptr;
        //One possible command for each key
        SDL_Scancode keyForCommand[inputCommand::endOfCommandEnum];

        //Is there a command to process?
        bool keyToProcess[inputCommand::endOfCommandEnum];

        //We can't process a new instance of a command until the key has been released, and the last instance has been processed:
        bool keyPressed[inputCommand::endOfCommandEnum];
        bool keyPolled[inputCommand::endOfCommandEnum];

        bool supressed = false;

        public:

        void resetKeyPresses()
        {
            for(int a = 0; a<inputCommand::endOfCommandEnum; a++)
            {
                keyPolled[a] = false;
                keyToProcess[a] = false;
                keyPressed[a] = false;
            }
        }

        SDL_Scancode getBoundKey(inputCommand forWhat)
        {
            if(forWhat > nothingBound && forWhat < endOfCommandEnum)
                return keyForCommand[forWhat];
            else
                return (SDL_Scancode)0;
        }

        //Bind a key to a particular command
        void bindKey(inputCommand command,SDL_Scancode key);

        //Call once per frame
        void getKeyStates(){keystates = SDL_GetKeyboardState(NULL);}

        //Your code should look like this:
        /* while(SDL_PollEvent(&event))
           {
                doOtherStuffWithEvent(event);
                myInputMap.handleInput(event);
           }
        */
        void handleInput(SDL_Event &event);

        //This checks if the key bound to a command is *currently* pressed down
        bool commandKeyDown(inputCommand command);

        //Checks and then clears flag for a key press of a given command
        //E.g. was the key pressed at some point since the last call to commandPressed
        bool commandPressed(inputCommand command);

        //Add key binds to preference holder
        void toPrefs(preferenceFile *file);

        //Get key binds from preference holder
        void fromPrefs(preferenceFile *file);

        void supress(bool in)
        {
            supressed = in;
        }

        //Specify a file path in order to load from a file, or else default bindings will be made
        inputMap(std::string filePath = "");
    };

}

#endif // INPUTMAP_H_INCLUDED
