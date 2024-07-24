#include "entity.h"
#include "level.h"
#include "all_entities.h"

Entity *create_entity(struct Level *level, size_t size_of_entity, int32_t entity_type_id) {
    Entity *created = NULL;

    Entity *first_unused_of_type = level->entities_unused.first_of_type[entity_type_id];
    if(first_unused_of_type != NULL) {
        created = first_unused_of_type;
        entity_pool_remove(&level->entities_unused, first_unused_of_type, entity_type_id);
    } else {
        assert((level->memory_used + size_of_entity) <= level->memory_size);
        uint8_t *memory_ptr = level->memory + level->memory_used;
        created = (Entity *)memory_ptr;
        level->memory_used += size_of_entity;
    }

    assert(created != NULL);
    zero_memory(created, size_of_entity);
    entity_pool_push(&level->entities, created, entity_type_id);

    created->level = level;
    created->in_use = true;
    created->entity_type_id = entity_type_id;
    created->size_of_entity = size_of_entity;
    created->unique_id = level->next_entity_id++;

    return created;
}

void delete_entity(Entity *entity) {
    bool in_deletion_list = false;
    for(Entity *e : *entity->level->to_be_deleted) {
        if(entity == e) {
            in_deletion_list = true;
            break;
        }
    }

    if(!entity->deleted && !in_deletion_list) {
        entity->deleted = true;
        entity->level->to_be_deleted->push_back(entity);
    }
}

void delete_entity_imm(Entity *entity) {
    if(entity->delete_proc != NULL) {
        entity->delete_proc(entity);
    }

    auto level = entity->level;
    entity_pool_remove(&level->entities, entity, entity->entity_type_id);
    entity_pool_push(&level->entities_unused, entity, entity->entity_type_id);
    entity->in_use = false;
}

void entity_pool_push(EntityPool *pool, Entity *entity, int32_t entity_type_id) {
    assert(entity->next == NULL && entity->next_of_type == NULL);

    pool->count += 1;
    pool->count_of_type[entity_type_id] += 1;

    if(pool->first == NULL) {
        entity->next = NULL;
        pool->first = entity;
        pool->last  = entity;
    } else {
        pool->last->next = entity;
        pool->last = entity;
        entity->next = NULL;
    }

    Entity **first_of_type = &pool->first_of_type[entity_type_id];
    Entity **last_of_type  = &pool->last_of_type [entity_type_id];
    if(*first_of_type == NULL) {
        entity->next_of_type = NULL;
        (*first_of_type) = entity;
        (*last_of_type)  = entity;
    } else {
        (*last_of_type)->next_of_type = entity;
        (*last_of_type) = entity;
        entity->next_of_type = NULL;
    }
}

void entity_pool_remove(EntityPool *pool, Entity *entity, int32_t entity_type_id) {
    assert(entity == pool->last || (entity != pool->last && entity->next != NULL));
    assert(entity == pool->last_of_type[entity_type_id] || (entity != pool->last_of_type[entity_type_id] && entity->next_of_type != NULL));

    pool->count -= 1;
    pool->count_of_type[entity_type_id] -= 1;

    Entity *prev = NULL; // Find previous entity in pool
    for(Entity *e_iter = pool->first; e_iter != entity && e_iter != NULL; e_iter = e_iter->next) {
        if(e_iter->next == entity) {
            prev = e_iter;
            prev->next = entity->next;
            break;
        }
    }

    if(pool->first == entity && pool->last == entity) {
        pool->first = NULL;
        pool->last  = NULL;
    } else if(pool->first == entity) {
        pool->first = entity->next;
    } else if(pool->last == entity) {
        pool->last = prev;
    }

    entity->next = NULL;

    Entity **first_of_type = &pool->first_of_type[entity_type_id];
    Entity **last_of_type  = &pool->last_of_type [entity_type_id];

    Entity *prev_of_type = NULL;
    for(Entity *e_iter = *first_of_type; e_iter != entity && e_iter != NULL; e_iter = e_iter->next_of_type) {
        if(e_iter->next_of_type == entity) {
            prev_of_type = e_iter;
            prev_of_type->next_of_type = entity->next_of_type;
            break;
        }
    }

    if(*first_of_type == entity && *last_of_type == entity) {
        *first_of_type = NULL;
        *last_of_type  = NULL;
    } else if(*first_of_type == entity) {
        *first_of_type = entity->next_of_type;
    } else if(*last_of_type == entity) {
        *last_of_type = prev_of_type;
    }

    entity->next_of_type = NULL;
}

