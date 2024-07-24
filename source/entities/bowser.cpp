#include "bowser.h"
#include "all_entities.h"
#include "data.h"

namespace {
    constexpr int32_t segment_width  = 16;
    constexpr int32_t segment_height = 16;

    constexpr float32_t bowser_gravity     = 135.0f;
    constexpr float32_t bowser_speed       = 25.0f;
    constexpr float32_t bowser_jump_power  = 110.0f;
    constexpr float32_t bowser_wait_time   = 0.5f;
    constexpr int32_t bowser_health_points = 10;
    constexpr int32_t bowser_max_moves_default = 2;

    constexpr float32_t bowser_after_breath_anim_timer  = 0.5f;
    constexpr float32_t bowser_breath_fire_segment_time = 0.5f;

    constexpr float32_t bowser_fire_x_speed = 80.0f;
    constexpr float32_t bowser_fire_y_speed = 16.0f; // 14.0f;

    AnimSet bowser_anim_set;
    AnimSet bowser_breathing_anim_set;
    AnimSet bowser_fire_anim_set;
}

void init_bowser_common_data(void) {
    bowser_anim_set = init_anim_set();
    next_anim_frame(&bowser_anim_set, global_data::get_sprite(SPRITE_BOWSER_1), 0.25f);
    next_anim_frame(&bowser_anim_set, global_data::get_sprite(SPRITE_BOWSER_2), 0.25f);

    bowser_breathing_anim_set = init_anim_set();
    next_anim_frame(&bowser_breathing_anim_set, global_data::get_sprite(SPRITE_BOWSER_3), 0.25f);
    next_anim_frame(&bowser_breathing_anim_set, global_data::get_sprite(SPRITE_BOWSER_4), 0.25f);

    bowser_fire_anim_set = init_anim_set();
    next_anim_frame(&bowser_fire_anim_set, global_data::get_sprite(SPRITE_BOWSER_FIRE_1), 0.15f);
    next_anim_frame(&bowser_fire_anim_set, global_data::get_sprite(SPRITE_BOWSER_FIRE_2), 0.15f);
}

