#include <Audio.h>

#include <iostream>
#include <cstring>

#include <util/Container.h>

#include <debug/Debug.h>

#include <AL/alext.h>

using namespace Audio;

std::unique_ptr<Audio::AudioMgr> AudioMgr::audioMgr_;

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

    for(auto& [id, sSource] : streamSources)
    {
        alDeleteBuffers(4, sSource.streamBuffers);
        alDeleteSources(1, &sSource.source.handle);
    }

    for(auto& [id, file] : staticFiles)
    {
        alDeleteBuffers(1, &file.alBuffer);
    }

    for(auto& [id, file] : streamedFiles)
    {
        SDL_FreeWAV(file.wavBuffer);
    }

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

    for(auto& [id, source] : streamSources)
    {
        UpdateStreamedAudio(source);
    }
}

void AudioMgr::Shutdown()
{
    for(auto& [id, source] : sources)
    {
        alSourceStop(source.handle);
        alDeleteSources(1, &source.handle);
    }
    sources.clear();

    for(auto& [id, sSource] : streamSources)
    {
        alSourceStop(sSource.source.handle);
        alDeleteBuffers(4, sSource.streamBuffers);
        alDeleteSources(1, &sSource.source.handle);
    }
    streamSources.clear();

    for(auto& [id, file] : staticFiles)
    {
        alDeleteBuffers(1, &file.alBuffer);
    }
    staticFiles.clear();

    for(auto& [id, file] : streamedFiles)
    {
        SDL_FreeWAV(file.wavBuffer);
    }
    streamedFiles.clear();
}

// Audio Files

std::pair<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStaticWAVOrNull(ID id, std::filesystem::path path)
{
    std::pair<ID, LoadWAVError> ret = {NULL_AUDIO_ID, LoadWAVError::NO_ERR};
    auto res = LoadStaticWAV(id, path);
    if(res.isOk())
        ret.first = res.unwrap();
    else
        ret.second = res.unwrapErr();

    return ret;
}

std::pair<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStaticWAVOrNull(std::filesystem::path path)
{
    auto id = GetFreeId(staticFiles);
    return LoadStaticWAVOrNull(id, path);
}

Result<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStaticWAV(std::filesystem::path path)
{
    auto id = GetFreeId(staticFiles);
    return LoadStaticWAV(id, path);
}

Result<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStaticWAV(ID id, std::filesystem::path path)
{
    auto res = LoadWAV(path);
    if(res.isOk())
    {
        StaticFile sfile;
        auto file = res.unwrap();
        sfile.file = file;

        // Gen OpenAL buffer
        alGenBuffers(1, &sfile.alBuffer);
        alBufferData(sfile.alBuffer, file.format, file.wavBuffer, file.wavLength, file.audioSpec.freq);

        // Delete from main RAM
        SDL_FreeWAV(sfile.file.wavBuffer);

        assertm(!Util::Map::Contains(staticFiles, id), "A static WAV file with that id already exists");

        // Import file
        staticFiles[id] = std::move(sfile);

        return Ok(id);
    }

    return Err(res.unwrapErr());
}

std::pair<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStreamedWAVOrNull(ID id, std::filesystem::path path)
{
    std::pair<ID, LoadWAVError> ret = {NULL_AUDIO_ID, LoadWAVError::NO_ERR};
    auto res = LoadStreamedWAV(id, path);
    if(res.isOk())
        ret.first = res.unwrap();
    else
        ret.second = res.unwrapErr();

    return ret;
}

std::pair<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStreamedWAVOrNull(std::filesystem::path path)
{
    auto id = GetFreeId(streamedFiles);
    return LoadStreamedWAVOrNull(id, path);
}

Result<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStreamedWAV(std::filesystem::path path)
{
    auto id = GetFreeId(streamedFiles);
    return LoadStreamedWAV(id, path);
}

Result<ID, AudioMgr::LoadWAVError> AudioMgr::LoadStreamedWAV(ID id, std::filesystem::path path)
{
    auto res = LoadWAV(path);
    if(res.isOk())
    {
        File sfile = res.unwrap();

        assertm(!Util::Map::Contains(streamedFiles, id), "A streamed WAV file with that id already exists");

        // Import file
        streamedFiles[id] = std::move(sfile);

        return Ok(id);
    }

    return Err(res.unwrapErr());
}

// Audio Sources

    // Audio Sources - Static

ID AudioMgr::CreateSource()
{
    AudioSource source;
    alGenSources(1, &source.handle);
    SetSourceParams(source);

    alSourcei(source.handle, AL_SOURCE_RELATIVE, AL_FALSE);
    
    auto id = GetFreeId(sources);
    sources[id] = std::move(source);

    return id;
}

