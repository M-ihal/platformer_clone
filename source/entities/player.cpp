#include "player.h"
#include "data.h"
#include "all_entities.h"

#define CHEATOS aaaaa

namespace {
    namespace consts {
        constexpr float32_t gravity               = 1410.0f;
        constexpr float32_t acceleration          = 300.0f;
        constexpr float32_t acceleration_air      = 140.0f;
        constexpr float32_t friction              = 430.0f;
        constexpr float32_t friction_air          = 20.0f;
        constexpr float32_t max_speed             = 100.0f;
        constexpr float32_t max_speed_run         = 180.0f;
        constexpr float32_t max_fall_speed        = 340.0f;
        constexpr float32_t jump_power            = 380.0f;
        constexpr float32_t jump_time             = 0.14f;
        constexpr float32_t jump_time_min         = 0.05f;
        constexpr float32_t stomp_jump_power      = 180.0f;
        constexpr float32_t powerup_anim_time     = 1.0f;
        constexpr float32_t star_time             = 10.0f;
        constexpr float32_t tp_time               = 1.0f;
        constexpr float32_t fp_rest_time          = 1.0f;
        constexpr float32_t dead_anim_wait_time   = 0.2f;
        constexpr float32_t dead_anim_time        = 2.0f;
        constexpr float32_t dead_anim_dist        = 48.0f;
        constexpr float32_t wait_after_death_time = 1.5f;
        constexpr float32_t invincible_time       = 1.5f;
        constexpr int32_t   max_fireball_num      = 3;
    };

    AnimSet anim_sets[PLAYER_MODE__COUNT][P_VIS__COUNT];
    AnimSet fireball_anim_set;
    AnimSet fireball_explosion_anim_set;
};

void set_player_mode(Player *player, EPlayerMode new_mode) {
    EPlayerMode old_mode = player->mode;
    
    switch(new_mode) {
        default: assert(0); return;

        case PLAYER_IS_SMALL: {
            player->has_collider = &player->collider_small;
        } break;

        case PLAYER_IS_FIRE: { } // go through

        case PLAYER_IS_BIG: {
            player->has_collider = &player->collider_big;
        } break;
    }
    player->mode = new_mode;

    set_anim(&player->anim_player, &anim_sets[player->mode][player->vis_state]);
}

void apply_star_powerup(Player *player) {
    player->has_star_powerup = true;
    player->star_powerup_t = 0.0f;

    set_level_music_and_play(player->level, ELevelMusic::during_star_power);
}

