#ifndef _BACKGROUND_H
#define _BACKGROUND_H

#include "common.h"
#include "entity.h"
#include "save_data.h"
#include "renderer.h"

/* --- BackgroundPlane --- */

ENTITY_SERIALIZE_PROC(BackgroundPlane);
ENTITY_DESERIALIZE_PROC(BackgroundPlane);

struct BackgroundPlane : Entity {
    vec2i size;
    vec3  color;
};

BackgroundPlane *spawn_background_plane(Level *level, vec2i position, vec2i size, vec3 color);

/* --- BackgroundSprite --- */

ENTITY_SERIALIZE_PROC(BackgroundSprite);
ENTITY_DESERIALIZE_PROC(BackgroundSprite);

struct BackgroundSprite : Entity {
    int32_t sprite_id;
};

BackgroundSprite *spawn_background_sprite(Level *level, vec2i position, int32_t sprite_id);

/* --- TimedSprite --- */

struct TimedSprite : Entity {
    Sprite sprite;
    int32_t z_position;
    float32_t duration;
    float32_t timer;
};

TimedSprite *spawn_timed_sprite(Level *level, vec2i position, float32_t duration, Sprite sprite);

/* --- TimedAnim --- */

struct TimedAnim : Entity {
    enum Type : int32_t {
        TYPE_TIMER = 1,
        TYPE_LOOPS = 2
    };

    Type       type;
    bool       centered;
    int32_t    z_pos;
    int32_t    loops;        // TYPE_LOOPS
    int32_t    loops_passed; // TYPE_LOOPS
    float32_t  timer;        // TYPE_TIMER
    float32_t  life_time;    // TYPE_TIMER
    AnimPlayer anim_player;  // 'anim_set' needs to be valid until the entity dies
};

TimedAnim *spawn_timed_anim(Level *level, vec2i position, AnimSet *anim_set);
TimedAnim *spawn_timed_anim_loops(Level *level, vec2i position, AnimSet *anim_set, int32_t   loops);
TimedAnim *spawn_timed_anim_timer(Level *level, vec2i position, AnimSet *anim_set, float32_t life_time);

/* --- EnemyFallAnim --- */

struct EnemyFallAnim : Entity {
    Sprite sprite;

    float32_t timer;
    float32_t speed_x;
    float32_t speed_y;
    float32_t offset_x;
    float32_t offset_y;
};

EnemyFallAnim *spawn_enemy_fall_anim(Level *level, vec2i position, int32_t dir, Sprite sprite);

/* --- FloatingText --- */

struct FloatingText : Entity {
    static const int32_t text_max_length = 32;
    bool      render_centered;
    char      text[text_max_length];
    float32_t timer;
};

FloatingText *spawn_floating_text(Level *level, vec2i position, char text[FloatingText::text_max_length]);
FloatingText *spawn_floating_text_number(Level *level, vec2i position, int32_t number);

/* --- Image --- */

ENTITY_SERIALIZE_PROC(BackgroundImage);
ENTITY_DESERIALIZE_PROC(BackgroundImage);

struct BackgroundImage : Entity {
    int32_t image_id;
    float32_t opacity;
};

BackgroundImage *spawn_background_image(Level *level, vec2i position, int32_t image_id);

#endif /* _BACKGROUND_H */