Entity *get_entity_by_unique_id(struct Level *level, uint32_t unique_id) {
    Entity *found = NULL;
    for_every_entity(level, e) {
        if(is_entity_used(e) || e->unique_id != unique_id) {
            continue;
        }
        found = e;
        break;
    }
    return found;
}

Entity *get_entity_by_unique_id(struct Level *level, uint32_t unique_id, int32_t entity_type_id) {
    Entity *found = NULL;
    for_entity_type_id(level, entity_type_id, e) {
        if(!is_entity_used(e))        { continue; }
        if(e->unique_id != unique_id) { continue; }
        found = e;
        break;
    }
    return found;
}

bool aabb(vec2i a_pos, vec2i a_size, vec2i b_pos, vec2i b_size) {
    return !(a_pos.x >= (b_pos.x + b_size.x) || (a_pos.x + a_size.x) <= b_pos.x || a_pos.y >= (b_pos.y + b_size.y) || (a_pos.y + a_size.y) <= b_pos.y);
}

bool intersect(Entity *entity_a, Entity *entity_b) {
    if(entity_a->has_collider == NULL || entity_b->has_collider == NULL) {
        return false;
    }
    return aabb(entity_a->position + entity_a->has_collider->offset, entity_a->has_collider->size, entity_b->position + entity_b->has_collider->offset, entity_b->has_collider->size);
}

bool is_colliding(Level *level, vec2i position, Collider *collider, FindCollisionsOpts opts, vec2i offset) {
    auto do_checks = [&] (Entity *check_e) -> bool {
        if(check_e->deleted || check_e->has_collider == NULL || check_e->unique_id == opts.id_to_ignore || (check_e->entity_flags & opts.entity_flags) != opts.entity_flags) {
            return false;
        }
        return aabb(position + collider->offset + offset, collider->size, offset_position(check_e, check_e->has_collider), check_e->has_collider->size);
    };

    if(opts.entity_type_id == -1) {
        for_every_entity(level, check_e) {
            if(do_checks(check_e) == true) {
                return true;
            }
        }
    } else {
        for(Entity *check_e = level->entities.first_of_type[opts.entity_type_id]; check_e != NULL; check_e = check_e->next_of_type) {
            if(do_checks(check_e) == true) {
                return true;
            }
        }
    }
    return false;
}

std::vector<Entity *> find_collisions(Level *level, vec2i position, Collider *collider, FindCollisionsOpts opts, vec2i offset) {
    auto do_checks = [&] (Entity *check_e) -> bool {
        if(check_e->deleted || !check_e->in_use || check_e->has_collider == NULL || check_e->unique_id == opts.id_to_ignore || (check_e->entity_flags & opts.entity_flags) != opts.entity_flags) {
            return false;
        }
        return aabb(position + collider->offset + offset, collider->size, offset_position(check_e, check_e->has_collider), check_e->has_collider->size);
    };

    auto found = std::vector<Entity *>();
    if(opts.entity_type_id == -1) {
        for_every_entity(level, check_e) {
            if(do_checks(check_e) == true) {
                found.push_back(check_e);
            }
        }
    } else {
        for(Entity *check_e = level->entities.first_of_type[opts.entity_type_id]; check_e != NULL; check_e = check_e->next_of_type) {
            if(do_checks(check_e) == true) {
                found.push_back(check_e);
            }
        }
    }
    return found;
}

void gather_collisions(std::vector<Entity *> *destination, std::vector<Entity *> found) {
    destination->insert(destination->end(), found.begin(), found.end());
}

HitCallbackResult hit_callback_result(bool stop_move) {
    HitCallbackResult result = { };
    result.stop_move = stop_move;
    return result;
}

void zero_move_speed(MoveData *move_data) {
    move_data->speed = { 0, 0 };
    move_data->reminder = { 0, 0 };
}

static bool 
check_collision_with_offset(Entity *a_entity, Entity *b_entity, vec2i a_entity_offset) {
    if(a_entity->has_collider == NULL || b_entity->has_collider == NULL) {
        return false; // @note Keep?
    }

    return aabb(a_entity->position + a_entity->has_collider->offset + a_entity_offset, a_entity->has_collider->size,
                b_entity->position + b_entity->has_collider->offset,                   b_entity->has_collider->size);
}

