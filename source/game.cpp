#include "game.h"
#include "renderer.h"
#include "entity.h"
#include "all_entities.h"
#include "save_data.h"
#include "data.h"
#include "level_transition.h"
#include "main_menu.h"

static
void reload_all_levels(Game *game) {
    reset_session(game);

    for(int32_t world_idx = 0; world_idx < WORLD_NUM; ++world_idx) {
        for(int32_t level_idx = 0; level_idx < LEVEL_NUM; ++level_idx) {
            char level_name[64];
            sprintf_s(level_name, array_count(level_name), "%d_%d.level", world_idx + 1, level_idx + 1);
            std::string level_path = global_data::get_data_path() + "//levels//" + level_name;

            Level **level = &game->levels[world_idx][level_idx];
            LevelSaveData *save_data = &game->level_save_datas[world_idx][level_idx];
            
            recreate_empty_level(level);
            load_level(*level, level_path.c_str(), save_data);
        }
    }

    recreate_empty_level(&game->custom_level);
    load_level(game->custom_level, (global_data::get_data_path() + "//levels//custom.level").c_str(), &game->custom_level_save_data);
}

static
void reset_all_levels(Game *game) {
    reset_session(game);

    for(int32_t world_idx = 0; world_idx < WORLD_NUM; ++world_idx) {
        for(int32_t level_idx = 0; level_idx < LEVEL_NUM; ++level_idx) {
            recreate_empty_level(&game->levels[world_idx][level_idx]);
            load_level_from_save_data(game->levels[world_idx][level_idx], &game->level_save_datas[world_idx][level_idx]);
        }
    }

    recreate_empty_level(&game->custom_level);
    load_level_from_save_data(game->custom_level, &game->custom_level_save_data);
}

