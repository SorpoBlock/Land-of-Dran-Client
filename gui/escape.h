#ifndef ESCAPE_H_INCLUDED
#define ESCAPE_H_INCLUDED

#include <iostream>
#include "code/utility/ceguiHelper.h"
#include "code/networking/recv.h"
#include "code/utility/clientStuff.h"

namespace syj
{
    CEGUI::Window *addEscapeMenu(clientStuff *clientEnvironment,uniformsHolder *basicShader);
}

#endif // ESCAPE_H_INCLUDED
