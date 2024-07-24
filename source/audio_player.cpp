#include "audio_player.h"
#include "common.h"
#include "data.h"

#include <SDL_mixer.h>

namespace {
    bool _initialized = false;
    bool allow_play_sounds = true;

    int32_t last_set_non_zero_music_volume;
    Mix_Music *last_played_music;
}

void audio_player::a_set_allow_play_sounds(bool allow) {
    allow_play_sounds = allow;
}

void audio_player::a_set_sound_volume(uint8_t volume) {
    Mix_Volume(-1, volume);
}

void audio_player::a_set_music_volume(uint8_t volume) {
    Mix_VolumeMusic(volume);

    if(volume > 0) {
        last_set_non_zero_music_volume = volume;
    }
}

Sound *load_sound_wav(const char *filepath) {
    Sound *sound = malloc_and_zero_struct(Sound);
    if(sound == NULL) {
        // fprintf(stderr, "Couldn't create memory for Sound.\n");
        return NULL;
    }

    sound->chunk = Mix_LoadWAV(filepath);
    if(sound->chunk == NULL) {
        // fprintf(stderr, "Couldn't load .wav sound from filepath.\n");
        free(sound);
        return NULL;
    }

    return sound;
}

void delete_sound(Sound *sound) {
    assert(sound != NULL);
    Mix_FreeChunk((Mix_Chunk *)sound->chunk);
    free(sound);
}

Music *load_music(const char *filepath) {
    Music *music = malloc_and_zero_struct(Music);
    if(music == NULL) {
        fprintf(stderr, "Couldn't create memory for Music.\n");
        return NULL;
    }

    music->music = Mix_LoadMUS(filepath);
    if(music->music == NULL) {
        fprintf(stderr, "Couldn't load music file from filepath.\n");
        free(music);
        return NULL;
    }

    return music;
}

void delete_music(Music *music) {
    assert(music != NULL);
    Mix_FreeMusic((Mix_Music *)music->music);
    free(music);
}

bool audio_player::a_init(void) {
    assert(_initialized == false);

    bool audio_open = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == 0;
    if(!audio_open) {
        fprintf(stderr, "Couldn't open audio device.");
        return false;
    }

    Mix_AllocateChannels(32);

    last_set_non_zero_music_volume = 15;

    a_set_sound_volume(DEF_SOUND_VOLUME);
    a_set_music_volume(DEF_MUSIC_VOLUME);

    // a_set_music_volume(0);

    _initialized = true;
    return true;
}

void audio_player::a_quit(void) {
    Mix_CloseAudio();
}

void audio_player::a_play_sound(Sound *sound, int32_t loops) {
    if(allow_play_sounds == false) {
        return;
    }

    assert(sound != NULL && sound->chunk != NULL);
    bool channel = Mix_PlayChannel(-1, (Mix_Chunk *)sound->chunk, loops);
    if(channel == -1) {
	    fprintf(stderr, "No available channel for chunk!\n");
    }
}

void audio_player::a_stop_sounds(void) {
    Mix_HaltChannel(-1);
}

void audio_player::a_play_music(Music *music, int32_t loops) {
    if(music == NULL) {
        audio_player::a_stop_music();
        return;
    }

    assert(music->music != NULL);
    Mix_PlayMusic((Mix_Music *)music->music, loops);
    last_played_music = (Mix_Music *)music->music;
}

void audio_player::a_stop_music(void) {
    Mix_HaltMusic();
    last_played_music = NULL;
}

void audio_player::a_pause_music(void) {
    Mix_PauseMusic();
}

void audio_player::a_resume_music(void) {
    Mix_ResumeMusic();
}

bool audio_player::a_is_music_paused(void) {
    return Mix_PausedMusic() == 1;
}

bool audio_player::a_is_music_playing(void) {
    return Mix_PlayingMusic() == 1;
}

bool audio_player::a_is_music(Music *music) {
    if(last_played_music == NULL && music == NULL) return true;

    assert(music != NULL && music->music != NULL);
    return music->music == last_played_music;
}

int32_t audio_player::a_get_music_volume(void) {
    return Mix_VolumeMusic(-1);
}

int32_t audio_player::a_get_last_set_music_volume(void) {
    return last_set_non_zero_music_volume;
}
