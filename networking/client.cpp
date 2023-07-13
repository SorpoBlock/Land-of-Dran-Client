#include "client.h"

namespace syj
{
    //Returns latest error then clears it
    syjError client::getLastError()
    {
        syjError ret = lastError;
        lastError = syjError::noError;
        return ret;
    }

    bool client::waitForConnectionResponse()
    {
        unsigned int start = SDL_GetTicks();

        //Use last packet for this
        UDPpacket *toUse = sdlPackets[prefs.maxPackets-1];

        while(SDL_GetTicks() - start < prefs.timeoutMS)
        {
            //Don't let CPU go crazy
            SDL_Delay(1);

            //If this worked right, timeoutMS would only really work to about 0.5 sec resolution
            //It doesn't actually seem to wait 500ms though
            if(SDLNet_CheckSockets(socketSet,500) != 1)
                continue;

            //Shouldn't happen, but sometimes it does oddly enough.
            if(!SDLNet_SocketReady(mainSocket))
            {
                lastError = syjError::readyButNoData;
                continue;
            }

            //Same as above
            if(SDLNet_UDP_Recv(mainSocket,toUse) != 1)
            {
                lastError = syjError::readyButNoData;
                continue;
            }

            //Copy response into one of our pakcets
            packet response(toUse->data,toUse->len);

            //Server said it was full
            if(response.readUInt(32) == syjNET_serverFullMessage)
            {
                lastError = syjError::serverFull;
                return false;
            }

            //Go to start of packet again since the server wasn't full
            response.seek(0);

            //The server isn't a proper syjNet server
            if(response.readUInt(32) != syjNET_acceptClientMessage)
            {
                lastError = syjError::improperPacket;
                //TODO: Change this to return false?
                continue;
            }

            //Get networking preferences from the server
            clientIDBits = response.readUInt(8);
            clientID = response.readUInt(clientIDBits);
            packetIDBits = response.readUInt(8);

            //That's all we need, we're ready!
            return true;
        }

        //We didn't ever manage to get a response
        lastError = syjError::noConnection;
        return false;
    }

    //Create a client and connect to the server using an IP and some other prefs
    client::client(networkingPreferences &_prefs,std::string ip)
    {
        //Basic init
        userData = 0;
        timeOfOldestAck = 0;
        lastError = syjError::noError;
        prefs = _prefs;
        kickHandle = 0;
        receiveHandle = 0;
        sdlPackets = 0;
        socketSet = 0;
        mainSocket = 0;

        //Prefs checks:

        //I don't believe you can even host on port 0, passing 0 as port on the server just results in a random port being chosen by SDL
        if(prefs.port == 0)
        {
            lastError = syjError::badPreference;
            return;
        }

        //Need at least one packet
        if(prefs.maxPackets == 0)
        {
            lastError = syjError::badPreference;
            return;
        }

        //Need space for headers and such
        if(prefs.maxPacketSize < 8)
        {
            lastError = syjError::badPreference;
            return;
        }

        //I'm assuming if it was set so low that was a mistake, maybe not?
        if(prefs.timeoutMS < 1000)
        {
            lastError = syjError::badPreference;
            return;
        }

        //Client chooses a random remote port, doesn't really matter
        //The port argument is used in the packets themselves
        mainSocket = SDLNet_UDP_Open(0);
        if(!mainSocket)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Allocate space for outgoing and incoming packets
        sdlPackets = SDLNet_AllocPacketV(prefs.maxPackets,prefs.maxPacketSize);
        if(!sdlPackets)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Here's where we actually use the IP and port
        SDLNet_ResolveHost(&serverIP,ip.c_str(),prefs.port);

        //DNS resolution of a domain did not work or other similar error
        if(serverIP.host == INADDR_NONE)
        {
            lastError = syjError::noConnection;
            return;
        }

        //Assign IP to all packets, the packets themselves are what are given an address in UDP
        for(unsigned int a = 0; a<prefs.maxPackets; a++)
        {
            sdlPackets[a]->channel = -1;
            sdlPackets[a]->address = serverIP;
        }

        //Allocate space for socket set (used to check for incoming data)
        socketSet = SDLNet_AllocSocketSet(1);
        if(!socketSet)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Shouldn't really be any issues here.
        if(SDLNet_UDP_AddSocket(socketSet,mainSocket) != 1)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Now that we've checked our prefs and allocated our stuff, it's time to actually connect!
        packet *connectionPacket = new packet;

        //Magic number is all we need
        connectionPacket->writeUInt(syjNET_clientConnectMessage,32);

        //Actually send packet
        sendPacket(connectionPacket);
        delete connectionPacket;

        //No need to set error, the function did that for us
        if(!waitForConnectionResponse())
            return;

        //We connected!

        //Set a few other things
        nextPacketID = 0;
        maxPacketID = (1 << packetIDBits) - 1;
        maxAcksInPacket = (8*(prefs.maxPacketSize-4))/prefs.criticalIDBits;

        //And we have a client set up and ready to go!
    }

