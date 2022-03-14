#pragma once

#include <memory>
#include <unordered_map>
#include <filesystem>
#include <optional>

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

#include <oktal/result.h>

#include <AL/al.h>
#include <AL/alc.h>

namespace Audio
{
    using ID = uint16_t;
    static const ID NULL_ID = 0;

    // Streamed data params
    static const uint BUFFERS_NUM = 4;
    static const uint BUFFER_SIZE = 65536; // 32kb

    struct AudioSource
    {
        ALuint handle = 0;
        ID audioId = 0;

        struct Params{
            float pitch = 1.0f;
            float gain = 1.0f;
            float orientation = 0.0f; // radians
            glm::vec3 pos = glm::vec3{0.0f};
            glm::vec3 velocity = glm::vec3{0.0f};
            bool looping = false;
        } params;
    };

    struct StreamAudioSource
    {
        AudioSource source;

        ALuint streamBuffers[BUFFERS_NUM] = {0, 0, 0, 0};
        uint32_t cursor = 0;
        bool isPlaying = false;
    };

    class AudioMgr
    {
    public:
        ~AudioMgr();

        static AudioMgr* Get();

        void Init();
        void Update();

        // Audio Files
        enum LoadWAVError
        {
            NO_ERROR,
            INVALID_PATH,
            INVALID_FORMAT
        };
        std::pair<ID, LoadWAVError> LoadStaticWAVOrNull(std::filesystem::path path);
        std::pair<ID, LoadWAVError> LoadStaticWAVOrNull(ID id, std::filesystem::path path);
        Result<ID, LoadWAVError> LoadStaticWAV(std::filesystem::path path);
        Result<ID, LoadWAVError> LoadStaticWAV(ID id, std::filesystem::path path);

        std::pair<ID, LoadWAVError> LoadStreamedWAVOrNull(std::filesystem::path path);
        std::pair<ID, LoadWAVError> LoadStreamedWAVOrNull(ID id, std::filesystem::path path);
        Result<ID, LoadWAVError> LoadStreamedWAV(std::filesystem::path path);
        Result<ID, LoadWAVError> LoadStreamedWAV(ID id, std::filesystem::path path);

        // Audio sources

            // Static
        ID CreateSource();
        void SetSourceParams(ID sourceId, glm::vec3 pos, float orientation = 0.0f, bool looping = false, float gain = 1.0f , float pitch = 1.0f, glm::vec3 velocity = glm::vec3{0.0f});
        void SetSourceParams(ID sourceId, AudioSource::Params params);
        void SetSourceAudio(ID source, ID audioFile);
        std::optional<AudioSource::Params> GetSourceParams(ID srcId);
        void PlaySource(ID srcId);

            // Streamed
        ID CreateStreamSource();
        void SetStreamSourceParams(ID streamSrcId, AudioSource::Params params);
        void SetStreamSourceAudio(ID streamSrcId, ID streamFileId);
        std::optional<AudioSource::Params> GetStreamSourceParams(ID streamSrcId);
        void PlayStreamSource(ID srcId);

        // Listener
        void SetListenerParams(glm::vec3 pos, float orientation = 0.0f, float gain = 1.0f, glm::vec3 velocity = glm::vec3{0.0f});

        // Config
        inline void SetEnabled(bool enabled)
        {
            this->enabled = enabled;
        }

        inline bool IsEnabled()
        {
            return this->enabled;
        }

    private:

        // Files
        struct File
        {
            // SDL data
            SDL_AudioSpec audioSpec;
            uint8_t* wavBuffer = nullptr;
            uint32_t wavLength = 0;

            // OpenAl
            ALenum format = 0;
        };
        struct StaticFile
        {
            File file;
            ALuint alBuffer = 0;
        };
        Result<File, LoadWAVError> LoadWAV(std::filesystem::path path);
        std::optional<StaticFile> GetStaticFile(ID fileId);
        std::optional<File> GetStreamFile(ID stFileId);

        // Sources
        void SetSourceParams(AudioSource source);
        void SetSourceAudio(AudioSource source, ID srcId);
        void UpdateStreamedAudio(StreamAudioSource& source);

        static std::unique_ptr<AudioMgr> audioMgr_;
        ALCdevice* device_ = nullptr;
        ALCcontext* context_ = nullptr;

        // Sources
        std::unordered_map<ID, AudioSource> sources;
        ID lastSourceId = 0;

        std::unordered_map<ID, StreamAudioSource> streamSources;
        ID lastStreamSourceId = 0;

        // Files
        std::unordered_map<ID, StaticFile> staticFiles;
        ID lastStaticId = 1;

        std::unordered_map<ID, File> streamedFiles;
        ID lastSreamedId = 1;

        // General params
        bool enabled = true;
        float refDistance = 1.0f;

        AudioMgr();
    };
}