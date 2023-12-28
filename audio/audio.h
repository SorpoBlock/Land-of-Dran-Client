#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "code/utility/logger.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>
#include "code/physics/player.h"
#include "code/utility/ceguiHelper.h"
#include "code/physics/brickPhysics.h"
#include "code/networking/location.h"

// Pass to playSound's loop ID argument for non-loops
#define audioPlayerNotLooping -1

// Sound count (These should be powers of 2)
#define LOOPING_SOUND_COUNT 16
#define GENERAL_SOUND_COUNT 64
#define GENERAL_SOUND_COUNT_MINUS_1 63 // I don't trust gcc with (GENERAL_SOUND_COUNT-1), rather play it safe

namespace syj {
	// Holds a sound file and metadata
	struct sound {
		std::string fileName;	// Path to file, for loading and debugging
		std::string scriptName; // What you'd pass to playSound in lua on the server
		int serverID = -1;
		ALuint buffer = 0; // Handle to actual audio data for OpenAL

		sound(std::string _fileName, std::string _scriptName);
		~sound();
	};

	// Holds one instance of a looping sound on the server, i.e. a music brick's music
	struct loopingSound {
		location *loc = 0;

		// Used for removing the loop per server request:
		int serverId = -1;
		// Which of audioPlayer::loopingSources is currently used for this sound:
		int mostRecentSource = -1;
		// Which track to play when we get a (new/different) source:
		int soundIdx = -1;
		// Where we left off playing last if loop gets too far away from player...
		ALint sampleBytesOffset = 0;

		// Settings for this loop
		float pitch = 1.0;
		float volume = 1.0;

		loopingSound(location *src) {
			loc = src;
		}

		loopingSound(const loopingSound &src) {
			loc = src.loc;
			mostRecentSource = src.mostRecentSource;
			soundIdx = src.soundIdx;
			sampleBytesOffset = src.sampleBytesOffset;
			pitch = src.pitch;
			volume = src.volume;
			serverId = src.serverId;
		}

		// loopingSound(){}
	};

	struct audioPlayer {
		// Updated to player position in update, used for sorting loops by distance:
		static glm::vec3 musicSortPosition;

		// wav files:
		std::vector<sound *> sounds;
		// isMusic just adds the song to the music selection list drop down
		void loadSound(int serverID, std::string filepath, std::string scriptName, bool isMusic);
		int resolveSound(std::string scriptName);

		// For looping sounds, only up to 16 can play at the same time...
		ALuint loopingSounds[LOOPING_SOUND_COUNT];

		// The 16 closest loops will be selected from this (possibly) larger list to actually play:
		std::vector<loopingSound> allLoops;

		// Non-looping audio sources:
		ALuint generalSounds[GENERAL_SOUND_COUNT];
		location *soundLocations[GENERAL_SOUND_COUNT];
		glm::vec3 lastLocations[GENERAL_SOUND_COUNT];

		// Where the search for a free/non-playing source will start:
		int lastUsedGeneralSound = 0;

		void playSound(int soundID, location *loc = NULL, float pitch = 1.0, float volume = 1.0, int loopID = audioPlayerNotLooping);
		void removeLoop(int loopID);

		// EFX
		void loadEFX();

		// Sorts music loops by distance and passes our player transform to OpenAL.
		// TODO: Up vector is always 0,1,0 for now, but we do lean sometimes...?
		void update(glm::vec3 microphonePosition, glm::vec3 microphoneDirection);
		// TODO: From the settings menu, currently called every frame even though that's not really necessary
		// Needs to iterate through all 16 looping sources each time
		void setVolumes(float master, float music);

		ALuint effect = 0, effectSlot = 0;
		void setEffect(std::string effectStr);

		audioPlayer();
		~audioPlayer();
	};
} // namespace syj

#endif // AUDIO_H_INCLUDED
