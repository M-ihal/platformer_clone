#include "tilemap.h"
#include "data.h"
#include "all_entities.h"

namespace {
    constexpr float32_t hit_anim_time = 0.1f;
    constexpr int32_t   hit_anim_distance = 10;
    constexpr Collider  tile_collider = { TILE_SIZE_2, { 0, 0 } };
    Sprite  sprites[TILE_SPRITE__COUNT];
    AnimSet anim_sets[TILE_ANIM__COUNT];
};

TileInfo Tile::get_ti(void) {
    assert(this->tilemap != NULL, "Invalid tile.");
    return tile_info(vec2i{ this->tile_x, this->tile_y } + this->tilemap->get_position_ti().tile);
}

void clear_tile(Tile *tile) {
    tile->is_not_empty = false;
}

static
void set_tile_sprite(Tile *tile, ETileSprite sprite) {
    tile->tile_desc.is_animated = false;
    tile->tile_desc.tile_sprite = sprite;
}

static
void set_tile_anim(Tile *tile, ETileAnim anim) {
    tile->tile_desc.is_animated = true;
    tile->tile_desc.tile_anim = anim;
    set_anim(&tile->anim_player, &anim_sets[anim]);
}

ENTITY_SERIALIZE_PROC(Tilemap) {
    auto  tilemap = self_base->as<Tilemap>();

    // Always serialize tile-aligned...
    vec2i position = tilemap->get_position_ti().position;

    EntitySaveData es_data = { };
    es_data.add_int32("position", position.e, 2);
    es_data.add_int32("x_tiles", &tilemap->x_tiles, 1);
    es_data.add_int32("y_tiles", &tilemap->y_tiles, 1);

    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            Tile *tile = tilemap->get_tile(x_tile, y_tile);
            if(tile == NULL) {
                continue;
            }

            char token[64];
            sprintf_s(token, array_count(token), "[%d,%d]", x_tile, y_tile);

            std::string tile_strings[] = {
                 std::to_string(tile->tile_desc.tile_flags),               // tile_flag
                 tile_drop_cstr[tile->tile_desc.tile_drop],                // tile_drop
                 std::to_string(tile->tile_desc.drops_left),               // drops_left
                 std::to_string((int32_t)tile->tile_desc.is_animated),     // is_animated
                 tile->tile_desc.is_animated ? tile_anim_cstr[tile->tile_desc.tile_anim] : tile_sprite_cstr[tile->tile_desc.tile_sprite], // tile_anim or tile_sprite
                 tile_sprite_cstr[tile->tile_desc.tile_sprite_after_drop], // tile_sprite_after_drop
                 (!(tile->tile_desc.tile_flags & TILE_FLAG_BREAKABLE) || tile->tile_desc.tile_break_anim < 0 || tile->tile_desc.tile_break_anim >= TILE_BREAK_ANIM__COUNT) ? "0" : tile_break_anim_cstr[tile->tile_desc.tile_break_anim], // tile_break_anim
            };

            es_data.add_string(token, tile_strings, array_count(tile_strings));
        }
    }

    return es_data;
}

ENTITY_DESERIALIZE_PROC(Tilemap) {
    vec2i   position;
    int32_t x_tiles;
    int32_t y_tiles;

    if(!es_data->try_get_int32("position", position.e, 2) ||
       !es_data->try_get_int32("x_tiles", &x_tiles, 1) ||
       !es_data->try_get_int32("y_tiles", &y_tiles, 1)) {
        return NULL;
    }
    
    // Always spawn with tile-aligned position...
    Tilemap *tilemap = spawn_tilemap(level, tile_info_at(position).tile, x_tiles, y_tiles);

    // Load tiles
    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            char buffer[64];
            sprintf_s(buffer, array_count(buffer), "[%d,%d]", x_tile, y_tile);
            
            std::vector<std::string> values;
            if(es_data->try_get_all(buffer, &values)) {
                TileDesc desc;

                desc.tile_flags  = std::stoi(values[0]);
                desc.tile_drop   = tile_drop_from_cstr(values[1].c_str());
                desc.drops_left  = std::stoi(values[2]);
                desc.is_animated = (bool)std::stoi(values[3]);
                if(desc.is_animated) {
                    desc.tile_anim = tile_anim_from_cstr(values[4].c_str());
                } else {
                    desc.tile_sprite = tile_sprite_from_cstr(values[4].c_str());
                }
                desc.tile_sprite_after_drop = tile_sprite_from_cstr(values[5].c_str());

                if(values.size() > 6) {
                    desc.tile_break_anim = tile_break_anim_from_cstr(values[6].c_str());
                } else {
                    desc.tile_break_anim = TILE_BREAK_ANIM_BRICK;
                }

                tilemap->set_tile(x_tile, y_tile, desc);
            }
        }
    }
    return tilemap;
}

