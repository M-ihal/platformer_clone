#ifndef _DATA_H
#define _DATA_H

#include "common.h"
#include "opengl_abs.h"
#include "renderer.h"
#include "audio_player.h"

bool read_file(const char *filepath, void **out_file_data, size_t *out_size, bool null_terminated = false);
void free_file(void *file_data);
bool save_file(const char *filepath, void *buffer, size_t buffer_size);

#define DATA_DIRECTORY_NAME  "data"
#define SPRITESHEET_FILENAME "spritesheet.png"
#define SMALL_FONT_FILENAME  "SuperMarioWorldTextBoxRegular-Y86j.ttf"

/* --- Sprites --- */

#define SPRITE(sprite_name, cell_w, cell_h, cell_x, cell_y, sprite_w, sprite_h)
#define SPRITES\
    SPRITE(SPRITE_MARIO_IDLE,      16, 16, 0,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_RUN_1,     16, 16, 1,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_RUN_2,     16, 16, 2,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_RUN_3,     16, 16, 3,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_TURN,      16, 16, 4,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_JUMP,      16, 16, 5,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_CLIMB_1,   16, 16, 6,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_CLIMB_2,   16, 16, 7,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_SWIM_1,    16, 16, 8,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_SWIM_2,    16, 16, 9,  19, 16, 16)\
    SPRITE(SPRITE_MARIO_SWIM_3,    16, 16, 10, 19, 16, 16)\
    SPRITE(SPRITE_MARIO_SWIM_4,    16, 16, 11, 19, 16, 16)\
    SPRITE(SPRITE_MARIO_SWIM_5,    16, 16, 12, 19, 16, 16)\
    SPRITE(SPRITE_MARIO_GAME_OVER, 16, 16, 13, 19, 16, 16)\
\
    SPRITE(SPRITE_MARIO_BIG_IDLE,    16, 16, 0, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_CROUTCH, 16, 16, 1, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_RUN_1,   16, 16, 2, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_RUN_2,   16, 16, 3, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_RUN_3,   16, 16, 4, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_TURN,    16, 16, 5, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_JUMP,    16, 16, 6, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_CLIMB_1, 16, 16, 7, 17, 16, 32)\
    SPRITE(SPRITE_MARIO_BIG_CLIMB_2, 16, 16, 8, 17, 16, 32)\
\
    SPRITE(SPRITE_MARIO_FIRE_IDLE,    16, 16, 0, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_CROUTCH, 16, 16, 1, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_RUN_1,   16, 16, 2, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_RUN_2,   16, 16, 3, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_RUN_3,   16, 16, 4, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_TURN,    16, 16, 5, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_JUMP,    16, 16, 6, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_CLIMB_1, 16, 16, 7, 15, 16, 32)\
    SPRITE(SPRITE_MARIO_FIRE_CLIMB_2, 16, 16, 8, 15, 16, 32)\
\
    SPRITE(SPRITE_FIREBALL_1, 8, 8, 28, 38, 8, 8)\
    SPRITE(SPRITE_FIREBALL_2, 8, 8, 28, 39, 8, 8)\
    SPRITE(SPRITE_FIREBALL_3, 8, 8, 29, 39, 8, 8)\
    SPRITE(SPRITE_FIREBALL_4, 8, 8, 29, 38, 8, 8)\
    SPRITE(SPRITE_FIRE_EXPLOSION_1, 16, 16, 15, 19, 16, 16)\
    SPRITE(SPRITE_FIRE_EXPLOSION_2, 16, 16, 16, 19, 16, 16)\
    SPRITE(SPRITE_FIRE_EXPLOSION_3, 16, 16, 17, 19, 16, 16)\
\
    SPRITE(SPRITE_BLOCK_BRICK_BIG,      16, 16, 11, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_COBBLESTONE,    16, 16, 13, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_BRICK,          16, 16, 14, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_SOLID,          16, 16, 15, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_BRICK_2,        16, 16, 16, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_QUESTION_1,     16, 16, 17, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_QUESTION_2,     16, 16, 18, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_QUESTION_3,     16, 16, 19, 31, 16, 16)\
    SPRITE(SPRITE_BLOCK_QUESTION_EMPTY, 16, 16, 20, 31, 16, 16)\
    SPRITE(SPRITE_BRICK_PARTICLE_1, 8, 8, 24, 62, 8, 8)\
    SPRITE(SPRITE_BRICK_PARTICLE_2, 8, 8, 25, 62, 8, 8)\
    SPRITE(SPRITE_BRICK_PARTICLE_3, 8, 8, 24, 63, 8, 8)\
    SPRITE(SPRITE_BRICK_PARTICLE_4, 8, 8, 25, 63, 8, 8)\