Game *create_game(void) {
    auto game = malloc_and_zero_struct(Game);

    game->should_quit_game = false;

    /* Create levels */ {
        game->custom_level = create_empty_level();

        for(int32_t world_idx = 0; world_idx < WORLD_NUM; ++world_idx) {
            for(int32_t level_idx = 0; level_idx < LEVEL_NUM; ++level_idx) {
                game->levels[world_idx][level_idx] = create_empty_level();
            }
        }

        reload_all_levels(game);

        game->world_idx = 0;
        game->level_idx = 0;
    }

    game->start_world_idx = 0;
    game->start_level_idx = 0;

    game->game_mode = EGameMode::MAIN_MENU;
    game->next_game_mode = game->game_mode;
    prepare_main_menu();

    game->fit_to_window = false;
    game->render_pixel_multiplier = 0;
    game->framebuffer = create_framebuffer(GAME_WIDTH, GAME_HEIGHT);
    game->framebuffer->color->set_filter_mag(GL_NEAREST);
    game->framebuffer->color->set_filter_min(GL_NEAREST);

    /* --- debug stuff --- */

    game->debug_panel_perc = 1.0f;
    game->debug_panel_open = false;

    auto view = &game->debug_render_view;
    view->left   = -(GAME_WIDTH  / 2);
    view->bottom = -(GAME_HEIGHT / 2);
    view->right  =   GAME_WIDTH  / 2;
    view->top    =   GAME_HEIGHT / 2;
    view->near   = -10.0f;
    view->far    =  10.0f;
    view->scale  =  1.0f;
    view->x_translate = 128.0f;
    view->y_translate = 128.0f;
    game->use_debug_render_view = false;

    /* --- init gameplay panel stuff --- */

    game->gp_info_font = load_ttf_font((global_data::get_data_path() + "SuperMarioWorldTextBoxRegular-Y86j.ttf").c_str(), 8, 1024, 1024);
    game->gp_info_font->texture->set_filter_mag(GL_NEAREST);
    game->gp_info_font->texture->set_filter_min(GL_NEAREST);

    init_anim_set(&game->gp_info_ui_coin_anim_set);
    next_anim_frame(&game->gp_info_ui_coin_anim_set, global_data::get_sprite(SPRITE_UI_COIN_1), 1.2f);
    next_anim_frame(&game->gp_info_ui_coin_anim_set, global_data::get_sprite(SPRITE_UI_COIN_2), 0.4f);
    next_anim_frame(&game->gp_info_ui_coin_anim_set, global_data::get_sprite(SPRITE_UI_COIN_3), 0.4f);
    game->gp_info_coin_anim_player = init_anim_player(&game->gp_info_ui_coin_anim_set);

    reset_session(game);

    /* --- Set input binds --- */

    game->bind_move_l = init_binds();
    add_bind(&game->bind_move_l, BindType::KEY, key_a);
    add_bind(&game->bind_move_l, BindType::KEY, key_arrow_left);
    add_bind(&game->bind_move_l, BindType::GAMEPAD_BUTTON, gamepad_dpad_left);

    game->bind_move_r = init_binds();
    add_bind(&game->bind_move_r, BindType::KEY, key_d);
    add_bind(&game->bind_move_r, BindType::KEY, key_arrow_right);
    add_bind(&game->bind_move_r, BindType::GAMEPAD_BUTTON, gamepad_dpad_right);

    game->bind_run = init_binds();
    add_bind(&game->bind_run, BindType::KEY, key_left_shift);
    add_bind(&game->bind_run, BindType::GAMEPAD_BUTTON, gamepad_btn_x);

    game->bind_jump = init_binds();
    add_bind(&game->bind_jump, BindType::KEY, key_space);
    add_bind(&game->bind_jump, BindType::GAMEPAD_BUTTON, gamepad_btn_a);
    
    game->bind_croutch = init_binds();
    add_bind(&game->bind_croutch, BindType::KEY, key_s);
    add_bind(&game->bind_croutch, BindType::KEY, key_arrow_down);
    add_bind(&game->bind_croutch, BindType::GAMEPAD_BUTTON, gamepad_dpad_down);

    game->bind_throw = init_binds();
    add_bind(&game->bind_throw, BindType::KEY, key_x);
    add_bind(&game->bind_throw, BindType::GAMEPAD_BUTTON, gamepad_btn_x);
    // add_bind(&game->bind_throw, BindType::GAMEPAD_BUTTON, gamepad_btn_b);

    game->bind_goto_menu = init_binds();
    add_bind(&game->bind_goto_menu, BindType::KEY, key_escape);
    add_bind(&game->bind_goto_menu, BindType::GAMEPAD_BUTTON, gamepad_btn_y); // @todo SELECT/MENU

    return game;
}

static
void set_level(Game *game, int32_t world_idx, int32_t level_idx) {
    assert(world_idx >= 0 && world_idx < WORLD_NUM && level_idx >= 0 && level_idx < LEVEL_NUM, "Invalid level.");
    game->world_idx = world_idx;
    game->level_idx = level_idx;
}

static 
Level *advance_level(Game *game, int32_t delta) {
    assert(delta == -1 || delta == 1);

    game->level_idx += delta;
    if(game->level_idx >= LEVEL_NUM) {
        game->level_idx = 0;
        game->world_idx += 1;
        if(game->world_idx >= WORLD_NUM) {
            game->world_idx = 0;
        }
    } else if(game->level_idx < 0) {
        game->level_idx = LEVEL_NUM - 1;
        game->world_idx -= 1;
        if(game->world_idx < 0) {
            game->world_idx = WORLD_NUM - 1;
        }
    }

    return game->levels[game->world_idx][game->level_idx];
}

static
void recalculate_draw_rect(Game *game) {
    const float32_t window_aspect = (float32_t)game->window_w / (float32_t)game->window_h;

    if(game->fit_to_window) {
        if(window_aspect > GAME_ASPECT) {
            game->draw_rect.wh = vec2i { (int32_t)roundf(game->window_h * GAME_ASPECT), game->window_h };
        } else {
            game->draw_rect.wh = vec2i { game->window_w, (int32_t)roundf(game->window_w / GAME_ASPECT) };
        }
    } else {
        if(game->render_pixel_multiplier == 0) {
            float32_t diff = (window_aspect > GAME_ASPECT) ? (game->window_h / (float32_t)GAME_HEIGHT) : (game->window_w / (float32_t)GAME_WIDTH);
            game->draw_rect.wh = vec2i { GAME_WIDTH, GAME_HEIGHT } * clamp_min((int32_t)floorf(diff), 1);
        } else {
            game->draw_rect.wh = vec2i { GAME_WIDTH, GAME_HEIGHT } * game->render_pixel_multiplier;
        }
    }

    game->draw_rect.xy = { game->window_w / 2 - game->draw_rect.w / 2, game->window_h / 2 - game->draw_rect.h / 2 };
}