    //Send a packet without adding anything or waiting at all
    bool client::sendPacket(packet *data)
    {
        //Use the last packet allocated for this purpose
        UDPpacket *toUse = sdlPackets[prefs.maxPackets-1];
        //Length is only to current stream pos, assumes source packet had just been written to
        //or someone called seek/move to the end of what they wanted to copy
        toUse->len = (data->getStreamPos()+7)/8;

        //If the length is too big or 0
        if(toUse->len >= prefs.maxPacketSize || toUse->len == 0)
        {
            lastError = syjError::badArgument;
            return false;
        }

        //Actually copy data
        memcpy(toUse->data,data->data,toUse->len);

        //Actually send
        if(SDLNet_UDP_Send(mainSocket,-1,toUse) != 1)
        {
            lastError = syjError::didNotSend;
            return false;
        }

        return true;
    }

    bool client::receivePackets()
    {
        //Any data waiting to be received
        if(SDLNet_CheckSockets(socketSet,0) != 1)
            return true;

        //Shouldn't happen, but sometimes it does oddly enough.
        if(!SDLNet_SocketReady(mainSocket))
        {
            lastError = syjError::readyButNoData;
            return false;
        }

        //Same as above
        int received = SDLNet_UDP_RecvV(mainSocket,sdlPackets);
        if(received < 1)
        {
            lastError = syjError::readyButNoData;
            return false;
        }

        numPacketsReceived += received;

        //Convert packets and put them in the processing queue
        for(int a = 0; a<received; a++)
        {
            //If the packet has a different IP than our server, discard it
            if(sdlPackets[a]->address.host != serverIP.host)
                continue;

            packet *data = new packet(sdlPackets[a]->data,sdlPackets[a]->len);
            //Just in case I change how the constructor works, here's a seek function call:
            data->seek(0);
            toProcess.push_back(data);
        }

        return true;
    }

    bool client::processKickPacket(packet *data)
    {
        //This is the entire rest of the packet
        unsigned int reason = data->readUInt(32);

        //If there wasn't a valid kick reason
        if(!(reason == syjNET_clientKickedMessage || reason == syjNET_compromisedMessage || reason == syjNET_timedOutMessage))
        {
            lastError = syjError::improperPacket;
            //Still need to let end user know they were kicked
            if(kickHandle)
                kickHandle(this,reason,userData);
            delete data;
            data = 0;
            return false;
        }

        //Let end user know
        if(kickHandle)
            kickHandle(this,reason,userData);

        delete data;
        data = 0;
        return true;
    }

    bool client::processNormalPacket(packet *data)
    {
        //Not a whole lot to do here, just give end user the packet
        if(receiveHandle)
            receiveHandle(this,data,userData);

        delete data;
        data = 0;
        return true;
    }

    bool client::processStreamPacket(packet *data)
    {
        bool returnValue = true;
        delete data;
        data = 0;
        return returnValue;
    }

