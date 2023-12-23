//-----------------------------------------------------------------------------
//
// DESCRIPTION:  General sound playing code
//
//-----------------------------------------------------------------------------
#include "audio.h"

namespace syj {
	int audioPlayer::resolveSound(std::string scriptName) {
		for(int a = 0; a < sounds.size(); a++) {
			if(sounds[a]->scriptName == scriptName) {
				return sounds[a]->serverID;
			}
		}
		return -1;
	}

	void audioPlayer::removeLoop(int loopID) {
		for(int a = 0; a < allLoops.size(); a++) {
			if(allLoops[a].serverId == loopID) {
				if(allLoops[a].mostRecentSource != -1) { alSourceStop(loopingSounds[allLoops[a].mostRecentSource]); }
				if(allLoops[a].loc) { delete allLoops[a].loc; }
				allLoops[a].loc = NULL;
				allLoops.erase(allLoops.begin() + a);
				return;
			}
		}
	}

	void audioPlayer::playSound(int soundID, location *loc, float pitch, float volume, int loopID) {
		// Turn an ID into an Index, ideally these could just be the same...
		int soundIdx = -1;
		for(int a = 0; a < sounds.size(); a++) {
			if(sounds[a]->serverID == soundID) { soundIdx = a; break; }
		}
		if(soundIdx == -1) { return; }

		// Handle loops later in audioPlayer::update as a special case
		if(loopID != audioPlayerNotLooping) {
			if(!loc) {
				error("playSound: Tried to play a looped sound without a location.");
				return;
			}
			loopingSound tmp(loc);
			tmp.soundIdx = soundIdx;
			tmp.pitch = pitch;
			tmp.volume = volume;
			tmp.serverId = loopID;
			allLoops.push_back(tmp);
			return;
		}

		// Basically, we go through the list of sources, starting at the one after the last one we set
		// Trying to find a source that is not currently playing a sound, and use it.
		int idx = lastUsedGeneralSound; int searchedSources; ALint isPlaying;
		for(int searchedSources = 0; searchedSources < GENERAL_SOUND_COUNT_MINUS_1; searchedSources++) {
			idx = (idx + 1) & GENERAL_SOUND_COUNT_MINUS_1;
			alGetSourcei(generalSounds[idx], AL_SOURCE_STATE, &isPlaying);
			if(isPlaying != AL_PLAYING) { break; } // Oh hey we found a free source.
		}

		//Let's play the sound here
		if(isPlaying == AL_PLAYING) { alSourceStop(generalSounds[idx]); }
		alSourcef(generalSounds[idx], AL_PITCH, pitch); alSourcef(generalSounds[idx], AL_GAIN, volume);
		alSource3f(generalSounds[idx], AL_POSITION, 0, 0, 0); alSource3f(generalSounds[idx], AL_VELOCITY, 0, 0, 0);
		alSourcei(generalSounds[idx], AL_SOURCE_RELATIVE, loc ? AL_FALSE : AL_TRUE);
		alSourcei(generalSounds[idx], AL_LOOPING, AL_FALSE);
		alSourcei(generalSounds[idx], AL_BUFFER, sounds[soundIdx]->buffer);
		alSourcePlay(generalSounds[idx]);
		lastUsedGeneralSound = idx;

		if(soundLocations[idx]) { delete soundLocations[idx]; }
		soundLocations[idx] = loc;

		ALenum errr = alGetError();
		if(errr != AL_NO_ERROR) { error("playSound error: " + std::to_string(errr)); }
	}
} // namespace syj