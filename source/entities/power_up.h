#ifndef _POWER_UP_H
#define _POWER_UP_H

#include "common.h"
#include "maths.h"
#include "level.h"
#include "renderer.h"
#include "save_data.h"

struct PowerUpSpawnAnim {
    bool      finished;
    float32_t y_target;
    float32_t y_current;
};

ENTITY_SERIALIZE_PROC(Mushroom);
ENTITY_DESERIALIZE_PROC(Mushroom);

struct Mushroom : Entity {
    Collider collider;
    MoveData move_data;
    PowerUpSpawnAnim spawn_anim;

    int32_t direction;
    bool change_dir_after_move;
};

Mushroom *spawn_mushroom(Level *level, vec2i position);
Mushroom *spawn_mushroom(Level *level, Tile *tile);

struct FirePlant : Entity {
    Collider collider;
    AnimPlayer anim_player;
    PowerUpSpawnAnim spawn_anim;
};

FirePlant *spawn_fire_plant(Level *level, vec2i position);
FirePlant *spawn_fire_plant(Level *level, Tile *tile);

struct Star : Entity {
    Collider collider;
    MoveData move_data;
    AnimPlayer anim_player;
    PowerUpSpawnAnim spawn_anim;

    int32_t direction;
    bool bounce_after_move;
    bool change_dir_after_move;
};

Star *spawn_star(Level *level, vec2i position);
Star *spawn_star(Level *level, Tile *tile);

#endif /* _POWER_UP_H */