#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "code/networking/common.h"
#include "code/networking/packet.h"
#include "code/networking/errors.h"

namespace syj
{
    class client
    {
        //Debug:
        public:
        //Holds preferences as well as some values transmitted by the server
        networkingPreferences prefs;

        //The last error thrown or syjError::noError if no errors through since last call to getLastError
        syjError lastError;

        //SDLNet stuff
        UDPsocket mainSocket;
        UDPpacket **sdlPackets;
        SDLNet_SocketSet socketSet;
        IPaddress serverIP;

        //Stuff we're sent from the server to use:

        //Bits to use for clientID
        unsigned int clientIDBits;
        //The client ID that identifies us to the server
        unsigned int clientID;
        //How many bits a critical packet gets for its ID
        unsigned int packetIDBits;

        //Stuff calculated from server prefs:

        //The highest value for nextPacketID, used with critical packets
        unsigned int maxPacketID;
        //How many packet ids can we acknowledge in one packet
        unsigned int maxAcksInPacket;
        //The ID of the next critical packet
        unsigned int nextPacketID;
        //How old is the oldest critical packet we received that we have to acknowledge
        unsigned int timeOfOldestAck;
        //TODO: Debug:
        unsigned int mostRecentServerCritID;
        unsigned int highestServerCritID = 0;

        //Send a packet directly to the server adding nothing and not waiting at all, true on success
        bool sendPacket(packet *data);

        //Called after we send a connection request in the constructor
        bool waitForConnectionResponse();

        bool processKickPacket(packet *data);
        bool processNormalPacket(packet *data);
        bool processStreamPacket(packet *data);
        bool processCriticalPacket(packet *data);
        bool processAcknowledgePacket(packet *data);
        bool processClearPacket(packet *data);

        //Sort packets into their proper types and call specific functions
        bool processPackets(unsigned int numPackets,unsigned int rateLimit = 0);

        //Packets to be processed
        std::vector<packet*> toProcess;
        //Packets to be send (no modification)
        std::vector<packet*> queuedPackets;
        //Critical packets awaiting (re)sends
        std::vector<packet*> criticalPackets;

        //Receive packets, convert them, and put them in toProcess, false on error
        bool receivePackets();

        //Critical packet ids, added on processCriticalPacket:

        //Cleared upon receipt of a clear packet
        std::vector<unsigned int> idsToIgnore;
        //Cleared with a call to sendAcknowledgments
        std::vector<unsigned int> idsToAcknowledge;

        //Doesn't actually send any packets, just queues ack packets until there's no more ids left to ack this tick
        bool sendAcknowledgements();

        //Send up to maxPackets packets in queuedPackets and delete them
        bool sendQueuedPackets(int maxPackets);

        bool sendCriticalPackets(int maxPackets);

        public:

        //debug stuff, hopefully self-explanatory
        unsigned int numPacketsSent = 0;//a
        unsigned int numPacketsReceived = 0;//a
        unsigned int numSentCrits = 0;//a
        unsigned int numReceivedResends = 0;//a
        unsigned int numReceivedAcks = 0;
        unsigned int numReceivedClears = 0;
        unsigned int numSentAcks = 0;
        //Return all of the above in a big string
        std::string printDebugs() const;

        //The main send method for the end user
        bool send(packet *data,bool critical);

        //Checks for new packets, sends acks and clears, resends critical packets, etc. Returns false on error
        bool run(unsigned int rateLimit = 0);

        //No connection handle since if the server is created and lastError == syjError::noError that means you just connected
        std::function<void(client *theClient,unsigned int reason,void *userData)> kickHandle;
        std::function<void(client *theClient,packet *data,void *userData)> receiveHandle;
        void *userData = 0;

        //Returns the current error then clears it
        syjError getLastError();

        //Create a client based on some preferences, the IP is the IP to connect to, port is passed in the prefs struct
        client(networkingPreferences &_prefs,std::string ip);
        //Free SDLNet allocations and delete packets
        ~client();
    };
}

#endif // CLIENT_H_INCLUDED
