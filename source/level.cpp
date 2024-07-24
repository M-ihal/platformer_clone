#include "level.h"
#include "game.h"
#include "all_entities.h"
#include "renderer.h"
#include "data.h"

namespace {
    const int32_t   wake_up_rect_half_width  = GAME_WIDTH / 2 + TILE_SIZE * 2;
    const int32_t   wake_up_rect_half_height = GAME_HEIGHT / 2;
    const int32_t   starting_level_time      = 400;
    const float32_t time_point_real_time     = 0.2f;   // How much seconds is a level time point
    const float32_t time_per_time_point      = 0.005f; // How much time it takes for one in the finishing sequence
    const float32_t time_per_coin            = 0.01f;  // How much time it takes for one in the finishing sequence
    const int32_t   points_per_time_point    = 50;
    const int32_t   points_per_coin          = 400;
    const float32_t wait_time_before_finishing_level = 2.0f;
    const float32_t wait_time_before_finishing_world = 3.5f;
}

static inline void recalculate_wake_up_rect(Level *level) {
    const vec2i wake_up_rect_half_size = { wake_up_rect_half_width, wake_up_rect_half_height };
    level->wake_up_rect_position = vec2i{ (int32_t)roundf(level->render_view.x_translate), (int32_t)roundf(level->render_view.y_translate) } - wake_up_rect_half_size;
    level->wake_up_rect_size = wake_up_rect_half_size * 2;
}

Level *create_empty_level(void) {
    Level *level = malloc_and_zero_struct(Level);

    // Allocate memory
    level->memory_size = LEVEL_MEMORY_TO_ALLOC;
    level->memory_used = 0;
    level->memory = (uint8_t *)malloc_and_zero(level->memory_size);

    level->next_entity_id = 1;
    zero_struct(&level->entities);
    zero_struct(&level->entities_unused);
    level->to_be_deleted = new std::vector<Entity *>();

    zero_array(level->no_paused_entities);
    level->no_paused_entities_count = 0;

    level->player_has_died = false;
    level->pause_state = UNPAUSED;
    level->level_state = PLAYING;

    level->disable_level_timer = false;
    level->update_level_time = true;
    level->level_time = starting_level_time;

    auto view = &level->render_view;
    view->left   = -(GAME_WIDTH  / 2);
    view->bottom = -(GAME_HEIGHT / 2);
    view->right  =   GAME_WIDTH  / 2;
    view->top    =   GAME_HEIGHT / 2;
    view->near   = -10.0f;
    view->far    =  10.0f;
    view->scale  =  1.0f;
    view->x_translate = 128.0f;
    view->y_translate = 128.0f;

    level->level_music_id[(int32_t)ELevelMusic::regular]                       = MUSIC_NONE;
    level->level_music_id[(int32_t)ELevelMusic::during_star_power]             = MUSIC_NONE;
    level->level_music_id[(int32_t)ELevelMusic::during_player_enter_pipe_anim] = MUSIC_NONE;
    level->level_music_id[(int32_t)ELevelMusic::on_level_completed]            = MUSIC_NONE;

    return level;
}

void recreate_empty_level(Level **level) {
    if(*level != NULL) {
        delete_level(*level);
    }
    *level = create_empty_level();
}

void delete_level(Level *level) {
    if(level == NULL) {
        return;
    }

    if(level->to_be_deleted != NULL) {
        delete level->to_be_deleted;
    }

    while(level->entities.first != NULL) {
        delete_entity_imm(level->entities.first);
    }

    free(level->memory);
    free(level);
}