static bool
is_entity_above_or_eq(Entity *e, Entity *other) {
    return e->position.y + e->has_collider->offset.y >= other->position.y + other->has_collider->offset.y + other->has_collider->size.y;
}

static bool
neighbouring_tile_doesnt_block(Tile *tile, vec2i offset) {
    Tile *other = tile->tilemap->get_tile(tile->tile_x + offset.x, tile->tile_y + offset.y);
    if(other == NULL) return true;

    if(other->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE) return true;

    return false;
}

static // @todo
void _move(int32_t distance, Entity *e, float32_t delta_time, EAxis axis) {
    assert(axis == X_AXIS || axis == Y_AXIS);

    vec2i vec = axis == X_AXIS ? vec2i { 1, 0 } : vec2i { 0, 1 };

    auto level     = e->level;
    auto collider  = e->has_collider;
    auto move_data = e->has_move_data;

    bool      stop_move = false;

    while(distance) {
        int32_t unit = sign(distance);

        FindCollisionsOpts opts;
        opts.id_to_ignore = e->unique_id;
        opts.entity_flags = E_FLAG_IS_GAMEPLAY_ENTITY;

        if(!(e->entity_flags & E_FLAG_DOES_NOT_COLLIDE_WITH_TILES)) {
            for_entity_type(level, Tilemap, tilemap) {
                if(tilemap->deleted) continue;

                auto tiles = find_collisions(e->position, e->has_collider, tilemap, vec * unit);
                for(auto tile : tiles) {

                    // Invisible tiles do not stop movement
                    if(tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT && !(tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE)) {
                        if(axis == Y_AXIS && unit == +1) {
                            const int32_t diff_r = (tile->get_ti().position.x + TILE_SIZE) - (e->position.x + e->has_collider->offset.x);
                            const int32_t diff_l = (e->position.x + e->has_collider->offset.x + e->has_collider->size.x) - tile->get_ti().position.x;
                            bool should_be_pushed_r = diff_r <= 2 && neighbouring_tile_doesnt_block(tile, { +1, 0 });
                            bool should_be_pushed_l = diff_l <= 2 && neighbouring_tile_doesnt_block(tile, { -1, 0 });

                            // If hitting tile from below -> maybe push player left or right if hit the edge of tile

                            if(should_be_pushed_l) {
                                e->position.x -= diff_l;
                            } else if(should_be_pushed_r) {
                                e->position.x += diff_r;
                            } else {
                                stop_move = true;
                            }
                        } else if(axis == X_AXIS) {
                            const int32_t diff = (tile->get_ti().position.y + TILE_SIZE) - (e->position.y + e->has_collider->offset.y);
                            bool empty_above_tile = neighbouring_tile_doesnt_block(tile, { 0, +1 });

                            // If hitting tile from left or right -> maybe push player up so can make it through the gap

                            if(e->entity_type_id == entity_type_id(Player) && diff <= 2 && empty_above_tile && e->has_move_data->is_grounded == false && e->has_move_data->speed.y <= 0.0f) {
                                e->position.y += diff;
                            } else {
                                stop_move = true;
                            }
                        } else {
                            stop_move = true;
                        }
                    }

                    if(move_data->hit_callback) {
                        CollisionInfo c_info;
                        c_info.axis = axis;
                        c_info.unit = unit;
                        c_info.with_entity = NULL;
                        c_info.with_tile = tile;

                        auto callback_result = move_data->hit_callback(e, c_info);
                        if(callback_result.stop_move) {
                            stop_move = true;
                        }
                    }
                }
            }
        }

        auto collided = find_collisions(level, e->position, collider, opts, vec * unit);
        for(auto other : collided) {
            if(other->entity_flags & E_FLAG_ALWAYS_BLOCKS_MOVEMENT) {
                stop_move = true;
            }

            if(other->entity_flags & E_FLAG_BLOCKS_MOVEMENT_FROM_ABOVE_ONLY) {
                if(axis == EAxis::Y_AXIS && unit == -1) {
                    const bool was_above_before_move = is_entity_above_or_eq(e, other); // @cleanup
                    if(was_above_before_move) {
                        stop_move = true;
                    }
                }
            }

            if(move_data->hit_callback) {
                CollisionInfo c_info;
                c_info.axis = axis;
                c_info.unit = unit;
                c_info.with_entity = other;
                c_info.with_tile = NULL;

                auto callback_result = move_data->hit_callback(e, c_info);
                if(callback_result.stop_move) {
                    stop_move = true;
                }
            }
        }
        
        if(stop_move) {
            break;
        }

        e->position.x += vec.x * unit;
        e->position.y += vec.y * unit;
        distance -= unit;
    }

    if(stop_move) {
        if(vec.x) { 
            move_data->speed.x    = 0.0f;
            move_data->reminder.x = 0.0f;
        }
        if(vec.y) { 
            move_data->speed.y    = 0.0f;
            move_data->reminder.y = 0.0f; 
        }
    }
}