ENTITY_UPDATE_PROC(update_tilemap);
ENTITY_RENDER_PROC(render_tilemap);
ENTITY_DELETE_PROC(delete_tilemap);

Tilemap *spawn_tilemap(Level *level, vec2i tile, int32_t x_tiles, int32_t y_tiles) {
    assert(x_tiles > 0 && y_tiles > 0);

    auto tilemap = create_entity_m(level, Tilemap);
    tilemap->update_proc = update_tilemap;
    tilemap->render_proc = render_tilemap;
    tilemap->delete_proc = delete_tilemap;

    auto ti = tile_info(tile);
    tilemap->position = ti.position;

    tilemap->tiles = malloc_and_zero_array(Tile, x_tiles * y_tiles);
    tilemap->x_tiles = x_tiles;
    tilemap->y_tiles = y_tiles;

    // Initialize tiles
    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            Tile *tile = &tilemap->tiles[y_tile * tilemap->x_tiles + x_tile];
            zero_struct(tile);
            tile->is_not_empty = false;
            tile->tilemap = tilemap;
            tile->tile_x = x_tile;
            tile->tile_y = y_tile;
        }
    }

    return tilemap;
}

static
void maybe_drop_the_drop_after_hit_anim(Tile *tile) {

#define _CHANGE_SPRITE_AFTER_DROP\
    if(tile->tile_desc.tile_flags & TILE_FLAG_CHANGES_SPRITE_AFTER_DROP) {\
        tile->tile_desc.tile_flags &= ~TILE_FLAG_CHANGES_SPRITE_AFTER_DROP;\
        set_tile_sprite(tile, tile->tile_desc.tile_sprite_after_drop);\
    }

    switch(tile->tile_desc.tile_drop) {
        default:
        case TILE_DROP_NONE: {
            // nothing
        } break;

        case TILE_DROP_POWERUP: {
            if(tile->tilemap->level->entities.first_of_type[entity_type_id(Player)]->as<Player>()->mode == PLAYER_IS_SMALL) {
                spawn_mushroom(tile->tilemap->level, tile);
            } else {
                spawn_fire_plant(tile->tilemap->level, tile);
            }

            tile->tile_desc.tile_drop = TILE_DROP_NONE;
            tile->tile_desc.tile_flags = tile->tile_desc.tile_flags & ~TILE_FLAG_DO_ANIM_ON_HIT;

            _CHANGE_SPRITE_AFTER_DROP;
        } break;

        case TILE_DROP_STAR: {
            spawn_star(tile->tilemap->level, tile);

            tile->tile_desc.tile_drop = TILE_DROP_NONE;
            tile->tile_desc.tile_flags = tile->tile_desc.tile_flags & ~TILE_FLAG_DO_ANIM_ON_HIT;

            _CHANGE_SPRITE_AFTER_DROP;
        } break;

        case TILE_DROP_COINS: {
            if(tile->tile_desc.drops_left > 0) {
                tile->tile_desc.drops_left -= 1;
            }
            
            if(tile->tile_desc.drops_left <= 0) {
                tile->tile_desc.tile_drop = TILE_DROP_NONE;
                tile->tile_desc.tile_flags = tile->tile_desc.tile_flags & ~TILE_FLAG_DO_ANIM_ON_HIT;

                _CHANGE_SPRITE_AFTER_DROP;
            }
        } break;
    }
}

ENTITY_UPDATE_PROC(update_tilemap) {
    auto tilemap = self_base->as<Tilemap>();

    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            auto tile = tilemap->get_tile(x_tile, y_tile);
            if(tile == NULL) {
                continue;
            }

            if(tile->hit_anim == Tile::HIT_ANIM_UP) {
                tile->hit_anim_counter += delta_time;
                if(tile->hit_anim_counter >= hit_anim_time) {
                    tile->hit_anim_counter = hit_anim_time;
                    tile->hit_anim = Tile::HIT_ANIM_DOWN;
                }
            } else if(tile->hit_anim == Tile::HIT_ANIM_DOWN) {
                tile->hit_anim_counter -= delta_time;
                if(tile->hit_anim_counter <= 0.0f) {
                    tile->hit_anim_counter = 0.0f;
                    tile->hit_anim = Tile::HIT_ANIM_NO;

                    // Drop stuff that drops after the hit animation finishes
                    maybe_drop_the_drop_after_hit_anim(tile);
                }
            }
            tile->hit_anim_perc = tile->hit_anim_counter / hit_anim_time;

            if(tile->tile_desc.is_animated) {
                update_anim(&tile->anim_player, delta_time);
            }
        }
    }
}