\
    SPRITE(SPRITE_BLOCK_UG_BRICK_BIG,      16, 16, 11, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_COBBLESTONE,    16, 16, 13, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_BRICK,          16, 16, 14, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_SOLID,          16, 16, 15, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_BRICK_2,        16, 16, 16, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_QUESTION_1,     16, 16, 17, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_QUESTION_2,     16, 16, 18, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_QUESTION_3,     16, 16, 19, 30, 16, 16)\
    SPRITE(SPRITE_BLOCK_UG_QUESTION_EMPTY, 16, 16, 20, 30, 16, 16)\
    SPRITE(SPRITE_UG_BRICK_PARTICLE_1, 8, 8, 24, 60, 8, 8)\
    SPRITE(SPRITE_UG_BRICK_PARTICLE_2, 8, 8, 25, 60, 8, 8)\
    SPRITE(SPRITE_UG_BRICK_PARTICLE_3, 8, 8, 24, 61, 8, 8)\
    SPRITE(SPRITE_UG_BRICK_PARTICLE_4, 8, 8, 25, 61, 8, 8)\
\
    SPRITE(SPRITE_BLOCK_CASTLE_BRIDGE,         16, 16, 10, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_BRICK_BIG,      16, 16, 11, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_COBBLESTONE,    16, 16, 13, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_BRICK,          16, 16, 14, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_SOLID,          16, 16, 15, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_BRICK_2,        16, 16, 16, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_QUESTION_1,     16, 16, 17, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_QUESTION_2,     16, 16, 18, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_QUESTION_3,     16, 16, 19, 29, 16, 16)\
    SPRITE(SPRITE_BLOCK_CASTLE_QUESTION_EMPTY, 16, 16, 20, 29, 16, 16)\
    SPRITE(SPRITE_CASTLE_BRICK_PARTICLE_1, 8, 8, 24, 59, 8, 8)\
    SPRITE(SPRITE_CASTLE_BRICK_PARTICLE_2, 8, 8, 25, 59, 8, 8)\
    SPRITE(SPRITE_CASTLE_BRICK_PARTICLE_3, 8, 8, 24, 60, 8, 8)\
    SPRITE(SPRITE_CASTLE_BRICK_PARTICLE_4, 8, 8, 25, 60, 8, 8)\
\
    SPRITE(SPRITE_GOOMBA_1,    16, 16, 0, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_2,    16, 16, 1, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_FLAT, 16, 16, 2, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_UNDERGROUND_1,    16, 16, 3, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_UNDERGROUND_2,    16, 16, 4, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_UNDERGROUND_FLAT, 16, 16, 5, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_CASTLE_1,    16, 16, 6, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_CASTLE_2,    16, 16, 7, 25, 16, 16)\
    SPRITE(SPRITE_GOOMBA_CASTLE_FLAT, 16, 16, 8, 25, 16, 16)\
\
    SPRITE(SPRITE_KOOPA_FLYING_1, 16, 16, 0, 13, 16, 24)\
    SPRITE(SPRITE_KOOPA_FLYING_2, 16, 16, 1, 13, 16, 24)\
    SPRITE(SPRITE_KOOPA_1,        16, 16, 2, 13, 16, 24)\
    SPRITE(SPRITE_KOOPA_2,        16, 16, 3, 13, 16, 24)\
    SPRITE(SPRITE_KOOPA_SHELL_1,  16, 16, 4, 13, 16, 16)\
    SPRITE(SPRITE_KOOPA_SHELL_2,  16, 16, 5, 13, 16, 16)\
\
    SPRITE(SPRITE_RED_KOOPA_FLYING_1, 16, 16, 6,  13, 16, 24)\
    SPRITE(SPRITE_RED_KOOPA_FLYING_2, 16, 16, 7,  13, 16, 24)\
    SPRITE(SPRITE_RED_KOOPA_1,        16, 16, 8,  13, 16, 24)\
    SPRITE(SPRITE_RED_KOOPA_2,        16, 16, 9,  13, 16, 24)\
    SPRITE(SPRITE_RED_KOOPA_SHELL_1,  16, 16, 10, 13, 16, 16)\
    SPRITE(SPRITE_RED_KOOPA_SHELL_2,  16, 16, 11, 13, 16, 16)\
