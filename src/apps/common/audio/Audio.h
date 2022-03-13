#pragma once

#include <memory>
#include <unordered_map>
#include <filesystem>

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

#include <oktal/result.h>

#include <AL/al.h>
#include <AL/alc.h>

namespace Audio
{
    using ID = uint16_t;

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
    };

    struct AudioSource
    {
        ALuint handle = 0;

        float pitch = 1;
        float gain = 1.0f;
        glm::vec3 pos;
        glm::vec3 velocity;
        bool looping = false;
        ID audioId = -1;
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
        Result<ID, LoadWAVError> LoadWAV(std::string name, std::filesystem::path path);

        // Audio sources
        ID CreateSource();
        void SetSourceParams(ID sourceId, glm::vec3 pos, bool looping = false, float gain = 1.0f , float pitch = 1.0f, glm::vec3 velocity = glm::vec3{0.0f});
        void SetSourceAudio(ID source, ID audioFile);
        void PlaySource(ID srcId);

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

        AudioMgr();
    };
}