ENTITY_RENDER_PROC(render_tilemap) {
    auto tilemap = self_base->as<Tilemap>();
    auto tilemap_ti = tilemap->get_position_ti();

    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            auto tile = tilemap->get_tile(x_tile, y_tile);
            if(tile == NULL) {
                continue;
            }

            // if is invisible -> do not render
            if(tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE || tile->tile_desc.tile_flags & TILE_FLAG_NO_RENDER) {
                continue;
            }

            // Calculate offset for hit anim
            int32_t y_offset = 0;
            if(tile->hit_anim != Tile::HIT_ANIM_NO) {
                y_offset = (int32_t)roundf(tile->hit_anim_perc * hit_anim_distance);
            }

            Sprite *sprite = NULL;
            if(tile->tile_desc.is_animated) {
                sprite = &get_current_frame(&tile->anim_player)->sprite;
            } else {
                sprite = &sprites[tile->tile_desc.tile_sprite];
            }

            const int32_t z_pos = tile->hit_anim != Tile::HIT_ANIM_NO ? Z_NEAR_POS : Z_TILEMAP_POS;

            auto ti = tile_info(vec2i{ tile->tile_x, tile->tile_y } + tilemap_ti.tile);
            render::r_sprite(ti.position + vec2i { 0, y_offset }, z_pos, { sprite->width, sprite->height }, *sprite, color::white);
        }
    }
}

ENTITY_DELETE_PROC(delete_tilemap) {
    auto tilemap = self_base->as<Tilemap>();
    free(tilemap->tiles);
}

Tile *Tilemap::get_tile(int32_t x, int32_t y) {
    if(x < 0 || y < 0 || x >= this->x_tiles || y >= this->y_tiles) {
        return NULL;
    }

    Tile *tile = &this->tiles[y * this->x_tiles + x];
    return tile->is_not_empty ? tile : NULL;
}

Tile *Tilemap::set_tile(int32_t x, int32_t y, TileDesc tile_desc) {
    if(x < 0 || y < 0 || x >= this->x_tiles || y >= this->y_tiles) {
        return NULL;
    }

    Tile *tile = &this->tiles[y * this->x_tiles + x];
    tile->is_not_empty = true;
    tile->tile_desc = tile_desc;
    if(tile->tile_desc.is_animated) {
        set_tile_anim(tile, tile_desc.tile_anim);
    }
    return tile;
}

