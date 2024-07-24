#ifndef _PLAYER_H
#define _PLAYER_H

#include "common.h"
#include "maths.h"
#include "level.h"
#include "renderer.h"
#include "save_data.h"
#include "level_entities.h" // for PortalInfo

enum EPlayerState {
    PLAYER_STATE_NORMAL = 0,
    PLAYER_STATE_POWERING_UP,
    PLAYER_STATE_ENTERING_TP,
    PLAYER_STATE_LEAVING_TP,
    PLAYER_STATE_ON_FLAG_POLE,        // Slide down the flag pole until any tile hit
    PLAYER_STATE_GET_OFF_FLAG_POLE,   // Wait a bit on flag pole
    PLAYER_STATE_FINISH_THE_LEVEL,    // Go right until any tile hit, and then finish the level
    PLAYER_STATE_DEADGE,
    PLAYER_STATE_ENTER_PIPE_CUTSCENE, // "Cutscene" where player enters pipe to start a level
    PLAYER_STATE__COUNT,
    PLAYER_STATE__INVALID
};

inline const char *player_state_string[] = {
    "player_state_normal",
    "player_state_powering_up",
    "player_state_entering_tp",
    "player_state_leaving_tp",
    "player_state_on_flag_pole",
    "player_state_get_off_flag_pole",
    "player_state_finish_the_level",
    "player_state_deadge",
    "player_enter_pipe_cutscene"
};

enum EPlayerMode {
    PLAYER_IS_SMALL = 0,
    PLAYER_IS_BIG   = 1,
    PLAYER_IS_FIRE  = 2,
    PLAYER_MODE__COUNT
};

inline const char *player_mode_string[] = {
    "player_is_small",
    "player_is_big",
    "player_is_fire"
};

enum EPlayerVisualState {
    P_VIS_IDLE = 0,
    P_VIS_RUN,
    P_VIS_JUMP,
    P_VIS_CROUTCH,
    P_VIS_TURN,
    P_VIS__COUNT,
    P_VIS__INVALID
};

static_assert(PLAYER_STATE__COUNT == array_count(player_state_string) && PLAYER_MODE__COUNT == array_count(player_mode_string));

namespace player_consts {
    constexpr float32_t sliding_flag_pole_speed = 50.0f;
};

ENTITY_SERIALIZE_PROC(Player);
ENTITY_DESERIALIZE_PROC(Player);

struct Player : Entity {
    MoveData move_data;
    Collider collider_small;
    Collider collider_big;

    EPlayerState next_state;
    EPlayerState state;
    EPlayerMode  mode;
    
    EPlayerVisualState vis_state;
    AnimPlayer anim_player;

    int32_t last_dir_on_ground;
    bool    is_croutching;

    // Jump stuff
    bool jump_input_last_frame;
    float32_t jump_t;
    bool bounce_up_after_stomp;

    // Star powerup
    bool has_star_powerup;
    float32_t star_powerup_t;

    // Invincibility after getting hit
    bool      in_invicible_state;
    float32_t invicible_state_t;

    // Loaded from level file
    bool player_starts_by_entering_pipe_cutscene;

    // vis_* prename?
    /* Visuals for the frame (set in update_player) */
    Sprite  sprite;
    vec4    color;
    int32_t face_direction;
    vec2i   draw_offset;
    int32_t draw_z_pos;

    /* PLAYER_STATE_POWERING_UP */
    float32_t powerup_anim_t;
    EPlayerMode powerup_from_mode;

    /* PLAYER_STATE_*_TP */
    PortalInfo tp_info;
    float32_t  tp_timer;
    float32_t  tp_dist;

    /* PLAYER_STATE_GET_OFF_FLAG_POLE */
    float32_t fp_rest_timer;

    /* PLATER_STATE_DEADGE */
    bool      dead_anim_wait;
    float32_t dead_anim_timer;
    bool      wait_after_death;
    float32_t wait_after_death_timer;
};

Player *spawn_player(Level *level, vec2i position);
void hit_player(Player *player);
void kill_player(Player *player);
void set_player_mode(Player *player, EPlayerMode new_mode);
void apply_star_powerup(Player *player);

inline Player *get_player(Level *level) {
    assert(level->entities.count_of_type[entity_type_id(Player)] <= 1);
    return level->entities.first_of_type[entity_type_id(Player)]->as<Player>();
}

struct Fireball : Entity {
    MoveData   move_data;
    Collider   collider;
    AnimPlayer anim_player;
    int32_t    bounce_count;
    int32_t    move_dir;
    bool       delete_self;
    bool       change_move_dir_next_frame;
    bool       bounce_next_frame;
};

Fireball *throw_fireball(Player *player);
Fireball *throw_fireball(Level *level, vec2i position, int32_t direction);

#endif /* _PLAYER_H */