void resize_game(Game *game, int32_t window_w, int32_t window_h) {
    game->window_w = window_w;
    game->window_h = window_h;

    recalculate_draw_rect(game);

    // Recalc debug framebuffer
    int32_t debug_fb_w = game->window_w;
    int32_t debug_fb_h = game->window_h;
    float32_t window_aspect = (float32_t)game->window_w / (float32_t)game->window_h;
    if(window_aspect > GAME_ASPECT) {
        debug_fb_w = (int32_t)roundf(game->window_h * GAME_ASPECT);
    } else {
        debug_fb_h = (int32_t)roundf(game->window_w / GAME_ASPECT);
    }
    delete_framebuffer(game->debug_framebuffer);
    game->debug_framebuffer = create_framebuffer(debug_fb_w, debug_fb_h);
}

void delete_game(Game *game) {
    // Delete levels
    delete_level(game->custom_level);
    for(int32_t world_idx = 0; world_idx < WORLD_NUM; ++world_idx) {
        for(int32_t level_idx = 0; level_idx < WORLD_NUM; ++level_idx) {
            Level *level = game->levels[world_idx][level_idx];
            delete_level(level);
        }
    }

    delete_font(game->gp_info_font);
    free(game);
}

static void _update_debug_game_stuff(Game *game, Input *input, bool *do_update_game);
static void _render_debug_panel(Game *game, Level *level, RenderStats game_render_stats);
static void _update_debug_render_view(Game *game, Input *input);

static void go_back_to_main_menu(Game *game) {
    game->top_score = max_value(game->top_score, game->points);
    reset_session(game);
    prepare_main_menu();
    game->next_game_mode = EGameMode::MAIN_MENU;
}