\
    SPRITE(SPRITE_MUSHROOM,    16, 16, 0, 23, 16, 16)\
    SPRITE(SPRITE_FIREPLANT,   16, 16, 0, 26, 16, 16)\
    SPRITE(SPRITE_FIREPLANT_2, 16, 16, 1, 26, 16, 16)\
    SPRITE(SPRITE_FIREPLANT_3, 16, 16, 2, 26, 16, 16)\
    SPRITE(SPRITE_FIREPLANT_4, 16, 16, 3, 26, 16, 16)\
    SPRITE(SPRITE_STAR_1,      16, 16, 0, 27, 16, 16)\
    SPRITE(SPRITE_STAR_2,      16, 16, 1, 27, 16, 16)\
    SPRITE(SPRITE_STAR_3,      16, 16, 2, 27, 16, 16)\
    SPRITE(SPRITE_STAR_4,      16, 16, 3, 27, 16, 16)\
\
    SPRITE(SPRITE_BOWSER_1, 16, 16, 0, 11, 32, 32)\
    SPRITE(SPRITE_BOWSER_2, 16, 16, 2, 11, 32, 32)\
    SPRITE(SPRITE_BOWSER_3, 16, 16, 4, 11, 32, 32)\
    SPRITE(SPRITE_BOWSER_4, 16, 16, 6, 11, 32, 32)\
\
    SPRITE(SPRITE_BOWSER_FIRE_1, 8, 8, 16, 24, 24, 8)\
    SPRITE(SPRITE_BOWSER_FIRE_2, 8, 8, 16, 25, 24, 8)\
\
    SPRITE(SPRITE_CASTLE_LEVER, 16, 16, 5, 30, 16, 16)\
\
    SPRITE(SPRITE_NPC_PEACH, 16, 16, 0, 7, 16, 24)\
\
    SPRITE(SPRITE_COIN_1,      16, 16, 3, 24, 16, 16)\
    SPRITE(SPRITE_COIN_2,      16, 16, 4, 24, 16, 16)\
    SPRITE(SPRITE_COIN_3,      16, 16, 5, 24, 16, 16)\
    SPRITE(SPRITE_COIN_4,      16, 16, 6, 24, 16, 16)\
\
    SPRITE(SPRITE_COIN_ENTITY_1, 16, 16, 0, 24, 16, 16)\
    SPRITE(SPRITE_COIN_ENTITY_2, 16, 16, 1, 24, 16, 16)\
    SPRITE(SPRITE_COIN_ENTITY_3, 16, 16, 2, 24, 16, 16)\
\
    SPRITE(SPRITE_PIRANHA_1,   16, 16, 0, 9, 16, 32)\
    SPRITE(SPRITE_PIRANHA_2,   16, 16, 1, 9, 16, 32)\
\
    SPRITE(SPRITE_CASTLE_1, 16, 16, 22, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_2, 16, 16, 23, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_3, 16, 16, 24, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_4, 16, 16, 25, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_5, 16, 16, 26, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_6, 16, 16, 27, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_7, 16, 16, 28, 31, 16, 16)\
    SPRITE(SPRITE_CASTLE_FLAG, 16, 16, 6, 1, 16, 16)\
\
    SPRITE(SPRITE_MUSHROOM_STEM,    16, 16, 28, 25, 16, 16)\
    SPRITE(SPRITE_MUSHROOM_CAP_L,   16, 16, 29, 25, 16, 16)\
    SPRITE(SPRITE_MUSHROOM_CAP,     16, 16, 30, 25, 16, 16)\
    SPRITE(SPRITE_MUSHROOM_CAP_R,   16, 16, 31, 25, 16, 16)\
