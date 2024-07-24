#ifndef _MAIN_MENU_H
#define _MAIN_MENU_H

#include "common.h"
#include "input.h"
#include "renderer.h"

enum EMenuOption {
    START_1_PLAYER,
    START_CUSTOM_LEVEL,
    QUIT_GAME,
    TOGGLE_PIXEL_MODE,
    TOGGLE_FULLSCREEN,
    CHANGE_PIXEL_SIZE,
    RESIZE_WINDOW_TO_DRAW_RECT,
    CHANGE_STARTING_WORLD,
    CHANGE_STARTING_LEVEL,
    RELOAD_LEVELS,
    MENU_OPTION__NONE
};

void        init_main_menu(void);
void        free_main_menu(void);
void        prepare_main_menu(void);
EMenuOption update_main_menu(Input *input, float32_t delta_time);
void        render_main_menu(Framebuffer *framebuffer, const struct Game *game /* for menu rendering */);

#endif /* _MAIN_MENU_H */