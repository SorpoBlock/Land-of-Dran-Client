#include "audio.h"

namespace syj
{
    EFXEAXREVERBPROPERTIES getEffectSettings(std::string in,bool &stopEffect)
    {
        stopEffect = false;
        in = lowercase(in);

        if(in == "default" || in == "normal" || in == "" || in == " ")
        {
            stopEffect = true;
            return EFX_REVERB_PRESET_GENERIC;
        }
        else if(in == "generic")
            return EFX_REVERB_PRESET_GENERIC;
        else if(in == "paddedcell")
            return EFX_REVERB_PRESET_PADDEDCELL;
        else if(in == "auditorium")
            return EFX_REVERB_PRESET_AUDITORIUM;
        else if(in == "concerthall")
            return EFX_REVERB_PRESET_CONCERTHALL;
        else if(in == "cave")
            return EFX_REVERB_PRESET_CAVE;
        else if(in == "forest")
            return EFX_REVERB_PRESET_FOREST;
        else if(in == "plain")
            return EFX_REVERB_PRESET_PLAIN;
        else if(in == "underwater")
            return EFX_REVERB_PRESET_UNDERWATER;
        else if(in == "drugged")
            return EFX_REVERB_PRESET_DRUGGED;
        else if(in == "dizzy")
            return EFX_REVERB_PRESET_DIZZY;
        else if(in == "psychotic")
            return EFX_REVERB_PRESET_PSYCHOTIC;
        else if(in == "outhouse")
            return EFX_REVERB_PRESET_PREFAB_OUTHOUSE;
        else if(in == "heaven")
            return EFX_REVERB_PRESET_MOOD_HEAVEN;
        else if(in == "hell")
            return EFX_REVERB_PRESET_MOOD_HELL;
        else if(in == "memory")
            return EFX_REVERB_PRESET_MOOD_MEMORY;
        else if(in == "dustyroom")
            return EFX_REVERB_PRESET_DUSTYROOM;
        else if(in == "waterroom")
            return EFX_REVERB_PRESET_SMALLWATERROOM;
        else if(in == "racer")
            return EFX_REVERB_PRESET_DRIVING_INCAR_RACER;
        else if(in == "tunnel")
            return EFX_REVERB_PRESET_DRIVING_TUNNEL;

        error("Audio Effect Preset " + in + " not found!");
        return EFX_REVERB_PRESET_GENERIC;
    }

    glm::vec3 audioPlayer::musicSortPosition = glm::vec3(0,0,0);

    sound::sound(std::string _fileName,std::string _scriptName)
    {
        scriptName = _scriptName;
        fileName = _fileName;

        if(_fileName.length() < 1)
            return;

        AudioFile<float> audioFile;
        audioFile.load(fileName);

        ALenum format;
        if(audioFile.getNumChannels() == 1 && audioFile.getBitDepth() == 8)
            format = AL_FORMAT_MONO8;
        else if(audioFile.getNumChannels() == 1 && audioFile.getBitDepth() == 16)
            format = AL_FORMAT_MONO16;
        else if(audioFile.getNumChannels() == 2 && audioFile.getBitDepth() == 8)
            format = AL_FORMAT_STEREO8;
        else if(audioFile.getNumChannels() == 2 && audioFile.getBitDepth() == 16)
            format = AL_FORMAT_STEREO16;
        else
            error("Weird audio format: " + std::to_string(audioFile.getNumChannels()) + " channels and " + std::to_string(audioFile.getBitDepth()) + " bit depth.");

        debug("Loading audio " + fileName + " " + std::to_string(audioFile.getNumChannels()) +"/"+ std::to_string(audioFile.getBitDepth()) +"/"+ std::to_string(audioFile.getSampleRate()));

        alGenBuffers(1,&buffer);

        if(audioFile.getBitDepth() == 16)
        {
            short *soundData = new short[audioFile.samples[0].size()];
            for(unsigned int a = 0; a<audioFile.samples[0].size(); a++)
                soundData[a] = (audioFile.samples[0][a]*32768);
            alBufferData(buffer,format,soundData,audioFile.samples[0].size()*sizeof(short),audioFile.getSampleRate());
            delete soundData;
        }
        else
        {
            unsigned char *soundData = new unsigned char[audioFile.samples[0].size()];
            for(unsigned int a = 0; a<audioFile.samples[0].size(); a++)
                soundData[a] = 128+(audioFile.samples[0][a]*127);

            alBufferData(buffer,format,soundData,audioFile.samples[0].size()*sizeof(unsigned char),audioFile.getSampleRate());
            delete soundData;
        }

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("OpenAL error: " + std::to_string(errr) + " Bit depth: " + std::to_string(audioFile.getBitDepth()) + " Samples size: " + std::to_string(audioFile.samples[0].size()) + " Sample rate: " + std::to_string(audioFile.getSampleRate()) + " Channels: " + std::to_string(audioFile.getNumChannels()));
    }

