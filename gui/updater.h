#ifndef UPDATER_H_INCLUDED
#define UPDATER_H_INCLUDED

#include "code/utility/fileOpeartions.h"
#include "code/utility/ceguiHelper.h"
#include "code/utility/logger.h"
#include "code/networking/recv.h"
#include <CURL/curl.h>

namespace syj
{
    void getVersions(int &releaseVersion,int &networkVersion,std::string domain = "dran.land");
    CEGUI::Window *addUpdater(serverStuff *ohWow);
}

#endif // UPDATER_H_INCLUDED
