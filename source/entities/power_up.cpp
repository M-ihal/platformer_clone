#include "power_up.h"
#include "data.h"
#include "all_entities.h"

static void setup_spawn_anim(PowerUpSpawnAnim *anim, Tile *spawner, Collider *power_up_collider);
static bool update_spawn_anim(PowerUpSpawnAnim *anim, float32_t delta_time);

namespace {
    const float32_t mushroom_speed   = 60.0f;
    const float32_t mushroom_gravity = 240.0f;

    constexpr float32_t star_x_speed = 80.0f;  // 100.0f;
    constexpr float32_t star_gravity = 220.0f; // 100.0f;
    constexpr float32_t star_bounce_power = 150.0f;

    Sprite  sprite_mushroom;
    AnimSet star_anim_set;
    AnimSet plant_anim_set;
}

void init_power_up_common_data(void) {
    sprite_mushroom = global_data::get_sprite(SPRITE_MUSHROOM);

    init_anim_set(&star_anim_set);
    next_anim_frame(&star_anim_set, global_data::get_sprite(SPRITE_STAR_1), 0.25f);
    next_anim_frame(&star_anim_set, global_data::get_sprite(SPRITE_STAR_2), 0.25f);
    next_anim_frame(&star_anim_set, global_data::get_sprite(SPRITE_STAR_3), 0.25f);
    next_anim_frame(&star_anim_set, global_data::get_sprite(SPRITE_STAR_4), 0.25f);

    init_anim_set(&plant_anim_set);
    next_anim_frame(&plant_anim_set, global_data::get_sprite(SPRITE_FIREPLANT),   0.6f);
    next_anim_frame(&plant_anim_set, global_data::get_sprite(SPRITE_FIREPLANT_2), 0.2f);
    next_anim_frame(&plant_anim_set, global_data::get_sprite(SPRITE_FIREPLANT_3), 0.2f);
    next_anim_frame(&plant_anim_set, global_data::get_sprite(SPRITE_FIREPLANT_4), 0.2f);
    next_anim_frame(&plant_anim_set, global_data::get_sprite(SPRITE_FIREPLANT_3), 0.2f);
    next_anim_frame(&plant_anim_set, global_data::get_sprite(SPRITE_FIREPLANT_2), 0.2f);
}

