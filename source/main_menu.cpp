#include "main_menu.h"
#include "game.h"
#include "level.h"
#include "data.h"
#include "audio_player.h"

enum EMenuPage {
    PAGE_MAIN,
    PAGE_OPTIONS_1,
    PAGE_OPTIONS_2,
    PAGE_OPTIONS_3,
    PAGE__COUNT
};

#define OPTIONS_PER_PAGE 3

enum {
    // PAGE_MAIN
    OPTION_1_PLAYER_GAME = 0,
    OPTION_PLAY_CUSTOM_LEVEL,
    OPTION_QUIT_GAME,

    // PAGE_OPTIONS_1
    OPTION_TOGGLE_FULLSCREEN = 0,
    OPTION_TOOGLE_PIXEL_MODE,
    OPTION_CHANGE_PIXEL_SIZE,

    // PAGE_OPTIONS_2
    OPTION_RESIZE_GAME_WINDOW = 0,
    OPTION_CHANGE_STARTING_WORLD,
    OPTION_CHANGE_STARTING_LEVEL,

    // PAGE_OPTIONS_3
    OPTION_MUTE_MUSIC = 0,
    OPTION_RELOAD_LEVELS = 1
};

namespace {
    int32_t menu_cursor;
    int32_t menu_page_id;

    float32_t menu_setup_finished;
    float32_t menu_setup_timer;
    constexpr float32_t menu_setup_time = 0.35f;

    // Background level
    Level *menu_level;
    LevelSaveData menu_level_sd;

    InputBinds bind_menu_down;
    InputBinds bind_menu_up;
    InputBinds bind_menu_left;
    InputBinds bind_menu_right;
    InputBinds bind_menu_select;
    InputBinds bind_menu_exit;
}

void init_main_menu(void) {
    menu_level = create_empty_level();
    menu_level_sd = parse_level_save_data((global_data::get_data_path() + "\\levels\\main_menu.level").c_str());

    prepare_main_menu();

    bind_menu_down = init_binds();
    add_bind(&bind_menu_down, BindType::KEY, key_s);
    add_bind(&bind_menu_down, BindType::KEY, key_arrow_down);
    add_bind(&bind_menu_down, BindType::GAMEPAD_BUTTON, gamepad_dpad_down);

    bind_menu_up = init_binds();
    add_bind(&bind_menu_up, BindType::KEY, key_w);
    add_bind(&bind_menu_up, BindType::KEY, key_arrow_up);
    add_bind(&bind_menu_up, BindType::GAMEPAD_BUTTON, gamepad_dpad_up);

    bind_menu_left = init_binds();
    add_bind(&bind_menu_left, BindType::KEY, key_a);
    add_bind(&bind_menu_left, BindType::KEY, key_arrow_left);
    add_bind(&bind_menu_left, BindType::GAMEPAD_BUTTON, gamepad_dpad_left);

    bind_menu_right = init_binds();
    add_bind(&bind_menu_right, BindType::KEY, key_d);
    add_bind(&bind_menu_right, BindType::KEY, key_arrow_right);
    add_bind(&bind_menu_right, BindType::GAMEPAD_BUTTON, gamepad_dpad_right);

    bind_menu_select = init_binds();
    add_bind(&bind_menu_select, BindType::KEY, key_enter);
    add_bind(&bind_menu_select, BindType::GAMEPAD_BUTTON, gamepad_btn_a);

    bind_menu_exit = init_binds();
    add_bind(&bind_menu_exit, BindType::KEY, key_escape);
    add_bind(&bind_menu_exit, BindType::GAMEPAD_BUTTON, gamepad_btn_b);
}

void free_main_menu(void) {
    menu_level_sd.entity_save_data.clear();
    delete_level(menu_level);
    menu_level = NULL;
}

void prepare_main_menu(void) {
    recreate_empty_level(&menu_level);
    load_level_from_save_data(menu_level, &menu_level_sd);
    menu_cursor         = 0;
    menu_page_id        = 0;
    menu_setup_timer    = 0.0f;
    menu_setup_finished = false;

    // No music in main menu
    audio_player::a_pause_music();
}

