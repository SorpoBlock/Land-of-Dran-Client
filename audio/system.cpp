//-----------------------------------------------------------------------------
//
// DESCRIPTION:  General audio system code.
//   This handles the initialization of OpenAL.
//
//-----------------------------------------------------------------------------
#include "audio.h"

namespace syj {
	//Audio player system initialization
	audioPlayer::audioPlayer() {
		for(int a = 0; a < GENERAL_SOUND_COUNT; a++) {
			soundLocations[a] = 0;
		}

		alGenSources(LOOPING_SOUND_COUNT, loopingSounds);
		alGenSources(GENERAL_SOUND_COUNT, generalSounds);

		ALenum errr = alGetError();
		if(errr != AL_NO_ERROR) {
			error("Allocating sources, OpenAL error: " + std::to_string(errr));
		}

	#if __STDC_VERSION__ >= 199901L
	#define FUNCTION_CAST(T, ptr) (union {void *p; T f; }){ptr}.f
	#elif defined(__cplusplus)
	#define FUNCTION_CAST(T, ptr) reinterpret_cast<T>(ptr)
	#else
	#define FUNCTION_CAST(T, ptr) (T)(ptr)
	#endif

	#define LOAD_PROC(T, x) ((x) = FUNCTION_CAST(T, alGetProcAddress(#x)))
		// FUNCTION_CAST(LPALGENEFFECTS,alGetProcAddress(alGenEffects));
		LOAD_PROC(LPALGENEFFECTS, alGenEffects);
		LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
		LOAD_PROC(LPALISEFFECT, alIsEffect);
		LOAD_PROC(LPALEFFECTI, alEffecti);
		LOAD_PROC(LPALEFFECTIV, alEffectiv);
		LOAD_PROC(LPALEFFECTF, alEffectf);
		LOAD_PROC(LPALEFFECTFV, alEffectfv);
		LOAD_PROC(LPALGETEFFECTI, alGetEffecti);
		LOAD_PROC(LPALGETEFFECTIV, alGetEffectiv);
		LOAD_PROC(LPALGETEFFECTF, alGetEffectf);
		LOAD_PROC(LPALGETEFFECTFV, alGetEffectfv);

		LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
		LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
		LOAD_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
		LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
		LOAD_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
		LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
		LOAD_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);
		LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
		LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv);
		LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);
		LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv);
	#undef LOAD_PROC

		errr = alGetError();
		if(errr != AL_NO_ERROR) {
			error("Loading EFX, OpenAL error: " + std::to_string(errr));
		}
	}

	//Cleanup
	audioPlayer::~audioPlayer() {
		alDeleteSources(LOOPING_SOUND_COUNT, loopingSounds);
		alDeleteSources(GENERAL_SOUND_COUNT, generalSounds);
	}

	// Music/Sound distance sorting.
	glm::vec3 audioPlayer::musicSortPosition = glm::vec3(0, 0, 0);
	bool sortMusic(const loopingSound &a, const loopingSound &b) {
		return glm::distance2(a.loc->getPosition(), audioPlayer::musicSortPosition) < glm::distance2(b.loc->getPosition(), audioPlayer::musicSortPosition);
	}

	// Update audio player.
	void audioPlayer::update(glm::vec3 microphonePosition, glm::vec3 microphoneDirection) {
		// Actually pass position and direction data to OpenAL:
		alListener3f(AL_VELOCITY, 0, 0, 0); // TODO: Velocity for doppler effect
		alListener3f(AL_POSITION, microphonePosition.x, microphonePosition.y, microphonePosition.z);
		float ori[6];
		ori[0] = microphoneDirection.x;
		ori[1] = 0;
		ori[2] = microphoneDirection.z;
		ori[3] = 0;
		ori[4] = 1;
		ori[5] = 0;
		alListenerfv(AL_ORIENTATION, ori);
		// std::cout<<microphoneDirection.x<<","<<microphoneDirection.z<<" ("<<glm::length(microphoneDirection)<<") "<<microphonePosition.x<<","<<microphonePosition.y<<","<<microphonePosition.z<<"\n";

		// Update locations of non looping audio:
		for(int a = 0; a < GENERAL_SOUND_COUNT; a++) {
			ALint isPlaying; alGetSourcei(generalSounds[a], AL_SOURCE_STATE, &isPlaying);
			if(isPlaying != AL_PLAYING) { continue; }

			glm::vec3 pos = glm::vec3(0, 1, 0);
			if(soundLocations[a]) { pos = soundLocations[a]->getPosition(); }
			if(glm::length(lastLocations[a] - pos) < 0.005) { continue; }
			lastLocations[a] = pos;
			alSource3f(generalSounds[a], AL_POSITION, pos.x, pos.y, pos.z);
		}

		musicSortPosition = microphonePosition;
		std::sort(allLoops.begin(), allLoops.end(), sortMusic);

		// Stop playing any loops that are too far and unbind them from their source...
		for(int a = LOOPING_SOUND_COUNT; a < allLoops.size(); a++) {
			// Was also too far last time, nothing needs to be done
			if(allLoops[a].mostRecentSource == -1) { continue; }
			alGetSourcei(loopingSounds[allLoops[a].mostRecentSource], AL_BYTE_OFFSET, &allLoops[a].sampleBytesOffset);
			alSourceStop(loopingSounds[allLoops[a].mostRecentSource]);
			allLoops[a].mostRecentSource = -1;
		}

		// Start playing any loops that are close enough if they weren't already:
		for(int a = 0; a < std::min(LOOPING_SOUND_COUNT, (int)allLoops.size()); a++) {
			// Already playing, carry on...
			if(allLoops[a].mostRecentSource != -1) {
				// Except we gotta update positions for moving objects
				glm::vec3 pos = allLoops[a].loc->getPosition();
				alSource3f(loopingSounds[allLoops[a].mostRecentSource], AL_POSITION, pos.x, pos.y, pos.z);
				continue;
			}

			// By unbinding all sources when they move too far away (above) or when they are deleted, we ensure there is always enough free sources
			for(int b = 0; b < LOOPING_SOUND_COUNT; b++) {
				ALint res; alGetSourcei(loopingSounds[b], AL_SOURCE_STATE, &res);
				if(res != AL_PLAYING) {
					// We got a free source
					allLoops[a].mostRecentSource = b;

					glm::vec3 pos = allLoops[a].loc->getPosition();

					// Actually play the loop:
					alSourcef(loopingSounds[b], AL_PITCH, allLoops[a].pitch);
					alSourcef(loopingSounds[b], AL_GAIN, allLoops[a].volume);
					alSource3f(loopingSounds[b], AL_POSITION, pos.x, pos.y, pos.z);
					alSource3f(loopingSounds[b], AL_VELOCITY, 0, 0, 0);
					alSourcei(loopingSounds[b], AL_LOOPING, AL_TRUE);
					alSourcei(loopingSounds[b], AL_BUFFER, sounds[allLoops[a].soundIdx]->buffer);
					alSourcei(loopingSounds[b], AL_BYTE_OFFSET, allLoops[a].sampleBytesOffset);
					alSourcePlay(loopingSounds[b]);

					break;
				}
			}
		}

		//Check for errors
		ALenum errr = alGetError();
		if(errr != AL_NO_ERROR) { error("update OpenAL error: " + std::to_string(errr)); }
	}

	//Set volumes
	void audioPlayer::setVolumes(float master, float music) {
		master = std::clamp(master, 0.001f, 1.0f);
		alListenerf(AL_GAIN, master);

		music = std::clamp(music, 0.001f, 1.0f);
		for(unsigned int a = 0; a < LOOPING_SOUND_COUNT; a++) {
			alSourcef(loopingSounds[a], AL_GAIN, music);
		}

		ALenum errr = alGetError();
		if(errr != AL_NO_ERROR) {
			error("setVolumes OpenAL error: " + std::to_string(errr));
		}
	}
} // namespace syj