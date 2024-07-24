#include "goomba.h"
#include "all_entities.h"
#include "data.h"

namespace {
    constexpr float32_t goomba_gravity      = 290.0f;
    constexpr float32_t goomba_speed        = 38.0f;
    constexpr float32_t goomba_despawn_time = 1.0f; // seconds
    constexpr float32_t goomba_stomp_time   = 1.0f; // seconds

    Sprite sprite_goomba_flat[GOOMBA_TYPE__COUNT];
    AnimSet anim_sets        [GOOMBA_TYPE__COUNT];
};

ENTITY_SERIALIZE_PROC(Goomba) {
    auto goomba = self_base->as<Goomba>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", goomba->position.e, 2);
    es_data.add_int32("goomba_type", (int32_t *)&goomba->goomba_type, 1);
    es_data.add_int32("move_dir", &goomba->move_dir, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Goomba) {
    int32_t goomba_type;
    vec2i   position;
    int32_t move_dir;

    if(!es_data->try_get_int32("goomba_type", &goomba_type, 1),
       !es_data->try_get_int32("position",    position.e,   2),
       !es_data->try_get_int32("move_dir",    &move_dir,    1)) {
        return NULL;
    }

    auto goomba = spawn_goomba(level, position, (EGoombaType)goomba_type);
    goomba->move_dir = move_dir;
    return goomba;
}

ENTITY_UPDATE_PROC(update_goomba);
ENTITY_RENDER_PROC(render_goomba);
HIT_CALLBACK_PROC(goomba_hit_callback);

Goomba *spawn_goomba(Level *level, vec2i position, EGoombaType goomba_type) {
    auto goomba = create_entity_m(level, Goomba);
    goomba->update_proc = update_goomba;
    goomba->render_proc = render_goomba;
    goomba->position = position;

    goomba->goomba_type = goomba_type;

    set_entity_flag(goomba, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(goomba, E_FLAG_DOES_DAMAGE_TO_PLAYER);
    set_entity_flag(goomba, E_FLAG_STOMPABLE);

    auto collider = &goomba->collider;
    collider->size   = { 12, 10 };
    collider->offset = { 2,  1 };
    goomba->has_collider = collider;

    auto move_data = &goomba->move_data;
    move_data->hit_callback = goomba_hit_callback;
    goomba->has_move_data = move_data;

    goomba->move_dir = 1;
    goomba->anim_player = init_anim_player(&anim_sets[goomba_type]);

    make_asleep(goomba);

    return goomba;
}

ENTITY_UPDATE_PROC(update_goomba) {
    auto goomba = self_base->as<Goomba>();
    
    if(goomba->change_move_dir_next_frame) {
        goomba->move_dir *= -1;
        goomba->change_move_dir_next_frame = false;
    }

    const int32_t dir = sign(goomba->move_dir);
    auto move_data = &goomba->move_data;
    move_data->speed.x = dir * goomba_speed;
    move_data->speed.y -= goomba_gravity * delta_time;
    do_move(goomba, &goomba->move_data, delta_time);

    update_anim(&goomba->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_goomba) {
    auto goomba = self_base->as<Goomba>();
    auto frame = get_current_frame(&goomba->anim_player);
    const Sprite *sprite = &frame->sprite;

    render::r_sprite(goomba->position, Z_GOOMBA_POS, { sprite->width, sprite->height }, *sprite, color::white);
}

HIT_CALLBACK_PROC(goomba_hit_callback) {
    auto goomba = self_base->as<Goomba>();

    if(collision.with_tile) {
        Tile *tile = collision.with_tile;
        if(tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT && collision.axis == X_AXIS) {
            goomba->change_move_dir_next_frame = true;
            return hit_callback_result(true);
        }
    } else {
        switch(collision.with_entity->entity_type_id) {
            case entity_type_id(Goomba): {
                if(collision.axis == X_AXIS) {
                    goomba->change_move_dir_next_frame = true;
                    return hit_callback_result(true);
                }
            } break;
        }
    }

    return hit_callback_result(false);
}

void kill_goomba(Goomba *goomba, bool by_stomping, int32_t fall_dir /* matters if !by_stomping */) {
    const auto level = goomba->level;

    add_points_with_text_above_entity(level, POINTS_FOR_GOOMBA, goomba);
    
    if(by_stomping) {
        auto t_sprite = spawn_timed_sprite(goomba->level, goomba->position, goomba_stomp_time, sprite_goomba_flat[goomba->goomba_type]);
        t_sprite->z_position = Z_GOOMBA_POS;
        audio_player::a_play_sound(global_data::get_sound(SOUND_STOMP));
    } else {
        Sprite *sprite = &anim_sets[goomba->goomba_type].frames[0].sprite;
        spawn_enemy_fall_anim(goomba->level, goomba->position, fall_dir, *sprite);
        audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));
    }
    delete_entity(goomba);
    return;
}

void init_goomba_common_data(void) {
    sprite_goomba_flat[GOOMBA_TYPE_OVERWORLD]   = global_data::get_sprite(SPRITE_GOOMBA_FLAT);
    sprite_goomba_flat[GOOMBA_TYPE_UNDERGROUND] = global_data::get_sprite(SPRITE_GOOMBA_UNDERGROUND_FLAT);
    sprite_goomba_flat[GOOMBA_TYPE_CASTLE]      = global_data::get_sprite(SPRITE_GOOMBA_CASTLE_FLAT);

    init_anim_set(&anim_sets[GOOMBA_TYPE_OVERWORLD]);
    next_anim_frame(&anim_sets[GOOMBA_TYPE_OVERWORLD], global_data::get_sprite(SPRITE_GOOMBA_1), 0.3f);
    next_anim_frame(&anim_sets[GOOMBA_TYPE_OVERWORLD], global_data::get_sprite(SPRITE_GOOMBA_2), 0.3f);

    init_anim_set(&anim_sets[GOOMBA_TYPE_UNDERGROUND]);
    next_anim_frame(&anim_sets[GOOMBA_TYPE_UNDERGROUND], global_data::get_sprite(SPRITE_GOOMBA_UNDERGROUND_1), 0.3f);
    next_anim_frame(&anim_sets[GOOMBA_TYPE_UNDERGROUND], global_data::get_sprite(SPRITE_GOOMBA_UNDERGROUND_2), 0.3f);

    init_anim_set(&anim_sets[GOOMBA_TYPE_CASTLE]);
    next_anim_frame(&anim_sets[GOOMBA_TYPE_CASTLE], global_data::get_sprite(SPRITE_GOOMBA_CASTLE_1), 0.3f);
    next_anim_frame(&anim_sets[GOOMBA_TYPE_CASTLE], global_data::get_sprite(SPRITE_GOOMBA_CASTLE_2), 0.3f);
}