void set_level_render_view(Level *level) {
    RenderView *view = &level->render_view;

    auto region = get_entity_by_unique_id_m(level, level->current_region_unique_id, CameraRegion);
    if(region == NULL) {
        return;
    }

    auto player = get_player(level);
    if(player == NULL) {
        return;
    }

    const vec2 view_dims        = view->calc_dims();
    const vec2 view_dims_scaled = view->calc_dims_scaled();
    const vec2 player_center    = player->position.to_vec2() + player->has_collider->offset.to_vec2() + player->has_collider->size.to_vec2() * 0.5f;

    view->scale = 1.0f; // view_dims.y / (float32_t)region->const_height_in_pixels;
    view->y_translate = region->position.y + view_dims_scaled.y * 0.5f;

    if(view_dims_scaled.x > (float32_t)region->x_width) {
        view->x_translate = region->position.x + (float32_t)region->x_width * 0.5f;
    } else {
        view->x_translate = player_center.x;

        float32_t min = view->x_translate - view_dims_scaled.x * 0.5f;
        float32_t max = view->x_translate + view_dims_scaled.x * 0.5f;

        if(min <= (float32_t)region->position.x) {
            view->x_translate = (float32_t)region->position.x + view_dims_scaled.x * 0.5f;
        } else if(max >= ((float32_t)region->position.x + (float32_t)region->x_width)) {
            view->x_translate = (float32_t)region->position.x + (float32_t)region->x_width - view_dims_scaled.x * 0.5f;
        }
    }
}

void set_best_camera_region(Level *level) {
    auto player = get_player(level);
    if(player == NULL) {
        return;
    }

    // Find regions the player collides with
    std::vector<CameraRegion *> regions;
    for_entity_type(level, CameraRegion, region_check) {
        if(aabb(player->position + player->has_collider->offset, player->has_collider->size, region_check->position, { region_check->x_width, region_check->const_height_in_pixels })) {
            regions.push_back(region_check);
        }
    }

    // Determine best region
    CameraRegion *best = NULL;
    for(CameraRegion *region_check : regions) {
        // @temp For now just pick first one
        best = region_check;
        break;
    }

    if(best != NULL) {
        level->current_region_unique_id = best->unique_id;
    } else {
        // Ensure current region is valid
        auto exists = get_entity_by_unique_id_m(level, level->current_region_unique_id, CameraRegion);
        if(exists == NULL) {
            level->current_region_unique_id = 0;
        }
    }
}

