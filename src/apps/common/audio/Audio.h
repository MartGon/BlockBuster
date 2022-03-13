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

    // Streamed data params
    static const uint BUFFERS_NUM = 4;
    static const uint BUFFER_SIZE = 65536; // 32kb

    class AudioFile
    {
    public:
        AudioFile() = default;
        ~AudioFile();

        AudioFile(const AudioFile& other) = delete;
        AudioFile& operator=(const AudioFile& other) = delete;

        AudioFile(AudioFile&& other);
        AudioFile& operator=(AudioFile&& other);

        std::string name;

        // SDL data
        SDL_AudioSpec audioSpec;
        uint8_t* wavBuffer = nullptr;
        uint32_t wavLength = 0;

        // OpenAl
        ALuint alBuffer = 0;
        ALenum format = 0;

        // Implementantion
        bool isStreamed = false;
        ALuint streamBuffers[BUFFERS_NUM] = {0, 0, 0, 0};
        uint32_t cursor = 0;
        bool isPlaying = false;
    };

    struct AudioSource
    {
        ALuint handle = 0;
        ID audioId = -1;

        struct Params{
            float pitch = 1;
            float gain = 1.0f;
            float orientation = 0.0f; // radians
            glm::vec3 pos = glm::vec3{0.0f};
            glm::vec3 velocity = glm::vec3{0.0f};
            bool looping = false;
        } params;
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
            INVALID_PATH,
            INVALID_FORMAT
        };
        Result<ID, LoadWAVError> LoadWAV(std::string name, std::filesystem::path path, bool isStreamed = false);

        // Audio sources
        ID CreateSource();
        void SetSourceParams(ID sourceId, glm::vec3 pos, float orientation = 0.0f, bool looping = false, float gain = 1.0f , float pitch = 1.0f, glm::vec3 velocity = glm::vec3{0.0f});
        void SetSourceParams(ID sourceId, AudioSource::Params params);
        void SetSourceAudio(ID source, ID audioFile);
        std::optional<AudioSource::Params> GetSourceParams(ID srcId);
        void PlaySource(ID srcId);
        void UpdateStreamedAudio(AudioSource& source);

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

        void SetSourceParams(AudioSource source);

        static std::unique_ptr<AudioMgr> audioMgr_;
        ALCdevice* device_ = nullptr;
        ALCcontext* context_ = nullptr;
        
        std::unordered_map<ID, AudioSource> sources;
        ID lastSourceId = 0;
        std::unordered_map<ID, AudioFile> files;
        ID lastId = 0;

        bool enabled = true;

        // General params
        float refDistance = 1.0f;

        AudioMgr();
    };
}