ENTITY_SERIALIZE_PROC(Bowser) {
    auto bowser = self_base->as<Bowser>();

    EntitySaveData es_data = { };
    es_data.add_int32("position",      bowser->position.e, 2);
    es_data.add_int32("max_moves",    &bowser->max_moves_in_any_direction, 1);
    es_data.add_int32("vision_range", &bowser->vision_range, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Bowser) {
    vec2i position;
    if(!es_data->try_get_int32("position", position.e, 2)) {
        return NULL;
    }

    int32_t max_moves = bowser_max_moves_default;
    es_data->try_get_int32("max_moves", &max_moves, 1);

    auto bowser = spawn_bowser(level, position);
    bowser->max_moves_in_any_direction = max_moves;

    es_data->try_get_int32("vision_range", &bowser->vision_range);

    return bowser;
}

ENTITY_UPDATE_PROC(update_bowser);
ENTITY_RENDER_PROC(render_bowser);

Bowser *spawn_bowser(Level *level, vec2i position) {
    auto bowser = create_entity_m(level, Bowser);
    bowser->update_proc = update_bowser;
    bowser->render_proc = render_bowser;
    bowser->position = position;

    set_entity_flag(bowser, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(bowser, E_FLAG_DOES_DAMAGE_TO_PLAYER);
    set_entity_flag(bowser, E_FLAG_SLEEPS_UNTIL_AWAKEN);

    auto collider = &bowser->collider;
    collider->offset = vec2i{ TILE_SIZE - 8, 1 };
    collider->size = { TILE_SIZE + 4, TILE_SIZE * 2 - 6 };
    bowser->has_collider = collider;

    auto move_data = &bowser->move_data;
    bowser->has_move_data = move_data;

    // bowser->anim_player = init_anim_player(&bowser_anim_set);
    bowser->anim_player = init_anim_player(&bowser_breathing_anim_set);

    bowser->health_points = bowser_health_points;
    bowser->max_moves_in_any_direction = bowser_max_moves_default;
    bowser->vision_range = 128;
    bowser->saw_player = false;

    bowser->move_dirs_combined = 0;
    bowser->position_x_target = bowser->position.x;

    return bowser;
}

ENTITY_UPDATE_PROC(update_bowser) {
    auto bowser    = self_base->as<Bowser>();
    auto move_data = &bowser->move_data;

    if(bowser->after_breath_t > 0.0f) {
        bowser->after_breath_t -= delta_time;
        if(bowser->after_breath_t <= 0.0f) {
            set_anim(&bowser->anim_player, &bowser_anim_set);
        }
    }

    if(!bowser->saw_player) {
        auto player = get_player(bowser->level);
        if(player != NULL) {
            const int32_t player_l = player->position.x + player->has_collider->offset.x;
            const int32_t bowser_c = bowser->position.x + bowser->collider.offset.x + bowser->collider.size.x / 2;
            if(player_l > bowser_c) {
                bowser->facing_dir = 1;
            } else {
                bowser->facing_dir = -1;
            }

            const int32_t player_r = player_l + player->has_collider->size.x;
            const int32_t vis_l = bowser_c - bowser->vision_range;
            if(player_r >= vis_l) {
                bowser->saw_player = true;
            }
        }
    }

    if(bowser->facing_dir != -1 || !bowser->saw_player) {
        bowser->breath_t = 0.0f;
    }

    bowser->breath_t += delta_time;
    if(bowser->breath_t >= bowser_breath_fire_segment_time) {
        bowser->breath_t -= bowser_breath_fire_segment_time;
        
        const int32_t rng = rand() % 3; // What chances to breath fire?
        if(rng == 1) {

            // Decide if the fire should move into different y position
            int32_t y_move = 0;
            const int32_t rng_y_move = rand() % 7;
            if(rng_y_move == 0 || rng_y_move == 3) { // 2/7
                y_move = -1;
            } else if(rng_y_move == 1) { // 1/7
                y_move = +1;
            }

            spawn_bowser_fire(bowser->level, bowser, y_move);
            audio_player::a_play_sound(global_data::get_sound(SOUND_TILE_HIT)); // @placeholder Fire breath sound

            bowser->after_breath_t = bowser_after_breath_anim_timer;
            set_anim(&bowser->anim_player, &bowser_breathing_anim_set);
        }
    }
    
    move_data->speed.x = bowser->move_dir * bowser_speed;
    move_data->speed.y -= bowser_gravity * delta_time;
    do_move(bowser, move_data, delta_time);

    if(bowser->wait_t >= 0.0f) {
        bowser->wait_t -= delta_time;
    }

    const bool should_change_direction = (bowser->wait_t <= 0.0f && bowser->move_dir == 0) 
        || (bowser->move_dir == -1 && bowser->position.x <= bowser->position_x_target) 
        || (bowser->move_dir == 1 && bowser->position.x >= bowser->position_x_target);

    if(should_change_direction) {
        int32_t rng = rand() % 3 - 1;

        const bool cant_do_the_move = (bowser->move_dirs_combined <= -bowser->max_moves_in_any_direction && rng == -1) 
            || (bowser->move_dirs_combined >= bowser->max_moves_in_any_direction && rng == 1);

        if(cant_do_the_move) {
            rng *= -1; // Move in oposite direction
        }

        bowser->position.x = bowser->position_x_target;
        bowser->move_dir = rng;
        bowser->move_dirs_combined += bowser->move_dir;
        bowser->position_x_target = bowser->position.x + rng * bowser_move_width;

        if(rng == 0) {
            bowser->wait_t = bowser_wait_time;
        }

        if(move_data->is_grounded) {
            int32_t rng_jump = rand() % 8; // 1/x if should jump while changing direction
            if(rng_jump == 0) {
                move_data->speed.y = bowser_jump_power;
            }
        }
    }

    if(bowser->wait_t <= 0.0f && bowser->move_data.is_grounded == true) {
        update_anim(&bowser->anim_player, delta_time);
    }
}

ENTITY_RENDER_PROC(render_bowser) {
    auto bowser = self_base->as<Bowser>();
    auto frame  = get_current_frame(&bowser->anim_player);

    render::r_set_flip_x_quads(bowser->facing_dir == -1);
    render::r_sprite(bowser->position, Z_BOWSER_POS, frame->size(), frame->sprite, color::white);
}

void hit_bowser(Bowser *bowser, int32_t fall_dir) {
    bowser->health_points -= 1;
    if(bowser->health_points <= 0) {
        kill_bowser(bowser, fall_dir);
        return;
    }
    audio_player::a_play_sound(global_data::get_sound(SOUND_TILE_HIT));
}

void kill_bowser(Bowser *bowser, int32_t fall_dir) {
    add_points_with_text_above_entity(bowser->level, POINTS_FOR_BOWSER, bowser);
    spawn_enemy_fall_anim(bowser->level, bowser->position, fall_dir, global_data::get_sprite(SPRITE_BOWSER_1));
    delete_entity(bowser);
    audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));
}