void update_level(Level *level, SimInput input, Game *game, float64_t delta_time) {
    level->coins_collected_this_frame = 0;
    level->points_aquired_this_frame  = 0;

    auto maybe_update_entity = [&] (Entity *e) {
        if(e->update_proc != NULL && is_entity_used(e)) {
            if(e->entity_flags & E_FLAG_SLEEPS_UNTIL_AWAKEN && e->is_asleep) {
                return;
            } else {
                e->update_proc(e, input, delta_time);
            }
        }
    };

    switch(level->level_state) {
        case PLAYING: {
            if(level->pause_state == TO_BE_PAUSED) {
                level->pause_state = PAUSED;
            }

            level->frames_elapsed += 1;
            level->delta_time      = delta_time;
            level->elapsed_time   += delta_time;

            // Function that updates all entities in the order of declaration in 'all_entities.h'
            auto update_entities_in_defined_order = [&] (void) {
#define ENTITY_TYPE(Type)\
                for_entity_type(level, Type, entity) {\
                    maybe_update_entity(entity);\
                }
                ENTITY_TYPES;
#undef ENTITY_TYPE
            };

            if(level->pause_state != PAUSED) {
                update_entities_in_defined_order();

                // Update level time
                if(!level->disable_level_timer && level->update_level_time) {
                    level->level_time_accumulator += delta_time;
                    while(level->level_time_accumulator >= time_point_real_time) {
                        level->level_time_accumulator -= time_point_real_time;
                        level->level_time -= 1;

                        if(level->level_time == 0) {
                            auto player = get_player(level);
                            if(player != NULL) {
                                kill_player(player);
                            }
                            break;
                        }
                    }
                }
            } else {
                // Update unpaused entities while level is paused
                if(level->no_paused_entities_count > 0) {
                    for(int32_t idx = 0; idx < level->no_paused_entities_count; ++idx) {
                        Entity *entity = level->no_paused_entities[idx];
                        if(entity->update_proc) {
                            maybe_update_entity(entity);
                        }
                    }
                }
            }

            if(game != NULL) {
                game->coins  += level->coins_collected_this_frame;
                game->points += level->points_aquired_this_frame;
            }

            if(level->coins_collected_this_frame != 0) {
                audio_player::a_play_sound(global_data::get_sound(SOUND_COIN));
            }

            // Setup finishing level sequence
            if(level->player_has_reached_the_end) {
                level->finishing_level_timer = 0.0f;
                level->finishing_level_stage = 0;
                level->level_state = FINISHING_LEVEL;
            } else if(level->player_has_died) {
                level->level_state = FINISHED;
            }
        } break;

        case FINISHING_LEVEL: {
#define update_entities_of_type(Type) { for_entity_type(level, Type, _entity) { maybe_update_entity(_entity); } }

            // Keep updating those entities...
            update_entities_of_type(FlagPole);
            update_entities_of_type(Coin);
            update_entities_of_type(FloatingText);
            update_entities_of_type(CoinDropAnim);
            update_entities_of_type(TimedSprite);
            update_entities_of_type(EnemyFallAnim);
            update_entities_of_type(TileBreakAnim);
            update_entities_of_type(Tilemap);

            switch(level->finishing_level_stage) {
                case 0: /* Points for collected coins */ {
                    level->finishing_level_timer += delta_time;
                    while(level->finishing_level_timer >= time_per_coin) {
                        if(game->coins > 0) {
                            level->finishing_level_timer -= time_per_coin;
                            game->coins  -= 1;
                            game->points += points_per_coin;
                        } else {
                            break;
                        }
                    }

                    if(game->coins == 0) {
                        level->finishing_level_timer = 0.0f;
                        level->finishing_level_stage = 1;
                    }
                } break;

                case 1: /* Points for time left */ {
                    level->finishing_level_timer += delta_time;
                    while(level->finishing_level_timer >= time_per_time_point) {
                        if(level->level_time > 0) {
                            level->finishing_level_timer -= time_per_time_point;
                            level->level_time -= 1;
                            game->points += points_per_time_point;
                        } else {
                            break;
                        }
                    }

                    if(level->level_time == 0) {
                        level->finishing_level_timer = 0.0f;

                        if(game->level_idx == LEVEL_NUM - 1) {
                            level->finishing_level_stage = 8; // Finishing world
                        } else {
                            level->finishing_level_stage = 9; // Finishing level
                        }
                    }
                } break;

                case 8: /* Wait a bit (Finishing world) */ {
                    level->finishing_level_timer += delta_time;
                    if(level->finishing_level_timer >= wait_time_before_finishing_world) {
                        level->finishing_level_timer = 0.0f;
                        level->finishing_level_stage = 10;
                    }
                } break;

                case 9: /* Wait a bit (Finishing level) */ {
                    level->finishing_level_timer += delta_time;
                    if(level->finishing_level_timer >= wait_time_before_finishing_level) {
                        level->finishing_level_timer = 0.0f;
                        level->finishing_level_stage = 10;
                    }
                } break;

                case 10: /* Stages completed */
                default: {
                    level->level_state = FINISHED;
                } break;
            }
        } break;

        case FINISHED: {
            // Do nothing
        } break;
    }

    for(Entity *e : *level->to_be_deleted) {
        if(e->in_use == false) {
            continue; // avoid deleting deleted entities
        }
        delete_entity_imm(e);
    }
    level->to_be_deleted->clear();

    set_best_camera_region(level);
    set_level_render_view(level);

    // Wake up entities
    recalculate_wake_up_rect(level);
    for_every_entity(level, entity) {
        if(!(entity->entity_flags & E_FLAG_SLEEPS_UNTIL_AWAKEN && entity->is_asleep) || entity->has_collider == NULL) {
            continue;
        }

        if(aabb(entity->position + entity->has_collider->offset, entity->has_collider->size, level->wake_up_rect_position, level->wake_up_rect_size)) {
            entity->is_asleep = false;
        }
    }
}

