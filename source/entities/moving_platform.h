#ifndef _MOVING_PLATFORM_H
#define _MOVING_PLATFORM_H

#include "common.h"
#include "level.h"
#include "save_data.h"

constexpr int32_t moving_platform_x_segments    = 4; // how much 8x8 segments by default
constexpr int32_t moving_platform_segment_size  = 8;


ENTITY_SERIALIZE_PROC(MovingPlatform);
ENTITY_DESERIALIZE_PROC(MovingPlatform);

// @note Could merge into one Entity type.

struct MovingPlatform : Entity {
    Collider collider;
    MoveData move_data;
    int32_t y_start;
    int32_t y_end;
    int32_t num_of_segments;
    float32_t seconds_per_distance;

    void set_num_of_segments(int32_t num_of_segments);
};

MovingPlatform *spawn_moving_platform(Level *level, vec2i position, int32_t y_start, int32_t y_end);

ENTITY_SERIALIZE_PROC(SideMovingPlatform);
ENTITY_DESERIALIZE_PROC(SideMovingPlatform);

struct SideMovingPlatform : Entity {
    Collider  collider;
    int32_t   x_distance;
    int32_t   num_of_segments;
    float32_t seconds_per_distance;
    float32_t time_accumulator;

    void set_num_of_segments(int32_t num_of_segments);
    void recalculate(void); // i.e. after setting x_distance
};

SideMovingPlatform *spawn_side_moving_platform(Level *level, vec2i position, int32_t x_distance);
int32_t calc_side_moving_platform_offset(const SideMovingPlatform *platform);

void _render_debug_moving_platform_metrics(MovingPlatform *platform, int32_t z_pos);
void _render_debug_side_moving_platform_metrics(SideMovingPlatform *platform, int32_t z_pos);

#endif /* _MOVING_PLATFORM_H */