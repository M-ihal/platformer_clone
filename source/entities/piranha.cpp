#include "piranha.h"
#include "all_entities.h"
#include "data.h"

namespace {
    constexpr float32_t piranha_wait_time = 2.0f;
    constexpr float32_t piranha_hide_time = 1.0f;
    constexpr int32_t   piranha_move_dist = 24;

    AnimSet anim_set;
}

void init_piranha_common_data(void) {
    init_anim_set(&anim_set);
    next_anim_frame(&anim_set, global_data::get_sprite(SPRITE_PIRANHA_1), 0.2f);
    next_anim_frame(&anim_set, global_data::get_sprite(SPRITE_PIRANHA_2), 0.2f);
}

ENTITY_SERIALIZE_PROC(Piranha) {
    auto piranha = self_base->as<Piranha>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", piranha->position.e, 2);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Piranha) {
    vec2i position;

    if(!es_data->try_get_int32("position", position.e, 2)) {
        return NULL;
    }

    return spawn_piranha(level, position);
}

ENTITY_UPDATE_PROC(update_piranha);
ENTITY_RENDER_PROC(render_piranha);

Piranha *spawn_piranha(Level *level, vec2i position) {
    auto piranha = create_entity_m(level, Piranha);
    piranha->update_proc = update_piranha;
    piranha->render_proc = render_piranha;
    piranha->position = position;

    set_entity_flag(piranha, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(piranha, E_FLAG_DOES_DAMAGE_TO_PLAYER);

    auto collider = &piranha->collider;
    collider->offset = { 4, 2 };
    collider->size = { 8, 13 };
    piranha->has_collider = collider;

    piranha->anim_player = init_anim_player(&anim_set);
    
    piranha->wait_t = 0.0f;
    piranha->move_t = 0.0f;
    piranha->state = PIRANHA_MOVES_DOWN;

    make_asleep(piranha);

    return piranha;
}

ENTITY_UPDATE_PROC(update_piranha) {
    auto piranha = self_base->as<Piranha>();

    bool piranha_out_of_camera = !aabb(piranha->position + piranha->collider.offset, piranha->collider.size, piranha->level->wake_up_rect_position, piranha->level->wake_up_rect_size);
    if(piranha_out_of_camera) {
        piranha->state = PIRANHA_HIDES;
        piranha->move_t = 0.0f;
        piranha->wait_t = 0.0f;
    }

    EPiranhaState next_state = piranha->state;
    switch(piranha->state) {
        default: assert(false); break;

        case PIRANHA_HIDES: {
            piranha->wait_t += delta_time;
            if(piranha->wait_t >= piranha_hide_time) {
                bool can_move_up = true;
                auto player = get_player(piranha->level);
                if(player != NULL) {
                    int32_t box_radius_x = 10;
                    int32_t box_radius_y = 14;
                    if(aabb(player->position + player->has_collider->offset, player->has_collider->size, 
                       piranha->position + piranha->collider.offset - vec2i{ box_radius_x, box_radius_y }, piranha->collider.size + vec2i::make(box_radius_x, box_radius_y) * 2)) {
                        can_move_up = false;
                    }
                }

                if(can_move_up) {
                    piranha->wait_t = 0.0f;
                    next_state = PIRANHA_MOVES_UP;
                }
            }
        } break;

        case PIRANHA_MOVES_UP: {
            piranha->move_t += delta_time;
            if(piranha->move_t >= 1.0f) {
                piranha->move_t -= (piranha->move_t - 1.0f);
                next_state = PIRANHA_WAITS;
            }
        } break;

        case PIRANHA_WAITS: {
            piranha->wait_t += delta_time;
            if(piranha->wait_t >= piranha_wait_time) {
                piranha->wait_t = 0.0f;
                next_state = PIRANHA_MOVES_DOWN;
            }
        } break;

        case PIRANHA_MOVES_DOWN: {
            piranha->move_t -= delta_time;
            if(piranha->move_t <= 0.0f) {
                piranha->move_t = 0.0f;
                next_state = PIRANHA_HIDES;
            }
        } break;
    }
    piranha->state = next_state;

    piranha->offset = (int32_t)roundf(piranha->move_t * piranha_move_dist);
    piranha->collider.offset.y = 2 + piranha->offset;

    update_anim(&piranha->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_piranha) {
    auto piranha = self_base->as<Piranha>();
    auto frame = get_current_frame(&piranha->anim_player);

    render::r_sprite(piranha->position + vec2i { 0, piranha->offset }, Z_PIRANHA_POS, frame->size(), frame->sprite, color::white);
}

void kill_piranha(Piranha *piranha, int32_t fall_dir) {
    add_points_with_text_above_entity(piranha->level, POINTS_FOR_PIRANHA, piranha);
    spawn_enemy_fall_anim(piranha->level, piranha->position, fall_dir, anim_set.frames[0].sprite);
    audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));
    delete_entity(piranha);
    return;
}