void update_game(Game *game, Input *input, float64_t delta_time) {
    game->delta_time = delta_time;
    game->elapsed_time += game->delta_time;
    game->elapsed_frames += 1;

    game->game_mode = game->next_game_mode;
    game->next_game_mode = game->game_mode;

#if defined(_DEBUG_MODE)
    bool do_update_game = true;
    _update_debug_game_stuff(game, input, &do_update_game);
    if(!do_update_game) {
        return;
    }
#endif /* defined(_DEBUG_MODE) */

    switch(game->game_mode) {
        case EGameMode::PLAYING: {
            if(check_state(&game->bind_goto_menu, input, input_released)) {
                game->game_lost = false;
                game->game_won  = false;
                go_back_to_main_menu(game);
                break;
            }

            SimInput s_input = { };
            s_input.player_move_r  = check_state(&game->bind_move_r,  input, input_is_down);
            s_input.player_move_l  = check_state(&game->bind_move_l,  input, input_is_down);
            s_input.player_run     = check_state(&game->bind_run,     input, input_is_down);
            s_input.player_jump    = check_state(&game->bind_jump,    input, input_is_down);
            s_input.player_croutch = check_state(&game->bind_croutch, input, input_is_down);
            s_input.player_throw   = check_state(&game->bind_throw,   input, input_pressed);
            s_input.player_try_entering_pipe_v = check_state(&game->bind_croutch, input, input_pressed);
            s_input.player_try_entering_pipe_h = check_state(&game->bind_move_l, input, input_is_down) || check_state(&game->bind_move_r, input, input_is_down);

            _update_debug_render_view(game, input);

            auto level = get_current_level(game);
            update_level(level, s_input, game, game->delta_time);

            if(level->player_has_died == true) {
                game->game_lost = true;
                game->game_won  = false;
                go_back_to_main_menu(game);
                break;
            } else if(level->level_state == ELevelState::FINISHED) {
                if((game->level_idx == LEVEL_NUM - 1 && game->world_idx == WORLD_NUM - 1) || game->use_custom_level) { // Last level finished
                    game->game_lost = false;
                    game->game_won  = true;
                    go_back_to_main_menu(game);
                    break;
                } else {
                    Player *player = get_player(level);
                    Level  *next   = advance_level(game, 1);
                    Player *player_in_next_level = get_player(next);
                    if(player_in_next_level != NULL && player != NULL) {
                        set_player_mode(player_in_next_level, player->mode);
                    }

                    setup_level_transition(game->world_idx + 1, game->level_idx + 1);
                    game->next_game_mode = EGameMode::LEVEL_TRANSITION;
                    break;
                }
            }
        } break;

        case EGameMode::LEVEL_TRANSITION: {
            update_level_transition(delta_time);
            if(level_transition_finished()) {
                game->next_game_mode = EGameMode::PLAYING;

                auto level = get_current_level(game);
                set_starting_level_music(level);
            }
        } break;

        case EGameMode::MAIN_MENU: {
            EMenuOption option = update_main_menu(input, delta_time);
            switch(option) {
                case EMenuOption::START_1_PLAYER: {
                    reset_session(game);
                    reset_all_levels(game);

                    game->use_custom_level = false;
                    set_level(game, game->start_world_idx, game->start_level_idx);
                    setup_level_transition(game->start_world_idx + 1, game->start_level_idx + 1);
                    game->next_game_mode = EGameMode::LEVEL_TRANSITION;
                } break;

                case EMenuOption::START_CUSTOM_LEVEL: {
                    if(game->custom_level == NULL || game->custom_level->entities.count == 0) break;

                    reset_session(game);
                    reset_all_levels(game);

                    game->use_custom_level= true;
                    setup_level_transition(0, 0);
                    game->next_game_mode = EGameMode::LEVEL_TRANSITION;
                } break;

                case EMenuOption::QUIT_GAME: {
                   game->should_quit_game = true;
                } break;

                case EMenuOption::TOGGLE_FULLSCREEN: {
                    game->request_fullscreen = !game->request_fullscreen;
                } break;

                case EMenuOption::TOGGLE_PIXEL_MODE: {
                    game->fit_to_window = !game->fit_to_window;
                    recalculate_draw_rect(game);
                } break;

                case EMenuOption::CHANGE_PIXEL_SIZE: {
                    if(game->fit_to_window) {
                        break;
                    }
                    const float32_t diff = ((float32_t)game->window_w / (float32_t)game->window_h > GAME_ASPECT) ? (game->window_h / (float32_t)GAME_HEIGHT) : (game->window_w / (float32_t)GAME_WIDTH);
                    
                    const int32_t max_multiplier = 4;
                    game->render_pixel_multiplier += 1;
                    if(game->render_pixel_multiplier > max_multiplier) {
                        game->render_pixel_multiplier = 0;
                    }

                    recalculate_draw_rect(game);

                    if((int32_t)floorf(diff) < game->render_pixel_multiplier) {
                        game->request_fit_window_to_draw_rect = !game->request_fit_window_to_draw_rect;
                    }
                } break;

                case EMenuOption::RESIZE_WINDOW_TO_DRAW_RECT: {
                    game->request_fit_window_to_draw_rect = !game->request_fit_window_to_draw_rect;
                } break;

                case EMenuOption::CHANGE_STARTING_WORLD: {
                    game->start_world_idx += 1;
                    if(game->start_world_idx >= WORLD_NUM) {
                        game->start_world_idx = 0;
                    }
                } break;

                case EMenuOption::CHANGE_STARTING_LEVEL: {
                    game->start_level_idx += 1;
                    if(game->start_level_idx >= LEVEL_NUM) {
                        game->start_level_idx = 0;
                    }
                } break;

                case EMenuOption::RELOAD_LEVELS: {
                    reload_all_levels(game);
                } break;
            }
        } break;
    }
}

void reset_session(Game *game) {
    game->coins  = 0;
    game->points = 0;
}

#include "data.h"

