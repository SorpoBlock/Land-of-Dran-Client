#ifndef EVALWINDOW_H_INCLUDED
#define EVALWINDOW_H_INCLUDED

#include "code/utility/ceguiHelper.h"
#include "code/networking/recv.h"
#include <bearssl/bearssl_hash.h>

namespace syj
{
    bool closeEvalWindow(const CEGUI::EventArgs &e);
    CEGUI::Window *configureEvalWindow(CEGUI::Window *hud,serverStuff *common);
}

#endif // EVALWINDOW_H_INCLUDED
