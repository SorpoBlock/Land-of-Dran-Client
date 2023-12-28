//-----------------------------------------------------------------------------
//
// DESCRIPTION:  OpenAL reverb effects.
//
//-----------------------------------------------------------------------------
#include "audio.h"

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

namespace syj {
	//Load EFX
	void audioPlayer::loadEFX() {
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
	}

	//Get effect settings
	EFXEAXREVERBPROPERTIES getEffectSettings(std::string in, bool &stopEffect) {
		stopEffect = false;
		in = lowercase(in);
		if(in == "default" || in == "normal" || in == "" || in == " ") { stopEffect = true; return EFX_REVERB_PRESET_GENERIC; }
		else if(in == "generic") { return EFX_REVERB_PRESET_GENERIC; }
		else if(in == "paddedcell") { return EFX_REVERB_PRESET_PADDEDCELL; }
		else if(in == "auditorium") { return EFX_REVERB_PRESET_AUDITORIUM; }
		else if(in == "concerthall") { return EFX_REVERB_PRESET_CONCERTHALL; }
		else if(in == "cave") { return EFX_REVERB_PRESET_CAVE; }
		else if(in == "forest") { return EFX_REVERB_PRESET_FOREST; }
		else if(in == "plain") { return EFX_REVERB_PRESET_PLAIN; }
		else if(in == "underwater") { return EFX_REVERB_PRESET_UNDERWATER; }
		else if(in == "drugged") { return EFX_REVERB_PRESET_DRUGGED; }
		else if(in == "dizzy") { return EFX_REVERB_PRESET_DIZZY; }
		else if(in == "psychotic") { return EFX_REVERB_PRESET_PSYCHOTIC; }
		else if(in == "outhouse") { return EFX_REVERB_PRESET_PREFAB_OUTHOUSE; }
		else if(in == "heaven") { return EFX_REVERB_PRESET_MOOD_HEAVEN; }
		else if(in == "hell") { return EFX_REVERB_PRESET_MOOD_HELL; }
		else if(in == "memory") { return EFX_REVERB_PRESET_MOOD_MEMORY; }
		else if(in == "dustyroom") { return EFX_REVERB_PRESET_DUSTYROOM; }
		else if(in == "waterroom") { return EFX_REVERB_PRESET_SMALLWATERROOM; }
		else if(in == "racer") { return EFX_REVERB_PRESET_DRIVING_INCAR_RACER; }
		else if(in == "tunnel") { return EFX_REVERB_PRESET_DRIVING_TUNNEL; }

		error("Audio Effect Preset " + in + " not found!");
		return EFX_REVERB_PRESET_GENERIC;
	}

	//Set effects
	void audioPlayer::setEffect(std::string effectStr) {
		bool cancelEffect = false;
		EFXEAXREVERBPROPERTIES reverb = getEffectSettings(effectStr, cancelEffect);
		if(cancelEffect) {
			for(int a = 0; a < GENERAL_SOUND_COUNT; a++) { alSource3i(generalSounds[a], AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL); }
			for(int a = 0; a < LOOPING_SOUND_COUNT; a++) { alSource3i(loopingSounds[a], AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL); }
			return;
		}

		alGenEffects(1, &effect);
		if(alGetEnumValue("AL_EFFECT_EAXREVERB") != 0) {
			info("Using EAX effect!");
			alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
			alEffectf(effect, AL_EAXREVERB_DENSITY, reverb.flDensity);
			alEffectf(effect, AL_EAXREVERB_DIFFUSION, reverb.flDiffusion);
			alEffectf(effect, AL_EAXREVERB_GAIN, reverb.flGain);
			alEffectf(effect, AL_EAXREVERB_GAINHF, reverb.flGainHF);
			alEffectf(effect, AL_EAXREVERB_GAINLF, reverb.flGainLF);
			alEffectf(effect, AL_EAXREVERB_DECAY_TIME, reverb.flDecayTime);
			alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, reverb.flDecayHFRatio);
			alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, reverb.flDecayLFRatio);
			alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain);
			alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay);
			alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, reverb.flReflectionsPan);
			alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain);
			alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay);
			alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, reverb.flLateReverbPan);
			alEffectf(effect, AL_EAXREVERB_ECHO_TIME, reverb.flEchoTime);
			alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, reverb.flEchoDepth);
			alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, reverb.flModulationTime);
			alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, reverb.flModulationDepth);
			alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF);
			alEffectf(effect, AL_EAXREVERB_HFREFERENCE, reverb.flHFReference);
			alEffectf(effect, AL_EAXREVERB_LFREFERENCE, reverb.flLFReference);
			alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor);
			alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit);
		} else {
			info("Using non-EAX standard effect!");
			alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
			alEffectf(effect, AL_REVERB_DENSITY, reverb.flDensity);
			alEffectf(effect, AL_REVERB_DIFFUSION, reverb.flDiffusion);
			alEffectf(effect, AL_REVERB_GAIN, reverb.flGain);
			alEffectf(effect, AL_REVERB_GAINHF, reverb.flGainHF);
			alEffectf(effect, AL_REVERB_DECAY_TIME, reverb.flDecayTime);
			alEffectf(effect, AL_REVERB_DECAY_HFRATIO, reverb.flDecayHFRatio);
			alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain);
			alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay);
			alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain);
			alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay);
			alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF);
			alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor);
			alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit);
		}

		alDeleteAuxiliaryEffectSlots(1, &effectSlot);
		effectSlot = 0;

		alGenAuxiliaryEffectSlots(1, &effectSlot);
		alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, (ALint)effect);

		alDeleteEffects(1, &effect);

		for(int a = 0; a < GENERAL_SOUND_COUNT; a++) { alSource3i(generalSounds[a], AL_AUXILIARY_SEND_FILTER, (ALint)effectSlot, 0, AL_FILTER_NULL); }
		for(int a = 0; a < LOOPING_SOUND_COUNT; a++) { alSource3i(loopingSounds[a], AL_AUXILIARY_SEND_FILTER, (ALint)effectSlot, 0, AL_FILTER_NULL); }
	}
}