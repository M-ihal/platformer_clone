#ifndef _BOWSER_H
#define _BOWSER_H

#include "common.h"
#include "maths.h"
#include "level.h"
#include "save_data.h"
#include "renderer.h"

inline constexpr int32_t bowser_move_width = 16;

ENTITY_SERIALIZE_PROC(Bowser);
ENTITY_DESERIALIZE_PROC(Bowser);

struct Bowser : Entity {
    Collider collider;
    MoveData move_data;
    AnimPlayer anim_player;

    int32_t health_points;
    int32_t max_moves_in_any_direction;
    int32_t vision_range;
    bool    saw_player;

    int32_t   move_dirs_combined;
    int32_t   move_dir;
    int32_t   position_x_target;
    float32_t wait_t;

    int32_t facing_dir;
    int32_t last_move_dir_on_ground;

    float32_t breath_t;
    float32_t after_breath_t;
};

Bowser *spawn_bowser(Level *level, vec2i position);

void hit_bowser(Bowser *bowser, int32_t fall_dir);
void kill_bowser(Bowser *bowser, int32_t fall_dir);

struct BowserFire : Entity {
    Collider collider;
    MoveData move_data;
    AnimPlayer anim_player;
    int32_t y_target;
};

BowserFire *spawn_bowser_fire(Level *level, vec2i position);
BowserFire *spawn_bowser_fire(Level *level, Bowser *bowser, int32_t y_move = 0 /* (-1, 0, 1) Should the fire go TILE_SIZE up or down */);

void _render_debug_bowser_info(Bowser *bowser);

#endif /* _BOWSER_H */