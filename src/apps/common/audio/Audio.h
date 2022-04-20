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
    static const unsigned int BUFFERS_NUM = 4;
    static const unsigned int BUFFER_SIZE = 65536; // 32kb

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
            float relDistance = 8.0f;
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


        static constexpr int NULL_AUDIO_ID = 0;
        static AudioMgr* Get();

        void Init();
        void Update();
        void Shutdown();

        // Audio Files
        enum LoadWAVError
        {
            NO_ERR,
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
        void SetSourceParams(ID sourceId, glm::vec3 pos, float orientation = 0.0f, bool looping = false, float relDistance = 1.0f, float gain = 1.0f , float pitch = 1.0f, glm::vec3 velocity = glm::vec3{0.0f});
        void SetSourceParams(ID sourceId, AudioSource::Params params);
        void SetSourceTransform(ID sourceId, glm::vec3 pos, float orientation = 0.0f);
        void SetSourceAudio(ID source, ID audioFile);
        std::optional<AudioSource::Params> GetSourceParams(ID srcId);
        void PlaySource(ID srcId);
        bool IsSourcePlaying(ID source);

            // Streamed
        ID CreateStreamSource();
        void SetStreamSourceParams(ID streamSrcId, AudioSource::Params params);
        void SetStreamSourceAudio(ID streamSrcId, ID streamFileId);
        std::optional<AudioSource::Params> GetStreamSourceParams(ID streamSrcId);
        void PlayStreamSource(ID srcId);

        // Listener
        void SetListenerParams(glm::vec3 pos, float orientation = 0.0f, float gain = 1.0f, glm::vec3 velocity = glm::vec3{0.0f});
        void SetListenerTransform(glm::vec3 pos, float orientation = 0.0f);
        void SetListenerGain(float gain);

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

        // ID
        template <typename T>
        ID GetFreeId(const std::unordered_map<ID, T>& map)
        {
            for(ID id = 1; id < std::numeric_limits<ID>::max(); id++)
                if(map.find(id) == map.end())
                    return id;

            return 0;
        }

        // Singleton
        static std::unique_ptr<AudioMgr> audioMgr_;
        ALCdevice* device_ = nullptr;
        ALCcontext* context_ = nullptr;

        // Sources
        std::unordered_map<ID, AudioSource> sources;
        std::unordered_map<ID, StreamAudioSource> streamSources;

        // Files
        std::unordered_map<ID, StaticFile> staticFiles;
        std::unordered_map<ID, File> streamedFiles;

        // Listener
        glm::vec3 pos{0.0f};
        float orientation = 0.0f;
        float gain = 1.0f;

        // General params
        bool enabled = true;

        AudioMgr();
    };
}