EMenuOption update_main_menu(Input *input, float32_t delta_time) {
    if(menu_setup_finished == false) {
        menu_setup_timer += delta_time;
        if(menu_setup_timer >= menu_setup_time) {
            menu_setup_finished = true;
        } else {
            return MENU_OPTION__NONE;
        }
    }

    audio_player::a_set_allow_play_sounds(false);

    SimInput s_input = { };
    s_input.player_move_r = true;
    s_input.player_try_entering_pipe_h = true;
    update_level(menu_level, s_input, NULL, delta_time);

    audio_player::a_set_allow_play_sounds(true);

    // Move menu cursor
    int32_t opt_dir = 0;
    if(check_state(&bind_menu_up, input, input_pressed))   opt_dir -= 1;
    if(check_state(&bind_menu_down, input, input_pressed)) opt_dir += 1;
    if(opt_dir != 0) {
        menu_cursor += opt_dir;
        if(menu_cursor < 0) {
            menu_cursor = OPTIONS_PER_PAGE - 1;
        } else if(menu_cursor >= OPTIONS_PER_PAGE) {
            menu_cursor = 0;
        }
        audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));
    }

    if(check_state(&bind_menu_right, input, input_pressed)) {
        menu_page_id += 1;
        if(menu_page_id >= PAGE__COUNT) {
            menu_page_id = 0;
        }
        audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));
    }

    if(check_state(&bind_menu_left, input, input_pressed)) {
        menu_page_id -= 1;
        if(menu_page_id < 0) {
            menu_page_id = PAGE__COUNT - 1;
        }
        audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));
    }

    if(check_state(&bind_menu_select, input, input_pressed)) {
        audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));


        switch(menu_page_id) {
            case PAGE_MAIN: {
                switch(menu_cursor) {
                    case OPTION_1_PLAYER_GAME: {
                        return EMenuOption::START_1_PLAYER;
                    };
                    case OPTION_PLAY_CUSTOM_LEVEL: {
                        return EMenuOption::START_CUSTOM_LEVEL;
                    } break;
                    case OPTION_QUIT_GAME: {
                        return EMenuOption::QUIT_GAME;
                    };
                }
            } break;

            case PAGE_OPTIONS_1: {
                switch(menu_cursor) {
                    case OPTION_TOGGLE_FULLSCREEN: {
                        return EMenuOption::TOGGLE_FULLSCREEN;
                    } break;
                    case OPTION_TOOGLE_PIXEL_MODE: {
                        return EMenuOption::TOGGLE_PIXEL_MODE;
                    };
                    case OPTION_CHANGE_PIXEL_SIZE: {
                        return EMenuOption::CHANGE_PIXEL_SIZE;
                    };
                }
            } break;

            case PAGE_OPTIONS_2: {
                switch(menu_cursor) {
                    case OPTION_RESIZE_GAME_WINDOW: {
                        return EMenuOption::RESIZE_WINDOW_TO_DRAW_RECT;
                    };

                    case OPTION_CHANGE_STARTING_WORLD: {
                        return EMenuOption::CHANGE_STARTING_WORLD;
                    };

                    case OPTION_CHANGE_STARTING_LEVEL: {
                        return EMenuOption::CHANGE_STARTING_LEVEL;
                    };
                }
            } break;

            case PAGE_OPTIONS_3: {
                switch(menu_cursor) {
                    case OPTION_MUTE_MUSIC: {
                        // @todo Must be better way
                        int32_t volume = audio_player::a_get_music_volume();
                        if(volume == 0) {
                            int32_t last_volume = audio_player::a_get_last_set_music_volume();
                            audio_player::a_set_music_volume(last_volume);
                        } else {
                            audio_player::a_set_music_volume(0);
                        }
                    } break;

                    case OPTION_RELOAD_LEVELS: {
                        return EMenuOption::RELOAD_LEVELS;
                    };
                }
            } break;
        }
        return MENU_OPTION__NONE;
    }

    if(check_state(&bind_menu_exit, input, input_released)) { 
        return EMenuOption::QUIT_GAME;
    }

    return MENU_OPTION__NONE;
}