    //isMusic just adds the song to the music selection list drop down
    void audioPlayer::loadSound(int serverID,std::string filepath,std::string scriptName,bool isMusic)
    {
        sound *tmp = new sound(filepath,scriptName);
        tmp->serverID = serverID;

        if(isMusic)
        {
            CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
            CEGUI::Window *wrench = root->getChild("HUD/Wrench");
            dropBoxAdd(wrench->getChild("MusicDropdown"),scriptName,serverID);
        }

        sounds.push_back(tmp);
    }


    bool sortMusic(const loopingSound &a,const loopingSound &b)
    {
        return glm::distance2(a.getPosition(),audioPlayer::musicSortPosition) < glm::distance2(b.getPosition(),audioPlayer::musicSortPosition);
    }

    void audioPlayer::removeLoop(int loopID)
    {
        for(int a = 0; a<allLoops.size(); a++)
        {
            if(allLoops[a].serverId == loopID)
            {
                if(allLoops[a].mostRecentSource != -1)
                    alSourceStop(loopingSounds[allLoops[a].mostRecentSource]);


                allLoops.erase(allLoops.begin() + a);
                return;
            }
        }
    }

    void audioPlayer::playSound3D(int soundID,location loc,float pitch,float volume,int loopID)
    {
        //Turn an ID into an Index, ideally these could just be the same...
        int soundIdx = -1;
        for(int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->serverID == soundID)
            {
                soundIdx = a;
                break;
            }
        }
        if(soundIdx == -1)
            return;

        //Handle loops later in audioPlayer::update as a special case
        if(loopID != audioPlayerNotLooping)
        {
            loopingSound tmp(loc);
            tmp.soundIdx = soundIdx;
            tmp.pitch = pitch;
            tmp.volume = volume;
            tmp.serverId = loopID;
            allLoops.push_back(tmp);
            return;
        }

        //Basically, we go through the list of 32 sources, starting at the one after the last one we set
        //Trying to find a source that is not currently playing a sound, and use it
        //If none are free, we just stop the first one we searched and use that
        int idx = lastUsedGeneralSound+1;
        if(idx > 31)
            idx = 0;

        int searchedSources = 0;
        while(searchedSources < 31)
        {
            ALint isPlaying;
            alGetSourcei(generalSounds[idx],AL_SOURCE_STATE,&isPlaying);
            if(isPlaying != AL_PLAYING)
            {
                alSourcef(generalSounds[idx], AL_PITCH, pitch);
                alSourcef(generalSounds[idx], AL_GAIN, volume);
                alSource3f(generalSounds[idx], AL_POSITION, 0,0,0);
                alSource3f(generalSounds[idx], AL_VELOCITY, 0, 0, 0);
                alSourcei(generalSounds[idx], AL_SOURCE_RELATIVE, AL_FALSE);
                alSourcei( generalSounds[idx], AL_LOOPING, AL_FALSE);
                alSourcei( generalSounds[idx], AL_BUFFER, sounds[soundIdx]->buffer);
                alSourcePlay(generalSounds[idx]);

                soundLocations[idx] = loc;

                lastUsedGeneralSound = idx;

                ALenum errr = alGetError();
                if(errr != AL_NO_ERROR)
                    error("playSound3D OpenAL error: " + std::to_string(errr));
                return;
            }

            idx++;
            if(idx > 31)
                idx = 0;
            searchedSources++;
        }

        //Every source was still playing a sound...
        //Start at what should be the least recently queued sound
        idx = lastUsedGeneralSound+1;
        if(idx > 31)
            idx = 0;

