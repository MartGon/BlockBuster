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

    // Music 
    auto spawnId = LoadStreamedAudio("soundtrack.wav");
    musicIds.Add(SPAWN_THEME_ID, spawnId);

    // Wep Sounds
    for(int i = Entity::WeaponTypeID::ASSAULT_RIFLE; i < Entity::WeaponTypeID::CHEAT_SMG; i++)
    {
        auto filename = "weapon-shot-" + std::to_string(i) + ".wav";
        auto audioId = LoadStaticAudio(filename);
        wepSoundIds.Add(i, audioId);
    }
}

Audio::ID Gallery::GetSoundId(SoundID soundId)
{
    auto ret = Audio::AudioMgr::NULL_AUDIO_ID;
    if(auto audioId = soundIds.Get(soundId))
        ret = audioId.value();

    return ret;
}

Audio::ID Gallery::GetMusicId(MusicID musicId)
{
    auto ret = Audio::AudioMgr::NULL_AUDIO_ID;
    if(auto audioId = musicIds.Get(musicId))
        ret = audioId.value();

    return ret;
}

Audio::ID Gallery::GetWepSoundID(Entity::WeaponTypeID wepId)
{
    auto ret = Audio::AudioMgr::NULL_AUDIO_ID;
    if(auto audioId = wepSoundIds.Get(wepId))
        ret = audioId.value();

    return ret;
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