ENTITY_SERIALIZE_PROC(Mushroom) {
    EntitySaveData es_data = { };
    es_data.add_int32("position",   self_base->position.e, 2);
    es_data.add_int32("direction", &self_base->as<Mushroom>()->direction, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Mushroom) {
    vec2i   position;
    int32_t direction = 0;
    if(!es_data->try_get_int32("position", position.e, 2)) {
        return NULL;
    }
    es_data->try_get_int32("direction", &direction);

    auto mushroom = spawn_mushroom(level, position);
    if(direction) {
        mushroom->direction = direction;
    }
    return mushroom;
}

ENTITY_UPDATE_PROC(update_mushroom);
ENTITY_RENDER_PROC(render_mushroom);
HIT_CALLBACK_PROC(mushroom_hit_callback);

Mushroom *spawn_mushroom(Level *level, vec2i position) {
    auto mushroom = create_entity_m(level, Mushroom);
    mushroom->position = position;
    mushroom->update_proc = update_mushroom;
    mushroom->render_proc = render_mushroom;
    mushroom->entity_flags |= E_FLAG_IS_GAMEPLAY_ENTITY;

    auto collider = &mushroom->collider;
    collider->size   = { 14, 14 };
    collider->offset = { 1,  1  };
    mushroom->has_collider = collider;

    auto move_data = &mushroom->move_data;
    move_data->hit_callback = mushroom_hit_callback;
    mushroom->has_move_data = move_data;

    mushroom->spawn_anim.finished = true;
    mushroom->direction = 1;

    return mushroom;
}

Mushroom *spawn_mushroom(Level *level, Tile *tile) {
    auto mushroom = spawn_mushroom(level, tile->get_ti().position);
    setup_spawn_anim(&mushroom->spawn_anim, tile, &mushroom->collider);
    return mushroom;
}

ENTITY_UPDATE_PROC(update_mushroom) {
    auto mushroom = self_base->as<Mushroom>();

    if(!mushroom->spawn_anim.finished) {
        if(update_spawn_anim(&mushroom->spawn_anim, delta_time)) {
            mushroom->position.y = mushroom->spawn_anim.y_target;
        }
        return;
    }

    mushroom->move_data.speed.x = mushroom->direction * mushroom_speed;
    mushroom->move_data.speed.y -= mushroom_gravity * delta_time;

    do_move(mushroom, mushroom->has_move_data, delta_time);

    if(mushroom->change_dir_after_move) {
        mushroom->change_dir_after_move = false;
        mushroom->direction *= -1;
    }
}

ENTITY_RENDER_PROC(render_mushroom) {
    auto mushroom = self_base->as<Mushroom>();

    int32_t z_pos = Z_MUSHROOM_POS;
    vec2i position = mushroom->position;
    if(!mushroom->spawn_anim.finished) {
        position.y = mushroom->spawn_anim.y_current;
        z_pos = Z_POWERUP_SPAWN_POS;
    }

    render::r_sprite(position, z_pos, TILE_SIZE_2, sprite_mushroom, color::white);
}

HIT_CALLBACK_PROC(mushroom_hit_callback) {
    auto mushroom = self_base->as<Mushroom>();

    if(collision.axis == X_AXIS && collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT) {
        mushroom->change_dir_after_move = true;
        return hit_callback_result(true);
    }

    return hit_callback_result(false);
}

ENTITY_UPDATE_PROC(update_fire_plant);
ENTITY_RENDER_PROC(render_fire_plant);

FirePlant *spawn_fire_plant(Level *level, vec2i position) {
    auto fire_plant = create_entity_m(level, FirePlant);
    fire_plant->position = position;
    fire_plant->update_proc = update_fire_plant;
    fire_plant->render_proc = render_fire_plant;
    fire_plant->entity_flags |= E_FLAG_IS_GAMEPLAY_ENTITY;

    auto collider = &fire_plant->collider;
    collider->size   = { 14, 14 };
    collider->offset = { 1,  1  };
    fire_plant->has_collider = collider;

    fire_plant->anim_player = init_anim_player(&plant_anim_set);
    fire_plant->spawn_anim.finished = true;

    return fire_plant;
}

FirePlant *spawn_fire_plant(Level *level, Tile *tile) {
    auto fire_plant = spawn_fire_plant(level, tile->get_ti().position);
    setup_spawn_anim(&fire_plant->spawn_anim, tile, &fire_plant->collider);
    return fire_plant;
}

ENTITY_UPDATE_PROC(update_fire_plant) {
    auto fire_plant = self_base->as<FirePlant>();

    if(!fire_plant->spawn_anim.finished) {
        if(update_spawn_anim(&fire_plant->spawn_anim, delta_time)) {
            fire_plant->position.y = fire_plant->spawn_anim.y_target;
        }
        return;
    }

    update_anim(&fire_plant->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_fire_plant) {
    auto fire_plant = self_base->as<FirePlant>();

    int32_t z_pos = Z_FIREPLANT_POS;
    vec2i position = fire_plant->position;
    if(!fire_plant->spawn_anim.finished) {
        position.y = fire_plant->spawn_anim.y_current;
        z_pos = Z_POWERUP_SPAWN_POS;
    }

    auto frame = get_current_frame(&fire_plant->anim_player);
    render::r_sprite(position, z_pos, frame->size(), frame->sprite, color::white);
}

ENTITY_UPDATE_PROC(update_star);
ENTITY_RENDER_PROC(render_star);
HIT_CALLBACK_PROC(star_hit_callback);

Star *spawn_star(Level *level, vec2i position) {
    auto star = create_entity_m(level, Star);
    star->position = position;
    star->update_proc = update_star;
    star->render_proc = render_star;
    star->entity_flags |= E_FLAG_IS_GAMEPLAY_ENTITY;

    star->direction = +1;

    auto collider = &star->collider;
    collider->size   = { 14, 14 };
    collider->offset = { 1,  1  };
    star->has_collider = collider;

    auto move_data = &star->move_data;
    move_data->speed.x = star_x_speed;
    move_data->speed.y = star_bounce_power;
    move_data->hit_callback = star_hit_callback;
    star->has_move_data = move_data;

    star->anim_player = init_anim_player(&star_anim_set);
    star->spawn_anim.finished = true;

    return star;
}

Star *spawn_star(Level *level, Tile *tile) {
    auto star = spawn_star(level, tile->get_ti().position);
    setup_spawn_anim(&star->spawn_anim, tile, &star->collider);
    return star;
}

ENTITY_UPDATE_PROC(update_star) {
    auto star = self_base->as<Star>();

    if(!star->spawn_anim.finished) {
        if(update_spawn_anim(&star->spawn_anim, delta_time)) {
            star->position.y = star->spawn_anim.y_target;
        }
        return;
    }

    star->move_data.speed.x = star->direction * star_x_speed;
    star->move_data.speed.y -= star_gravity * delta_time;
    
    do_move(star, &star->move_data, delta_time);

    if(star->bounce_after_move) {
        star->bounce_after_move = false;
        star->move_data.speed.y = star_bounce_power;
    }

    if(star->change_dir_after_move) {
        star->change_dir_after_move = false;
        star->direction *= -1.0f;
    }

    update_anim(&star->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_star) {
    auto star = self_base->as<Star>();
    auto frame = get_current_frame(&star->anim_player);

    int32_t z_pos = Z_STAR_POS;
    vec2i position = star->position;
    if(!star->spawn_anim.finished) {
        position.y = star->spawn_anim.y_current;
        z_pos = Z_POWERUP_SPAWN_POS;
    }

    render::r_sprite(position, z_pos, frame->size(), frame->sprite, color::white);
}

HIT_CALLBACK_PROC(star_hit_callback) {
    auto star = self_base->as<Star>();

    if(collision.axis == Y_AXIS && collision.unit == -1 && collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT) {
        star->bounce_after_move = true;
        return hit_callback_result(true);
    }

    if(collision.axis == X_AXIS && collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT) {
        star->change_dir_after_move = true;
        return hit_callback_result(true);
    }

    return hit_callback_result(false);
}

static
void setup_spawn_anim(PowerUpSpawnAnim *anim, Tile *spawner, Collider *power_up_collider) {
    assert(anim != NULL && spawner != NULL && power_up_collider != NULL);
    anim->finished = false;

    const auto spawner_ti = spawner->get_ti();
    anim->y_current = spawner_ti.position.y;
    anim->y_target = spawner_ti.position.y + TILE_SIZE - power_up_collider->offset.y;
}

static
bool update_spawn_anim(PowerUpSpawnAnim *anim, float32_t delta_time) {
    const float32_t spawn_anim_speed = 32.0f;

    if(anim->finished) {
        return false;
    }

    anim->y_current += spawn_anim_speed * delta_time;
    if(anim->y_current >= anim->y_target) {
        anim->y_current = anim->y_target;
        anim->finished = true;
        return true;
    }

    return false;
}