        alSourceStop(generalSounds[idx]);
        alSourcef(generalSounds[idx], AL_PITCH, pitch);
        alSourcef(generalSounds[idx], AL_GAIN, volume);
        alSource3f(generalSounds[idx], AL_POSITION, 0,0,0);
        alSource3f(generalSounds[idx], AL_VELOCITY, 0, 0, 0);
        alSourcei(generalSounds[idx], AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcei( generalSounds[idx], AL_LOOPING, AL_FALSE);
        alSourcei( generalSounds[idx], AL_BUFFER, sounds[soundIdx]->buffer);
        alSourcePlay(generalSounds[idx]);

        soundLocations[idx] = loc;

        lastUsedGeneralSound = idx;

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("playSound3D OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::playSound2D(int soundID,float pitch,float volume)
    {
        //Turn an ID into an Index, ideally these could just be the same...
        int soundIdx = -1;
        for(int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->serverID == soundID)
            {
                soundIdx = a;
                break;
            }
        }
        if(soundIdx == -1)
            return;

        //Basically, we go through the list of 32 sources, starting at the one after the last one we set
        //Trying to find a source that is not currently playing a sound, and use it
        //If none are free, we just stop the first one we searched and use that
        int idx = lastUsedGeneralSound+1;
        if(idx > 31)
            idx = 0;

        int searchedSources = 0;
        while(searchedSources < 31)
        {
            ALint isPlaying;
            alGetSourcei(generalSounds[idx],AL_SOURCE_STATE,&isPlaying);
            if(isPlaying != AL_PLAYING)
            {
                alSourcef(generalSounds[idx], AL_PITCH, pitch);
                alSourcef(generalSounds[idx], AL_GAIN, volume);
                alSource3f(generalSounds[idx], AL_POSITION, 0,1.0,0);
                alSource3f(generalSounds[idx], AL_VELOCITY, 0, 0, 0);
                alSourcei( generalSounds[idx], AL_SOURCE_RELATIVE, AL_TRUE);
                alSourcei( generalSounds[idx], AL_LOOPING, AL_FALSE);
                alSourcei( generalSounds[idx], AL_BUFFER, sounds[soundIdx]->buffer);
                alSourcePlay(generalSounds[idx]);

                soundLocations[idx] = location(glm::vec3(0,1,0));

                lastUsedGeneralSound = idx;

                ALenum errr = alGetError();
                if(errr != AL_NO_ERROR)
                    error("playSound2D OpenAL error: " + std::to_string(errr));
                return;
            }

            idx++;
            if(idx > 31)
                idx = 0;
            searchedSources++;
        }

        //Every source was still playing a sound...
        //Start at what should be the least recently queued sound
        idx = lastUsedGeneralSound+1;
        if(idx > 31)
            idx = 0;

        alSourceStop(generalSounds[idx]);
        alSourcef(generalSounds[idx], AL_PITCH, pitch);
        alSourcef(generalSounds[idx], AL_GAIN, volume);
        alSource3f(generalSounds[idx], AL_POSITION, 0,1.0,0.0);
        alSource3f(generalSounds[idx], AL_VELOCITY, 0, 0, 0);
        alSourcei( generalSounds[idx], AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei( generalSounds[idx], AL_LOOPING, AL_FALSE);
        alSourcei( generalSounds[idx], AL_BUFFER, sounds[soundIdx]->buffer);
        alSourcePlay(generalSounds[idx]);
        lastUsedGeneralSound = idx;

        soundLocations[idx] = location(glm::vec3(0,1,0));

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("playSound2D OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::update(glm::vec3 microphonePosition,glm::vec3 microphoneDirection)
    {
        //Actually pass position and direction data to OpenAL:
        alListener3f(AL_VELOCITY,0,0,0); //TODO: Velocity for doppler effect
        alListener3f(AL_POSITION,microphonePosition.x,microphonePosition.y,microphonePosition.z);
        float ori[6];
        ori[0] = microphoneDirection.x;
        ori[1] = 0;
        ori[2] = microphoneDirection.z;
        ori[3] = 0;
        ori[4] = 1;
        ori[5] = 0;
        alListenerfv(AL_ORIENTATION,ori);
        //std::cout<<microphoneDirection.x<<","<<microphoneDirection.z<<" ("<<glm::length(microphoneDirection)<<") "<<microphonePosition.x<<","<<microphonePosition.y<<","<<microphonePosition.z<<"\n";

        //Update locations of non looping audio:
        for(int a = 0; a<32; a++)
        {
            ALint isPlaying;
            alGetSourcei(generalSounds[a],AL_SOURCE_STATE,&isPlaying);
            if(isPlaying != AL_PLAYING)
                continue;

            glm::vec3 pos = soundLocations[a].getPosition();
            if(glm::length(lastLocations[a]-pos) < 0.005)
                continue;
            lastLocations[a] = pos;

            alSource3f( generalSounds[a], AL_POSITION, pos.x,pos.y,pos.z);
        }

        musicSortPosition = microphonePosition;
        std::sort(allLoops.begin(),allLoops.end(),sortMusic);

        //Stop playing any loops that are too far and unbind them from their source...
        for(int a = 16; a<allLoops.size(); a++)
        {
            //Was also too far last time, nothing needs to be done
            if(allLoops[a].mostRecentSource == -1)
                continue;

            alGetSourcei(loopingSounds[allLoops[a].mostRecentSource],AL_BYTE_OFFSET,&allLoops[a].sampleBytesOffset);
            alSourceStop(loopingSounds[allLoops[a].mostRecentSource]);
            allLoops[a].mostRecentSource = -1;
        }

        //Start playing any loops that are close enough if they weren't already:
        for(int a = 0; a<std::min(16,(int)allLoops.size()); a++)
        {
            //Already playing, carry on...
            if(allLoops[a].mostRecentSource != -1)
            {
                //Except we gotta update positions for moving objects
                glm::vec3 pos = allLoops[a].getPosition();
                alSource3f( loopingSounds[allLoops[a].mostRecentSource], AL_POSITION, pos.x,pos.y,pos.z);
                continue;
            }

            //By unbinding all sources when they move too far away (above) or when they are deleted, we ensure there is always enough free sources
            for(int b = 0; b<16; b++)
            {
                int res = 0;
                alGetSourcei(loopingSounds[b],AL_SOURCE_STATE,&res);
                if(res != AL_PLAYING)
                {
                    //We got a free source
                    allLoops[a].mostRecentSource = b;

                    glm::vec3 pos = allLoops[a].getPosition();

                    //Actually play the loop:
                    alSourcef( loopingSounds[b], AL_PITCH, allLoops[a].pitch);
                    alSourcef( loopingSounds[b], AL_GAIN, allLoops[a].volume);
                    alSource3f( loopingSounds[b], AL_POSITION, pos.x,pos.y,pos.z);
                    alSource3f( loopingSounds[b], AL_VELOCITY, 0, 0, 0);
                    alSourcei( loopingSounds[b], AL_LOOPING, AL_TRUE);
                    alSourcei( loopingSounds[b], AL_BUFFER, sounds[allLoops[a].soundIdx]->buffer);
                    alSourcei( loopingSounds[b] ,AL_BYTE_OFFSET, allLoops[a].sampleBytesOffset);
                    alSourcePlay(loopingSounds[b]);

                    break;
                }
            }
        }

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("update OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::setEffect(std::string effectStr)
    {
        bool cancelEffect = false;
        EFXEAXREVERBPROPERTIES reverb = getEffectSettings(effectStr,cancelEffect);
        if(cancelEffect)
        {
            for(int a = 0; a<32; a++)
                alSource3i(generalSounds[a], AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
            for(int a = 0; a<16; a++)
                alSource3i(loopingSounds[a], AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);

            return;
        }

        alGenEffects(1, &effect);

        if(alGetEnumValue("AL_EFFECT_EAXREVERB") != 0)
        {
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
        }
        else
        {
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

        alDeleteAuxiliaryEffectSlots(1,&effectSlot);
        effectSlot = 0;

        alGenAuxiliaryEffectSlots(1,&effectSlot);
        alAuxiliaryEffectSloti(effectSlot,AL_EFFECTSLOT_EFFECT,(ALint)effect);

        alDeleteEffects(1,&effect);

        for(int a = 0; a<32; a++)
            alSource3i(generalSounds[a], AL_AUXILIARY_SEND_FILTER, (ALint)effectSlot, 0, AL_FILTER_NULL);
        for(int a = 0; a<16; a++)
            alSource3i(loopingSounds[a], AL_AUXILIARY_SEND_FILTER, (ALint)effectSlot, 0, AL_FILTER_NULL);
    }

    void audioPlayer::setVolumes(float master,float music)
    {
        master = std::clamp(master,0.001f,1.0f);
        alListenerf(AL_GAIN,master);

        music = std::clamp(music,0.001f,1.0f);
        for(unsigned int a = 0; a<16; a++)
            alSourcef(loopingSounds[a],AL_GAIN,music);

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("setVolumes OpenAL error: " + std::to_string(errr));
    }

    audioPlayer::audioPlayer()
    {
        alGenSources(16,loopingSounds);
        alGenSources(32,generalSounds);

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("Allocating sources, OpenAL error: " + std::to_string(errr));

        #if __STDC_VERSION__ >= 199901L
        #define FUNCTION_CAST(T, ptr) (union{void *p; T f;}){ptr}.f
        #elif defined(__cplusplus)
        #define FUNCTION_CAST(T, ptr) reinterpret_cast<T>(ptr)
        #else
        #define FUNCTION_CAST(T, ptr) (T)(ptr)
        #endif

        #define LOAD_PROC(T, x)  ((x) = FUNCTION_CAST(T, alGetProcAddress(#x)))
        //FUNCTION_CAST(LPALGENEFFECTS,alGetProcAddress(alGenEffects));
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
        if(errr != AL_NO_ERROR)
            error("Loading EFX, OpenAL error: " + std::to_string(errr));
    }

    audioPlayer::~audioPlayer()
    {
        alDeleteSources(16,loopingSounds);
        alDeleteSources(32,generalSounds);
    }
}
