void render_gameplay_info(Game *game) {
    Font *gp_info_font = game->gp_info_font;
    const int32_t margin = 32;
    const int32_t margin_top = 8;
    const int32_t gp_info_text_z = 0;
    const int32_t base_y_pos = GAME_HEIGHT - gp_info_font->height - margin_top;

    /* --- POINTS (1) --- */
    const char *n1_text_top = "MARIO";
    char        n1_text_bot[64] = { };
    sprintf_s(n1_text_bot, array_count(n1_text_bot), "%.06d", game->points);

    const vec2i n1_pos_top = { margin, base_y_pos };
    const vec2i n1_pos_bot = { n1_pos_top.x, n1_pos_top.y - gp_info_font->height };

    render::r_text(n1_pos_top, gp_info_text_z, n1_text_top, gp_info_font);
    render::r_text(n1_pos_bot, gp_info_text_z, n1_text_bot, gp_info_font);

    /* --- LIVES (2) --- */
    char n2_text[64];
    sprintf_s(n2_text, array_count(n2_text), "%.02d", game->coins);
    const vec2i n2_pos = { n1_pos_bot.x + 76, n1_pos_bot.y };
    render::r_text(n2_pos, gp_info_text_z, n2_text, gp_info_font);

    Sprite sprite_ui_x = global_data::get_sprite(SPRITE_UI_X);
    Sprite sprite_coin = get_current_frame(&game->gp_info_coin_anim_player)->sprite;

    const vec2i n2_x_pos    = { n2_pos.x - sprite_ui_x.width, n2_pos.y };
    const vec2i n2_coin_pos = { n2_x_pos.x - sprite_coin.width, n2_x_pos.y };

    render::r_sprite(n2_x_pos,    gp_info_text_z, { sprite_ui_x.width, sprite_ui_x.height }, sprite_ui_x, color::white);
    render::r_sprite(n2_coin_pos, gp_info_text_z, { sprite_coin.width, sprite_coin.height }, sprite_coin, color::white);

    update_anim(&game->gp_info_coin_anim_player, game->delta_time);

    /* --- TIME (4) --- */
    const char *n4_text_top = "TIME";
    char        n4_text_bot[64] = { };

    auto current_level = get_current_level(game);
    if(current_level != NULL && current_level->disable_level_timer != true) {
        sprintf_s(n4_text_bot, array_count(n4_text_bot), "%.03d", current_level->level_time);
    }

    const int32_t n4_text_top_width = (int32_t)roundf(gp_info_font->calc_string_width(n4_text_top));
    const vec2i n4_pos_top = { GAME_WIDTH - margin - n4_text_top_width, GAME_HEIGHT - gp_info_font->height - margin_top };
    const vec2i n4_pos_bot = { n4_pos_top.x, n4_pos_top.y - gp_info_font->height };

    render::r_text(n4_pos_top, gp_info_text_z, n4_text_top, gp_info_font);
    if(game->game_mode == EGameMode::PLAYING) {
        render::r_text(n4_pos_bot, gp_info_text_z, n4_text_bot, gp_info_font);
    }

    /* --- WORLD (3) --- */
    const char *n3_text_top = "WORLD";
    char        n3_text_bot[64];
    sprintf_s(n3_text_bot, array_count(n3_text_bot), "%d-%d", game->world_idx + 1, game->level_idx + 1);
    
    const float32_t n3_text_top_width = gp_info_font->calc_string_width(n3_text_top);
    const float32_t n3_text_bot_width = gp_info_font->calc_string_width(n3_text_bot);

    const vec2i n3_pos_top = { n4_pos_top.x - 56, base_y_pos };
    const vec2i n3_pos_bot = { n3_pos_top.x + (int32_t)roundf((n3_text_top_width - n3_text_bot_width) * 0.5f), n3_pos_top.y - gp_info_font->height };

    render::r_text(n3_pos_top, gp_info_text_z, n3_text_top, gp_info_font);
    render::r_text(n3_pos_bot, gp_info_text_z, n3_text_bot, gp_info_font);

#if 0
    // @delete maybe
    if(get_player(get_current_level(game)) && get_player(get_current_level(game))->mode == PLAYER_IS_FIRE) {
        char b[32];
        sprintf_s(b,32,"%d fireballs", 3 - get_current_level(game)->entities.count_of_type[entity_type_id(Fireball)]);
        render::r_text(n1_pos_bot - vec2i{0, gp_info_font->height + 4}, gp_info_text_z, b, gp_info_font);
    }
#endif
}

