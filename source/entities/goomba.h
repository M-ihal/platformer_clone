#ifndef _GOOMBA_H
#define _GOOMBA_H

#include "common.h"
#include "entity.h"
#include "level.h"
#include "renderer.h"
#include "save_data.h"

enum EGoombaType {
    GOOMBA_TYPE_OVERWORLD,
    GOOMBA_TYPE_UNDERGROUND,
    GOOMBA_TYPE_CASTLE,
    GOOMBA_TYPE__COUNT
};

inline const char *goomba_type_cstr[] = {
    "Overworld",
    "Underground",
    "Castle"
};

static_assert(GOOMBA_TYPE__COUNT == array_count(goomba_type_cstr));

ENTITY_SERIALIZE_PROC(Goomba);
ENTITY_DESERIALIZE_PROC(Goomba);

struct Goomba : Entity {
    EGoombaType goomba_type;
    Collider    collider;
    MoveData    move_data;
    AnimPlayer  anim_player;
    int32_t     move_dir;
    bool        change_move_dir_next_frame;
};

Goomba *spawn_goomba(Level *level, vec2i position, EGoombaType goomba_type = GOOMBA_TYPE_OVERWORLD);
void kill_goomba(Goomba *goomba, bool by_stomping, int32_t fall_dir = 0 /* matters if !by_stomping */);

#endif /* _GOOMBA_H */