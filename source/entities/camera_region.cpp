#include "camera_region.h"

ENTITY_SERIALIZE_PROC(CameraRegion) {
    auto region = self_base->as<CameraRegion>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", region->position.e, 2);
    es_data.add_int32("x_tiles_width", &region->x_tiles_width, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(CameraRegion) {
    vec2i   position;
    int32_t x_tiles_width;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_int32("x_tiles_width", &x_tiles_width, 1)) {
        return NULL;
    }

    return spawn_camera_region(level, position.x, position.y, x_tiles_width);
}

CameraRegion *spawn_camera_region(Level *level, int32_t x_origin, int32_t y_origin, int32_t x_tiles_width) {
    auto region = create_entity_m(level, CameraRegion);
    region->position = { x_origin, y_origin };
    region->x_tiles_width = x_tiles_width;
    region->x_width = x_tiles_width * TILE_SIZE;
    return region;
}