void AudioMgr::SetSourceParams(ID sourceId, glm::vec3 pos, float orientation, bool looping, float relDistance, float gain, float pitch, glm::vec3 velocity)
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
        params.relDistance = relDistance;

        SetSourceParams(source);
    }
}

void AudioMgr::SetSourceParams(ID sourceId, AudioSource::Params params)
{
    SetSourceParams(sourceId, params.pos, params.orientation, params.looping, params.relDistance, params.gain, params.pitch, params.velocity);
}

void AudioMgr::SetSourceTransform(ID sourceId, glm::vec3 pos, float orientation)
{
    if(!Util::Map::Contains(sources, sourceId))
        return;

    auto& src = sources[sourceId];
    auto& params = src.params;

    params.pos = pos;
    params.orientation = orientation;

    SetSourceParams(src);
}

void AudioMgr::SetSourceAudio(ID srcId, ID audioFile)
{
    if(Util::Map::Contains(sources, srcId) && Util::Map::Contains(staticFiles, audioFile) && !IsSourcePlaying(srcId))
    {
        auto& src = sources[srcId];
        src.audioId = audioFile;
        SetSourceAudio(src, src.audioId);
    }
}

std::optional<AudioSource::Params> AudioMgr::GetSourceParams(ID srcId)
{
    std::optional<AudioSource::Params> params;
    if(Util::Map::Contains(sources, srcId))
        params = sources[srcId].params;

    return params;
}

void AudioMgr::PlaySource(ID srcId)
{
    if(Util::Map::Contains(sources, srcId))
    {
        auto& source = sources[srcId];
        alSourcePlay(source.handle);
    }
}

bool AudioMgr::IsSourcePlaying(ID srcId)
{
    bool isPlaying = false;
    
    if(Util::Map::Contains(sources, srcId))
    {
        auto& source = sources[srcId];
        ALenum state;
        alGetSourcei(source.handle, AL_SOURCE_STATE, &state);        
        isPlaying = state == AL_PLAYING;    
    }
    
    return isPlaying;
}

    // Audio Sources - Streamed

ID AudioMgr::CreateStreamSource()
{
    StreamAudioSource sSource;
    auto& source = sSource.source;
    alGenSources(1, &source.handle);
    SetSourceParams(source);

    alGenBuffers(BUFFERS_NUM, sSource.streamBuffers);
    alSourcei(source.handle, AL_SOURCE_RELATIVE, AL_FALSE);
    
    auto id = GetFreeId(streamSources);
    streamSources[id] = std::move(sSource);

    return id;
}

void AudioMgr::SetStreamSourceParams(ID streamSrcId, AudioSource::Params params)
{
    if(Util::Map::Contains(streamSources, streamSrcId))
    {
        auto& sSource = streamSources[streamSrcId];
        SetSourceParams(sSource.source.handle, params);
    }
}

void AudioMgr::SetStreamSourceAudio(ID streamSrcId, ID streamFileId)
{
    if(Util::Map::Contains(streamSources, streamSrcId) && Util::Map::Contains(streamedFiles, streamFileId))
    {
        auto& sSource = streamSources[streamSrcId];
        sSource.source.audioId = streamFileId;
    }
}

std::optional<AudioSource::Params> AudioMgr::GetStreamSourceParams(ID srcId)
{
    std::optional<AudioSource::Params> params;
    if(Util::Map::Contains(streamSources, srcId))
        params = streamSources[srcId].source.params;

    return params;
}

void AudioMgr::PlayStreamSource(ID streamSrcId)
{
    if(Util::Map::Contains(streamSources, streamSrcId))
    {        
        auto& sSource = streamSources[streamSrcId];
        if(!sSource.isPlaying)
        {
            if(auto audio = GetStreamFile(sSource.source.audioId))
            {
                for(auto i = 0; i < Audio::BUFFERS_NUM; ++i)
                {
                    auto offset = i * BUFFER_SIZE;
                    alBufferData(sSource.streamBuffers[i], audio->format, audio->wavBuffer + offset, BUFFER_SIZE, audio->audioSpec.freq);
                }

                sSource.cursor = BUFFERS_NUM * BUFFER_SIZE;

                auto& source = sSource.source;
                alSourceQueueBuffers(source.handle, BUFFERS_NUM, sSource.streamBuffers);
                alSourcePlay(source.handle);
                sSource.isPlaying = true;
            }
        }
    }
}

