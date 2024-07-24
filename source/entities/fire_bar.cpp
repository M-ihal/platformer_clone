#include "fire_bar.h"
#include "renderer.h"
#include "data.h"
#include "all_entities.h"

namespace {
    constexpr vec2i     fire_size          = { 8, 8 };
    constexpr float32_t fire_spacing       = 8.0f;
    constexpr float32_t fire_rotation_time = 0.125f; // 0.2f;
    AnimSet anim_set_fireball;
}

void init_fire_bar_common_data(void) {
    const float32_t v = 0.5f;
    init_anim_set(&anim_set_fireball);
    next_anim_frame(&anim_set_fireball, global_data::get_sprite(SPRITE_FIREBALL_1), v);
    next_anim_frame(&anim_set_fireball, global_data::get_sprite(SPRITE_FIREBALL_2), v);
    next_anim_frame(&anim_set_fireball, global_data::get_sprite(SPRITE_FIREBALL_3), v);
    next_anim_frame(&anim_set_fireball, global_data::get_sprite(SPRITE_FIREBALL_4), v);
}

ENTITY_SERIALIZE_PROC(FireBar) {
    auto fire_bar = self_base->as<FireBar>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", fire_bar->position.e,  2);
    es_data.add_int32("fire_num", &fire_bar->fire_num,   1);
    es_data.add_int32("angle_idx", &fire_bar->angle_idx, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(FireBar) {
    vec2i   position;
    int32_t fire_num;
    int32_t angle_idx;

    if(!es_data->try_get_int32("position",  position.e, 2),
       !es_data->try_get_int32("fire_num",  &fire_num,  1),
       !es_data->try_get_int32("angle_idx", &angle_idx, 1)) {
        return NULL;
    }

    auto fire_bar = spawn_fire_bar(level, position, fire_num);
    fire_bar->angle_idx = angle_idx;
    fire_bar->angle = calc_fire_bar_angle(angle_idx);
    return fire_bar;
}

ENTITY_UPDATE_PROC(update_fire_bar);
ENTITY_RENDER_PROC(render_fire_bar);

vec2i calc_one_fire_position(FireBar *fire_bar, int32_t fire_idx) {
    assert(fire_idx < fire_bar->fire_num, "Invalid fire_idx");
    vec2i result = fire_bar->position + vec2i::make(fire_bar->angle * fire_idx * fire_spacing) - fire_bar->fire_size / 2;
    return result;
}

vec2 calc_fire_bar_angle(int32_t angle_idx) {
    vec2 result = vec2::normalize(vec2::from_angle(fire_bar_angles[angle_idx]));
    return result;
}

FireBar *spawn_fire_bar(Level *level, vec2i position, int32_t fire_num) {
    auto fire_bar = create_entity_m(level, FireBar);
    fire_bar->update_proc = update_fire_bar;
    fire_bar->render_proc = render_fire_bar;
    fire_bar->position = position;

    fire_bar->anim_player = init_anim_player(&anim_set_fireball);

    fire_bar->counter   = 0.0f;
    fire_bar->fire_num  = fire_num;
    fire_bar->fire_size = fire_size;
    fire_bar->angle_idx = 0;
    fire_bar->angle     = calc_fire_bar_angle(fire_bar->angle_idx);

    return fire_bar;
}

ENTITY_UPDATE_PROC(update_fire_bar) {
    auto fire_bar = self_base->as<FireBar>();

    fire_bar->counter += delta_time;
    while(fire_bar->counter >= fire_rotation_time) {
        fire_bar->counter -= fire_rotation_time;

        int32_t angle_idx = fire_bar->angle_idx + 1;
        if(angle_idx >= array_count(fire_bar_angles)) {
            angle_idx = 0;
        }

        fire_bar->angle_idx = angle_idx;
        fire_bar->angle = calc_fire_bar_angle(angle_idx);
    }

    auto player = get_player(fire_bar->level);
    if(player != NULL) {
        for(int32_t idx = 0; idx < fire_bar->fire_num; ++idx) {
            vec2i fire_pos = calc_one_fire_position(fire_bar, idx);
            
            // Check if any fire collides with player
            if(aabb(fire_pos, fire_bar->fire_size, player->position + player->has_collider->offset, player->has_collider->size)) {
                hit_player(player);
            }
        }
    }

    update_anim(&fire_bar->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_fire_bar) {
    auto fire_bar = self_base->as<FireBar>();
    auto frame = get_current_frame(&fire_bar->anim_player);

    for(int32_t idx = 0; idx < fire_bar->fire_num; ++idx) {
        vec2i fire_pos = calc_one_fire_position(fire_bar, idx);
        render::r_sprite(fire_pos, 0, frame->size(), frame->sprite, color::white);
    }
}