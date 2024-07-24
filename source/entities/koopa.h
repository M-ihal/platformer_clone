#ifndef _KOOPA_H
#define _KOOPA_H

#include "common.h"
#include "maths.h"
#include "level.h"
#include "renderer.h"
#include "all_entities.h"

enum EKoopaType {
    KOOPA_TYPE_GREEN = 0,
    KOOPA_TYPE_RED,
    KOOPA_TYPE_GREEN_FLYING,
    KOOPA_TYPE_RED_FLYING,
    KOOPA_TYPE__COUNT
};

inline const char *koopa_type_cstr[] = {
    "Green",
    "Red",
    "Flying Green",
    "Flying Red"
};

static_assert(KOOPA_TYPE__COUNT == array_count(koopa_type_cstr));

ENTITY_SERIALIZE_PROC(Koopa);
ENTITY_DESERIALIZE_PROC(Koopa);

struct Koopa : Entity {
    EKoopaType koopa_type;
    Collider collider;
    MoveData move_data;
    int32_t  move_dir;
    AnimPlayer anim_player;
    bool change_move_dir_next_frame;
    bool bounce_next_frame;
};

Koopa *spawn_koopa(Level *level, vec2i position, EKoopaType koopa_type = EKoopaType::KOOPA_TYPE_GREEN);
void kill_koopa(Koopa *koopa, int32_t fall_dir);
void hit_koopa(Koopa *koopa);

struct KoopaShell : Entity {
    EKoopaType koopa_type;
    Collider collider;
    MoveData move_data;
    int32_t  move_dir;
    AnimPlayer anim_player;
    bool change_move_dir_next_frame;
};

KoopaShell *spawn_koopa_shell(Koopa *koopa, EKoopaType koopa_type);
void kill_koopa_shell(KoopaShell *shell, int32_t fall_dir);
void push_koopa_shell(KoopaShell *shell, int32_t dir /* -1, 1, or 0 for stopping */);

#endif /* _KOOPA_H */