#pragma once

#include <util/Table.h>

#include <entity/Weapon.h>

#include <audio/Audio.h>

namespace Game::Sound
{
    enum SoundID{
        GRENADE_SOUND_ID,
        SOUND_ID_COUNT
    };

    enum MusicID{
        SPAWN_THEME_ID,
        MUSIC_ID_COUNT
    };

    class Gallery
    {
    public:
        void Start();

        inline void SetDefaultFolder(std::filesystem::path folder)
        {
            this->defaultFolder = folder;
        }

        Audio::ID GetSoundId(SoundID soundId);
        Audio::ID GetMusicId(MusicID musicId);
        Audio::ID GetWepSoundID(Entity::WeaponTypeID wepId);

    private:

        Audio::ID LoadStaticAudio(std::filesystem::path filePath);
        Audio::ID LoadStreamedAudio(std::filesystem::path filePath);

        std::filesystem::path defaultFolder;
        Util::Table<Audio::ID> soundIds;
        Util::Table<Audio::ID> musicIds;
        Util::Table<Audio::ID> wepSoundIds;
    };
}