#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "code/utility/logger.h"
#include <AL/al.h>
#include <AL/alc.h>
#include "code/external/AudioFile.h"
#include "code/physics/player.h"
#include "code/utility/ceguiHelper.h"
#include "code/physics/brickPhysics.h"

namespace syj
{
    struct sound
    {
        std::string fileName;
        std::string scriptName;
        int serverID;
        ALuint buffer = 0;
        bool isMusic = false;

        sound(std::string _fileName,std::string _scriptName);
    };

    struct audioPlayer
    {
        glm::vec3 lastPos = glm::vec3(0,0,0);
        std::vector<sound*> sounds;
        void loadSound(int serverID,std::string filepath,std::string scriptName,bool isMusic);
        void playSound(int serverID,bool loop,float x,float y,float z);
        void loopSound(int serverID,int loopID,float x,float y,float z,float pitch = 1.0);
        void loopSound(int serverID,int loopID,livingBrick *car,float pitch = 1.0);
        void playSound(int serverID,bool loop);
        void playSound(std::string name,bool loop,float x,float y,float z);
        void microphone(glm::vec3 pos,glm::vec3 dir);
        void setVolumes(float master,float music);

        livingBrick *carToTrack[32];
        ALuint sources[32];
        int currentSource = 0;

        bool sourceShouldTrackMicrophone[32];
        ALuint loopSources[32];

        audioPlayer();
    };
}

#endif // AUDIO_H_INCLUDED