void tile_hit(Tile *tile, Player *player) {
    assert(tile != NULL && player != NULL);

    if(!tile->is_not_empty) {
        return;
    }

    // Can be hit only if finished.
    if(tile->tile_desc.tile_flags & TILE_FLAG_DO_ANIM_ON_HIT && tile->hit_anim != Tile::HIT_ANIM_NO) {
        return;
    }

    if(tile->tile_desc.tile_flags & TILE_FLAG_BECOMES_VISIBLE_AFTER_HIT) {
        tile->tile_desc.tile_flags &= ~TILE_FLAG_IS_INVISIBLE;
        tile->tile_desc.tile_flags &= ~TILE_FLAG_BECOMES_VISIBLE_AFTER_HIT;
    }

    if(tile->tile_desc.tile_flags & TILE_FLAG_DO_ANIM_ON_HIT) {
        tile->hit_anim = Tile::HIT_ANIM_UP;
        tile->hit_anim_counter  = 0.0f;

        if(tile->tile_desc.tile_drop == TILE_DROP_NONE && player->mode == PLAYER_IS_SMALL) {
            audio_player::a_play_sound(global_data::get_sound(SOUND_TILE_HIT));
        } else if(tile->tile_desc.tile_drop != TILE_DROP_NONE && tile->tile_desc.tile_drop != TILE_DROP_COINS) {
            audio_player::a_play_sound(global_data::get_sound(SOUND_TILE_DROP));
        }

        switch(tile->tile_desc.tile_drop) {
            case TILE_DROP_COINS: {
                spawn_coin_drop_anim(tile->tilemap->level, tile->get_ti());
                add_coins(tile->tilemap->level, 1);
                // Tile values change after the animation
            } break;
        }

        FindCollisionsOpts opts;
        opts.entity_flags = E_FLAG_IS_GAMEPLAY_ENTITY;

        Collider collider;
        collider.size = { TILE_SIZE + 2, 8 };
        collider.offset = { -1, TILE_SIZE };
        auto found = find_collisions(tile->tilemap->level, tile->get_ti().position, &collider, opts);

        // @todo can get hit 2 times by two different tiles
        for(Entity *entity : found) {
            switch(entity->entity_type_id) {
                case entity_type_id(Mushroom): {
                    entity->has_move_data->speed.y = 75.0f;
                } break;

                case entity_type_id(Goomba): {
                    kill_goomba(entity->as<Goomba>(), false, 1);
                } break;

                case entity_type_id(Koopa): {
                    kill_koopa(entity->as<Koopa>(), 1);
                } break;
            }
        }
    }

    if(tile->tile_desc.tile_flags & TILE_FLAG_BREAKABLE && player->mode != PLAYER_IS_SMALL) {
        const ETileBreakAnim break_anim = tile->tile_desc.tile_break_anim == TILE_BREAK_ANIM__INVALID ? TILE_BREAK_ANIM_BRICK : tile->tile_desc.tile_break_anim;
        spawn_tile_break_anim(tile->tilemap->level, tile->get_ti(), break_anim);
        clear_tile(tile);
        audio_player::a_play_sound(global_data::get_sound(SOUND_DESTROY_BLOCK));
        return;
    }
}

void render_tilemap_mesh(Tilemap *tilemap, int32_t z_pos, vec4 color) {
    const vec2i pos = tilemap->position;
    const int32_t tile_size = TILE_SIZE;
    
    render::r_set_line_width(1.5f);
    for(int32_t x = 0; x <= tilemap->x_tiles; ++x) {
        int32_t height = tilemap->y_tiles * tile_size;
        render::r_line({ pos.x + x * tile_size, pos.y }, { pos.x + x * tile_size, pos.y + height }, z_pos, color);
    }
    for(int32_t y = 0; y <= tilemap->y_tiles; ++y) {
        int32_t width = tilemap->x_tiles * tile_size;
        render::r_line({ pos.x, pos.y + y * tile_size }, { pos.x + width, pos.y + y * tile_size }, z_pos, color);
    }
}

bool is_colliding_with_any_tile(vec2i position, Collider *collider, Tilemap *tilemap, vec2i offset, FindTileCollisionOpts opts) {
    position += offset;

    // Tiles to check
    const auto ti_min = tile_info_at(position + collider->offset);
    const auto ti_max = tile_info_at(position + collider->offset + collider->size);

    auto origin_tile = tilemap->get_position_ti().tile;

    vec2i tile_min = ti_min.tile - origin_tile;
    vec2i tile_max = ti_max.tile - origin_tile;

    if(tile_max.x < 0 || tile_max.y < 0 || tile_min.x >= tilemap->x_tiles || tile_min.y >= tilemap->y_tiles) {
        return false;
    }

    clamp_min(&tile_min.x, 0);
    clamp_min(&tile_min.y, 0);
    clamp_max(&tile_max.x, tilemap->x_tiles - 1);
    clamp_max(&tile_max.y, tilemap->y_tiles - 1);

    for(int32_t y = tile_min.y; y <= tile_max.y; ++y) {
        for(int32_t x = tile_min.x; x <= tile_max.x; ++x) {
            auto tile = tilemap->get_tile(x, y);
            if(tile == NULL) {
                continue;
            }

            if(opts.tile_flags_required != 0) {
                if((tile->tile_desc.tile_flags & opts.tile_flags_required) != opts.tile_flags_required) {
                    continue;
                }
            }

            if(opts.tile_flags_forbidden != 0) {
                if((tile->tile_desc.tile_flags & opts.tile_flags_forbidden) != 0) {
                    continue;
                }
            }

            const auto ti = tile_info({ x, y });

            if(aabb(position + collider->offset, collider->size, ti.position + tilemap->position, TILE_SIZE_2)) {
                return true;
            }
        }
    }
    return false;
}