void do_move(Entity *entity, MoveData *move_data, float32_t delta_time) {
    assert(entity != NULL && move_data != NULL && entity->has_move_data == move_data);

    float32_t to_move_x = move_data->speed.x * delta_time + move_data->reminder.x;
    float32_t to_move_y = move_data->speed.y * delta_time + move_data->reminder.y;

    move_data->reminder.x = to_move_x - (float32_t)((int32_t)to_move_x);
    move_data->reminder.y = to_move_y - (float32_t)((int32_t)to_move_y);

    int32_t distance_x = (int32_t)to_move_x;
    int32_t distance_y = (int32_t)to_move_y;

    if(entity->has_collider == NULL) {
        entity->position.x += distance_x;
        entity->position.y += distance_y;

        move_data->is_grounded_prev = false;
        move_data->is_grounded      = false;
    } else {

        // Call hit callback before moving, for tiles and entities

        if(!(entity->entity_flags & E_FLAG_DOES_NOT_COLLIDE_WITH_TILES)) {
            for_entity_type(entity->level, Tilemap, tilemap) {
                if(tilemap->deleted) continue;

                auto tiles = find_collisions(entity->position, entity->has_collider, tilemap);
                for(auto tile : tiles) {
                    if(move_data->hit_callback) {
                        CollisionInfo c_info;
                        c_info.axis = EAxis::NO_AXIS;
                        c_info.unit = 0;
                        c_info.with_entity = NULL;
                        c_info.with_tile = tile;
                        move_data->hit_callback(entity, c_info);
                    }
                }
            }
        }

        FindCollisionsOpts _opts;
        _opts.id_to_ignore = entity->unique_id;
        _opts.entity_flags = E_FLAG_IS_GAMEPLAY_ENTITY;

        auto collided = find_collisions(entity->level, entity->position, entity->has_collider, _opts);
        for(auto other : collided) {
            if(move_data->hit_callback) {
                CollisionInfo c_info;
                c_info.axis = NO_AXIS;
                c_info.unit = 0;
                c_info.with_entity = other;
                c_info.with_tile = NULL;
                move_data->hit_callback(entity, c_info);
            }
        }

        _move(distance_y, entity, delta_time, Y_AXIS);
        _move(distance_x, entity, delta_time, X_AXIS);

        FindCollisionsOpts opts;
        opts.id_to_ignore = entity->unique_id;
        opts.entity_flags = E_FLAG_IS_GAMEPLAY_ENTITY;
        
        // @todo Better is_grounded check
        move_data->is_grounded_prev = move_data->is_grounded;
        move_data->is_grounded = false;
        if(move_data->is_grounded == false) {
            for_entity_type(entity->level, Tilemap, tilemap) {
                if(is_colliding_with_any_tile(entity->position, entity->has_collider, tilemap, { 0, -1 }, { 0, TILE_FLAG_IS_INVISIBLE })) {
                    move_data->is_grounded = true;
                    break;
                }
            }

            auto collided = find_collisions(entity->level, entity->position, entity->has_collider, opts, { 0, -1 });
            for(auto other : collided) {
                if(other->entity_flags & E_FLAG_ALWAYS_BLOCKS_MOVEMENT || (other->entity_flags & E_FLAG_BLOCKS_MOVEMENT_FROM_ABOVE_ONLY && aabb(entity->position + entity->has_collider->offset + vec2i{ 0, 1 }, entity->has_collider->size, other->position + other->has_collider->offset, other->has_collider->size) == false)) {
                    move_data->is_grounded = true;
                    break;
                }
            }
        }
    }
}