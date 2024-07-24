#ifndef _FIRE_BAR_H
#define _FIRE_BAR_H

#include "common.h"
#include "level.h"
#include "all_entities.h"

ENTITY_SERIALIZE_PROC(FireBar);
ENTITY_DESERIALIZE_PROC(FireBar);

struct FireBar : Entity {
    AnimPlayer anim_player;
    float32_t counter;
    int32_t   fire_num;
    vec2i     fire_size;
    int32_t   angle_idx;
    vec2      angle;
};

FireBar *spawn_fire_bar(Level *level, vec2i position, int32_t fire_num);
vec2i calc_one_fire_position(FireBar *fire_bar, int32_t fire_idx);
vec2  calc_fire_bar_angle(int32_t angle_idx);

constexpr float32_t fire_bar_angles[] = { 
    M_PI * 0.0f,
    M_PI * 0.0625f,
    M_PI * 0.125f,
    M_PI * 0.1875f,
    M_PI * 0.25f,
    M_PI * 0.3125f,
    M_PI * 0.375f,
    M_PI * 0.4375f,
    M_PI * 0.5f,
    M_PI * (0.0625f + 0.5f),
    M_PI * (0.125f + 0.5f),
    M_PI * (0.1875f + 0.5f),
    M_PI * (0.25f + 0.5f),
    M_PI * (0.3125f + 0.5f),
    M_PI * (0.375f + 0.5f),
    M_PI * (0.4375f + 0.5f),
    M_PI * (0.5f + 0.5f),
    M_PI * (0.0625f + 1.0f),
    M_PI * (0.125f + 1.0f),
    M_PI * (0.1875f + 1.0f),
    M_PI * (0.25f + 1.0f),
    M_PI * (0.3125f + 1.0f),
    M_PI * (0.375f + 1.0f),
    M_PI * (0.4375f + 1.0f),
    M_PI * (0.5f + 1.0f),
    M_PI * (0.0625f + 1.5f),
    M_PI * (0.125f + 1.5f),
    M_PI * (0.1875f + 1.5f),
    M_PI * (0.25f + 1.5f),
    M_PI * (0.3125f + 1.5f),
    M_PI * (0.375f + 1.5f),
    M_PI * (0.4375f + 1.5f),
};

#endif /* _FIRE_BAR_H */