void render_level(Level *level) {
    for_every_entity(level, e) {
        if(e->render_proc != NULL && is_entity_used(e)) {
            e->render_proc(e);
        }
    }
}

void set_level_music_and_play(Level *level, ELevelMusic music) {
    auto how_many_loops = [] (ELevelMusic _music) -> int32_t {
        switch(_music) {
            default:
            case ELevelMusic::regular:
            case ELevelMusic::during_star_power:
            case ELevelMusic::during_player_enter_pipe_anim: {
                return -1;
            }

            case ELevelMusic::on_level_completed: return 0;
        }
    };

    if(music == ELevelMusic::__NOT_SET) {
        if(level->current_level_music == ELevelMusic::__NOT_SET) return;

        Music *expected = global_data::get_music(level->level_music_id[(int32_t)level->current_level_music]);
        if(audio_player::a_is_music(expected)) {
            if(audio_player::a_is_music_paused()) {
                audio_player::a_resume_music();
            } else {
                return;
            }
        } else {
            audio_player::a_play_music(expected, how_many_loops(level->current_level_music));
            audio_player::a_resume_music();
        }
    } else {
        level->current_level_music = music;
        Music *handle = global_data::get_music(level->level_music_id[(int32_t)music]);
        audio_player::a_play_music(handle, how_many_loops(music));
        audio_player::a_resume_music();
    }
}

void set_starting_level_music(Level *level) {
    auto player = get_player(level);
    set_level_music_and_play(level, ELevelMusic::regular);
}

void pause_level(Level *level, Entity *exception) {
    if(level->pause_state != PAUSED) {
        level->pause_state = TO_BE_PAUSED;
    }
    level->no_paused_entities_count = 1;
    level->no_paused_entities[0] = exception;
}

void unpause_level(Level *level) {
    level->pause_state = UNPAUSED;
    level->no_paused_entities_count = 0;
    zero_array(level->no_paused_entities);
}

void add_points(Level *level, int32_t points) {
    level->points_aquired_this_frame += points;
}

void add_coins(Level *level, int32_t coins) {
    level->coins_collected_this_frame += coins;
}

void add_points_with_text_above_entity(Level *level, int32_t points, Entity *entity) {
    add_points(level, points);

    if(entity->has_collider == NULL) {
        return;
    }

    const vec2i text_position = { 
        entity->position.x + entity->has_collider->offset.x + entity->has_collider->size.x / 2, 
        entity->position.y + entity->has_collider->offset.y + entity->has_collider->size.y + 16 
    };

    auto f_text = spawn_floating_text_number(level, text_position, points);
    f_text->render_centered = true;
}