void render_main_menu(Framebuffer *framebuffer, const struct Game *game) {
#define MAX_OPTION_LEN 128
    char (menu_options[PAGE__COUNT][3])[MAX_OPTION_LEN] = {
        {
            "1 PLAYER GAME",
            "PLAY CUSTOM LEVEL",
            "QUIT",
        },
        {
            "", // Set below
            "", // Set below
            ""  // Set below
        },
        {
            "RESIZE WINDOW",
            "", // Set below
            "", // Set below
        },
        {
            "", // Set below
            "RELOAD LEVELS", 
            "",
        }
    };
    strcpy_s(menu_options[PAGE_OPTIONS_1][OPTION_TOGGLE_FULLSCREEN],  MAX_OPTION_LEN, game->request_fullscreen ? "SET WINDOWED" : "SET FULLSCREENED");
    strcpy_s(menu_options[PAGE_OPTIONS_1][OPTION_TOOGLE_PIXEL_MODE],  MAX_OPTION_LEN, game->fit_to_window ? "STRETCH TO WINDOW" : "PIXEL PERFECT");
    if(game->render_pixel_multiplier == 0) {
        strcpy_s(menu_options[PAGE_OPTIONS_1][OPTION_CHANGE_PIXEL_SIZE],  MAX_OPTION_LEN, "AUTO PIXEL SIZE");
    } else {
        sprintf_s(menu_options[PAGE_OPTIONS_1][OPTION_CHANGE_PIXEL_SIZE], MAX_OPTION_LEN, "PIXEL SIZE %d", game->render_pixel_multiplier);
    }
    sprintf_s(menu_options[PAGE_OPTIONS_2][OPTION_CHANGE_STARTING_WORLD], MAX_OPTION_LEN, "WORLD %d", game->start_world_idx + 1);
    sprintf_s(menu_options[PAGE_OPTIONS_2][OPTION_CHANGE_STARTING_LEVEL], MAX_OPTION_LEN, "LEVEL %d", game->start_level_idx + 1);
    sprintf_s(menu_options[PAGE_OPTIONS_3][OPTION_MUTE_MUSIC], MAX_OPTION_LEN, audio_player::a_get_music_volume() == 0 ? "UNMUTE MUSIC" : "MUTE MUSIC");

    if(menu_setup_finished == false) {
        RenderSetup setup;
        setup.proj_m = mat4x4::orthographic(0, 0, GAME_WIDTH, GAME_HEIGHT, -10, 10);
        setup.view_m = mat4x4::identity();
        setup.viewport = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
        setup.framebuffer = framebuffer;
        render::r_begin(setup);
        render::r_quad({ 0, 0 }, 0, vec2i { GAME_WIDTH, GAME_HEIGHT }, color::black);
        render::r_end();
        return;
    }

    Font *font = global_data::get_small_font();

    /* Render background level */ {
        RenderSetup _level_setup;
        _level_setup.proj_m = menu_level->render_view.calc_proj();
        _level_setup.view_m = menu_level->render_view.calc_view();
        _level_setup.viewport = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
        _level_setup.framebuffer = framebuffer;
        render::r_begin(_level_setup);
        render_level(menu_level);
        render::r_end();
    }

    RenderSetup setup;
    setup.proj_m = mat4x4::orthographic(0, 0, GAME_WIDTH, GAME_HEIGHT, -10, 10);
    setup.view_m = mat4x4::identity();
    setup.viewport = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
    setup.framebuffer = framebuffer;

    render::r_begin(setup);
    render::r_set_line_width(1.0f);

    vec2i logo_pos  = { };
    vec2i logo_size = { };

    /* Game logo */ {
        const auto logo_sprite = global_data::get_sprite(SPRITE_UI_MENU_LOGO);

        logo_size  = { logo_sprite.width, logo_sprite.height };
        logo_pos = {
            GAME_WIDTH / 2 - logo_size.x / 2,
            GAME_HEIGHT - logo_size.y - 8 * 4
        };

        render::r_sprite(logo_pos, 1, logo_size, logo_sprite, color::white);
    }

    // Text that appears in main menu if you've finished or lost the game
    if(game->game_mode == EGameMode::MAIN_MENU) {
        const auto    font  = game->gp_info_font;
        const int32_t z_pos = -5;
        const vec2i   text_pos_base = logo_pos + logo_size / 2 + vec2i{8, font->height + 19};

        const vec4 text_color = vec4{ 255.0f, 206.0f, 197.0f, 255.0f } / 255.0f;

        auto draw_text_lines = [&](const char *text_array[], size_t array_count, int32_t shadow_offset, vec4 text_color, vec4 shadow_color) {
            // Shadow
            vec2i text_pos = text_pos_base + vec2i { shadow_offset, -shadow_offset };
            for(int32_t idx = 0; idx < array_count; ++idx) {
                render::r_text(text_pos, z_pos, text_array[idx], font, shadow_color);
                text_pos.y -= font->height;
            }

            text_pos = text_pos_base;
            for(int32_t idx = 0; idx < array_count; ++idx) {
                render::r_text(text_pos, z_pos, text_array[idx], font, text_color);
                text_pos.y -= font->height;
            }
        };

#if !defined(_DEBUG_MODE)
        if(game->game_won) {
            const char *lines[] = {
                "Good job,",
                "You've won!"
            };
            draw_text_lines(lines, array_count(lines), 1, text_color, color::black);
        } else if(game->game_lost) {
            const char *lines[] = {
                "You've lost!"
            };
            draw_text_lines(lines, array_count(lines), 1, text_color, color::black);
        }
#else
        const char *text[] = {
            "Debug Mode",
            "F2 - panel",
            "CTRL - slow"
        };
        draw_text_lines(text, array_count(text), 1, text_color, color::black);
#endif
    }

    int32_t last_menu_option_y_pos = 0;

    // @todo Maybe check differently if custom level was loaded
    const bool custom_level_available = game->custom_level != NULL && game->custom_level->entities.count > 0;

    /* Menu options */ {
        for(int32_t idx = 0; idx < array_count(menu_options[0]); ++idx) {

            if(menu_options[menu_page_id][idx] == NULL) {
                continue;
            }

            const int32_t width = (int32_t)font->calc_string_width(menu_options[menu_page_id][idx]);
        
            const vec2i pos = {
                GAME_WIDTH / 2 - width / 2,
                logo_pos.y - font->height - 8 * 2 - 8 * (2 + idx * 2)
            };
        
            vec4 color = color::white;

            // gray out
            if((menu_page_id == PAGE_MAIN && idx == OPTION_PLAY_CUSTOM_LEVEL && !custom_level_available) 
               || (menu_page_id == PAGE_OPTIONS_1 && idx == OPTION_CHANGE_PIXEL_SIZE && game->fit_to_window == true)) {
                color = { 0.7f, 0.7f, 0.7f, 1.0f };
            }

            render::r_text(pos, -1, menu_options[menu_page_id][idx], font, color);
        
            // Menu option cursor
            if(menu_cursor == idx) {
                render::r_sprite({ pos.x - 8 * 2, pos.y }, 0, { 8, 8 }, global_data::get_sprite(SPRITE_UI_SHROOM), color::white);
            }
        
            last_menu_option_y_pos = pos.y;
        }
    }

    /* Top score */ {
        char text[32] = { };
        sprintf_s(text, array_count(text), "TOP - %06d", game->top_score);
        const int32_t width = (int32_t)font->calc_string_width(text);

        const vec2i pos = {
            logo_pos.x + logo_size.x - width,
            logo_pos.y - font->height
        };

        const vec4 color = vec4{ 255.0f, 206.0f, 197.0f, 255.0f } / vec4::make(255.0f);
        render::r_text(pos, -1, text, font, color);
    }

    render::r_end();
}