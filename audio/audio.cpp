#include "audio.h"

namespace syj
{
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

        debug("Loading audio " + filename + " " + std::to_string(audioFile.getNumChannels()) +"/"+ std::to_string(audioFile.getBitDepth()) +"/"+ std::to_string(audioFile.getSampleRate()));
        
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

    void audioPlayer::loadSound(int serverID,std::string filepath,std::string scriptName,bool isMusic)
    {
        sound *tmp = new sound(filepath,scriptName);
        tmp->serverID = serverID;
        tmp->isMusic = isMusic;

        if(isMusic)
        {
            CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
            CEGUI::Window *wrench = root->getChild("HUD/Wrench");
            dropBoxAdd(wrench->getChild("MusicDropdown"),scriptName,serverID);
        }

        sounds.push_back(tmp);
    }

    audioPlayer::audioPlayer()
    {
        for(int a = 0; a<32; a++)
        {
            sourceShouldTrackMicrophone[a] = false;
            carToTrack[a] = 0;
        }
        alGenSources(32,sources);
        alGenSources(32,loopSources);
        ALenum errr = alGetError();
        info("Generated audio sources.");
        if(errr != AL_NO_ERROR)
            error("OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::loopSound(int serverID,int loopID,livingBrick *car,float pitch)
    {
        if(loopID > 31)
        {
            error("Only 32 loops allowed!");
            return;
        }

        if(serverID == 0)
        {
            carToTrack[loopID] = 0;
            alSourceStop(loopSources[loopID]);
            return;
        }

        int idx = -1;
        for(unsigned int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->serverID == serverID)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Could not find sound ID: " + std::to_string(serverID));
            return;
        }

        alSourcef( loopSources[loopID], AL_PITCH, pitch);
        alSourcef( loopSources[loopID], AL_GAIN, 1.0f);
        alSource3f( loopSources[loopID], AL_POSITION, 1000, 1000, 1000);
        alSource3f( loopSources[loopID], AL_VELOCITY, 0, 0, 0);
        alSourcei( loopSources[loopID], AL_LOOPING, AL_TRUE);
        alSourcei( loopSources[loopID], AL_BUFFER, sounds[idx]->buffer);

        alSourcePlay(loopSources[loopID]);

        carToTrack[loopID] = car;
    }

    void audioPlayer::setVolumes(float master,float music)
    {
        master = std::clamp(master,0.001f,1.0f);
        alListenerf(AL_GAIN,master);

        music = std::clamp(music,0.001f,1.0f);
        for(unsigned int a = 0; a<32; a++)
            alSourcef(loopSources[a],AL_GAIN,music);
    }

    void audioPlayer::loopSound(int serverID,int loopID,float x,float y,float z,float pitch)
    {
        if(loopID > 31)
        {
            error("Only 32 loops allowed!");
            return;
        }

        if(serverID == 0)
        {
            alSourceStop(loopSources[loopID]);
            return;
        }

        int idx = -1;
        for(unsigned int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->serverID == serverID)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Could not find sound ID: " + std::to_string(serverID));
            return;
        }

        alSourcef( loopSources[loopID], AL_PITCH, pitch);
        alSourcef( loopSources[loopID], AL_GAIN, 1.0f);
        alSource3f( loopSources[loopID], AL_POSITION, x, y, z);
        alSource3f( loopSources[loopID], AL_VELOCITY, 0, 0, 0);
        alSourcei( loopSources[loopID], AL_LOOPING, AL_TRUE);

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("0Music OpenAL error: " + std::to_string(errr));

        alSourcei( loopSources[loopID], AL_BUFFER, sounds[idx]->buffer);

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("1Music OpenAL error: " + std::to_string(errr));

        alSourcePlay(loopSources[loopID]);

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("2Music OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::playSound(std::string name,bool loop,float x,float y,float z)
    {
        int idx = -1;
        for(unsigned int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->scriptName == name)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Could not find sound name: " + name);
            return;
        }

        alSourcef( sources[currentSource], AL_PITCH, 1);
        alSourcef( sources[currentSource], AL_GAIN, 1.0f);
        alSource3f( sources[currentSource], AL_POSITION, x, y, z);
        alSource3f( sources[currentSource], AL_VELOCITY, 0, 0, 0);

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("0OpenAL error: " + std::to_string(errr));

        sourceShouldTrackMicrophone[currentSource] = false;
        alSourcei( sources[currentSource], AL_LOOPING, loop ? AL_TRUE : AL_FALSE);


        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("1OpenAL error: " + std::to_string(errr));

        alSourcei( sources[currentSource], AL_BUFFER, sounds[idx]->buffer);

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("2OpenAL error: " + std::to_string(errr));

        alSourcePlay(sources[currentSource]);

        currentSource++;
        if(currentSource >= 32)
            currentSource = 0;

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("3OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::playSound(int serverID,bool loop)
    {
        int idx = -1;
        for(unsigned int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->serverID == serverID)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Could not find sound ID: " + std::to_string(serverID));
            return;
        }

        alSourcef( sources[currentSource], AL_PITCH, 1);
        alSourcef( sources[currentSource], AL_GAIN, 1.0f);
        alSource3f( sources[currentSource], AL_POSITION, lastPos.x,lastPos.y,lastPos.z);
        alSource3f( sources[currentSource], AL_VELOCITY, 0, 0, 0);

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("0OpenAL error: " + std::to_string(errr));

        alSourcei( sources[currentSource], AL_LOOPING, loop ? AL_TRUE : AL_FALSE);


        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("1OpenAL error: " + std::to_string(errr));

        alSourcei( sources[currentSource], AL_BUFFER, sounds[idx]->buffer);

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("2OpenAL error: " + std::to_string(errr));

        alSourcePlay(sources[currentSource]);
        sourceShouldTrackMicrophone[currentSource] = true;

        currentSource++;
        if(currentSource >= 32)
            currentSource = 0;

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("3OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::playSound(int serverID,bool loop,float x,float y,float z)
    {
        int idx = -1;
        for(unsigned int a = 0; a<sounds.size(); a++)
        {
            if(sounds[a]->serverID == serverID)
            {
                idx = a;
                break;
            }
        }
        if(idx == -1)
        {
            error("Could not find sound ID: " + std::to_string(serverID));
            return;
        }

        alSourcef( sources[currentSource], AL_PITCH, 1);
        alSourcef( sources[currentSource], AL_GAIN, 1.0f);
        alSource3f( sources[currentSource], AL_POSITION, x, y, z);
        alSource3f( sources[currentSource], AL_VELOCITY, 0, 0, 0);

        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("0OpenAL error: " + std::to_string(errr));

        sourceShouldTrackMicrophone[currentSource] = false;
        alSourcei( sources[currentSource], AL_LOOPING, loop ? AL_TRUE : AL_FALSE);


        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("1OpenAL error: " + std::to_string(errr));

        alSourcei( sources[currentSource], AL_BUFFER, sounds[idx]->buffer);

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("2OpenAL error: " + std::to_string(errr));

        alSourcePlay(sources[currentSource]);

        currentSource++;
        if(currentSource >= 32)
            currentSource = 0;

        errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("3OpenAL error: " + std::to_string(errr));
    }

    void audioPlayer::microphone(glm::vec3 pos,glm::vec3 dir)
    {
        for(int a = 0; a<32; a++)
        {
            if(!sourceShouldTrackMicrophone[a])
                continue;

            alSource3f( sources[a], AL_POSITION, pos.x,pos.y,pos.z);
        }

        alListener3f(AL_VELOCITY,0,0,0);
        alListener3f(AL_POSITION,pos.x,pos.y,pos.z);
        float ori[6];
        ori[0] = dir.x;
        ori[1] = 0;//dir.y;
        ori[2] = dir.z;
        ori[3] = 0;
        ori[4] = 1;
        ori[5] = 0;
        lastPos = pos;
        alListenerfv(AL_ORIENTATION,ori);
        ALenum errr = alGetError();
        if(errr != AL_NO_ERROR)
            error("4OpenAL error: " + std::to_string(errr));
    }
}