// @todo
std::vector<Tile *> find_collisions(vec2i position, Collider *collider, Tilemap *tilemap, vec2i offset, FindTileCollisionOpts opts) {
    position += offset;

    // Tiles to check
    const auto ti_min = tile_info_at(position + collider->offset);
    const auto ti_max = tile_info_at(position + collider->offset + collider->size);

    auto origin_tile = tilemap->get_position_ti().tile;

    vec2i tile_min = ti_min.tile - origin_tile;
    vec2i tile_max = ti_max.tile - origin_tile;

    if(tile_max.x < 0 || tile_max.y < 0 || tile_min.x >= tilemap->x_tiles || tile_min.y >= tilemap->y_tiles) {
        return { };
    }

    clamp_min(&tile_min.x, 0);
    clamp_min(&tile_min.y, 0);
    clamp_max(&tile_max.x, tilemap->x_tiles - 1);
    clamp_max(&tile_max.y, tilemap->y_tiles - 1);

    auto collisions = std::vector<Tile *>();
    for(int32_t y = tile_min.y; y <= tile_max.y; ++y) {
        for(int32_t x = tile_min.x; x <= tile_max.x; ++x) {
            Tile *tile = tilemap->get_tile(x, y);
            if(tile == NULL) {
                continue;
            }

            if(opts.tile_flags_required != 0) {
                if((tile->tile_desc.tile_flags & opts.tile_flags_required) != opts.tile_flags_required) {
                    continue;
                }
            }

            if(opts.tile_flags_forbidden != 0) {
                if((tile->tile_desc.tile_flags & opts.tile_flags_forbidden) != 0) {
                    continue;
                }
            }

            const auto ti = tile_info({ x, y });

            if(aabb(position + collider->offset, collider->size, ti.position + tilemap->position, TILE_SIZE_2)) {
                collisions.push_back(tile);
            }
        }
    }
    return collisions;
}

namespace {
    constexpr float32_t tile_break_anim_gravity = 280.0f;
    constexpr float32_t tile_break_anim_time    = 3.0f;

    AnimSet tile_break_anim_sets[TILE_BREAK_ANIM__COUNT];
}

ENTITY_UPDATE_PROC(update_tile_break_anim);
ENTITY_RENDER_PROC(render_tile_break_anim);

TileBreakAnim *spawn_tile_break_anim(Level *level, TileInfo ti, ETileBreakAnim tile_break_anim) {
    auto anim = create_entity_m(level, TileBreakAnim);
    anim->update_proc = update_tile_break_anim;
    anim->render_proc = render_tile_break_anim;
    anim->position = ti.position;

    anim->timer = 0.0f;
    anim->anim_player = init_anim_player(&tile_break_anim_sets[(int32_t)tile_break_anim]);

    anim->speed_top  = { 40.0f, 140.0f };
    anim->speed_bot  = { 35.0f, 110.0f };
    anim->offset_top = { 0.0f,  0.0f };
    anim->offset_bot = { 0.0f,  0.0f };

    return anim;
}

ENTITY_UPDATE_PROC(update_tile_break_anim) {
    auto anim = self_base->as<TileBreakAnim>();

    anim->timer += delta_time;
    update_anim(&anim->anim_player, delta_time);

    anim->speed_top.y -= tile_break_anim_gravity * delta_time;
    anim->speed_bot.y -= tile_break_anim_gravity * delta_time;
    anim->offset_top  += anim->speed_top * delta_time;
    anim->offset_bot  += anim->speed_bot * delta_time;

    if(anim->timer >= tile_break_anim_time) {
        delete_entity(anim);
        return;
    }
}

ENTITY_RENDER_PROC(render_tile_break_anim) {
    auto anim = self_base->as<TileBreakAnim>();
    auto frame = get_current_frame(&anim->anim_player);

    auto _draw_particle = [&] (vec2i offset) {
        const vec2i position = anim->position + offset;
        render::r_sprite(position, 0, frame->size(), frame->sprite, color::white);
    };

    const vec2i offset_bot_l = vec2i::make(anim->offset_bot) * vec2i { -1, 1 } + vec2i { -frame->size().x, 0 };
    const vec2i offset_top_l = vec2i::make(anim->offset_top) * vec2i { -1, 1 } + vec2i { -frame->size().x, frame->size().y };
    const vec2i offset_bot_r = vec2i::make(anim->offset_bot) + vec2i { frame->size().x, 0 };
    const vec2i offset_top_r = vec2i::make(anim->offset_top) + vec2i { frame->size().x, frame->size().y };
    
    render::r_set_flip_x_quads(2);
    _draw_particle(offset_bot_l);
    _draw_particle(offset_top_l);

    _draw_particle(offset_bot_r);
    _draw_particle(offset_top_r);
}

