#ifndef JOINSERVER_H_INCLUDED
#define JOINSERVER_H_INCLUDED

#include "code/utility/ceguiHelper.h"
#include "code/networking/recv.h"
#include <CURL/curl.h>

namespace syj
{
    CEGUI::Window *loadJoinServer(serverStuff *ohWow);
}

#endif // JOINSERVER_H_INCLUDED
