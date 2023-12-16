#ifndef JOINSERVER_H_INCLUDED
#define JOINSERVER_H_INCLUDED

#include "code/utility/clientStuff.h"
#include "code/utility/ceguiHelper.h"
#include "code/networking/recv.h"
#include <curl/curl.h>

namespace syj
{
    CEGUI::Window *loadJoinServer(clientStuff *serverData);
    void checkForSessionKey(clientStuff *clientEnvironment,preferenceFile &prefs);
}

#endif // JOINSERVER_H_INCLUDED
