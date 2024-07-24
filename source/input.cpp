#include "input.h"

Input *create_input(void) {
    Input *input = malloc_struct(Input);
    zero_struct(input);
    return input;
}

void delete_input(Input *input) {
    free(input);
}

void begin_input_frame(Input *input) {
    for(int32_t idx = 0; idx < KB_KEY__COUNT; ++idx)      { input->keys[idx]    &= ~(input_pressed | input_released); }
    for(int32_t idx = 0; idx < MOUSE_BTN__COUNT; ++idx)   { input->btns[idx]    &= ~(input_pressed | input_released); }
    for(int32_t idx = 0; idx < GAMEPAD_BTN__COUNT; ++idx) { input->gp_btns[idx] &= ~(input_pressed | input_released); }
    input->scroll_move = 0;

    input->x_mouse_prev = input->x_mouse;
    input->y_mouse_prev = input->y_mouse;
    input->x_mouse_move = 0;
    input->y_mouse_move = 0;

    // Check for controllers
    input->num_used_controllers = 0;
    input->used_controllers[0] = NULL;
    input->used_controllers[1] = NULL;
    input->used_controllers[2] = NULL;
    input->used_controllers[3] = NULL;
    for(int32_t controller_idx = 0; controller_idx < SDL_NumJoysticks() && controller_idx < 4; ++controller_idx) {
        if(!SDL_IsGameController(controller_idx)) {
            continue;
        }

        input->used_controllers[controller_idx] = SDL_GameControllerOpen(controller_idx);
        input->num_used_controllers += 1;
    }
}

void catch_input(Input *input, SDL_Event *sdl_event) {
    switch(sdl_event->type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            auto kb_event = &sdl_event->key;
            auto key_sym = &kb_event->keysym;

            int32_t key = from_sdl_key(key_sym->sym);
            if(key == -1 || key >= KB_KEY__COUNT) {
                break;
            }

            input->keys[key] &= ~input_repeat;
            if((kb_event->type == SDL_KEYDOWN) && (kb_event->repeat != 1)) {
                input->keys[key] |= input_pressed;
                input->keys[key] |= input_is_down;
            } else if(kb_event->repeat == 1) {
                input->keys[key] |= input_repeat;
            } else if(kb_event->type == SDL_KEYUP) {
                input->keys[key] |= input_released;
                input->keys[key] &= ~input_is_down;
            }
        } break;

        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
            auto btn_event = &sdl_event->button;

            int32_t btn = from_sdl_btn(btn_event->button);
            if(btn == -1 || btn >= MOUSE_BTN__COUNT) {
                break;
            }

            if(btn_event->type == SDL_MOUSEBUTTONDOWN) {
                input->btns[btn] |= input_pressed;
                input->btns[btn] |= input_is_down;
            } else if(btn_event->type == SDL_MOUSEBUTTONUP) {
                input->btns[btn] |= input_released;
                input->btns[btn] &= ~input_is_down;
            }
        } break;

        case SDL_MOUSEWHEEL: {
            auto wheel_event = &sdl_event->wheel;
            input->scroll_move = wheel_event->y;
        } break;

        case SDL_MOUSEMOTION: {
            auto motion_event = &sdl_event->motion;
            
            input->x_mouse = motion_event->x;
            input->y_mouse = motion_event->y;
            input->x_mouse_move = motion_event->xrel;
            input->y_mouse_move = motion_event->yrel;
        } break;

        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERBUTTONDOWN: {
            auto gamepad_event = &sdl_event->cdevice;
            auto gamepad_btn_event = &sdl_event->cbutton;

            for(int32_t controller_idx = 0; controller_idx < 4; ++controller_idx) {
                SDL_GameController *controller = input->used_controllers[controller_idx];
                if(controller != NULL && gamepad_event->which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
                    int32_t btn = from_gamepad_sdl_btn(gamepad_btn_event->button);
                    if(btn == -1 || btn >= GAMEPAD_BTN__COUNT) {
                        break;
                    }

                    if(gamepad_event->type == SDL_CONTROLLERBUTTONDOWN) {
                        input->gp_btns[btn] |= input_pressed;
                        input->gp_btns[btn] |= input_is_down;
                    } else if(gamepad_event->type == SDL_CONTROLLERBUTTONUP) {
                        input->gp_btns[btn] |= input_released;
                        input->gp_btns[btn] &= ~input_is_down;
                    }
                }
            }
        } break;
    }
}

InputBinds init_binds(void) {
    InputBinds binds = { };
    return binds;
}

void add_bind(InputBinds *binds, BindType type, int32_t code) {
    int32_t bind_idx = binds->num_binds;
    if((bind_idx + 1) >= InputBinds::max_binds) {
        // @error
        return;
    }
    binds->num_binds += 1;
    
    InputBinds::Bind *bind = &binds->binds[bind_idx];
    bind->type = type;
    bind->code = code;
}

bool check_state(InputBinds *binds, Input *input, uint32_t input_state) {
    for(int32_t idx = 0; idx < binds->num_binds; ++idx) {
        auto bind = &binds->binds[idx];
        switch(bind->type) {
            default: break;

            case BindType::KEY: {
                if(input->keys[bind->code] & input_state) {
                    return true;
                }
            } break;

            case BindType::BUTTON: {
                if(input->btns[bind->code] & input_state) {
                    return true;
                }
            } break;

            case BindType::GAMEPAD_BUTTON: {
                if(input->gp_btns[bind->code] & input_state) {
                    return true;
                }
            } break;
        }
    }
    return false;
}