\
    SPRITE(SPRITE_PIPE_V_L,       16, 16, 0, 30, 16, 16)\
    SPRITE(SPRITE_PIPE_V_R,       16, 16, 1, 30, 16, 16)\
    SPRITE(SPRITE_PIPE_V_L_ENTRY, 16, 16, 0, 31, 16, 16)\
    SPRITE(SPRITE_PIPE_V_R_ENTRY, 16, 16, 1, 31, 16, 16)\
    SPRITE(SPRITE_PIPE_H_T,       16, 16, 3, 31, 16, 16)\
    SPRITE(SPRITE_PIPE_H_B,       16, 16, 3, 30, 16, 16)\
    SPRITE(SPRITE_PIPE_H_T_ENTRY, 16, 16, 2, 31, 16, 16)\
    SPRITE(SPRITE_PIPE_H_B_ENTRY, 16, 16, 2, 30, 16, 16)\
    SPRITE(SPRITE_PIPE_T_CONNECT, 16, 16, 4, 31, 16, 16)\
    SPRITE(SPRITE_PIPE_B_CONNECT, 16, 16, 4, 30, 16, 16)\
\
    SPRITE(SPRITE_CLOUD_1, 16, 16, 0, 1, 16, 32)\
    SPRITE(SPRITE_CLOUD_2, 16, 16, 1, 1, 16, 32)\
    SPRITE(SPRITE_CLOUD_3, 16, 16, 2, 1, 16, 32)\
\
    SPRITE(SPRITE_BUSH_1, 16, 16, 0, 3, 16, 16)\
    SPRITE(SPRITE_BUSH_2, 16, 16, 1, 3, 16, 16)\
    SPRITE(SPRITE_BUSH_3, 16, 16, 2, 3, 16, 16)\
\
    SPRITE(SPRITE_HILL_1, 16, 16, 0, 0, 16, 16)\
    SPRITE(SPRITE_HILL_2, 16, 16, 1, 0, 16, 16)\
    SPRITE(SPRITE_HILL_3, 16, 16, 2, 0, 16, 16)\
    SPRITE(SPRITE_HILL_4, 16, 16, 3, 0, 16, 16)\
    SPRITE(SPRITE_HILL_5, 16, 16, 4, 0, 16, 16)\
    SPRITE(SPRITE_HILL_6, 16, 16, 5, 0, 16, 16)\
\
    SPRITE(SPRITE_BRIDGE_CHAIN, 16, 16, 3, 2, 16, 16)\
\
    SPRITE(SPRITE_FLAG_POLE,      16, 16, 3, 1, 16, 16)\
    SPRITE(SPRITE_FLAG_POLE_TOP,  16, 16, 4, 1, 16, 16)\
    SPRITE(SPRITE_FLAG_POLE_FLAG, 16, 16, 5, 1, 16, 16)\
\
    SPRITE(SPRITE_LAVA,        16, 16, 7, 0, 16, 16)\
    SPRITE(SPRITE_WATER,       16, 16, 8, 0, 16, 16)\
    SPRITE(SPRITE_WATER_LIGHT, 16, 16, 9, 0, 16, 16)\
    SPRITE(SPRITE_LAVA_WAVE,        16, 16, 7, 1, 16, 16)\
    SPRITE(SPRITE_WATER_WAVE,       16, 16, 8, 1, 16, 16)\
    SPRITE(SPRITE_WATER_LIGHT_WAVE, 16, 16, 9, 1, 16, 16)\
\
    SPRITE(SPRITE_UI_COIN_1, 8, 8, 10, 62, 8, 8)\
    SPRITE(SPRITE_UI_COIN_2, 8, 8, 11, 62, 8, 8)\
    SPRITE(SPRITE_UI_COIN_3, 8, 8, 12, 62, 8, 8)\
    SPRITE(SPRITE_UI_X,      8, 8, 13, 62, 8, 8)\
    SPRITE(SPRITE_UI_SHROOM, 8, 8, 14, 62, 8, 8)\
    SPRITE(SPRITE_UI_MENU_LOGO, 8, 8, 42, 0, 176, 88)\
\
    SPRITE(SPRITE_PLATFORM_PIECE, 8, 8, 0, 57, 8, 8)\
\

#undef SPRITE

#define SPRITE(sprite_name, ...) sprite_name,
enum : int32_t { SPRITES SPRITE__COUNT, SPRITE__INVALID };
#undef SPRITE

#define SPRITE(sprite_name, ...) #sprite_name,
inline const char *sprite_string[] = { SPRITES };
#undef SPRITE

#define SPRITE(sprite_name, ...) if(std::string(sprite_string) == #sprite_name) return sprite_name;
inline int32_t sprite_id_from_string(const char *sprite_string) { SPRITES; return SPRITE__INVALID; }
#undef SPRITE

/* --- Images --- */

#define IMAGE(image_name, filename, data_format, data_type, internal_format)
#define ALL_IMAGES\
    IMAGE(IMAGE_AFI, "afi.jpg", GL_RGB, GL_UNSIGNED_BYTE, GL_RGB)