    bool client::sendCriticalPackets(int maxPackets)
    {
        //TODO: I just copied this from server heh

        //If we have any critical packets in queue
        int packetsToSend = criticalPackets.size();
        if(packetsToSend > 0)
        {
            //Cap it in case of network issues
            if(packetsToSend > maxPackets)
                packetsToSend = maxPackets;

            //We're figuring out which packets haven't been resent in the longest period of time
            std::vector<packet*> oldestPackets;
            for(int a = 0; a<packetsToSend; a++)
            {
                //Shouldn't be an issue but just in case
                if(!criticalPackets[a])
                    continue;

                //No matter what never resend a packet if it it's been less than packetResendMS milliseconds since its last send
                if(SDL_GetTicks() - criticalPackets[a]->creationTime < prefs.packetResendMS)
                    continue;

                //If there's not enough packets in the vector yet, just add the next packet
                //TODO: Signed?
                if((signed)oldestPackets.size() < maxPackets)
                {
                    oldestPackets.push_back(criticalPackets[a]);
                    continue;
                }

                //sendCriticalPackets is at worst an O(10*n) function with respect to amount of queued critical packets. Neat.
                //TODO: Actual sorting algorithm?
                for(unsigned int b = 0; b<oldestPackets.size(); b++)
                {
                    //If one of the packets in our little vector is newest (higher creation time) replace it
                    if(oldestPackets[b]->creationTime > criticalPackets[a]->creationTime)
                    {
                        oldestPackets[b] = criticalPackets[a];
                        break;
                    }
                }
            }

            for(unsigned int a = 0; a<oldestPackets.size(); a++)
            {
                //Update last send time
                oldestPackets[a]->creationTime = SDL_GetTicks();

                //Push it back to the queued packets vector, this one actually sends the packets
                //TODO: Copy it or just rewrite sendPacketQueues to not delete critical packets hmm
                queuedPackets.push_back(new packet(*oldestPackets[a]));
                numSentCrits++;
            }
        }

        //TODO: How would this function mess up and throw an error?
        return true;
    }

    bool client::send(packet *data,bool critical)
    {
        if(critical)
        {
            //critical packets start with 0b110
            packet *criticalPacket = new packet;
            criticalPacket->writeUInt(clientID,clientIDBits);

            criticalPacket->writeBit(true);
            criticalPacket->writeBit(true);
            criticalPacket->writeBit(false);

            //Write the packet ID
            criticalPacket->writeUInt(nextPacketID,prefs.criticalIDBits);
            criticalPacket->packetID = nextPacketID;
            //Let sender be able to read what packetID their packet got sent with
            data->packetID = nextPacketID;

            //Ensures it is sent out asap
            criticalPacket->creationTime = 0;

            //Increment the packet ID
            nextPacketID++;
            if(nextPacketID >= maxPacketID)
                nextPacketID = 0;

            //Copy over actual packet to send
            criticalPacket->writePacket(*data);

            //It'll get send one day
            criticalPackets.push_back(criticalPacket);
        }
        else
        {
            //normal packets start with 0b10
            packet *normalPacket = new packet;
            normalPacket->writeUInt(clientID,clientIDBits);
            normalPacket->writeBit(true);
            normalPacket->writeBit(false);

            //Copy over the data to send
            normalPacket->writePacket(*data);

            //Put it on the fast list to get sent hopefully this tick or in the next few
            queuedPackets.push_back(normalPacket);
        }

        //No real known way for this to fail yet
        return true;
    }

    bool client::processCriticalPacket(packet *data)
    {
        bool returnValue = true;

        //Get the packet ID
        unsigned int id = data->readUInt(prefs.criticalIDBits);

        mostRecentServerCritID = id;
        if(id > highestServerCritID)
            highestServerCritID = id;

        idsToAcknowledge.push_back(id);
        if(timeOfOldestAck == 0 || idsToAcknowledge.size() == 1)
            timeOfOldestAck = SDL_GetTicks();

        for(unsigned int a = 0; a<idsToIgnore.size(); a++)
        {
            if(idsToIgnore[a] == id)
            {
                //The packet was already sent to us, and is a resend, discard it
                numReceivedResends++;
                delete data;
                data = 0;
                return returnValue;
            }
        }

        //Ignore future packets with this ID until we get a clear packet and be sure to acknowledge it
        idsToIgnore.push_back(id);

        //Record the time we started building the current idsToAcknowledge queue

        //In case the end user cares
        data->critical = true;
        data->packetID = id;

        if(receiveHandle)
            receiveHandle(this,data,userData);

        delete data;
        data = 0;
        return returnValue;
    }