ENTITY_UPDATE_PROC(update_bowser_fire);
ENTITY_RENDER_PROC(render_bowser_fire);
HIT_CALLBACK_PROC(bowser_fire_hit_callback);

constexpr vec2i bowser_fire_size = { 24, 8 };

BowserFire *spawn_bowser_fire(Level *level, vec2i position) {
    auto b_fire = create_entity_m(level, BowserFire);
    b_fire->update_proc = update_bowser_fire;
    b_fire->render_proc = render_bowser_fire;
    b_fire->position = position;
    b_fire->y_target = position.y;

    set_entity_flag(b_fire, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(b_fire, E_FLAG_DOES_DAMAGE_TO_PLAYER);

    const int32_t pad = 2;

    auto collider = &b_fire->collider;
    collider->offset = { pad, pad };
    collider->size   =  bowser_fire_size - collider->offset * 2;
    b_fire->has_collider = collider;

    auto move_data = &b_fire->move_data;
    move_data->hit_callback = bowser_fire_hit_callback;
    b_fire->has_move_data = move_data;

    b_fire->anim_player = init_anim_player(&bowser_fire_anim_set);

    return b_fire;
}

BowserFire *spawn_bowser_fire(Level *level, Bowser *bowser, int32_t y_move) {
    vec2i position = { bowser->position.x - 20, bowser->position.y + TILE_SIZE + TILE_SIZE / 2 - bowser_fire_size.y / 2 };
    auto b_fire = spawn_bowser_fire(level, position);
    b_fire->y_target += sign(y_move) * TILE_SIZE;
    return b_fire;
}

ENTITY_UPDATE_PROC(update_bowser_fire) {
    auto b_fire = self_base->as<BowserFire>();
    auto move_data = &b_fire->move_data;

    update_anim(&b_fire->anim_player, delta_time);

    if(b_fire->position.y != b_fire->y_target) {
        if(absolute_value(b_fire->y_target - b_fire->position.y) == 1) {
            b_fire->y_target = b_fire->position.y;
        }
        const int32_t unit = sign(b_fire->y_target - b_fire->position.y);
        move_data->speed.y = unit * bowser_fire_y_speed;
    }

    move_data->speed.x = -bowser_fire_x_speed;
    do_move(b_fire, move_data, delta_time);
    if(b_fire->deleted) {
        return;
    }
}

ENTITY_RENDER_PROC(render_bowser_fire) {
    auto b_fire = self_base->as<BowserFire>();
    auto frame = get_current_frame(&b_fire->anim_player);
    render::r_sprite(b_fire->position, Z_BOWSER_FIRE_POS, frame->size(), frame->sprite, color::white);
}

HIT_CALLBACK_PROC(bowser_fire_hit_callback) {
    auto b_fire = self_base->as<BowserFire>();
    if(collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT && !(collision.with_tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE)) {
        delete_entity(b_fire); // @todo Shouldn't delete here probably
        return hit_callback_result(true);
    }
    return hit_callback_result(false);
}

void _render_debug_bowser_info(Bowser *bowser) {
    int32_t left  = bowser->position.x - bowser->max_moves_in_any_direction * bowser_move_width;
    int32_t right = bowser->position.x + bowser->max_moves_in_any_direction * bowser_move_width + TILE_SIZE * 2;
    render::r_line_quad(vec2i{ left, bowser->position.y }, 0, vec2i{ right - left, bowser->collider.size.y }, color::cyan);
    
    const vec4 vis_color = { 0.75f, 0.3f, 0.2f, 1.0f }; // { 0.3f, 0.8f, 0.2f, 1.0f };
    int32_t bowser_x_mid = bowser->position.x + bowser->collider.offset.x + bowser->collider.size.x / 2;
    render::r_set_line_width(2.0f);
    const int32_t y_mid = bowser->position.y + bowser->collider.offset.y + bowser->collider.size.y / 2;
    render::r_line(vec2i{ bowser_x_mid - bowser->vision_range, y_mid }, vec2i{ bowser_x_mid, y_mid }, 0, vis_color);
    const int32_t range = 8;
    render::r_line(vec2i{ bowser_x_mid - bowser->vision_range, y_mid - range }, vec2i{ bowser_x_mid - bowser->vision_range, y_mid + range }, 0, vis_color);
}