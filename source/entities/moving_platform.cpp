#include "moving_platform.h"
#include "renderer.h"
#include "data.h"
#include "all_entities.h"

namespace {
    // Default
    constexpr float32_t moving_platform_seconds_per_distance = 1.0f;
}

/* --- MovingPlatform --- */

ENTITY_SERIALIZE_PROC(MovingPlatform) {
    auto platform = self_base->as<MovingPlatform>();

    EntitySaveData es_data = { };
    es_data.add_int32("position",  platform->position.e, 2);
    es_data.add_int32("y_start",  &platform->y_start,    1);
    es_data.add_int32("y_end",    &platform->y_end,      1);
    es_data.add_int32("num_of_segments", &platform->num_of_segments, 1);
    es_data.add_float32("seconds_per_distance", &platform->seconds_per_distance, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(MovingPlatform) {
    vec2i position;
    int32_t y_start, y_end;

    if(!es_data->try_get_int32("position", position.e, 2) ||
       !es_data->try_get_int32("y_start", &y_start,    1) ||
       !es_data->try_get_int32("y_end",   &y_end,      1)) {
        return NULL;
    }
    
    auto platform = spawn_moving_platform(level, position, y_start, y_end);

    int32_t segments = 0;
    if(es_data->try_get_int32("num_of_segments", &segments) && segments > 0) {
        platform->set_num_of_segments(segments);
    }

    float32_t seconds_per_distance = 0.0f;
    if(es_data->try_get_float32("seconds_per_distance", &seconds_per_distance) && seconds_per_distance > 0.0f) {
        platform->seconds_per_distance = seconds_per_distance;
    }

    return platform;
}

ENTITY_UPDATE_PROC(update_moving_platform);
ENTITY_RENDER_PROC(render_moving_platform);
HIT_CALLBACK_PROC(moving_platform_hit_callback);

void MovingPlatform::set_num_of_segments(int32_t num_of_segments) {
    this->num_of_segments = num_of_segments;

    const vec2i platform_size = { num_of_segments * moving_platform_segment_size, moving_platform_segment_size };
    auto collider = &this->collider;
    collider->size = platform_size;
    collider->offset = -(platform_size / 2);
    this->has_collider = collider;
}

MovingPlatform *spawn_moving_platform(Level *level, vec2i position, int32_t y_start, int32_t y_end) {
    assert(y_start < y_end, "");
    if(y_start >= y_end) {
        return NULL;
    }

    auto platform = create_entity_m(level, MovingPlatform);
    platform->update_proc = update_moving_platform;
    platform->render_proc = render_moving_platform;
    platform->position = position;
    platform->y_start = y_start;
    platform->y_end = y_end;

    set_entity_flag(platform, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(platform, E_FLAG_BLOCKS_MOVEMENT_FROM_ABOVE_ONLY);
    set_entity_flag(platform, E_FLAG_DOES_NOT_COLLIDE_WITH_TILES);

    platform->set_num_of_segments(moving_platform_x_segments);
    platform->seconds_per_distance = moving_platform_seconds_per_distance;

    platform->move_data = { };
    platform->move_data.hit_callback = moving_platform_hit_callback;
    platform->has_move_data = &platform->move_data;

    return platform;
}

ENTITY_UPDATE_PROC(update_moving_platform) {
    auto platform = self_base->as<MovingPlatform>();

    platform->has_move_data->speed.y = absolute_value(platform->y_end - platform->y_start) / platform->seconds_per_distance;
    do_move(platform, platform->has_move_data, delta_time);

    while(platform->position.y > platform->y_end) {
        const int32_t overshoot = platform->position.y - platform->y_end;
        platform->position.y = platform->y_start + overshoot;
    }
}

ENTITY_RENDER_PROC(render_moving_platform) {
    auto sprite = global_data::get_sprite(SPRITE_PLATFORM_PIECE);
    auto platform = self_base->as<MovingPlatform>();
    
    const int32_t y_pos = platform->position.y + platform->collider.offset.y;
    const int32_t x_pos = platform->position.x + platform->collider.offset.x;

    for(int32_t idx = 0; idx < platform->num_of_segments; ++idx) {
        render::r_sprite({ x_pos + sprite.width * idx, y_pos }, Z_MOVING_PLATFORM_POS, { sprite.width, sprite.height }, sprite, color::white);
    }
}

HIT_CALLBACK_PROC(moving_platform_hit_callback) {
    if(collision.with_entity == NULL || collision.unit != +1 || collision.axis != Y_AXIS) {
        return hit_callback_result(false);
    }

    auto platform = self_base->as<MovingPlatform>();
    auto entity   = collision.with_entity;

    switch(entity->entity_type_id) {
        default: {
            return hit_callback_result(false);
        };

        // Move-able entities (must have colliders and move_data's)
        case entity_type_id(Player):
        case entity_type_id(Goomba):
        case entity_type_id(Koopa): { } break;
    }

    if(entity->has_move_data->speed.y <= 0.0f && (entity->position.y + entity->has_collider->offset.y) >= (platform->position.y + platform->has_collider->offset.y + platform->has_collider->size.y / 2)) {
        entity->has_move_data->speed.y = 0;
        entity->position.y = platform->position.y + collision.unit + platform->collider.size.y + platform->collider.offset.y;
        entity->position.y -= entity->has_collider->offset.y;
    }

    return hit_callback_result(false);
}

/* --- SideMovingPlatform --- */

ENTITY_SERIALIZE_PROC(SideMovingPlatform) {
    auto platform = self_base->as<SideMovingPlatform>();

    EntitySaveData es_data = { };
    es_data.add_int32("position",  platform->position.e, 2);
    es_data.add_int32("x_distance", &platform->x_distance, 1);
    es_data.add_int32("num_of_segments", &platform->num_of_segments, 1);
    es_data.add_float32("seconds_per_distance", &platform->seconds_per_distance, 1);
    es_data.add_float32("time_accumulator_start", &platform->time_accumulator, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(SideMovingPlatform) {
    vec2i   position;
    int32_t x_distance;

    if(!es_data->try_get_int32("position", position.e, 2) ||
       !es_data->try_get_int32("x_distance", &x_distance, 1)) {
        return NULL;
    }

    auto platform = spawn_side_moving_platform(level, position, x_distance);

    int32_t segments = 0;
    if(es_data->try_get_int32("num_of_segments", &segments) && segments > 0) {
        platform->set_num_of_segments(segments);
    }

    float32_t seconds_per_distance = 0.0f;
    if(es_data->try_get_float32("seconds_per_distance", &seconds_per_distance) && seconds_per_distance > 0.0f) {
        platform->seconds_per_distance = seconds_per_distance;
    }

    float32_t time_accumulator_start = 0.0f;
    if(es_data->try_get_float32("time_accumulator_start", &time_accumulator_start) && time_accumulator_start > 0.0f) {
        platform->time_accumulator = time_accumulator_start;
        platform->collider.offset.x = calc_side_moving_platform_offset(platform);
    }

    return platform;
}

ENTITY_UPDATE_PROC(update_side_moving_platform);
ENTITY_RENDER_PROC(render_side_moving_platform);
HIT_CALLBACK_PROC(side_moving_platform_hit_callback);

void SideMovingPlatform::set_num_of_segments(int32_t num_of_segments) {
    this->num_of_segments = num_of_segments;

    const vec2i platform_size = { num_of_segments * moving_platform_segment_size, moving_platform_segment_size };
    auto collider = &this->collider;
    collider->size = platform_size;
    collider->offset = { 0, 0 };
    this->has_collider = collider;
    this->x_distance = max_value(this->x_distance, platform_size.x);
}

void SideMovingPlatform::recalculate(void) {
    this->set_num_of_segments(this->num_of_segments);
}

SideMovingPlatform *spawn_side_moving_platform(Level *level, vec2i position, int32_t x_distance) {
    if(x_distance < 0) {
        return NULL;
    }

    auto platform = create_entity_m(level, SideMovingPlatform);
    platform->update_proc = update_side_moving_platform;
    platform->render_proc = render_side_moving_platform;
    platform->position = position;

    set_entity_flag(platform, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(platform, E_FLAG_BLOCKS_MOVEMENT_FROM_ABOVE_ONLY);

    assert(x_distance >= platform->collider.size.x);
    platform->x_distance = x_distance;
    platform->set_num_of_segments(moving_platform_x_segments);
    platform->seconds_per_distance = moving_platform_seconds_per_distance;

    return platform;
}

inline int32_t calc_side_moving_platform_x_right(const SideMovingPlatform *platform) {
    return platform->position.x + platform->x_distance;
}

inline int32_t calc_side_moving_platform_offset(const SideMovingPlatform *platform) {
    const float32_t perc_now = sinf((platform->time_accumulator * M_PI) / platform->seconds_per_distance - M_PI * 0.5f) * 0.5f + 0.5f;
    return (int32_t)roundf(perc_now * (platform->x_distance - platform->collider.size.x));
}

ENTITY_UPDATE_PROC(update_side_moving_platform) {
    auto platform = self_base->as<SideMovingPlatform>();

    bool carry_player = false;
    auto player = get_player(platform->level);
    if(player) {
        if(player->move_data.is_grounded == true && aabb(player->position + vec2i{ 0, -1 } + player->has_collider->offset, player->has_collider->size, platform->position + platform->collider.offset, platform->collider.size) && !aabb(player->position + vec2i{ 0, 1 } + player->has_collider->offset, player->has_collider->size, platform->position + platform->collider.offset, platform->collider.size)) {
            carry_player = true;
        }
    }

    const int32_t old_offset_x = platform->collider.offset.x;
    platform->collider.offset.x = calc_side_moving_platform_offset(platform);

    const int32_t diff = platform->collider.offset.x - old_offset_x;
    if(diff != 0) {
        if(carry_player) { // @todo For now carries player through walls and stuff
            player->position.x += diff;
        }
    }

    platform->time_accumulator += platform->level->delta_time;
    if(platform->time_accumulator > platform->seconds_per_distance * 2.0f) {
        platform->time_accumulator -= platform->seconds_per_distance * 2.0f;
    }
}

ENTITY_RENDER_PROC(render_side_moving_platform) {
    auto sprite = global_data::get_sprite(SPRITE_PLATFORM_PIECE);
    auto platform = self_base->as<SideMovingPlatform>();
    
    const int32_t y_pos = platform->position.y + platform->collider.offset.y;
    const int32_t x_pos = platform->position.x + platform->collider.offset.x;

    for(int32_t idx = 0; idx < platform->num_of_segments; ++idx) {
        render::r_sprite({ x_pos + sprite.width * idx, y_pos }, Z_MOVING_PLATFORM_POS, { sprite.width, sprite.height }, sprite, color::white);
    }
}

void _render_debug_moving_platform_metrics(MovingPlatform *platform, int32_t z_pos) {
    const vec4    color    = color::cyan;
    const int32_t x_center = platform->position.x;

    render::r_set_line_width(2.0f);
    render::r_line({ x_center, platform->y_start }, { x_center, platform->y_end }, z_pos, color);
    render::r_line_quad(platform->position + platform->collider.offset, z_pos, platform->collider.size, color);
}

void _render_debug_side_moving_platform_metrics(SideMovingPlatform *platform, int32_t z_pos) {
    const vec4    color    = { 0.2f, 0.6f, 0.1f, 1.0f };
    const int32_t y_center = platform->position.y + platform->collider.offset.y + platform->collider.size.y;
    
    render::r_set_line_width(2.0f);
    render::r_line({ platform->position.x, y_center }, { calc_side_moving_platform_x_right(platform), y_center }, z_pos, color);
    render::r_line_quad(platform->position + platform->collider.offset, z_pos, platform->collider.size, color);
}