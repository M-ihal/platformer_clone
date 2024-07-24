#ifndef _AUDIO_PLAYER_H
#define _AUDIO_PLAYER_H

#include "common.h"
#include "maths.h"

#define DEF_SOUND_VOLUME 8
#define DEF_MUSIC_VOLUME 3

struct Sound {
    void *chunk; // struct Mix_Chunk
};

Sound *load_sound_wav(const char *filepath);
void delete_sound(Sound *sound);

struct Music {
    void *music; // struct Mix_Music
};

Music *load_music(const char *filepath);
void delete_music(Music *music);

namespace audio_player {
    void a_set_allow_play_sounds(bool allow);

    bool a_init(void);
    void a_quit(void);
    void a_set_sound_volume(uint8_t volume);
    void a_set_music_volume(uint8_t volume);
    void a_play_sound(Sound *sound, int32_t loops = 0);
    void a_stop_sounds(void);
    void a_play_music(Music *music, int32_t loops = -1);
    void a_stop_music(void);
    void a_pause_music(void);
    void a_resume_music(void);
    bool a_is_music_paused(void);
    bool a_is_music_playing(void);
    bool a_is_music(Music *music); // Is this music currently playing
    int32_t a_get_music_volume(void);
    int32_t a_get_last_set_music_volume(void);
}

#endif /* _AUDIO_PLAYER_H */