void render_game(Game *game) {
    auto level = get_current_level(game);
    
    render::r_reset_stats();

    game->framebuffer->bind();
    gl_clear({ 0.2f, 0.2f, 0.2f, 1.0f }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    game->debug_framebuffer->bind();
    gl_clear({ 0.0f, 0.0f, 0.0f, 0.0f }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto view = game->use_debug_render_view ? &game->debug_render_view : &level->render_view;

    // Don't render frame when changing game modes
    if(game->game_mode == game->next_game_mode) {
        switch(game->game_mode) {
            case EGameMode::PLAYING: {
                RenderSetup setup;
                setup.proj_m = view->calc_proj();
                setup.view_m = view->calc_view();
                setup.viewport = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
                setup.framebuffer = game->framebuffer;

                render::r_begin(setup);
                render_level(level);
                render::r_end();

#if defined(_DEBUG_MODE)
                // Render debug stuff
                if(game->debug_panel_perc != 0.0f) {
                    RenderSetup dsetup;
                    dsetup.proj_m = view->calc_proj();
                    dsetup.view_m = view->calc_view();
                    dsetup.viewport = { 0, 0, game->debug_framebuffer->width, game->debug_framebuffer->height };
                    dsetup.framebuffer = game->debug_framebuffer;

                    render::r_begin(dsetup);
                    render_level_debug_stuff(level);
                    render::r_end();
                }
#endif /* defined(_DEBUG_MODE) */
            } break;

            case EGameMode::LEVEL_TRANSITION: {
                RenderSetup setup;
                setup.proj_m = mat4x4::orthographic(0, 0, GAME_WIDTH, GAME_HEIGHT, -100, 100);
                setup.view_m = mat4x4::identity();
                setup.viewport = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
                setup.framebuffer = game->framebuffer;

                render::r_begin(setup);
                render_level_transition(GAME_WIDTH, GAME_HEIGHT);
                render::r_end();
            } break;

            case EGameMode::MAIN_MENU: {
                render_main_menu(game->framebuffer, game);
            } break;
        }
    }

    // Render gameplay info
    const bool should_render_gameplay_info = game->game_mode == EGameMode::PLAYING || game->game_mode == EGameMode::LEVEL_TRANSITION || game->game_mode == EGameMode::MAIN_MENU;
    if(should_render_gameplay_info) {
        RenderSetup gp_info_setup;
        gp_info_setup.proj_m = mat4x4::orthographic(0, 0, GAME_WIDTH, GAME_HEIGHT, -100.0f, 100.0f);
        gp_info_setup.view_m = mat4x4::identity();
        gp_info_setup.viewport = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
        gp_info_setup.framebuffer = game->framebuffer;
        render::r_begin(gp_info_setup);
        render_gameplay_info(game);
        render::r_end();
    }

    auto game_render_stats = render::r_reset_stats();

    RenderSetup bsetup;
    bsetup.proj_m = mat4x4::orthographic(0.0f, 0.0f, game->window_w, game->window_h, -10.0f, 10.0f);
    bsetup.view_m = mat4x4::identity();
    bsetup.framebuffer = NULL;
    bsetup.viewport = { 0, 0, game->window_w, game->window_h };
    render::r_begin(bsetup);
    render::r_texture(game->draw_rect.xy, 6, game->draw_rect.wh, game->framebuffer->color);
    _render_debug_panel(game, level, game_render_stats);
    render::r_end();
    render::r_reset_stats();
}

/* Debug stuff */

static void _update_debug_render_view(Game *game, Input *input) {
#if defined(_DEBUG_MODE)
    if(game->use_debug_render_view) {
        game->mouse_in_view_space_prev = convert_to_view_space({ input->x_mouse_prev, game->window_h - input->y_mouse_prev }, game->draw_rect, &game->debug_render_view);
        game->mouse_in_view_space      = convert_to_view_space({ input->x_mouse, game->window_h - input->y_mouse },           game->draw_rect, &game->debug_render_view);
    
        game->debug_render_view.scale += input->scroll_move * game->debug_render_view.scale * 0.075f;
        clamp(&game->debug_render_view.scale, 0.2f, 5.0f);
    
        if(input->btns[btn_right] & input_is_down) {
            vec2 diff = game->mouse_in_view_space - game->mouse_in_view_space_prev;
            game->debug_render_view.x_translate -= diff.x;
            game->debug_render_view.y_translate -= diff.y;
        }
    }
#endif /* defined(_DEBUG_MODE) */
}

static void _render_debug_panel(Game *game, Level *level, RenderStats game_render_stats) {
    #if defined(_DEBUG_MODE)
    render::r_texture(game->draw_rect.xy, 5, game->draw_rect.wh, game->debug_framebuffer->color, { 1.0f, 1.0f, 1.0f, game->debug_panel_perc });

    // Draw debug panel
    if(game->debug_panel_perc != 0.0f) {
        std::vector<std::string> strings;

        int32_t max_width = 0;
        Font *font = render::def_font();
        int32_t y_pos = game->window_h - font->height;
        char buffer[128];
        auto text_l = [&] (const char *string, ...) {
            va_list args;
            va_start(args, string);
            vsprintf_s(buffer, array_count(buffer), string, args);
            int32_t width = (int32_t)roundf(font->calc_string_width(buffer));
            max_width = max_value(max_width, width);
            strings.push_back(buffer);
            y_pos -= font->height;
            va_end(args);
        };

        text_l("  delta time: %.10f", game->delta_time);
        text_l("elapsed time: %.9f", game->elapsed_time);
        text_l("elapsed frames: %d", game->elapsed_frames);
        text_l("1 / delta time: %d", (int32_t)round(1.0 / game->delta_time));
        text_l("game mode: %s", game_mode_cstr[(int32_t)game->game_mode]);
        text_l(" ---");
        text_l("game size:   %d %d", GAME_WIDTH, GAME_HEIGHT);
        text_l("render size: %d %d", game->draw_rect.w, game->draw_rect.h);
        text_l("scale mode:  %s", game->fit_to_window ? "fit to window" : "pixel perfect");
        text_l("debug render view: %s", BOOL_STRING(game->use_debug_render_view));
        text_l(" ---");
        text_l("draw calls:  %d", game_render_stats.draw_calls);
        text_l("quads drawn: %d", game_render_stats.quads);
        text_l("lines drawn: %d", game_render_stats.lines);
        text_l("text quads:  %d", game_render_stats.text_quads);
        text_l(" ---");
        text_l("music is playing: %s", BOOL_STRING(audio_player::a_is_music_playing()));
        text_l("music is paused:  %s", BOOL_STRING(audio_player::a_is_music_paused()));
        text_l(" ---");
        // text_l("level memory ptr: %p", level->memory);
        text_l("is paused: %s", BOOL_STRING(level->pause_state));
        auto __level_state_str = [] (Level *level) -> std::string {
            switch(level->level_state) {
                default: return "!Unknown!";
                case ELevelState::PLAYING: return "Playing";
                case ELevelState::FINISHING_LEVEL: return "Finishing";
                case ELevelState::FINISHED: return "Finished";
            }
        };
        text_l("level state: %s", __level_state_str(level).c_str());
        text_l("level time:  %.4f", level->elapsed_time);
        for(int32_t idx = 0; idx < level->no_paused_entities_count; ++idx) {
            if(idx >= 4) {
                text_l("... (%d more)", level->no_paused_entities_count - idx);
                break;
            }
            text_l("unpaused entity: %p", level->no_paused_entities[idx]);
        }
        text_l("best region: %p", get_entity_by_unique_id_m(level, level->current_region_unique_id, CameraRegion));
        text_l(" ---");
        text_l("entity types: %d", entity_type_count());
        text_l("memory size:  %db", level->memory_size);
        text_l("memory used:  %db, %.1f%%", level->memory_used, ((float64_t)level->memory_used / (float64_t)level->memory_size) * 100.0);
        text_l("  used entities: %d", level->entities.count);
        text_l("unused entities: %d", level->entities_unused.count);
        if(level->entities.first_of_type[entity_type_id(Player)]) {
            text_l(" ---");
            auto player = level->entities.first_of_type[entity_type_id(Player)]->as<Player>();
            text_l("Player     %p", player);
            text_l("position:  %d %d", player->position.x, player->position.y);
            text_l("state:     %s", player_state_string[player->state]);
            text_l("mode:      %s", player_mode_string[player->mode]);
            text_l("invincible %s", BOOL_STRING(player->in_invicible_state));
            text_l("grounded:  %s, prev: %s", BOOL_STRING(player->has_move_data->is_grounded), BOOL_STRING(player->has_move_data->is_grounded_prev));
            text_l("speed:     %+06.1f %+06.1f", player->move_data.speed.x, player->move_data.speed.y);
            text_l("jump_t:    %+.3f", player->jump_t);
            text_l("has_star:  %s", BOOL_STRING(player->has_star_powerup));
        }
        text_l(" ---");

        int32_t bg_y_size = game->window_h - y_pos + font->descent * font->scale_for_pixel_height;
        int32_t bg_x_size = (max_width + 8) * game->debug_panel_perc;

        render::r_scissor_begin({ 0, game->window_h - bg_y_size, bg_x_size, bg_y_size });
        render::r_quad({ 0, game->window_h - bg_y_size }, 1, { bg_x_size, bg_y_size }, { 0.0f, 0.0f, 0.0f, 0.5f });
        y_pos = game->window_h - font->height;
        for(const auto &entry : strings) {
            render::r_text({ 0, y_pos }, 0, entry.c_str(), font);
            y_pos -= font->height;
        }
        render::r_scissor_end();
    }
#endif /* defined(_DEBUG_MODE) */
}

static void _update_debug_game_stuff(Game *game, Input *input, bool *do_update_game) {
#if defined(_DEBUG_MODE)
    if(input->keys[key_left_ctrl] & input_is_down) { // Skip frames (slowmo)
        if((game->elapsed_frames % 6) != 0) {
            *do_update_game = false;
        }
    }

    if(input->keys[key_f01] & input_pressed)  { 
        auto player = get_player(get_current_level(game));
        if(player != NULL) {
            set_player_mode(player, PLAYER_IS_FIRE);
            apply_star_powerup(player);
        }
    }
    
    if(input->keys[key_f02] & input_pressed) {
        game->debug_panel_open = !game->debug_panel_open;
    }

    if(input->keys[key_f03] & input_pressed) {
        audio_player::a_stop_sounds();
        audio_player::a_stop_music();
        reload_all_levels(game);
        set_starting_level_music(get_current_level(game));
    }

    if(input->keys[key_f04] & input_pressed) {
        game->use_debug_render_view = !game->use_debug_render_view;
    }

    if(input->keys[key_f05] & input_pressed) { 
        game->use_custom_level = !game->use_custom_level;
    }

    if(input->keys[key_page_up]  & input_pressed) { 
        advance_level(game, +1);
        set_level_music_and_play(get_current_level(game));
    }

    if(input->keys[key_page_down] & input_pressed) { 
        advance_level(game, -1);
        set_level_music_and_play(get_current_level(game));
    }

    // Open / Close debug panel
    const float32_t eps = 0.01f;
    if(game->debug_panel_open && game->debug_panel_perc != 1.0f) {
        game->debug_panel_perc = move_toward(game->debug_panel_perc, 1.0f, game->delta_time * 4.0f);
        if(game->debug_panel_perc >= 1.0f - eps) {
            game->debug_panel_perc = 1.0f;
        }
    } else if(!game->debug_panel_open && game->debug_panel_perc != 0.0f) {
        game->debug_panel_perc = move_toward(game->debug_panel_perc, 0.0f, game->delta_time * 4.0f);
        if(game->debug_panel_perc <= eps) {
            game->debug_panel_perc = 0.0f;
        }
    }
#endif /* defined(_DEBUG_MODE) */
}