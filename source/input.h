#ifndef _INPUT_H
#define _INPUT_H

#include <SDL.h>
#include "common.h"

#define KB_KEY(key_name, sdl_key)
#define KB_KEYS\
    KB_KEY(key_w, SDLK_w)\
    KB_KEY(key_a, SDLK_a)\
    KB_KEY(key_s, SDLK_s)\
    KB_KEY(key_d, SDLK_d)\
    KB_KEY(key_c, SDLK_c)\
    KB_KEY(key_v, SDLK_v)\
    KB_KEY(key_x, SDLK_x)\
    KB_KEY(key_q, SDLK_q)\
    KB_KEY(key_e, SDLK_e)\
    KB_KEY(key_1, SDLK_1)\
    KB_KEY(key_2, SDLK_2)\
    KB_KEY(key_3, SDLK_3)\
    KB_KEY(key_4, SDLK_4)\
    KB_KEY(key_arrow_up, SDLK_UP)\
    KB_KEY(key_arrow_down, SDLK_DOWN)\
    KB_KEY(key_arrow_left, SDLK_LEFT)\
    KB_KEY(key_arrow_right, SDLK_RIGHT)\
    KB_KEY(key_f01, SDLK_F1)\
    KB_KEY(key_f02, SDLK_F2)\
    KB_KEY(key_f03, SDLK_F3)\
    KB_KEY(key_f04, SDLK_F4)\
    KB_KEY(key_f05, SDLK_F5)\
    KB_KEY(key_f06, SDLK_F6)\
    KB_KEY(key_f07, SDLK_F7)\
    KB_KEY(key_f08, SDLK_F8)\
    KB_KEY(key_f09, SDLK_F9)\
    KB_KEY(key_f10, SDLK_F10)\
    KB_KEY(key_f11, SDLK_F11)\
    KB_KEY(key_f12, SDLK_F12)\
    KB_KEY(key_delete, SDLK_DELETE)\
    KB_KEY(key_enter, SDLK_RETURN)\
    KB_KEY(key_space, SDLK_SPACE)\
    KB_KEY(key_home, SDLK_HOME)\
    KB_KEY(key_end, SDLK_END)\
    KB_KEY(key_escape, SDLK_ESCAPE)\
    KB_KEY(key_backspace, SDLK_BACKSPACE)\
    KB_KEY(key_tab, SDLK_TAB)\
    KB_KEY(key_left_ctrl, SDLK_LCTRL)\
    KB_KEY(key_left_shift, SDLK_LSHIFT)\
    KB_KEY(key_left_alt, SDLK_LALT)\
    KB_KEY(key_page_up, SDLK_PAGEUP)\
    KB_KEY(key_page_down, SDLK_PAGEDOWN)
#undef KB_KEY

#define KB_KEY(key_name, sdl_key) key_name,
enum : int32_t { KB_KEYS KB_KEY__COUNT };
#undef KB_KEY

#define KB_KEY(key_name, sdl_key) sdl_key,
inline const int32_t sdl_key[KB_KEY__COUNT] = { KB_KEYS };
#undef KB_KEY

#define KB_KEY(key_name, sdl_key) case sdl_key: return key_name;
inline int32_t from_sdl_key(int32_t sdl_key) {
    switch(sdl_key) { KB_KEYS }
    return -1;
}
#undef KB_KEY

#define KB_KEY(key_name, sdl_key) case key_name: return #key_name;
inline const char *kb_key_string(int32_t key) {
    switch(key) { KB_KEYS }
    return NULL;
}
#undef KB_KEY

#define MOUSE_BTN(btn_name, sdl_btn)
#define MOUSE_BTNS\
    MOUSE_BTN(btn_left, SDL_BUTTON_LEFT)\
    MOUSE_BTN(btn_right, SDL_BUTTON_RIGHT)\
    MOUSE_BTN(btn_middle, SDL_BUTTON_MIDDLE)
#undef MOUSE_BTN

#define MOUSE_BTN(btn_name, sdl_btn) btn_name,
enum : int32_t { MOUSE_BTNS MOUSE_BTN__COUNT };
#undef MOUSE_BTN

#define MOUSE_BTN(btn_name, sdl_btn) sdl_btn,
inline const int32_t sdl_btn[MOUSE_BTN__COUNT] = { MOUSE_BTNS };
#undef MOUSE_BTN