void render_level_debug_stuff(Level *level) {
#ifdef _DEBUG_MODE
    // Draw colliders
    for_every_entity(level, e) {
        auto collider = e->has_collider;
        if(collider != NULL) {
            auto color = lerp(color::red, color::black, 0.15f);
            render::r_set_line_width(2.0f);
            render::r_line_quad(offset_position(e, collider), 8, collider->size, color);
        }
    }

    for_entity_type(level, CameraRegion, region) {
        render::r_set_line_width(3.0f);
        render::r_line_quad(region->position, 0, { region->x_width, CameraRegion::const_height_in_pixels }, lerp(color::yellow, color::red, 0.14f));
    }

    for_entity_type(level, FireBar, fire_bar) {
        for(int32_t idx = 0; idx < fire_bar->fire_num; ++idx) {
            vec2i fire_pos = calc_one_fire_position(fire_bar, idx);
            render::r_set_line_width(1.5f);
            render::r_line_quad(fire_pos, 0, fire_bar->fire_size, color::yellow);
        }
    }

    for_entity_type(level, KillRegion, kill_region) {
        render::r_quad(kill_region->position, -1, kill_region->collider.size, { 1.0f, 0.0f, 0.0f, 0.6f });
        render::r_set_line_width(2.0f);
        render::r_line_quad(kill_region->position, -1, kill_region->collider.size, color::red);
    }

    for_entity_type(level, Trigger, trigger) {
        const vec3 color = trigger->already_triggered ? color::green.rgb : color::red.rgb;
        render::r_quad(trigger->position + trigger->collider.offset, -1, trigger->collider.size, vec4::make(color, 0.4f));
        render::r_set_line_width(2.0f);
        render::r_line_quad(trigger->position + trigger->collider.offset, -1, trigger->collider.size, vec4::make(color, 1.0f));
    }

#if 0
    for_entity_type(level, Portal, portal) {
        render::r_set_line_width(2.25f);
        render_connected_quads(portal->position, portal->get_size_for_editor(), portal->info.dest, TILE_SIZE_2, -1, { 0.0f, 1.0f, 0.0f, 0.8f }, { 0.0f, 1.0f, 0.0f, 0.7f });
        render::r_line_quad(portal->get_enter_pos(), -3, portal->get_enter_size(), color::red);
    }
#else
    // @copied
    for_entity_type(level, Portal, portal) {
        render::r_set_line_width(2.5f);
        render_connected_quads(portal->position, portal->get_size_for_editor(), portal->info.dest, TILE_SIZE_2, -1, color::yellow, { 0.0f, 1.0f, 0.0f, 0.75f });
        render::r_line_quad(portal->get_enter_pos(), -3, portal->get_enter_size(), color::green);

        // dont know bout that lel
        render::r_text({ portal->position.x, portal->position.y + render::def_font()->height }, -2, portal->info.vertical_enter ? "vertical" : "horizontal", global_data::get_small_font(), color::yellow);
        render::r_text({ portal->info.dest.x, portal->info.dest.y + render::def_font()->height }, -2, 
                       !portal->info.do_exit_anim ? "no exit anim" : (portal->info.vertical_exit ? "vertical" : "horizontal"), 
                       global_data::get_small_font(), color::yellow);
    }
#endif

#if 1 // Render if is asleep or not
    for_every_entity(level, entity) {
        if(entity->entity_flags & E_FLAG_SLEEPS_UNTIL_AWAKEN) {
            const char *text = entity->is_asleep ? "asleep" : "awake";
            render::r_text(entity->position + vec2i{ 0, render::def_font()->height }, -2, text, global_data::get_small_font(), color::yellow);
        }
    }
#endif

    

    // Draw tilemap mesh
#if 1
    for_entity_type(level, Tilemap, tilemap) {
        const vec2i pos = tilemap->position;
        const int32_t tile_size = TILE_SIZE;

        const vec4 color = { 0.8f, 0.8f, 0.8f, 0.7f };

        render::r_set_line_width(1.0f);
        for(int32_t x = 0; x <= tilemap->x_tiles; ++x) {
            int32_t height = tilemap->y_tiles * tile_size;
            render::r_line({ pos.x + x * tile_size, pos.y }, { pos.x + x * tile_size, pos.y + height }, 4, color);
        }
        for(int32_t y = 0; y <= tilemap->y_tiles; ++y) {
            int32_t width = tilemap->x_tiles * tile_size;
            render::r_line({ pos.x, pos.y + y * tile_size }, { pos.x + width, pos.y + y * tile_size }, 4, color);
        }
    }
#endif

    // Draw "invisible" blocks
    for_entity_type(level, Tilemap, tilemap) {
        for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
            for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
                Tile *tile = tilemap->get_tile(x_tile, y_tile);
                if(tile != NULL && (tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE || tile->tile_desc.tile_flags & TILE_FLAG_NO_RENDER)) {
                    const auto ti = tile->get_ti();
                    render::r_quad_marching_ants(ti.position, -10, TILE_SIZE_2, 8, { 1.0f, 1.0f, 1.0f, 0.8f }, level->elapsed_time);
                    render::r_sprite(ti.position, -8, TILE_SIZE_2, global_data::get_sprite(SPRITE_BLOCK_QUESTION_1), { 1.0f, 1.0f, 1.0f, 0.4f });
                }
            }
        }
    }

    recalculate_wake_up_rect(level);
    render::r_set_line_width(3.0f);
    render::r_line_quad(level->wake_up_rect_position, -9, level->wake_up_rect_size, color::cyan);

    // Render camera rect
    vec2i camera_min = vec2i::make(vec2{ level->render_view.x_translate, level->render_view.y_translate } - level->render_view.calc_dims_scaled() * 0.5f);
    vec2i camera_max = vec2i::make(vec2{ level->render_view.x_translate, level->render_view.y_translate } + level->render_view.calc_dims_scaled() * 0.5f);
    render::r_set_line_width(3.0f);
    render::r_line_quad(camera_min, 0, camera_max - camera_min, color::yellow);

    for_entity_type(level, MovingPlatform, platform) {
        _render_debug_moving_platform_metrics(platform, 1);
    }

    for_entity_type(level, SideMovingPlatform, platform) {
        _render_debug_side_moving_platform_metrics(platform, 1);
    }

    for_entity_type(level, Bowser, bowser) {
        _render_debug_bowser_info(bowser);
    }

    
