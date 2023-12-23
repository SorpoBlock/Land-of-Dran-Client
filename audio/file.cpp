//-----------------------------------------------------------------------------
//
// DESCRIPTION:  Sound file loading code.
//
//-----------------------------------------------------------------------------
#include "audio.h"

namespace syj {
	// soundfile loading functions
	sound::~sound() { alDeleteBuffers(1, &buffer); }
	sound::sound(std::string _fileName,std::string _scriptName) {
		scriptName = _scriptName;
		fileName = _fileName;
		if(_fileName.length() < 1) { return; }

		//Load audio file.
		AudioFile<int16_t> audioFile; audioFile.load(fileName);

		//Determine format for OpenAL.
		if(audioFile.getBitDepth() != 8 && audioFile.getBitDepth() != 16) { error(_fileName + " Weird audio format: " + std::to_string(audioFile.getNumChannels()) + " channels and " + std::to_string(audioFile.getBitDepth()) + " bit depth."); }

		//Generate OpenAL buffers.
		debug("Loading audio " + fileName + " " + std::to_string(audioFile.getNumChannels()) +"/"+ std::to_string(audioFile.getBitDepth()) +"/"+ std::to_string(audioFile.getSampleRate()));
		alGenBuffers(1,&buffer);
		alBufferData(buffer, AL_FORMAT_MONO16, audioFile.samples[0].data(), audioFile.samples[0].size() * 2, audioFile.getSampleRate());

		//Determine if there were any errors loading the audio.
		ALenum errr = alGetError();
		if(errr != AL_NO_ERROR) { error("OpenAL error: " + std::to_string(errr) + " Bit depth: " + std::to_string(audioFile.getBitDepth()) + " Samples size: " + std::to_string(audioFile.samples[0].size()) + " Sample rate: " + std::to_string(audioFile.getSampleRate()) + " Channels: " + std::to_string(audioFile.getNumChannels())); }
	}

	// isMusic just adds the song to the music selection list drop down
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