#include <Audio.h>

#include <iostream>

#include <util/Container.h>

using namespace Audio;

std::unique_ptr<AudioMgr> AudioMgr::audioMgr_;

AudioMgr* AudioMgr::Get()
{
    AudioMgr* ptr = audioMgr_.get();
    if(ptr == nullptr)
    {
        audioMgr_ = std::unique_ptr<AudioMgr>(new AudioMgr);
        ptr = audioMgr_.get();
    }

    return ptr;
}

AudioMgr::AudioMgr()
{
    device_ = alcOpenDevice(nullptr);
    if(!device_)
    {
        std::cerr << "OpenAL: Could not open default device\n";
        return;
    }

    context_ = alcCreateContext(device_, nullptr);
    if(!context_)
    {
        std::cerr << "OpenAL: Could not create context for device. Error: " << alcGetError(device_) << '\n';
        return;
    }

    ALCboolean madeCurrent = alcMakeContextCurrent(context_);
    if(!madeCurrent)
    {
        std::cerr << "OpenAL: Could not make context current. Error: " << alcGetError(device_) << '\n';
        return;
    }
}

AudioMgr::~AudioMgr()
{   
    for(auto& [id, source] : sources)
        alDeleteSources(1, &source.handle);

    files.clear();

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context_);
    alcCloseDevice(device_);
}

// Functions

void AudioMgr::Init()
{
}

void AudioMgr::Update()
{
    if(!enabled)
        return;

    for(auto& [id, source] : sources)
    {
        ALint state;
        alGetSourcei(source.handle, AL_SOURCE_STATE, &state);
        if(state == AL_PLAYING)
            alSourcePlay(source.handle);
    }
}

// Audio Files

Result<ID, AudioMgr::LoadWAVError> AudioMgr::LoadWAV(std::string name, std::filesystem::path path)
{
    AudioFile file;
    auto res = SDL_LoadWAV(path.c_str(), &file.audioSpec, &file.wavBuffer, &file.wavLength);

    auto id = lastId++;
    Result<ID, LoadWAVError> result = Ok(id);
    if(res)
    {
        file.name = name;

        // Find format
        ALenum format;
        auto audioSpec = file.audioSpec;
        if(audioSpec.channels == 1 && audioSpec.format == AUDIO_U8)
            format = AL_FORMAT_MONO8;
        else if(audioSpec.channels == 1 && audioSpec.format == AUDIO_U16)
            format = AL_FORMAT_MONO16;
        else if(audioSpec.channels == 2 && audioSpec.format == AUDIO_U8)
            format = AL_FORMAT_STEREO8;
        else if(audioSpec.channels == 2 && audioSpec.format == AUDIO_U16)
            format = AL_FORMAT_STEREO16;
        else
        {
            std::cerr << "ERROR: unrecognised wave format: " << std::to_string(audioSpec.channels) << " channels, " << audioSpec.format << " bps" << std::endl;
            return Err(INVALID_FORMAT);
        }

        // Gen OpenAL buffer
        alGenBuffers(1, &file.alBuffer);
        alBufferData(file.alBuffer, format, file.wavBuffer, file.wavLength, audioSpec.freq);

        // Import file
        files[id] = std::move(file);
    }
    else
    {
        std::cerr << "Could not load audio file: " << path << '\n';
        return Err(INVALID_PATH);
    }

    return result;
}

// Audio Sources

ID AudioMgr::CreateSource()
{
    AudioSource source;
    alGenSources(1, &source.handle);
    SetSourceParams(source);
    
    auto id = lastSourceId++;
    sources[id] = std::move(source);

    return id;
}

void AudioMgr::SetSourceParams(ID sourceId, glm::vec3 pos, bool looping, float gain, float pitch, glm::vec3 velocity)
{
    if(Util::Map::Contains(sources, sourceId))
    {
        auto& source = sources[sourceId];
        source.pos = pos;
        source.looping = looping;
        source.gain = gain;
        source.pitch = pitch;
        source.velocity = velocity;

        SetSourceParams(source);
    }
}

void AudioMgr::SetSourceAudio(ID srcId, ID audioFile)
{
    if(Util::Map::Contains(sources, srcId) && Util::Map::Contains(files, audioFile))
    {
        auto& src = sources[srcId];
        src.audioId = audioFile;
        SetSourceParams(src);
    }
    
}

void AudioMgr::PlaySource(ID srcId)
{
    if(Util::Map::Contains(sources, srcId))
    {
        auto& source = sources[srcId];
        alSourcePlay(source.handle);
    }
}

// Private

void AudioMgr::SetSourceParams(AudioSource source)
{
    alSourcef(source.handle, AL_PITCH, source.pitch);
    alSourcef(source.handle, AL_GAIN, source.gain);
    alSource3f(source.handle, AL_POSITION, source.pos.x, source.pos.y, source.pos.z);
    alSource3f(source.handle, AL_VELOCITY, source.velocity.x, source.velocity.y, source.velocity.z);
    alSourcei(source.handle, AL_LOOPING, source.looping);
    auto& file = files[source.audioId];
    alSourcei(source.handle, AL_BUFFER, file.alBuffer);
}

// Audio File

AudioFile::~AudioFile()
{
    alDeleteBuffers(1, &alBuffer);
    SDL_FreeWAV(wavBuffer);
}

AudioFile::AudioFile(AudioFile&& other)
{
    *this = std::move(other);
}

AudioFile& AudioFile::operator=(AudioFile&& other)
{
    alDeleteBuffers(1, &alBuffer);
    SDL_FreeWAV(this->wavBuffer);
    this->alBuffer = other.alBuffer;
    this->wavBuffer = other.wavBuffer;

    this->audioSpec = other.audioSpec;
    this->name = other.name;
    this->wavLength = other.wavLength;

    other.alBuffer = 0;
    other.wavBuffer = nullptr;

    return *this;
}