void AudioMgr::UpdateStreamedAudio(StreamAudioSource& sSource)
{
    auto& source = sSource.source;
    
    ALint state;
    alGetSourcei(source.handle, AL_SOURCE_STATE, &state);
    if(state == AL_PLAYING)
    {
        ALint buffersRead = 0;
        alGetSourcei(source.handle, AL_BUFFERS_PROCESSED, &buffersRead);

        if(buffersRead < 0)
            return;
        
        while(buffersRead-- > 0)
        {
            ALuint buffer;
            alSourceUnqueueBuffers(source.handle, 1, &buffer);
            
            // No need to check here. A null audio should never be played in the first place
            auto& audio = streamedFiles[source.audioId];

            auto dataPlayed = sSource.cursor;
            auto size = dataPlayed + BUFFER_SIZE > audio.wavLength ? audio.wavLength - dataPlayed : BUFFER_SIZE;
            auto bufferSrc = audio.wavBuffer + sSource.cursor;
        
            alBufferData(buffer, audio.format, bufferSrc, size, audio.audioSpec.freq);
            alSourceQueueBuffers(source.handle, 1, &buffer);

            sSource.cursor += size;
        }
    }
    else if(sSource.isPlaying)
    {
        alSourceUnqueueBuffers(source.handle, 4, sSource.streamBuffers);
        sSource.isPlaying = false;
    }
}

// Listener

void AudioMgr::SetListenerParams(glm::vec3 pos, float orientation, float gain, glm::vec3 vel)
{
    this->pos = pos;
    this->orientation = orientation;
    this->gain = gain;

    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    glm::vec3 rot = glm::vec3{glm::cos(orientation), 0.0f, glm::sin(orientation)};
    ALfloat ori[]={rot.x, rot.y, rot.z, 0.0, 1.0, 0.0};
    alListenerfv(AL_ORIENTATION, ori);
    alListenerf(AL_GAIN, gain);
    alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
}

void AudioMgr::SetListenerTransform(glm::vec3 pos, float orientation)
{
    SetListenerParams(pos, orientation, this->gain);
}

void AudioMgr::SetListenerGain(float gain)
{
    SetListenerParams(this->pos, this->orientation, gain);
}

// Private

// Files

Result<AudioMgr::File, AudioMgr::LoadWAVError> AudioMgr::LoadWAV(std::filesystem::path path)
{
    File file;
    auto res = SDL_LoadWAV(path.string().c_str(), &file.audioSpec, &file.wavBuffer, &file.wavLength);

    if(res)
    {
        // Find format
        ALenum format;
        auto audioSpec = file.audioSpec;
        if(audioSpec.channels == 1 && audioSpec.format == AUDIO_U8)
            format = AL_FORMAT_MONO8;
        else if(audioSpec.channels == 1 && (audioSpec.format == AUDIO_U16 || audioSpec.format == AUDIO_S16LSB || audioSpec.format == AUDIO_S16MSB))
            format = AL_FORMAT_MONO16;
        else if(audioSpec.channels == 1 && audioSpec.format == AUDIO_F32)
            format = AL_MONO32F_SOFT;
        else if(audioSpec.channels == 2 && audioSpec.format == AUDIO_U8)
            format = AL_FORMAT_STEREO8;
        else if(audioSpec.channels == 2 && (audioSpec.format == AUDIO_U16 || audioSpec.format == AUDIO_S16LSB || audioSpec.format == AUDIO_S16MSB))
            format = AL_FORMAT_STEREO16;
        else
        {
            std::cerr << "ERROR: unrecognised wave format: " << std::to_string(audioSpec.channels) << " channels, " << std::hex << audioSpec.format << " bps" << std::endl;
            return Err(INVALID_FORMAT);
        }
        file.format = format;

        return Ok(file);
    }

    std::cerr << "Could not load audio file: " << path << '\n';
    return Err(INVALID_PATH);    
}

std::optional<AudioMgr::StaticFile> AudioMgr::GetStaticFile(ID fileId)
{
    std::optional<StaticFile> file;
    if(Util::Map::Contains(staticFiles, fileId))
        file = staticFiles[fileId];
    
    return file;
}

std::optional<AudioMgr::File> AudioMgr::GetStreamFile(ID fileId)
{
    std::optional<File> file;
    if(Util::Map::Contains(streamedFiles, fileId))
        file = streamedFiles[fileId];

    return file;
}

// Sources

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
    alSourcef(source.handle, AL_REFERENCE_DISTANCE, params.relDistance);
}

void AudioMgr::SetSourceAudio(AudioSource source, ID audioId)
{
    if(auto file = GetStaticFile(audioId))
        alSourcei(source.handle, AL_BUFFER, file->alBuffer);
}