    bool client::processAcknowledgePacket(packet *data)
    {
        //How many packets to clear from criticalPackets
        unsigned int toAcknowledge = data->readUInt(10);

        //Like processClearPacket, if it's telling us to acknowledge 0 packets that's probably an error on their end
        if(toAcknowledge == 0)
        {
            lastError = syjError::improperPacket;
            delete data;
            data = 0;
            return false;
        }

        numReceivedAcks++;

        //Respond to an acknowledge packet with a clear (acknowledgment of acknowledgment) packet
        packet *clearPacket = new packet;
        clearPacket->writeUInt(clientID,clientIDBits);
        //The header for a clear packet
        clearPacket->writeBit(true);
        clearPacket->writeBit(true);
        clearPacket->writeBit(true);
        clearPacket->writeBit(true);
        clearPacket->writeBit(false);

        //It's just a number by number copy of the packet the server send us
        //TODO: Just copy the entire data stream directly and at once?
        clearPacket->writeUInt(toAcknowledge,10);

        for(unsigned int a = 0; a<toAcknowledge; a++)
        {
            //What's the actual ID of the packet to acknowledge
            unsigned int theID = data->readUInt(prefs.criticalIDBits);

            //Tell the server to clear each packet they told us to acknowledge
            clearPacket->writeUInt(theID,prefs.criticalIDBits);

            //Search for the packet and see if we're still trying to resend it, if so, delete
            for(unsigned int b=0; b<criticalPackets.size(); b++)
            {
                if(criticalPackets[b] == 0)
                    continue;
                if(criticalPackets[b]->packetID == theID)
                {
                    delete criticalPackets[b];
                    criticalPackets[b] = 0;
                    break;
                }
            }
        }

        queuedPackets.push_back(clearPacket);

        delete data;
        data = 0;
        return true;
    }

    bool client::processClearPacket(packet *data)
    {
        //How many ids to clear from the ignore list
        unsigned int toClear = data->readUInt(10);

        //Well I mean there's gotta be one id or else what was the point of the packet
        if(toClear == 0)
        {
            //That's an error on the senders end
            lastError = syjError::improperPacket;
            delete data;
            data = 0;
            return false;
        }

        numReceivedClears++;

        for(unsigned int a = 0; a<toClear; a++)
        {
            //Read the actual ID off the list of ids that makes up the rest of the packet
            unsigned int theID = data->readUInt(prefs.criticalIDBits);

            if(theID == 5000)
                std::cout<<"CLEAR 5000\n";

            //Push the ids that match and are to clear to the end of the packet
            auto it = remove(idsToIgnore.begin(),idsToIgnore.end(),theID);
            //If there was at least one ID pushed to the end of the packet, resize the vector to omit those ids
            if(it != idsToIgnore.end())
                idsToIgnore.erase(it,idsToIgnore.end());
        }

        delete data;
        data = 0;
        return true;
    }

    //Sort a certain number of packets to their specific functions, true on success
    bool client::processPackets(unsigned int numPackets)
    {
        if(numPackets == 0)
            return true;

        bool returnValue = true;

        if(numPackets > toProcess.size())
            numPackets = toProcess.size();

        for(unsigned int a = 0; a<numPackets; a++)
        {
            if(!toProcess[a])
                continue;

            packet *current = toProcess[a];
            current->seek(0);

            //Sort packet into it's correct processing function:

            if(!current->readBit())
            {
                if(!processStreamPacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!processNormalPacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!processCriticalPacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!processAcknowledgePacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!processClearPacket(current))
                    returnValue = false;
                continue;
            }
            else
            {
                if(!processKickPacket(current))
                    returnValue = false;
                continue;
            }
        }

        //Resize array to get rid of processed packets (note: does not delete)
        toProcess.erase(toProcess.begin(),toProcess.begin() + numPackets);

        return returnValue;
    }

    //Doesn't actually send any packets, just queues ack packets until there's no more ids left to ack this tick
    bool client::sendAcknowledgements()
    {
        bool returnValue = true;

        //if(idsToAcknowledge.size() > 0)
          //  std::cout<<"To ack: "<<idsToAcknowledge.size()<<"\n";
        //Just send all packet IDs that need to be acknowledged this tick
        while(idsToAcknowledge.size() > 0)
        {
            //This will be set to the current time once the stack receives its first ID to acknowledge next time around
            timeOfOldestAck = 0;

            unsigned int acksToSend = idsToAcknowledge.size();

            //Can't fit all of them in one packet
            if(acksToSend >= maxAcksInPacket)
                acksToSend = maxAcksInPacket-1;
            //Number of ids in packet is stored in a 10 bit number, limit it in the unlikely case someone's using massive packets.
            if(acksToSend >= 1023)
                acksToSend = 1023;

            packet *ackPacket = new packet;
            ackPacket->writeUInt(clientID,clientIDBits);
            ackPacket->writeBit(true);  //Ack packets start with 0b1110
            ackPacket->writeBit(true);
            ackPacket->writeBit(true);
            ackPacket->writeBit(false);

            //Tell the recipient how many ids are in this packet
            ackPacket->writeUInt(acksToSend,10);

            //Write the actual ids to the packet
            for(unsigned int b = 0; b<acksToSend; b++)
                ackPacket->writeUInt(idsToAcknowledge[b],prefs.criticalIDBits);

            //Send that packet off someday
            numSentAcks++;
            queuedPackets.push_back(ackPacket);

            //Clear all of the acknowledged ids from the to acknowledge list
            idsToAcknowledge.erase(idsToAcknowledge.begin(),idsToAcknowledge.begin() + acksToSend);
        }

        return returnValue;
    }

