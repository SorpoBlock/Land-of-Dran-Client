#ifndef CEGUIHELPER_H_INCLUDED
#define CEGUIHELPER_H_INCLUDED

#include <SDL2/SDL.h>
#include <CEGUI/CEGUI.h>
#define GLEW_NO_GLU
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
#include <CEGUI/ImageManager.h>
#include <iostream>

CEGUI::URect makeRelArea(float x,float y,float w,float h);

//Loads a GUI skin and creates a root window
CEGUI::Window *initLoadCEGUISkin(std::string skinName,int resX,int resY);
CEGUI::Window *addGUIFromFile(std::string fileName);
//Adds a line of text to a textbox in the most basic form possible
void textBoxAdd(CEGUI::Window *box,std::string text,int id = 0,bool showSelect = true);
//Coverts keycodes for use in handling events, used in processEventsCEGUI
CEGUI::Key::Scan SDLKeyToCEGUIKey(SDL_Keycode key);
//Helper function to add a selection to a drop down box
void dropBoxAdd(CEGUI::Window *box,std::string name,unsigned int value);
//Passes all of the input SDL records to CEGUI each frame
void processEventsCEGUI(SDL_Event &event,const Uint8 *keystates);

#endif // CEGUIHELPER_H_INCLUDED
