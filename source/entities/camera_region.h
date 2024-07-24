#ifndef _CAMERA_REGION_H
#define _CAMERA_REGION_H

#include "common.h"
#include "level.h"
#include "save_data.h"

ENTITY_SERIALIZE_PROC(CameraRegion);
ENTITY_DESERIALIZE_PROC(CameraRegion);

struct CameraRegion : Entity {
    int32_t x_tiles_width;
    int32_t x_width;

    static constexpr int32_t const_height_in_tiles  = 15;
    static constexpr int32_t const_height_in_pixels = const_height_in_tiles * TILE_SIZE;
};

CameraRegion *spawn_camera_region(Level *level, int32_t x_origin, int32_t y_origin, int32_t x_tiles_width);

#endif /* _CAMERA_REGION_H */