#include <sound/Gallery.h>

#include <ServiceLocator.h>

using namespace Game::Sound;

// Public

void Gallery::Start()
{
    auto audioMgr = Audio::AudioMgr::Get();

    // Sounds
    auto grenadeId = LoadStaticAudio("grenade.wav");
    soundIds.Add(GRENADE_SOUND_ID, grenadeId);

    for(auto i = 1; i < 3; i++)
    {
        auto filename = "hit-metal-" + std::to_string(i) + ".wav";
        auto soundId = LoadStaticAudio(filename);
        auto id = GRENADE_SOUND_ID + i;
        soundIds.Add(id, soundId);
    }

    // Music 
        // Spawn
    for(int i = MusicID::SPAWN_THEME_01_ID; i <= MusicID::SPAWN_THEME_04_ID; i++)
    {
        auto filename = "spawn-theme-" + std::to_string(i + 1) + ".wav";
        auto audioId = LoadStreamedAudio(filename);
        musicIds.Add(i, audioId);
    }
        // Victory
    for(int i = 0; i < 2; i++)
    {
        auto id = MusicID::VICTORY_THEME_01_ID + i;
        auto filename = "victory-theme-" + std::to_string(i + 1) + ".wav";
        auto audioId = LoadStreamedAudio(filename);
        musicIds.Add(id, audioId);
    }

    // Wep Sounds
    for(int i = Entity::WeaponTypeID::ASSAULT_RIFLE; i < Entity::WeaponTypeID::CHEAT_SMG; i++)
    {
        auto filename = "weapon-shot-" + std::to_string(i) + ".wav";
        auto audioId = LoadStaticAudio(filename);
        wepSoundIds.Add(i, audioId);

        filename = "weapon-reload-" + std::to_string(i) + ".wav";
        audioId = LoadStaticAudio(filename);
        wepReloadSoundIds.Add(i, audioId);
    }

    // Announcer sounds
    announcerIds.Add(ANNOUNCER_SOUND_BLUE_TEAM_WINS, LoadStaticAudio("blue-team-wins.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_RED_TEAM_WINS, LoadStaticAudio("red-team-wins.wav"));

    announcerIds.Add(ANNOUNCER_SOUND_VICTORY_CALL, LoadStaticAudio("victory-call.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_DEFEAT_CALL, LoadStaticAudio("defeat-call.wav"));

    announcerIds.Add(ANNOUNCER_SOUND_MODE_FFA, LoadStaticAudio("free-for-all.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_MODE_TEAM_DEATHMATCH, LoadStaticAudio("team-death-match.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_MODE_DOMINATION, LoadStaticAudio("domination.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_MODE_CAPTURE_THE_FLAG, LoadStaticAudio("capture-the-flag.wav"));

    announcerIds.Add(ANNOUNCER_SOUND_FLAG_RESET, LoadStaticAudio("flag-reset.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_FLAG_TAKEN, LoadStaticAudio("flag-taken.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_FLAG_DROPPED, LoadStaticAudio("flag-dropped.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_FLAG_RECOVERED, LoadStaticAudio("flag-recovered.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_FLAG_CAPTURED, LoadStaticAudio("flag-captured.wav"));

    announcerIds.Add(ANNOUNCER_SOUND_CAPTURE_POINT_LOST, LoadStaticAudio("capture-point-lost.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_CAPTURE_POINT_TAKEN, LoadStaticAudio("capture-point-taken.wav"));

    announcerIds.Add(ANNOUNCER_SOUND_ENEMY_KILLED_0, LoadStaticAudio("enemy-killed-1.wav"));
    announcerIds.Add(ANNOUNCER_SOUND_ENEMY_KILLED_1, LoadStaticAudio("enemy-killed-2.wav"));
}

Audio::ID Gallery::GetSoundId(SoundID soundId)
{
    return GetAudio(soundIds, soundId);
}

Audio::ID Gallery::GetMusicId(MusicID musicId)
{
    return GetAudio(musicIds, musicId);
}

Audio::ID Gallery::GetAnnouncerSoundId(AnnouncerSoundID asid)
{
    return GetAudio(announcerIds, asid);
}

Audio::ID Gallery::GetWepSoundID(Entity::WeaponTypeID wepId)
{
    return GetAudio(wepSoundIds, wepId);
}

Audio::ID Gallery::GetWepReloadSoundId(Entity::WeaponTypeID wepId)
{
    return GetAudio(wepReloadSoundIds, wepId);
}

// Private

Audio::ID Gallery::LoadStaticAudio(std::filesystem::path filePath)
{
    auto audioMgr = Audio::AudioMgr::Get();
    auto fullpath = defaultFolder / filePath;
    auto res = audioMgr->LoadStaticWAVOrNull(fullpath);

    if(res.second != Audio::AudioMgr::LoadWAVError::NO_ERR)
        App::ServiceLocator::GetLogger()->LogError("Could not load audio file: " + fullpath.string());

    return res.first;
}

Audio::ID Gallery::LoadStreamedAudio(std::filesystem::path filePath)
{
    auto audioMgr = Audio::AudioMgr::Get();
    auto fullpath = defaultFolder / filePath;
    auto res =  audioMgr->LoadStreamedWAVOrNull(fullpath);

    if(res.second != Audio::AudioMgr::LoadWAVError::NO_ERR)
        App::ServiceLocator::GetLogger()->LogError("Could not load streamed audio file: " + fullpath.string());

    return res.first;
}

Audio::ID Gallery::GetAudio(Util::Table<Audio::ID>& table, Util::Table<Audio::ID>::ID id)
{
    auto ret = Audio::AudioMgr::NULL_AUDIO_ID;
    if(auto audioId = table.Get(id))
        ret = audioId.value();

    return ret;
}