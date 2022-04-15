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

    enum AnnouncerSoundID{
        ANNOUNCER_SOUND_BLUE_TEAM_WINS,
        ANNOUNCER_SOUND_RED_TEAM_WINS,

        ANNOUNCER_SOUND_VICTORY_CALL,
        ANNOUNCER_SOUND_DEFEAT_CALL,

        ANNOUNCER_SOUND_MODE_FFA,
        ANNOUNCER_SOUND_MODE_TEAM_DEATHMATCH,
        ANNOUNCER_SOUND_MODE_DOMINATION,
        ANNOUNCER_SOUND_MODE_CAPTURE_THE_FLAG,

        ANNOUNCER_SOUND_FLAG_RESET,
        ANNOUNCER_SOUND_FLAG_TAKEN,
        ANNOUNCER_SOUND_FLAG_DROPPED,
        ANNOUNCER_SOUND_FLAG_RECOVERED,
        ANNOUNCER_SOUND_FLAG_CAPTURED,

        ANNOUNCER_SOUND_CAPTURE_POINT_LOST,
        ANNOUNCER_SOUND_CAPTURE_POINT_TAKEN,

        ANNOUNCER_SOUND_ENEMY_KILLED_0,
        ANNOUNCER_SOUND_ENEMY_KILLED_1,

        ANNOUNCER_SOUND_COUNT
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
        Audio::ID GetAnnouncerSoundId(AnnouncerSoundID asid);
        Audio::ID GetWepSoundID(Entity::WeaponTypeID wepId);

    private:

        Audio::ID LoadStaticAudio(std::filesystem::path filePath);
        Audio::ID LoadStreamedAudio(std::filesystem::path filePath);

        std::filesystem::path defaultFolder;
        Util::Table<Audio::ID> soundIds;
        Util::Table<Audio::ID> musicIds;
        Util::Table<Audio::ID> announcerIds;
        Util::Table<Audio::ID> wepSoundIds;
    };
}