#ifndef _GAME_H
#define _GAME_H

#include "common.h"
#include "maths.h"
#include "input.h"
#include "opengl_abs.h"
#include "level.h"
#include "renderer.h"
#include "save_data.h"

#define GAME_WIDTH  256
#define GAME_HEIGHT 240
#define GAME_ASPECT ((float32_t)GAME_WIDTH / (float32_t)GAME_HEIGHT)

enum class EGameMode : int32_t {
    PLAYING,
    MAIN_MENU,
    LEVEL_TRANSITION,
    GAME_MODE__COUNT
};

inline const char *game_mode_cstr[] = {
    "playing",
    "main menu",
    "level transisition"
};

static_assert(array_count(game_mode_cstr) == (int32_t)EGameMode::GAME_MODE__COUNT);

struct Game {
    float64_t delta_time;
    float64_t elapsed_time;
    int32_t   elapsed_frames;

    bool should_quit_game;

    EGameMode game_mode;
    EGameMode next_game_mode;

    int32_t start_world_idx;
    int32_t start_level_idx;

#define WORLD_NUM 1
#define LEVEL_NUM 4
    int32_t world_idx;
    int32_t level_idx;
    Level *levels[WORLD_NUM][LEVEL_NUM];
    LevelSaveData level_save_datas[WORLD_NUM][LEVEL_NUM];

    bool   use_custom_level;
    Level *custom_level;
    LevelSaveData custom_level_save_data;

    vec2 mouse_in_view_space_prev;
    vec2 mouse_in_view_space;
    
    /* --- rendering --- */
    bool         request_fit_window_to_draw_rect;
    bool         request_fullscreen;
    bool         fit_to_window;           // false for pixel perfect
    int32_t      render_pixel_multiplier; // 0 means auto
    int32_t      window_w;
    int32_t      window_h;
    Framebuffer *framebuffer;
    recti32      draw_rect;               // viewport for the framebuffer

    /* --- debug stuff --- */
    bool         use_debug_render_view;
    bool         debug_panel_open;
    float32_t    debug_panel_perc;
    RenderView   debug_render_view;
    Framebuffer *debug_framebuffer;

    /* --- gameplay info panel --- */
    Font       *gp_info_font;
    AnimSet     gp_info_ui_coin_anim_set;
    AnimPlayer  gp_info_coin_anim_player;

    /* --- game session --- */
    int32_t coins;
    int32_t points;
    int32_t top_score;
    bool    game_won;
    bool    game_lost;

    /* --- Binds --- */
    InputBinds bind_move_r;
    InputBinds bind_move_l;
    InputBinds bind_run;
    InputBinds bind_jump;
    InputBinds bind_croutch;
    InputBinds bind_throw;
    InputBinds bind_goto_menu;
};

Game *create_game(void);
void  resize_game(Game *game, int32_t window_w, int32_t window_h);
void  delete_game(Game *game);
void  update_game(Game *game, Input *input, float64_t delta_time);
void  render_game(Game *game);
void  reset_session(Game *game);

inline Level *get_current_level(Game *game) { return game->use_custom_level ? game->custom_level : game->levels[game->world_idx][game->level_idx]; }

#endif /* _GAME_H */