#define MOUSE_BTN(btn_name, sdl_btn) case sdl_btn: return btn_name;
inline int32_t from_sdl_btn(int32_t sdl_btn) {
    switch(sdl_btn) { MOUSE_BTNS }
    return -1;
}
#undef MOUSE_BTN

#define MOUSE_BTN(btn_name, sdl_btn) case btn_name: return #btn_name;
inline const char *mouse_btn_string(int32_t btn) {
    switch(btn) { MOUSE_BTNS }
    return NULL;
}
#undef MOUSE_BTN

#define GAMEPAD_BTN(gp_btn_name, sdl_gp_btn)
#define GAMEPAD_BTNS\
    GAMEPAD_BTN(gamepad_btn_a, SDL_CONTROLLER_BUTTON_A)\
    GAMEPAD_BTN(gamepad_btn_b, SDL_CONTROLLER_BUTTON_B)\
    GAMEPAD_BTN(gamepad_btn_y, SDL_CONTROLLER_BUTTON_Y)\
    GAMEPAD_BTN(gamepad_btn_x, SDL_CONTROLLER_BUTTON_X)\
    GAMEPAD_BTN(gamepad_dpad_up,    SDL_CONTROLLER_BUTTON_DPAD_UP)\
    GAMEPAD_BTN(gamepad_dpad_down,  SDL_CONTROLLER_BUTTON_DPAD_DOWN)\
    GAMEPAD_BTN(gamepad_dpad_left,  SDL_CONTROLLER_BUTTON_DPAD_LEFT)\
    GAMEPAD_BTN(gamepad_dpad_right, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
#undef GAMEPAD_BTN

#define GAMEPAD_BTN(gp_btn_name, gp_sdl_btn) gp_btn_name,
enum : int32_t { GAMEPAD_BTNS GAMEPAD_BTN__COUNT };
#undef GAMEPAD_BTN

#define GAMEPAD_BTN(gp_btn_name, gp_sdl_btn) gp_sdl_btn,
inline const int32_t gamepad_sdl_btn[GAMEPAD_BTN__COUNT] = { GAMEPAD_BTNS };
#undef GAMEPAD_BTN

#define GAMEPAD_BTN(gp_btn_name, gp_sdl_btn) case gp_sdl_btn: return gp_btn_name;
inline int32_t from_gamepad_sdl_btn(int32_t gp_sdl_btn) {
    switch(gp_sdl_btn) { GAMEPAD_BTNS }
    return -1;
}
#undef GAMEPAD_BTN

#define GAMEPAD_BTN(gp_btn_name, gp_sdl_btn) case gp_btn_name: return #gp_btn_name;
inline const char *gamepad_btn_string(int32_t btn) {
    switch(btn) { GAMEPAD_BTNS }
    return NULL;
}
#undef GAMEPAD_BTN

enum : uint32_t { 
    input_pressed  = 1 << 1,
    input_released = 1 << 2,
    input_is_down  = 1 << 3,
    input_repeat   = 1 << 4,

    input_repeat_or_press = (input_repeat | input_pressed)
};

struct Input {
    uint32_t keys[KB_KEY__COUNT];
    uint32_t btns[MOUSE_BTN__COUNT];

    int32_t scroll_move;
    int32_t x_mouse;
    int32_t y_mouse;
    int32_t x_mouse_prev;
    int32_t y_mouse_prev;
    int32_t x_mouse_move;
    int32_t y_mouse_move;

    SDL_GameController *used_controllers[4];
    uint32_t num_used_controllers;
    uint32_t gp_btns[GAMEPAD_BTN__COUNT];
};

Input *create_input(void);
void delete_input(Input *input);
void begin_input_frame(Input *input);
void catch_input(Input *input, SDL_Event *sdl_event);

enum class BindType {
    KEY, 
    BUTTON, 
    GAMEPAD_BUTTON
};

struct InputBinds {
    static const int32_t max_binds = 4;

    struct Bind {
        BindType type;
        int32_t  code;
    };
    
    int32_t num_binds;
    Bind binds[max_binds];
};

InputBinds init_binds(void);
void add_bind(InputBinds *binds, BindType type, int32_t code);
bool check_state(InputBinds *binds, Input *input, uint32_t input_state);

#endif /* _INPUT_H */