ENTITY_UPDATE_PROC(update_coin_drop_anim);
ENTITY_RENDER_PROC(render_coin_drop_anim);

namespace {
    constexpr float32_t coin_drop_anim_time = 0.75f;
    constexpr int32_t   coin_drop_anim_dist = 64;
    AnimSet anim_set_coin_drop;
}

CoinDropAnim *spawn_coin_drop_anim(Level *level, TileInfo ti) {
    auto coin_anim = create_entity_m(level, CoinDropAnim);
    coin_anim->update_proc = update_coin_drop_anim;
    coin_anim->render_proc = render_coin_drop_anim;
    coin_anim->position = ti.position + vec2i{ 0, 5 };

    coin_anim->anim_player = init_anim_player(&anim_set_coin_drop);
    return coin_anim;
}

ENTITY_UPDATE_PROC(update_coin_drop_anim) {
    auto coin_anim = self_base->as<CoinDropAnim>();

    coin_anim->timer += delta_time;
    coin_anim->timer_perc = coin_anim->timer / coin_drop_anim_time;

    update_anim(&coin_anim->anim_player, delta_time);

    if(coin_anim->timer >= coin_drop_anim_time) {
        delete_entity(coin_anim);
        return;
    }
}

ENTITY_RENDER_PROC(render_coin_drop_anim) {
    auto coin_anim = self_base->as<CoinDropAnim>();
    auto frame = get_current_frame(&coin_anim->anim_player);

    const vec2i position_offset = { 0, (int32_t)roundf(sinf(square(coin_anim->timer_perc) * M_PI) * coin_drop_anim_dist) };
    render::r_sprite(coin_anim->position + position_offset, Z_TILEMAP_POS + 5, frame->size(), frame->sprite, color::white);
}

void init_tilemap_common_data(void) {
    // Copy sprites
#define TILE_SPRITE(e_sprite, sprite_idx) sprites[e_sprite] = global_data::get_sprite(sprite_idx);
    TILE_SPRITES;
#undef TILE_SPRITE
    
    AnimSet *set = &anim_sets[TILE_ANIM_QUESTION];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_BLOCK_QUESTION_1), 1.33f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_BLOCK_QUESTION_2), 0.33f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_BLOCK_QUESTION_3), 0.33f);

    /* Initialize tile break anim sets */ {
        AnimSet *set = NULL;

        set = &tile_break_anim_sets[TILE_BREAK_ANIM_BRICK];
        init_anim_set(set);
        next_anim_frame(set, global_data::get_sprite(SPRITE_BRICK_PARTICLE_1), 0.2f);
        next_anim_frame(set, global_data::get_sprite(SPRITE_BRICK_PARTICLE_2), 0.2f);
        next_anim_frame(set, global_data::get_sprite(SPRITE_BRICK_PARTICLE_3), 0.2f);
        next_anim_frame(set, global_data::get_sprite(SPRITE_BRICK_PARTICLE_4), 0.2f);

        set = &tile_break_anim_sets[TILE_BREAK_ANIM_BRICK_UG];
        init_anim_set(set);
        next_anim_frame(set, global_data::get_sprite(SPRITE_UG_BRICK_PARTICLE_1), 0.2f);
        next_anim_frame(set, global_data::get_sprite(SPRITE_UG_BRICK_PARTICLE_2), 0.2f);
        next_anim_frame(set, global_data::get_sprite(SPRITE_UG_BRICK_PARTICLE_3), 0.2f);
        next_anim_frame(set, global_data::get_sprite(SPRITE_UG_BRICK_PARTICLE_4), 0.2f);
    }

    init_anim_set(&anim_set_coin_drop);
    next_anim_frame(&anim_set_coin_drop, global_data::get_sprite(SPRITE_COIN_1), 0.1f);
    next_anim_frame(&anim_set_coin_drop, global_data::get_sprite(SPRITE_COIN_2), 0.1f);
    next_anim_frame(&anim_set_coin_drop, global_data::get_sprite(SPRITE_COIN_3), 0.1f);
    next_anim_frame(&anim_set_coin_drop, global_data::get_sprite(SPRITE_COIN_4), 0.1f);
}

Sprite get_tile_sprite(ETileSprite e_sprite) {
    return sprites[e_sprite];
}