//-----------------------------------------------------------------------------
//
// DESCRIPTION:  Sound file loading code.
//
//-----------------------------------------------------------------------------
#include "audio.h"
#include "code/external/AudioFile.h"
#ifdef AUDIO_SUPPORT_LIBOPENMPT
	#include <include/stb_vorbis/stb_vorbis.c>
	#include <libopenmpt/libopenmpt.hpp>
#endif

namespace syj {
	//soundfile loading functions
	sound::~sound() { alDeleteBuffers(1, &buffer); }
	sound::sound(std::string _fileName,std::string _scriptName) {
		scriptName = _scriptName;
		fileName = _fileName;
		if(_fileName.length() < 1) { return; }
		debug("Loading audio " + fileName);

		//Generate OpenAL buffers.
		alGenBuffers(1,&buffer);

		//Determine extension.
		std::string format = "can't be recognized (no extension)";
		size_t pos = _fileName.find_last_of(".");
		if (pos != std::string::npos) { pos++; format = lowercase(_fileName.substr(pos)); }

		//Extension support
		if(format == "wav") {
			AudioFile<int16_t> audioFile; audioFile.load(fileName);
			if(audioFile.getBitDepth() != 8 && audioFile.getBitDepth() != 16) { error(_fileName + " Weird audio format: " + std::to_string(audioFile.getNumChannels()) + " channels and " + std::to_string(audioFile.getBitDepth()) + " bit depth."); }
			else {
				alBufferData(buffer, AL_FORMAT_MONO16, audioFile.samples[0].data(), audioFile.samples[0].size() * 2, audioFile.getSampleRate());
			}
		}
		//libopenmpt
		#ifdef AUDIO_SUPPORT_LIBOPENMPT
			else if(format == "ogg") {
				int channels, sample_rate;
				short* data;
				int samples = stb_vorbis_decode_filename(fileName.c_str(), &channels, &sample_rate, &data);
				if(channels != 1 && channels != 2) { error(_fileName + ": only Stereo/Mono oggs supported."); }
				else {
					if(data) {
						alBufferData(buffer, channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, samples*channels*sizeof(short), sample_rate);
					} else {
						error(_fileName + ": failed to allocate memory."); 
					}
				}
				delete data;
			}
			else if(format == "mod") {
    			std::ifstream file(fileName, std::ios::binary);
				if(file.is_open()) {
					openmpt::module mod(file);
					size_t parts = size_t(std::min(480.0, mod.get_duration_seconds()) * 24000.0) << 2; //api could potentially return infinite.
					short* data = malloc(parts); //read the stream
					if(data) {
						if(mod.read_interleaved_stereo(24000, parts >> 2, data)) { error(_fileName + ": could only read partial stream."); }
						alBufferData(buffer, AL_FORMAT_STEREO16, data, parts, 24000);
					} else {
						error(_fileName + ": failed to allocate memory."); 
					}
					file.close();
					delete data;
				} else {
					error(_fileName + ": could not read.");
				}
			}
		#endif
		else { error(_fileName + ": Unknown audio format: " + format); }

		//Determine if there were any errors loading the audio.
		ALenum errr = alGetError();
		if(errr != AL_NO_ERROR) { error("OpenAL error: " + std::to_string(errr)); }
	}

	//isMusic just adds the song to the music selection list drop down
	void audioPlayer::loadSound(int serverID, std::string filepath, std::string scriptName, bool isMusic) {
		sound *tmp = new sound(filepath, scriptName);
		tmp->serverID = serverID;
		if(isMusic) {
			CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
			CEGUI::Window *wrench = root->getChild("HUD/Wrench");
			dropBoxAdd(wrench->getChild("MusicDropdown"), scriptName, serverID);
		}
		sounds.push_back(tmp);
	}
} // namespace syj