#ifndef _PIRANHA_H
#define _PIRANHA_H

#include "common.h"
#include "maths.h"
#include "level.h"
#include "renderer.h"
#include "save_data.h"

enum EPiranhaState {
    PIRANHA_HIDES,
    PIRANHA_MOVES_UP,
    PIRANHA_WAITS,
    PIRANHA_MOVES_DOWN,
    PIRANHA_STATE__COUNT
};

ENTITY_SERIALIZE_PROC(Piranha);
ENTITY_DESERIALIZE_PROC(Piranha);

struct Piranha : Entity {
    Collider collider;
    AnimPlayer anim_player;

    EPiranhaState state;
    float32_t move_t;
    float32_t wait_t;

    int32_t offset; // in pixels
};

Piranha *spawn_piranha(Level *level, vec2i position);
void kill_piranha(Piranha *piranha, int32_t fall_dir);

#endif /* _PIRANHA_H */