#undef IMAGE

#define IMAGE(image_name, ...) image_name,
enum : int32_t { ALL_IMAGES IMAGE__COUNT, IMAGE__INVALID };
#undef IMAGE

#define IMAGE(image_name, ...) #image_name,
inline const char *image_string[] = { ALL_IMAGES };
#undef IMAGE

#define IMAGE(image_name, ...) if(std::string(image_string) == #image_name) return image_name;
inline int32_t image_id_from_string(const char *image_string) { ALL_IMAGES; return IMAGE__INVALID; }
#undef IMAGE

/* --- Sounds --- */

#define SOUND(sound_name, filename)
#define ALL_SOUNDS\
    SOUND(SOUND_COIN,          "sounds/coin.wav")\
    SOUND(SOUND_JUMP_1,        "sounds/jump_01.wav")\
    SOUND(SOUND_JUMP_2,        "sounds/jump_02.wav")\
    SOUND(SOUND_POWER_UP,      "sounds/power_up.wav")\
    SOUND(SOUND_PLAYER_HIT,    "sounds/player_hit.wav")\
    SOUND(SOUND_PIPE_ENTER,    "sounds/pipe_enter.wav")\
    SOUND(SOUND_STOMP,         "sounds/stomp.wav")\
    SOUND(SOUND_DESTROY_BLOCK, "sounds/destroy_block.wav")\
    SOUND(SOUND_KILL_ENEMY,    "sounds/kill_enemy.wav")\
    SOUND(SOUND_FIREBALL,      "sounds/fireball_throw.wav")\
    SOUND(SOUND_TILE_DROP,     "sounds/tile_drop.wav")\
    SOUND(SOUND_TILE_HIT,      "sounds/tile_hit.wav")\

#undef SOUND

#define SOUND(sound_name, ...) sound_name,
enum : int32_t { ALL_SOUNDS SOUND__COUNT, SOUND__INVALID };
#undef SOUND

#define SOUND(sound_name, ...) #sound_name,
inline const char *sound_string[] = { ALL_SOUNDS };
#undef SOUND

#define SOUND(sound_name, ...) if(std::string(sound_string) == #sound_name) return sound_name;
inline int32_t sound_id_from_string(const char *sound_string) { ALL_SOUNDS; return SOUND__INVALID; }
#undef SOUND

/* --- Music --- */

#define MUSIC(music_name, filename)
#define ALL_MUSIC\
    MUSIC(MUSIC_NONE, "no path for this")\
    MUSIC(MUSIC_OVERWORLD, "music/overworld.mp3")\
    MUSIC(MUSIC_UNDERGROUND, "music/underground.mp3")\
    MUSIC(MUSIC_UNDERWATER, "music/underwater.mp3")\
    MUSIC(MUSIC_CASTLE, "music/castle.mp3")\
    MUSIC(MUSIC_INVINCIBLE, "music/invincible.mp3")\
    MUSIC(MUSIC_YOU_HAVE_DIED, "music/you_have_died.mp3")\
    MUSIC(MUSIC_LEVEL_COMPLETE, "music/level_complete.mp3")\
    MUSIC(MUSIC_CASTLE_COMPLETE, "music/castle_complete.mp3")\
    MUSIC(MUSIC_SAVED_THE_PRINCESS, "music/saved_the_princess.mp3")
#undef MUSIC

#define MUSIC(music_name, ...) music_name,
enum : int32_t { ALL_MUSIC MUSIC__COUNT, MUSIC__INVALID };
#undef MUSIC

#define MUSIC(music_name, ...) #music_name,
inline const char *music_string[] = { ALL_MUSIC };
#undef MUSIC

#define MUSIC(music_name, ...) if(std::string(music_string) == #music_name) return music_name;
inline int32_t music_id_from_string(const char *music_string) { ALL_MUSIC; return MUSIC__INVALID; }
#undef MUSIC

namespace global_data {
    void init(void);
    void free(void);

    std::string get_data_path(void);
    Texture    *get_spritesheet(void);
    Sprite      get_sprite(int32_t sprite_enum);

    Texture *get_image(int32_t image_enum);
    Sound *get_sound(int32_t sound_enum);
    Music *get_music(int32_t music_enum);

    Font *get_small_font(void);
};

#endif /* _DATA_H */