    bool client::sendQueuedPackets(int maxPackets)
    {
        bool returnValue = true;

        //Same code as serverside pretty much

        int packetsToSend = queuedPackets.size();
        if(packetsToSend > 0)
        {
            //Cap it in case of network issues
            //TODO: Figure out dynamic maximum amount of packets to send out
            if(packetsToSend > maxPackets)
                packetsToSend = maxPackets;

            //Copy data to UDPpackets
            for(int b = 0; b<packetsToSend; b++)
            {
                if(!queuedPackets[b])
                    continue;

                //Assumes stream pos is at end of relevant data
                sdlPackets[b]->len = (queuedPackets[b]->getStreamPos()+7)/8;
                memcpy(sdlPackets[b]->data,queuedPackets[b]->data,sdlPackets[b]->len);

                //Don't need the packet anymore
                delete queuedPackets[b];
                queuedPackets[b] = 0;
            }

            //Actually send all of the packets we're going to this tick
            numPacketsSent += packetsToSend;
            if(SDLNet_UDP_SendV(mainSocket,sdlPackets,packetsToSend) != packetsToSend)
            {
                lastError = syjError::didNotSend;
                returnValue = false;
            }

            //Clear the sent packets off the list
            queuedPackets.erase(queuedPackets.begin(),queuedPackets.begin() + packetsToSend);
        }

        return returnValue;
    }

    bool client::run()
    {
        bool returnValue = true;

        //TODO: Dynamic values for these functions

        if(!receivePackets())
            returnValue = false;

        if(!processPackets(100))
            returnValue = false;

        //Resize criticalPacket array to remove critical packets that were acknowledged in processPackets
        std::vector<packet*>::iterator iter;
        for(iter = criticalPackets.begin(); iter != criticalPackets.end();)
        {
            packet *current = (*iter);
            if(!current)
            {
                iter = criticalPackets.erase(iter);
                continue;
            }

            ++iter;
        }

        //Don't bother sending acknowledgments more often than every quarter second
        //That way we can fit more in a single packet
        if(SDL_GetTicks() - timeOfOldestAck > 250)
        {
            if(!sendAcknowledgements())
                returnValue = false;
        }

        if(!sendCriticalPackets(40))
            returnValue = false;

        if(!sendQueuedPackets(50))
            returnValue = false;

        return returnValue;
    }

    client::~client()
    {
        if(socketSet)
            SDLNet_FreeSocketSet(socketSet);
        if(mainSocket)
            SDLNet_UDP_Close(mainSocket);
        if(sdlPackets)
            SDLNet_FreePacketV(sdlPackets);
        for(unsigned int a = 0; a<queuedPackets.size(); a++)
            if(queuedPackets[a])
                delete queuedPackets[a];
        for(unsigned int a = 0; a<toProcess.size(); a++)
            if(toProcess[a])
                delete toProcess[a];
        for(unsigned int a = 0; a<criticalPackets.size(); a++)
            if(criticalPackets[a])
                delete criticalPackets[a];
    }

    std::string client::printDebugs() const
    {
        std::stringstream ret;
        ret<<"Number of packets sent: "<<numPacketsSent<<"\n";
        ret<<"Number of packets received: "<<numPacketsReceived<<"\n";
        ret<<"Number of critical packets queued: "<<numSentCrits<<"\n";
        ret<<"Number of duplicate critical packets received: "<<numReceivedResends<<"\n";
        ret<<"Number of ack packets received: "<<numReceivedAcks<<"\n";
        ret<<"Number of clear packets received: "<<numReceivedClears<<"\n";
        ret<<"Number of sent acknowledgments: "<<numSentAcks<<"\n";
        ret<<"Number of critical packets unacknowledged: "<<criticalPackets.size()<<"\n";
        return ret.str();
    }
}
