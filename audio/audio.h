#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "code/utility/logger.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>
#include "code/external/AudioFile.h"
#include "code/physics/player.h"
#include "code/utility/ceguiHelper.h"
#include "code/physics/brickPhysics.h"
#include "code/networking/location.h"

//Pass to playSound's loop ID argument for non-loops
#define audioPlayerNotLooping -1

/* Effect object functions */
static LPALGENEFFECTS alGenEffects;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALISEFFECT alIsEffect;
static LPALEFFECTI alEffecti;
static LPALEFFECTIV alEffectiv;
static LPALEFFECTF alEffectf;
static LPALEFFECTFV alEffectfv;
static LPALGETEFFECTI alGetEffecti;
static LPALGETEFFECTIV alGetEffectiv;
static LPALGETEFFECTF alGetEffectf;
static LPALGETEFFECTFV alGetEffectfv;

static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
static LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
static LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
static LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
static LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
static LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
static LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
static LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

namespace syj
{
    //Holds a sound file and metadata
    struct sound
    {
        std::string fileName;   //Path to file, for loading and debugging
        std::string scriptName; //What you'd pass to playSound in lua on the server
        int serverID = -1;
        ALuint buffer = 0;      //Handle to actual audio data for OpenAL

        sound(std::string _fileName,std::string _scriptName);
    };

    //Holds one instance of a looping sound on the server, i.e. a music brick's music
    struct loopingSound
    {
        location *loc = 0;

        //Used for removing the loop per server request:
        int serverId = -1;
        //Which of audioPlayer::loopingSources is currently used for this sound:
        int mostRecentSource = -1;
        //Which track to play when we get a (new/different) source:
        int soundIdx = -1;
        //Where we left off playing last if loop gets too far away from player...
        ALint sampleBytesOffset = 0;

        //Settings for this loop
        float pitch = 1.0;
        float volume = 1.0;

        loopingSound(location *src)
        {
            loc = src;
        }

        loopingSound(const loopingSound &src)
        {
            loc = src.loc;
            mostRecentSource = src.mostRecentSource;
            soundIdx = src.soundIdx;
            sampleBytesOffset = src.sampleBytesOffset;
            pitch = src.pitch;
            volume = src.volume;
            serverId = src.serverId;
        }

        //loopingSound(){}
    };

    struct audioPlayer
    {
        //Updated to player position in update, used for sorting loops by distance:
        static glm::vec3 musicSortPosition;

        //wav files:
        std::vector<sound*> sounds;
        //isMusic just adds the song to the music selection list drop down
        void loadSound(int serverID,std::string filepath,std::string scriptName,bool isMusic);
        int resolveSound(std::string scriptName)
        {
            for(int a = 0; a<sounds.size(); a++)
            {
                if(sounds[a]->scriptName == scriptName)
                    return sounds[a]->serverID;
            }
            return -1;
        }

        //For music loops, only up to 16 can play at the same time...
        ALuint loopingSounds[16];
        //The 16 closest loops will be selected from this (possibly) larger list to actually play:
        std::vector<loopingSound> allLoops;

        //Non-looping audio sources:
        ALuint generalSounds[32];
        location *soundLocations[32];
        glm::vec3 lastLocations[32];
        //Where the search for a free/non-playing source will start:
        int lastUsedGeneralSound = 0;

        void playSound3D(int soundID,location *loc,float pitch = 1.0,float volume = 1.0,int loopID = audioPlayerNotLooping);
        void playSound2D(int soundID,float pitch = 1.0,float volume = 1.0);
        void removeLoop(int loopID);

        //Sorts music loops by distance and passes our player transform to OpenAL.
        //TODO: Up vector is always 0,1,0 for now, but we do lean sometimes...?
        void update(glm::vec3 microphonePosition,glm::vec3 microphoneDirection);
        //TODO: From the settings menu, currently called every frame even though that's not really necessary
        //Needs to iterate through all 16 looping sources each time
        void setVolumes(float master,float music);

        ALuint effect=0,effectSlot=0;
        void setEffect(std::string effectStr);

        audioPlayer();
        ~audioPlayer();
    };
}

#endif // AUDIO_H_INCLUDED