#endif /* DEBUG_BUILD */
}

vec2i pixel_from_real(vec2 real) {
    vec2i result = vec2i::make(real);
    if(real.x < 0.0f) { result.x -= 1; }
    if(real.y < 0.0f) { result.y -= 1; }
    return result;
}

TileInfo tile_info(vec2i tile) {
    TileInfo ti;
    ti.tile.x = tile.x;
    ti.tile.y = tile.y;
    ti.position.x = tile.x * TILE_SIZE;
    ti.position.y = tile.y * TILE_SIZE;
    return ti;
}

TileInfo tile_info_at(vec2i p) { // @todo
    TileInfo ti;
    if(p.x < -1) { p.x += 1; }
    if(p.y < -1) { p.y += 1; }
    ti.tile.x = abs(p.x) / TILE_SIZE;
    ti.tile.y = abs(p.y) / TILE_SIZE;
    if(p.x < 0) { ti.tile.x = -ti.tile.x - 1; }
    if(p.y < 0) { ti.tile.y = -ti.tile.y - 1; }
    ti.position.x = ti.tile.x * TILE_SIZE;
    ti.position.y = ti.tile.y * TILE_SIZE;
    return ti;
}

vec2 convert_to_view_space(vec2i viewport_position, recti32 viewport, RenderView *view) { // @todo
    const auto view_dims = view->calc_dims();
    
    viewport.w -= viewport.x;
    viewport.h -= viewport.y;

    const vec2 perc = { 
        safe_divide((float32_t)(viewport_position.x - viewport.x), (float32_t)(viewport.x + viewport.w)),
        safe_divide((float32_t)(viewport_position.y - viewport.y), (float32_t)(viewport.y + viewport.h))
    };

    vec2 result = {
        safe_divide((perc.x * view_dims.x + view->left),   view->scale) + view->x_translate,
        safe_divide((perc.y * view_dims.y + view->bottom), view->scale) + view->y_translate,
    };
    return result;
}

mat4x4 RenderView::calc_proj(void) {
    auto proj = mat4x4::orthographic(this->left, this->bottom, this->right, this->top, this->near, this->far);
    return proj;
}

mat4x4 RenderView::calc_view(void) { // @todo
    auto view = mat4x4::identity();
    view *= mat4x4::scale(this->scale, this->scale, 1.0f);
    view *= mat4x4::translate(-this->x_translate, -this->y_translate, 0.0f);
    return view;
}

vec2 RenderView::calc_dims(void) {
    return {
        this->right - this->left,
        this->top - this->bottom
    };
}

vec2 RenderView::calc_dims_scaled(void) {
    auto dims = this->calc_dims();
    if(this->scale != 0.0f) {
        dims /= this->scale;
        return dims;
    }
    return { 0.0f, 0.0f };
}