ENTITY_SERIALIZE_PROC(Player) {
    auto player = self_base->as<Player>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", player->position.e, 2);
    es_data.add_bool("player_starts_by_entering_pipe_cutscene", &player->player_starts_by_entering_pipe_cutscene, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Player) {
    vec2i position;

    if(!es_data->try_get_int32("position", position.e, 2)) {
        return NULL;
    }

    bool player_starts_by_entering_pipe_cutscene = false;
    es_data->try_get_bool("player_starts_by_entering_pipe_cutscene", &player_starts_by_entering_pipe_cutscene, 1);

    auto player = spawn_player(level, position);
    player->player_starts_by_entering_pipe_cutscene = player_starts_by_entering_pipe_cutscene;
    return player;
}

ENTITY_UPDATE_PROC(update_player);
ENTITY_RENDER_PROC(render_player);
HIT_CALLBACK_PROC(player_hit_callback);

Player *spawn_player(Level *level, vec2i position) {
    assert(level->entities.first_of_type[entity_type_id(Player)] == NULL, "Only one player allowed");

    auto player = create_entity_m(level, Player);
    player->position = position;
    player->update_proc = update_player;
    player->render_proc = render_player;

    set_entity_flag(player, E_FLAG_IS_GAMEPLAY_ENTITY);

    player->collider_small.offset = { 1, 1 };
    player->collider_small.size   = { TILE_SIZE - 2, TILE_SIZE - 2 };

    player->collider_big.offset = { 1, 1 };
    player->collider_big.size = { TILE_SIZE - 2, TILE_SIZE * 2 - 2 };

    player->has_move_data = &player->move_data;
    player->move_data.hit_callback = player_hit_callback;

    set_player_mode(player, PLAYER_IS_SMALL);
    player->state = PLAYER_STATE_NORMAL;
    player->last_dir_on_ground = 1;

    player->vis_state = P_VIS_IDLE;
    player->anim_player = init_anim_player(&anim_sets[player->mode][player->vis_state]);

    player->sprite = { };
    player->color = color::white;
    player->face_direction = 1;
    player->draw_z_pos  = Z_PLAYER_POS;
    player->draw_offset = { 0, 0 };

    player->player_starts_by_entering_pipe_cutscene = false;

    player->dead_anim_wait         = false;
    player->dead_anim_timer        = 0.0f;
    player->wait_after_death       = false;
    player->wait_after_death_timer = 0.0f;

    return player;
}

static void
update_player_move_data(MoveData *move_data, int32_t move_dir, bool running, float32_t delta_time) {
    assert(move_dir == 0 || move_dir == -1 || move_dir == 1);

    if(move_dir != 0) {
        float32_t acc = move_data->is_grounded ? consts::acceleration : consts::acceleration_air;
        float32_t speed = running ? consts::max_speed_run : consts::max_speed;
        move_data->speed.x = move_toward(move_data->speed.x, speed * move_dir, acc * delta_time);
    } else {
        const float32_t friction = move_data->is_grounded ? consts::friction : consts::friction_air;
        move_data->speed.x = move_toward(move_data->speed.x, 0.0f, friction * delta_time);
    }
}

static inline bool
can_be_hit(Player *player) {
    return player->in_invicible_state == false && player->state == PLAYER_STATE_NORMAL && player->has_star_powerup == false;
}

static void
set_player_vis_state_anim(Player *player) {
    set_anim(&player->anim_player, &anim_sets[player->mode][player->vis_state]);
}

static void
update_player_vis_state_and_set_anim(Player *player, int32_t move_dir) {
    EPlayerVisualState vis_state_old = player->vis_state;

    if(player->is_croutching) {
        player->vis_state = P_VIS_CROUTCH;
    } else if(!player->move_data.is_grounded) {
        player->vis_state = P_VIS_JUMP;
    } else if((player->last_dir_on_ground * sign(player->move_data.speed.x)) == -1) {
        player->vis_state = P_VIS_TURN;
    } else if(move_dir != 0) {
        player->vis_state = P_VIS_RUN;
    } else {
        player->vis_state = P_VIS_IDLE;
    }
    
    if(vis_state_old != player->vis_state) {
        set_player_vis_state_anim(player);
    }
}

ENTITY_UPDATE_PROC(update_player) {
    auto player = self_base->as<Player>();
    auto level  = player->level;
    
    // Set correct state for levels where player enters a pipe on start
    if(player->player_starts_by_entering_pipe_cutscene) {
        player->player_starts_by_entering_pipe_cutscene = false;
        player->next_state = PLAYER_STATE_ENTER_PIPE_CUTSCENE;

        // Set music for custscene
        set_level_music_and_play(level, ELevelMusic::during_player_enter_pipe_anim);
    }

    // Change state
    if(player->next_state != PLAYER_STATE__INVALID) {
        player->state = player->next_state;
        player->next_state = PLAYER_STATE__INVALID;
    }

    // Trigger triggers
    for_entity_type(level, Trigger, trigger) {
        if(intersect(player, trigger)) {
            trigger_the_trigger(trigger);
        }
    }

    switch(player->state) {
        default: { assert(0, "Undefined player state."); } break;

        case PLAYER_STATE_NORMAL: {
            player->draw_z_pos = Z_PLAYER_POS;

            // Invincibility update
            if(player->in_invicible_state) {
                player->invicible_state_t += delta_time;
                if(player->invicible_state_t >= consts::invincible_time) {
                    player->invicible_state_t = 0.0f;
                    player->in_invicible_state = false;
                }
            }
            
            // Try entering a portal
            for_entity_type(level, Portal, portal) {
                if(aabb(player->position + player->has_collider->offset, player->has_collider->size, portal->get_enter_pos(), portal->get_enter_size())) {
                    const bool enter_vertical   =  portal->info.vertical_enter && input.player_try_entering_pipe_v;
                    const bool enter_horizontal = !portal->info.vertical_enter && input.player_try_entering_pipe_h;
                    if(enter_vertical || enter_horizontal) {
                        player->tp_timer = 0.0f;
                        player->tp_dist = TILE_SIZE;
                        player->tp_info = portal->info;
                        player->next_state = PLAYER_STATE_ENTERING_TP; // @state_change
                        pause_level(level, player);
                        audio_player::a_play_sound(global_data::get_sound(SOUND_PIPE_ENTER));
                        if(enter_vertical) {
                            if(player->mode != PLAYER_IS_SMALL) {
                                player->vis_state = P_VIS_CROUTCH;
                                set_anim(&player->anim_player, &anim_sets[player->mode][player->vis_state]);
                                player->sprite = get_current_frame(&player->anim_player)->sprite;
                            }
                        }
                        return;
                    }
                }
            }

            // Maybe finish level?
            for_entity_type(level, FlagPole, flag_pole) {
                if(intersect(player, flag_pole)) {
                    const int32_t flag_bot = flag_pole->position.y + flag_pole->collider.offset.y;
                    const int32_t player_bot = player->position.y + player->has_collider->offset.y;
                    const int32_t flag_pole_tile_hit = (player_bot - flag_bot) / TILE_SIZE + 1;

                    const int32_t points_per_flag_pole_tile = POINTS_PER_FLAG_POLE_TILE;
                    const int32_t points_earned = flag_pole_tile_hit * points_per_flag_pole_tile;

                    spawn_floating_text_number(level, flag_pole->position + vec2i{ +16, flag_pole_tile_hit * TILE_SIZE }, points_earned);
                    move_the_flag_down(flag_pole);

                    player->is_croutching = false;
                    player->position.x = flag_pole->position.x + flag_pole->has_collider->offset.x + flag_pole->has_collider->size.x / 2 - player->has_collider->size.x / 2;
                    player->position.x -= 6;
                    player->next_state = PLAYER_STATE_ON_FLAG_POLE; // @state_change
                    level->update_level_time = false; // Stop updating level time

                    set_level_music_and_play(level, ELevelMusic::on_level_completed);
                    return;
                }
            }

            // Maybe finish level with lever
            for_entity_type(level, CastleLever, lever) {
                if(intersect(player, lever)) {
                    player->is_croutching = false;
                    player->next_state = PLAYER_STATE_FINISH_THE_LEVEL;
                    zero_move_speed(&player->move_data);
                    level->update_level_time = false; // Stop updating level time
                    for_entity_type(level, Bowser, bowser) { kill_bowser(bowser, 1); }
                    delete_entity(lever);
                    set_level_music_and_play(level, ELevelMusic::on_level_completed);
                    return;
                }
            }

            // Update star powerup state
            if(player->has_star_powerup) {
                player->star_powerup_t += delta_time;
                if(player->star_powerup_t >= consts::star_time) {
                    player->has_star_powerup = false;

                    set_level_music_and_play(player->level, ELevelMusic::regular);
                }
            }

            auto move_data      = &player->move_data;
            const bool move_l   = input.player_move_l;
            const bool move_r   = input.player_move_r;
            const bool running  = input.player_run;
            const bool jump     = input.player_jump;
            const bool croutch  = input.player_croutch;

            // Set croutch state @cleanup @todo
            if(player->mode != PLAYER_IS_SMALL) {
                if(croutch) {
                    player->has_collider = &player->collider_small;
                    player->is_croutching = true;
                } else {
                    bool can_be_uncroutched = true;
                    for_entity_type(level, Tilemap, tilemap) {
                        FindTileCollisionOpts opts = { };
                        opts.tile_flags_required = TILE_FLAG_BLOCKS_MOVEMENT;
                        if(find_collisions(player->position, &player->collider_big, tilemap, {}, opts).size() != 0) {
                            can_be_uncroutched = false;
                            break;
                        }
                    }
                    if(can_be_uncroutched) {
                        player->has_collider = &player->collider_big;
                        player->is_croutching = false;
                    } else {
                        player->has_collider = &player->collider_small;
                        player->is_croutching = true;
                    }
                }
            }

            int32_t move_dir = 0;

            if(!(player->is_croutching && player->move_data.is_grounded)) {
                if(move_l) { move_dir -= 1; }
                if(move_r) { move_dir += 1; }
            }

            if(move_dir != 0 && move_data->is_grounded) {
                player->last_dir_on_ground = move_dir;
            }

            // Update x speed
            update_player_move_data(&player->move_data, move_dir, running, delta_time);

            const bool grounded_past_2_frames = move_data->is_grounded || (!move_data->is_grounded && move_data->is_grounded_prev);
            if(jump && grounded_past_2_frames && !player->jump_input_last_frame) {
                player->jump_t = consts::jump_time;

                // Play jump sound effect
                audio_player::a_play_sound(global_data::get_sound(SOUND_JUMP_1));
            }

            move_data->speed.y -= consts::gravity * delta_time;

            // Jump timer
            if(player->jump_t > 0.0f) {
                move_data->speed.y = sqrtf(((consts::jump_time - player->jump_t) / consts::jump_time) + 0.01) * consts::jump_power;
                player->jump_t -= delta_time;
                if(!jump) {
                    if(player->jump_t <= (consts::jump_time - consts::jump_time_min)) {
                        player->jump_t = 0.0f;
                    }
                }
            } else {
                player->jump_t = 0.0f;
            }

            clamp_min(&move_data->speed.y, -consts::max_fall_speed);
            player->jump_input_last_frame = input.player_jump;

            do_move(player, move_data, delta_time);

            if(player->bounce_up_after_stomp) {
                player->move_data.speed.y = consts::stomp_jump_power;
                player->bounce_up_after_stomp = false;
            }

            const EPlayerMode prev_player_mode = player->mode;
                  EPlayerMode next_player_mode = player->mode;

            // Collect power-ups
            auto power_ups = std::vector<Entity *>();
            gather_collisions(&power_ups, find_collisions(level, player->position, player->has_collider, { entity_type_id(Mushroom),  0, 0 }));
            gather_collisions(&power_ups, find_collisions(level, player->position, player->has_collider, { entity_type_id(FirePlant), 0, 0 }));
            gather_collisions(&power_ups, find_collisions(level, player->position, player->has_collider, { entity_type_id(Star),      0, 0 }));
            for(Entity *e : power_ups) {
                if(player->mode == PLAYER_IS_SMALL && e->entity_type_id == entity_type_id(Mushroom)) {
                    next_player_mode = PLAYER_IS_BIG;
                } else if(player->mode != PLAYER_IS_FIRE && e->entity_type_id == entity_type_id(FirePlant)) {
                    next_player_mode = PLAYER_IS_FIRE;
                } else if(e->entity_type_id == entity_type_id(Star)) {
                    apply_star_powerup(player);
                    audio_player::a_play_sound(global_data::get_sound(SOUND_POWER_UP));
                } else {
                    add_points_with_text_above_entity(level, POINTS_FOR_POWERUP, e);
                    audio_player::a_play_sound(global_data::get_sound(SOUND_COIN));
                }
                delete_entity(e);
            }

            if(prev_player_mode != next_player_mode) {
                audio_player::a_play_sound(global_data::get_sound(SOUND_POWER_UP));

                player->powerup_from_mode = prev_player_mode;
                player->mode              = next_player_mode;
                player->powerup_anim_t    = 0.0f;
                player->next_state        = PLAYER_STATE_POWERING_UP; // @state_change
                pause_level(level, player);
                return;
            }

            if(player->mode == PLAYER_IS_FIRE && input.player_throw) {
                const int32_t fireball_num = level->entities.count_of_type[entity_type_id(Fireball)];
                if(fireball_num < consts::max_fireball_num) {
                    throw_fireball(player);
                    audio_player::a_play_sound(global_data::get_sound(SOUND_FIREBALL));
                }
            }

            update_player_vis_state_and_set_anim(player, move_dir);

            // anim speed up when running
            float32_t anim_speed_mult = clamp_min(absolute_value(player->move_data.speed.x) / consts::max_speed, 1.0f) * 0.8f;
            update_anim(&player->anim_player, delta_time * anim_speed_mult);

            auto frame = get_current_frame(&player->anim_player);
            player->sprite = frame->sprite;

            player->face_direction = player->last_dir_on_ground;
        } break;

        case PLAYER_STATE_POWERING_UP: {
            // Do not croutch when powering up
            if(player->is_croutching) {
                player->is_croutching = false;
                update_player_vis_state_and_set_anim(player, player->last_dir_on_ground);
            }

            if((player->powerup_anim_t += delta_time) >= consts::powerup_anim_time) {
                unpause_level(level);
                player->next_state = PLAYER_STATE_NORMAL; // @state_change
                set_anim(&player->anim_player, &anim_sets[player->mode][player->vis_state]); // reset
                return;
            }

            float32_t s = sinf(player->powerup_anim_t * M_PI * 8.0f);
            EPlayerMode _mode = s > 0.0f ? player->powerup_from_mode : player->mode;
            player->sprite = anim_sets[_mode][player->vis_state].frames[0].sprite;
        } break;

        case PLAYER_STATE_ENTERING_TP: {
            player->draw_z_pos = Z_TILEMAP_POS + 1;

            // Set sprites for entering and leaving portals
            if(player->tp_info.vertical_enter == true) {
                player->vis_state = player->mode != PLAYER_IS_SMALL ? P_VIS_CROUTCH : P_VIS_IDLE;
                set_player_vis_state_anim(player);
                player->sprite = get_current_frame(&player->anim_player)->sprite;
            }

            player->tp_timer += delta_time;
            if(player->tp_timer >= consts::tp_time) {
                player->position = player->tp_info.dest;
                player->position.y -= player->has_collider->offset.y;
                if(player->tp_info.do_exit_anim && !player->tp_info.vertical_exit) {
                    player->position.x -= player->has_collider->offset.x;
                }

                player->tp_timer = consts::tp_time;
                if(player->tp_info.vertical_exit) {
                    player->draw_offset.x = 0;
                    player->draw_offset.y = -player->tp_dist;
                } else {
                    player->draw_offset.y = 0;
                    player->draw_offset.x = -player->tp_dist;
                }

                // Set leaving tp anim
                if(player->tp_info.vertical_exit == true) {
                    player->vis_state = player->mode == PLAYER_IS_SMALL ? P_VIS_IDLE : P_VIS_CROUTCH;
                    set_player_vis_state_anim(player);
                    player->sprite = get_current_frame(&player->anim_player)->sprite;
                }

                player->next_state = PLAYER_STATE_LEAVING_TP; // @state_change
                return;
            }

            if(player->tp_info.vertical_enter) {
                player->draw_offset.y = (player->tp_timer / consts::tp_time) * -player->tp_dist;
            } else {
                player->draw_offset.x = (player->tp_timer / consts::tp_time) *  player->tp_dist;
            }
        } break;

        case PLAYER_STATE_LEAVING_TP: {
            player->draw_z_pos = Z_TILEMAP_POS + 1;

            auto return_state_to_normal = [] (Player *player) {
                zero_move_speed(&player->move_data);
                player->draw_offset = { };
                player->next_state  = PLAYER_STATE_NORMAL;
            };

            if(!player->tp_info.do_exit_anim) {
                return_state_to_normal(player);
                unpause_level(level);
                break;
            }

            player->tp_timer -= delta_time;
            if(player->tp_timer <= 0.0f) {
                return_state_to_normal(player);
                unpause_level(level);
                return;
            }

            if(player->tp_info.vertical_exit) {
                player->draw_offset.y = (player->tp_timer / consts::tp_time) * -player->tp_dist;
            } else {
                player->draw_offset.x = (player->tp_timer / consts::tp_time) * -player->tp_dist;
            }
        } break;

        case PLAYER_STATE_ON_FLAG_POLE: {
            player->move_data.speed.x = 0.0f;
            player->move_data.speed.y = -player_consts::sliding_flag_pole_speed;
            do_move(player, &player->move_data, delta_time);

            switch(player->mode) {
                case PLAYER_IS_SMALL: player->sprite = global_data::get_sprite(SPRITE_MARIO_CLIMB_1);      break;
                case PLAYER_IS_BIG:   player->sprite = global_data::get_sprite(SPRITE_MARIO_BIG_CLIMB_1);  break;
                case PLAYER_IS_FIRE:  player->sprite = global_data::get_sprite(SPRITE_MARIO_FIRE_CLIMB_1); break;
            }

            player->face_direction = 1;
            player->color          = color::white;
            player->draw_offset    = { };
            player->draw_z_pos     = Z_PLAYER_POS;

            // @check
            if(player->move_data.is_grounded) {
                player->position.x += 10; // Offset position to be to the right of the pole
                player->move_data = { };
                player->fp_rest_timer = 0.0f;
                player->next_state = PLAYER_STATE_GET_OFF_FLAG_POLE; // @state_change
                player->face_direction = -1;
                return;
            }
        } break;

        case PLAYER_STATE_GET_OFF_FLAG_POLE: {
            switch(player->mode) {
                case PLAYER_IS_SMALL: player->sprite = global_data::get_sprite(SPRITE_MARIO_CLIMB_1);      break;
                case PLAYER_IS_BIG:   player->sprite = global_data::get_sprite(SPRITE_MARIO_BIG_CLIMB_1);  break;
                case PLAYER_IS_FIRE:  player->sprite = global_data::get_sprite(SPRITE_MARIO_FIRE_CLIMB_1); break;
            }

            player->face_direction = -1;
            player->color          = color::white;
            player->draw_offset    = { };
            player->draw_z_pos     = Z_PLAYER_POS;

            if((player->fp_rest_timer += delta_time) >= consts::fp_rest_time) {
                zero_move_speed(&player->move_data);
                player->next_state = PLAYER_STATE_FINISH_THE_LEVEL; // @state_change
                return;
            }
        } break;

        case PLAYER_STATE_FINISH_THE_LEVEL: {
            int32_t move_dir = 0;
            update_player_move_data(&player->move_data, move_dir = 1, false, delta_time);
            player->last_dir_on_ground = move_dir;
            player->move_data.speed.y -= consts::gravity * delta_time;
            do_move(player, &player->move_data, delta_time);

            // Check if will collide with any block
            FindTileCollisionOpts opts;
            opts.tile_flags_required = TILE_FLAG_BLOCKS_MOVEMENT;
            bool collided_with_tile = false;
            for_entity_type(level, Tilemap, tilemap) {
                if(is_colliding_with_any_tile(player->position, player->has_collider, tilemap, { 1, 0 }, opts)) {
                    collided_with_tile = true;
                    break;
                }
            }

            // If collided with any tile (should be invisible tile), the level has been completed
            if(collided_with_tile) {
                player->vis_state = P_VIS_IDLE;
                set_anim(&player->anim_player, &anim_sets[player->mode][player->vis_state]);
                player->sprite = get_current_frame(&player->anim_player)->sprite;
                level->player_has_reached_the_end = true;
                player->draw_z_pos = 9;
                return;
            }

            update_player_vis_state_and_set_anim(player, move_dir);
            update_anim(&player->anim_player, delta_time);

            auto frame = get_current_frame(&player->anim_player);
            player->sprite = frame->sprite;
            player->face_direction = player->last_dir_on_ground;
        } break;

        case PLAYER_STATE_DEADGE: {
            player->sprite = global_data::get_sprite(SPRITE_MARIO_GAME_OVER);
            player->draw_z_pos = -8;

            if(player->dead_anim_wait) {
                if((player->dead_anim_timer += delta_time) >= consts::dead_anim_wait_time) {
                    player->dead_anim_wait = false;
                    player->dead_anim_timer = 0.0f;
                }
            } else {
                if(player->wait_after_death == false) {
                    player->dead_anim_timer += delta_time;
                    if(player->dead_anim_timer >= consts::dead_anim_time) {
                        player->wait_after_death       = true;
                        player->wait_after_death_timer = 0.0f;
                    }
                } else {
                    player->wait_after_death_timer += delta_time;
                    if(player->wait_after_death_timer >= consts::wait_after_death_time) {
                        player->level->player_has_died = true;
                        unpause_level(level);
                        return;
                    }
                }

                const float32_t dead_anim_perc = player->dead_anim_timer / consts::dead_anim_time;
                player->draw_offset.y = sinf(dead_anim_perc * M_PI * 1.5f) * consts::dead_anim_dist;
            }
        } break;

        case PLAYER_STATE_ENTER_PIPE_CUTSCENE: {
            pause_level(level, player);

            int32_t move_dir = 0;
            update_player_move_data(&player->move_data, move_dir = 1, false, delta_time);
            player->move_data.speed.y -= consts::gravity * delta_time;
            do_move(player, &player->move_data, delta_time);

            // Try entering a portal @copy
            for_entity_type(level, Portal, portal) {
                if(aabb(player->position + player->has_collider->offset, player->has_collider->size, portal->get_enter_pos(), portal->get_enter_size())) {
                    const bool enter_horizontal = !portal->info.vertical_enter;
                    if(enter_horizontal) {
                        player->tp_timer = 0.0f;
                        player->tp_dist = TILE_SIZE;
                        player->tp_info = portal->info;
                        player->next_state = PLAYER_STATE_ENTERING_TP; // @state_change
                        pause_level(level, player);

                        // Set regular music
                        set_level_music_and_play(player->level, ELevelMusic::regular);

                        return;
                    }
                }
            }

            update_player_vis_state_and_set_anim(player, move_dir);
            update_anim(&player->anim_player, delta_time);

            auto frame = get_current_frame(&player->anim_player);
            player->sprite = frame->sprite;
            player->face_direction = player->last_dir_on_ground;
        } break;
    }
}

ENTITY_RENDER_PROC(render_player) {
    auto player = self_base->as<Player>();

    if(player->in_invicible_state) {
        const float32_t perc = player->invicible_state_t / consts::invincible_time;
        const float32_t cos = cosf(perc * 60.0f);
        if(cos < 0.0f) {
            return;
        }
    }

    vec4 color = player->color;

    // @todo Star powerup effect
    if(player->has_star_powerup) {
        const float32_t perc = player->star_powerup_t / consts::star_time;
        const float32_t cos = cosf(player->level->elapsed_time * 28.0f);

        if(perc >= 0.8f) {
            if(cos < 0.0f) {
                return;
            }
        } else {
            if(cos < 0) {
                color = lerp(color::white, color::red, cos);
            }
        }
    }

    render::r_set_flip_x_quads(player->face_direction == -1);

    Sprite *sprite = &player->sprite;
    if(sprite->texture == NULL) {
        sprite = &global_data::get_sprite(SPRITE_MARIO_IDLE);
    }

    render::r_sprite(player->position + player->draw_offset, player->draw_z_pos, { sprite->width, sprite->height }, *sprite, color);
}

// @cleanup
HIT_CALLBACK_PROC(player_hit_callback) {
    auto player = self_base->as<Player>();

    if(player->state == PLAYER_STATE_FINISH_THE_LEVEL) {
        return hit_callback_result(false);
    }

    // Check for death by falling
    if(collision.with_entity != NULL && collision.with_entity->entity_type_id == entity_type_id(KillRegion)) {
        kill_player(player);
        return hit_callback_result(true);
    }

    if(collision.with_entity != NULL && collision.with_entity->entity_type_id == entity_type_id(Coin)) {
        collect_coin(collision.with_entity->as<Coin>());
        return hit_callback_result(false);
    }

    // Stomping
    if(collision.axis == Y_AXIS && collision.unit == -1 && collision.with_entity != NULL && !player->move_data.is_grounded && !player->has_star_powerup) {
        Entity *e = collision.with_entity;
        if(e->entity_type_id == entity_type_id(Goomba)) {
            kill_goomba(e->as<Goomba>(), true);
            player->bounce_up_after_stomp = true;
            return hit_callback_result(true);
        } else if(e->entity_type_id == entity_type_id(Koopa)) {
            hit_koopa(e->as<Koopa>());
            player->bounce_up_after_stomp = true;
            return hit_callback_result(true);
        } else if(e->entity_type_id == entity_type_id(KoopaShell)) {
            if(e->as<KoopaShell>()->move_dir == 0) {
                assert(player->has_collider && e->has_collider);
                float32_t player_center = player->position.x + player->has_collider->offset.x + (float32_t)player->has_collider->size.x * 0.5f;
                float32_t shell_center  = e->position.x + e->has_collider->offset.x + (float32_t)e->has_collider->size.x * 0.5f;
                int32_t dir = player_center >= shell_center ? -1 : 1;
                push_koopa_shell(e->as<KoopaShell>(), dir);
            } else {
                push_koopa_shell(e->as<KoopaShell>(), 0);
            }
            player->bounce_up_after_stomp = true;
            return hit_callback_result(true);
        }
    }

    if(collision.axis == X_AXIS && collision.with_entity != NULL && collision.with_entity->entity_type_id == entity_type_id(KoopaShell) && player->move_data.is_grounded) {
        if(collision.with_entity->as<KoopaShell>()->move_dir == 0) {
            push_koopa_shell(collision.with_entity->as<KoopaShell>(), collision.unit);
            return hit_callback_result(false);
        }
    }

    // Check for getting hit by other entity
    if(collision.with_entity != NULL && collision.with_entity->entity_flags & E_FLAG_DOES_DAMAGE_TO_PLAYER) {
        if(collision.with_entity->entity_flags & E_FLAG_STOMPABLE && (player->move_data.speed.y < 0.0f && !player->move_data.is_grounded)) {
            // if stompable -> don't get hit (x axis) when going into the entity
        } else {
            bool should_be_hit = true;

            // shouldn't get hit by shell if behind it
            if(collision.with_entity->entity_type_id == entity_type_id(KoopaShell)) {
                auto shell = collision.with_entity->as<KoopaShell>();
                if(shell->move_dir == 0) {
                    should_be_hit = false;
                } else if(shell->move_dir == 1) {
                    int32_t player_r = player->position.x + player->has_collider->offset.x + player->has_collider->size.x;
                    int32_t shell_r  = shell->position.x + shell->has_collider->offset.x + shell->has_collider->size.x;
                    if(player_r < shell_r) {
                        should_be_hit = false;
                    }
                } else if(shell->move_dir == -1) {
                    int32_t player_l = player->position.x + player->has_collider->offset.x;
                    int32_t shell_l  = shell->position.x + shell->has_collider->offset.x;
                    if(player_l > shell_l) {
                        should_be_hit = false;
                    }
                }
            }

            if(can_be_hit(player)) {
                if(should_be_hit) {
                    hit_player(player);
                    return hit_callback_result(true);
                }
            }
        }
    }

    if(player->has_star_powerup && collision.with_entity != NULL && collision.with_entity->entity_flags & E_FLAG_IS_GAMEPLAY_ENTITY) {

        const int32_t player_move_dir = sign(player->move_data.speed.x);
        Entity *e = collision.with_entity;

        switch(collision.with_entity->entity_type_id) {
            default: break;

            case entity_type_id(Goomba): {
                kill_goomba(e->as<Goomba>(), false, player_move_dir);
                return hit_callback_result(false);
            }
            case entity_type_id(Koopa): {
                kill_koopa(e->as<Koopa>(), player_move_dir);
                return hit_callback_result(false);
            }
            case entity_type_id(KoopaShell): {
                kill_koopa_shell(e->as<KoopaShell>(), player_move_dir);
                return hit_callback_result(false);
            }
            case entity_type_id(Piranha): {
                kill_piranha(e->as<Piranha>(), player_move_dir);
                return hit_callback_result(false);
            }
            case entity_type_id(Bowser): {
                kill_bowser(e->as<Bowser>(), player_move_dir);
                return hit_callback_result(false);
            }
        }
    }

    // Maybe hit the tile
    if(collision.axis == Y_AXIS && collision.unit == 1 && collision.with_tile != NULL) {
        Tile *tile = collision.with_tile;
        if(tile->is_not_empty) {
            const int32_t player_top = player->position.y + player->has_collider->offset.y + player->has_collider->size.y;
            const bool    hit_from_below = player_top - collision.unit < tile->get_ti().position.y;
            if(hit_from_below) {
                tile_hit(tile, player);
                player->jump_t = 0.0f;
            }
        }
    }

    if(collision.axis == Y_AXIS && collision.unit == 1 && collision.with_entity != NULL && collision.with_entity->entity_flags & E_FLAG_ALWAYS_BLOCKS_MOVEMENT) {
        player->jump_t = 0.0f;
    }

    return hit_callback_result(false);
}

void hit_player(Player *player) {
    if(!can_be_hit(player)) {
        return;
    }

    if(player->mode == PLAYER_IS_FIRE || player->mode == PLAYER_IS_BIG) {
        player->powerup_anim_t  = 0.0f;
        player->powerup_from_mode = player->mode;

        player->is_croutching = false;

        set_player_mode(player, PLAYER_IS_SMALL);
        pause_level(player->level, player);
        player->state = PLAYER_STATE_POWERING_UP;

        audio_player::a_play_sound(global_data::get_sound(SOUND_PLAYER_HIT));

        player->in_invicible_state = true;
        player->invicible_state_t  = 0.0f;
    } else {
        kill_player(player);
    }
}

void kill_player(Player *player) {
    set_player_mode(player, PLAYER_IS_SMALL);
    player->dead_anim_wait  = true;
    player->dead_anim_timer = 0.0f;
    player->next_state      = PLAYER_STATE_DEADGE;
    pause_level(player->level, player);

    audio_player::a_play_music(global_data::get_music(MUSIC_YOU_HAVE_DIED), 0);
}

ENTITY_UPDATE_PROC(update_fireball);
ENTITY_RENDER_PROC(render_fireball);
HIT_CALLBACK_PROC(fireball_hit_callback);

constexpr float32_t fireball_speed        = 200.0f;
constexpr float32_t fireball_gravity      = 1300.0f;
constexpr float32_t fireball_bounce_power = 200.0f;
constexpr float32_t fireball_max_bounces  = 3;
constexpr int32_t   fireball_size         = 6;


Fireball *throw_fireball(Player *player) {
    auto level = player->level;
    
    vec2i position;
    position.y = player->position.y + 16;

    if(player->last_dir_on_ground == 1) {
        position.x = player->position.x + player->collider_big.size.x + player->collider_big.offset.x;
    } else {
        position.x = player->position.x + player->collider_big.offset.x - fireball_size;
    }

    return throw_fireball(level, position, player->last_dir_on_ground);
}

Fireball *throw_fireball(Level *level, vec2i position, int32_t direction) {
    auto fireball = create_entity_m(level, Fireball);
    fireball->update_proc = update_fireball;
    fireball->render_proc = render_fireball;
    fireball->position = position;

    auto collider = &fireball->collider;
    collider->offset = { 1, 1 };
    collider->size = { fireball_size, fireball_size };
    fireball->has_collider = collider;

    auto move_data = &fireball->move_data;
    move_data->hit_callback = fireball_hit_callback;
    fireball->has_move_data = move_data;
    fireball->move_dir = direction;

    fireball->anim_player = init_anim_player(&fireball_anim_set);

    return fireball;
}

ENTITY_UPDATE_PROC(update_fireball) {
    auto fireball = self_base->as<Fireball>();
    auto level    = fireball->level;

    if(fireball->change_move_dir_next_frame) {
        fireball->move_dir *= -1;
        fireball->change_move_dir_next_frame = false;
    }

    if(fireball->bounce_next_frame) {
        fireball->bounce_count += 1;
        fireball->move_data.speed.y = fireball_bounce_power;
        fireball->bounce_next_frame = false;
    }

    fireball->move_data.speed.y -= fireball_gravity * delta_time;
    fireball->move_data.speed.x = fireball->move_dir * fireball_speed;

    do_move(fireball, &fireball->move_data, delta_time);
    
    if(fireball->bounce_count > fireball_max_bounces || fireball->delete_self) {
        auto t_anim = spawn_timed_anim(fireball->level, fireball->position + fireball->collider.offset + fireball->collider.size / 2, &fireball_explosion_anim_set);
        t_anim->centered = true;
        delete_entity(fireball);
        return;
    }

    // Delete the fireball if is offscreen
    auto dims_scaled = level->render_view.calc_dims_scaled();
    vec2i camera_min = vec2i::make(vec2{ level->render_view.x_translate, level->render_view.y_translate } - dims_scaled * 0.5f);
    vec2i camera_max = vec2i::make(vec2{ level->render_view.x_translate, level->render_view.y_translate } + dims_scaled * 0.5f);
    if(!aabb(camera_min, camera_max - camera_min, fireball->position + fireball->collider.offset, fireball->collider.size)) {
        delete_entity(fireball);
        return;
    }

    update_anim(&fireball->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_fireball) {
    auto fireball = self_base->as<Fireball>();
    auto frame = get_current_frame(&fireball->anim_player);

    render::r_sprite(fireball->position, 0, frame->size(), frame->sprite, color::white);
}

HIT_CALLBACK_PROC(fireball_hit_callback) {
    auto fireball = self_base->as<Fireball>();

    if(collision.axis == Y_AXIS && collision.unit == -1 && ((collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT && !(collision.with_tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE))
       || collision.with_entity != NULL && (collision.with_entity->entity_flags & E_FLAG_ALWAYS_BLOCKS_MOVEMENT || collision.with_entity->entity_flags & E_FLAG_BLOCKS_MOVEMENT_FROM_ABOVE_ONLY))) {
        fireball->bounce_next_frame = true;
        return hit_callback_result(true);
    } else if(collision.axis == X_AXIS && (collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT && !(collision.with_tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE))
              || (collision.with_entity != NULL && collision.with_entity->entity_flags & E_FLAG_ALWAYS_BLOCKS_MOVEMENT)) {
        fireball->change_move_dir_next_frame = true;
        return hit_callback_result(true);
    } 
    
    if(collision.with_entity != NULL) {
        fireball->delete_self = true;
        switch(collision.with_entity->entity_type_id) {
            default: { fireball->delete_self = false; } break;

            case entity_type_id(Goomba): {
                kill_goomba(collision.with_entity->as<Goomba>(), false, sign(fireball->move_data.speed.x));
                return hit_callback_result(true);
            } break;

            case entity_type_id(Koopa): {
                kill_koopa(collision.with_entity->as<Koopa>(), sign(fireball->move_data.speed.x));
                return hit_callback_result(true);
            } break;

            case entity_type_id(KoopaShell): {
                kill_koopa_shell(collision.with_entity->as<KoopaShell>(), sign(fireball->move_data.speed.x));
                return hit_callback_result(true);
            } break;

            case entity_type_id(Piranha): {
                kill_piranha(collision.with_entity->as<Piranha>(), sign(fireball->move_data.speed.x));
                return hit_callback_result(true);
            } break;

            case entity_type_id(Bowser): {
                hit_bowser(collision.with_entity->as<Bowser>(), sign(fireball->move_data.speed.x));
                return hit_callback_result(true);
            } break;
        }
        
    }

    return hit_callback_result(false);
}

void init_player_common_data(void) {
    /* PLAYER_IS_SMALL */ {
        auto mario_idle = &anim_sets[PLAYER_IS_SMALL][P_VIS_IDLE];
        init_anim_set(mario_idle);
        next_anim_frame(mario_idle, global_data::get_sprite(SPRITE_MARIO_IDLE), 1.0f);

        auto mario_run = &anim_sets[PLAYER_IS_SMALL][P_VIS_RUN];
        init_anim_set(mario_run);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_RUN_1), 0.2f);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_RUN_2), 0.2f);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_RUN_3), 0.2f);

        auto mario_jump = &anim_sets[PLAYER_IS_SMALL][P_VIS_JUMP];
        init_anim_set(mario_jump);
        next_anim_frame(mario_jump, global_data::get_sprite(SPRITE_MARIO_JUMP), 1.0f);

        auto mario_turn = &anim_sets[PLAYER_IS_SMALL][P_VIS_TURN];
        init_anim_set(mario_turn);
        next_anim_frame(mario_turn, global_data::get_sprite(SPRITE_MARIO_TURN), 1.0f);
    }
    
    /* PLAYER_IS_BIG */ {
        auto mario_idle = &anim_sets[PLAYER_IS_BIG][P_VIS_IDLE];
        init_anim_set(mario_idle);
        next_anim_frame(mario_idle, global_data::get_sprite(SPRITE_MARIO_BIG_IDLE), 1.0f);

        auto mario_run = &anim_sets[PLAYER_IS_BIG][P_VIS_RUN];
        init_anim_set(mario_run);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_BIG_RUN_1), 0.2f);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_BIG_RUN_2), 0.2f);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_BIG_RUN_3), 0.2f);

        auto mario_jump = &anim_sets[PLAYER_IS_BIG][P_VIS_JUMP];
        init_anim_set(mario_jump);
        next_anim_frame(mario_jump, global_data::get_sprite(SPRITE_MARIO_BIG_JUMP), 1.0f);

        auto mario_turn = &anim_sets[PLAYER_IS_BIG][P_VIS_TURN];
        init_anim_set(mario_turn);
        next_anim_frame(mario_turn, global_data::get_sprite(SPRITE_MARIO_BIG_TURN), 1.0f);

        auto mario_croutch = &anim_sets[PLAYER_IS_BIG][P_VIS_CROUTCH];
        init_anim_set(mario_croutch);
        next_anim_frame(mario_croutch, global_data::get_sprite(SPRITE_MARIO_BIG_CROUTCH), 1.0f);
    }

    /* PLAYER_IS_FIRE */ {
        auto mario_idle = &anim_sets[PLAYER_IS_FIRE][P_VIS_IDLE];
        init_anim_set(mario_idle);
        next_anim_frame(mario_idle, global_data::get_sprite(SPRITE_MARIO_FIRE_IDLE), 1.0f);

        auto mario_run = &anim_sets[PLAYER_IS_FIRE][P_VIS_RUN];
        init_anim_set(mario_run);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_FIRE_RUN_1), 0.2f);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_FIRE_RUN_2), 0.2f);
        next_anim_frame(mario_run, global_data::get_sprite(SPRITE_MARIO_FIRE_RUN_3), 0.2f);

        auto mario_jump = &anim_sets[PLAYER_IS_FIRE][P_VIS_JUMP];
        init_anim_set(mario_jump);
        next_anim_frame(mario_jump, global_data::get_sprite(SPRITE_MARIO_FIRE_JUMP), 1.0f);

        auto mario_turn = &anim_sets[PLAYER_IS_FIRE][P_VIS_TURN];
        init_anim_set(mario_turn);
        next_anim_frame(mario_turn, global_data::get_sprite(SPRITE_MARIO_FIRE_TURN), 1.0f);

        auto mario_croutch = &anim_sets[PLAYER_IS_FIRE][P_VIS_CROUTCH];
        init_anim_set(mario_croutch);
        next_anim_frame(mario_croutch, global_data::get_sprite(SPRITE_MARIO_FIRE_CROUTCH), 1.0f);
    }

    init_anim_set(&fireball_anim_set);
    next_anim_frame(&fireball_anim_set, global_data::get_sprite(SPRITE_FIREBALL_1), 0.25f);
    next_anim_frame(&fireball_anim_set, global_data::get_sprite(SPRITE_FIREBALL_2), 0.25f);
    next_anim_frame(&fireball_anim_set, global_data::get_sprite(SPRITE_FIREBALL_3), 0.25f);
    next_anim_frame(&fireball_anim_set, global_data::get_sprite(SPRITE_FIREBALL_4), 0.25f);

    init_anim_set(&fireball_explosion_anim_set);
    next_anim_frame(&fireball_explosion_anim_set, global_data::get_sprite(SPRITE_FIRE_EXPLOSION_1), 0.05f);
    next_anim_frame(&fireball_explosion_anim_set, global_data::get_sprite(SPRITE_FIRE_EXPLOSION_2), 0.05f);
    next_anim_frame(&fireball_explosion_anim_set, global_data::get_sprite(SPRITE_FIRE_EXPLOSION_3), 0.10f);
}