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
        else if(audioSpec.channels == 1 && (audioSpec.format == AUDIO_U16 || audioSpec.format == AUDIO_S16LSB))
            format = AL_FORMAT_MONO16;
        else if(audioSpec.channels == 2 && audioSpec.format == AUDIO_U8)
            format = AL_FORMAT_STEREO8;
        else if(audioSpec.channels == 2 && audioSpec.format == AUDIO_U16)
            format = AL_FORMAT_STEREO16;
        else
        {
            std::cerr << "ERROR: unrecognised wave format: " << std::to_string(audioSpec.channels) << " channels, " << std::hex << audioSpec.format << " bps" << std::endl;
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

    alSourcef(source.handle, AL_REFERENCE_DISTANCE, refDistance);
    alSourcei(source.handle, AL_SOURCE_RELATIVE, AL_FALSE);
    
    auto id = lastSourceId++;
    sources[id] = std::move(source);

    return id;
}

void AudioMgr::SetSourceParams(ID sourceId, glm::vec3 pos, float orientation, bool looping, float gain, float pitch, glm::vec3 velocity)
{
    if(Util::Map::Contains(sources, sourceId))
    {
        auto& source = sources[sourceId];
        auto& params = source.params;
        params.pos = pos;
        params.orientation = orientation;
        params.looping = looping;
        params.gain = gain;
        params.pitch = pitch;
        params.velocity = velocity;

        SetSourceParams(source);
    }
}

void AudioMgr::SetSourceParams(ID sourceId, AudioSource::Params params)
{
    SetSourceParams(sourceId, params.pos, params.orientation, params.looping, params.gain, params.pitch, params.velocity);
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

// Listener

void AudioMgr::SetListenerParams(glm::vec3 pos, float orientation, float gain, glm::vec3 vel)
{
    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    glm::vec3 rot = glm::vec3{glm::cos(orientation), 0.0f, glm::sin(orientation)};
    ALfloat ori[]={rot.x, rot.y, rot.z, 0.0, 1.0, 0.0};
    alListenerfv(AL_ORIENTATION, ori);
    alListenerf(AL_GAIN, gain);
    alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
}

// Private

void AudioMgr::SetSourceParams(AudioSource source)
{
    auto params = source.params;
    alSourcef(source.handle, AL_PITCH, params.pitch);
    alSourcef(source.handle, AL_GAIN, params.gain);
    alSource3f(source.handle, AL_POSITION, params.pos.x, params.pos.y, params.pos.z);
    glm::vec3 rot = glm::vec3{glm::cos(params.orientation), 0.0f, glm::sin(params.orientation)};
    ALfloat ori[]={rot.x, rot.y, rot.z, 0.0, 1.0, 0.0};
    alSourcefv(source.handle, AL_ORIENTATION, ori);
    alSource3f(source.handle, AL_VELOCITY, params.velocity.x, params.velocity.y, params.velocity.z);
    alSourcei(source.handle, AL_LOOPING, params.looping);
    auto& file = files[source.audioId];
    alSourcei(source.handle, AL_BUFFER, file.alBuffer);
}

std::optional<AudioSource::Params> AudioMgr::GetSourceParams(ID srcId)
{
    std::optional<AudioSource::Params> params;
    if(Util::Map::Contains(sources, srcId))
        params = sources[srcId].params;

    return params;
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
