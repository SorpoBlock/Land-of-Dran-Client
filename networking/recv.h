#ifndef RECV_H_INCLUDED
#define RECV_H_INCLUDED

#include "code/utility/clientStuff.h"

#define clientPacketType_requestName 0
#define clientPacketType_startLoadingStageTwo 1
#define clientPacketType_clicked 8

std::string GetHexRepresentation(const unsigned char *Bytes, size_t Length);

namespace syj
{
    void checkForCameraToBind(serverStuff *serverData);

    void recvHandle(client *theClient,packet *data,void *userData);
}

#endif // RECV_H_INCLUDED
