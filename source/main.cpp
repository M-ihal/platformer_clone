#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "common.h"
#include "input.h"
#include "opengl_abs.h"
#include "renderer.h"
#include "audio_player.h"
#include "data.h"
#include "all_entities.h"

#ifdef BUILD_EDITOR
#include "editor.h"
#else
#include "game.h"
#include "main_menu.h"
#endif

#include <filesystem>

#ifdef BUILD_EDITOR
    #define INIT_WINDOW_FULLSCREEN 0
    #define INIT_WINDOW_WIDTH      1440
    #define INIT_WINDOW_HEIGHT     720
    #define INIT_WINDOW_TITLE "no_name_editor"
    #define INIT_ENABLE_VSYNC  1
#else
    #define INIT_WINDOW_FULLSCREEN 0
#ifdef _DEBUG_MODE
    #define INIT_WINDOW_WIDTH      1440
    #define INIT_WINDOW_HEIGHT     720
#else
    #define INIT_WINDOW_WIDTH      768
    #define INIT_WINDOW_HEIGHT     720
#endif
    #define INIT_WINDOW_TITLE "no_name"
    #define INIT_ENABLE_VSYNC  0
#endif

static void toggle_fullscreen(SDL_Window *sdl_window, bool *out_is_fullscreen, bool set_to_fullscreen) {
    if(set_to_fullscreen) {
        uint32_t flags = SDL_GetWindowFlags(sdl_window);
        if(flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
            *out_is_fullscreen = true;
            return;
        }
        bool toggled = SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
    } else {
        uint32_t flags = SDL_GetWindowFlags(sdl_window);
        if(!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP || flags & SDL_WINDOW_FULLSCREEN)) {
           *out_is_fullscreen = false;
            return;
        }
        bool toggled = SDL_SetWindowFullscreen(sdl_window, 0x0) == 0;
    }

    uint32_t flags = SDL_GetWindowFlags(sdl_window);
    *out_is_fullscreen = flags & SDL_WINDOW_FULLSCREEN || flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

int main(int argc, char *argv[]) {
    SDL_SetMainReady();
    bool sdl_success = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) == 0;
    if(!sdl_success) {
        printf("Failed to initialize SDL2.\n");
        return -1;
    }

    // Set working directory to the .exe folder.
    // @check crossplatform?
    std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

    SDL_Window   *sdl_window = NULL;
    SDL_GLContext gl_context = NULL;

    uint32_t window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    sdl_window = SDL_CreateWindow(INIT_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, window_flags);
    if(sdl_window == nullptr) {
        printf("Failed to create window.\n");
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    gl_context = SDL_GL_CreateContext(sdl_window);
    if(gl_context == nullptr) {
        printf("Failed to create opengl context.\n");
        return -1;
    }

    bool glew_success = glewInit() == GLEW_OK;
    if(!glew_success) {
        printf("Failed to initialize glew.\n");
        return -1;
    }

    render::r_init();
    audio_player::a_init();
    global_data::init();
    init_entities_data();

    int32_t window_w;
    int32_t window_h;
    SDL_GetWindowSize(sdl_window, &window_w, &window_h);

    Input *input = create_input();

#ifdef BUILD_EDITOR
    Editor *editor = create_editor(sdl_window, gl_context);
    resize_editor(editor, window_w, window_h);
#else
    init_main_menu();    
    Game *game = create_game();
    resize_game(game, window_w, window_h);
#endif
       
    uint64_t  prev_frame_time = SDL_GetPerformanceCounter();
    float64_t delta_time   = 0.0;
    float64_t elapsed_time = 0.0;

    const int32_t target_frame_rate = 60;
    float64_t     target_delta_time = 1.0 / (double)target_frame_rate;
    float64_t     dt_accumulator    = 0.0;

    SDL_GL_SetSwapInterval(INIT_ENABLE_VSYNC ? 1 : 0);

    bool fullscreen = false;

#if INIT_WINDOW_FULLSCREEN == 1
    toggle_fullscreen(sdl_window, &fullscreen, true);
#ifndef BUILD_EDITOR
    game->request_fullscreen = true;
#endif
#endif

    bool break_loop = false;
    while(break_loop == false) {
        uint64_t frame_time = SDL_GetPerformanceCounter();
        delta_time = (float64_t)(frame_time - prev_frame_time) / (float64_t)SDL_GetPerformanceFrequency();
        prev_frame_time = frame_time;
        elapsed_time += delta_time;

        // @todo Make This Better This is prety bad
#ifndef BUILD_EDITOR
        dt_accumulator += delta_time;
        if(dt_accumulator < target_delta_time) {
            continue;
        } else {
            while(dt_accumulator >= target_delta_time) {
                dt_accumulator -= target_delta_time;
            }
        }
#endif

        begin_input_frame(input);

        SDL_Event sdl_event;
        while(SDL_PollEvent(&sdl_event)) {
#ifdef BUILD_EDITOR
            process_events_for_editor_imgui(&sdl_event);
#endif
            switch(sdl_event.type) {
                case SDL_QUIT: {
                    break_loop = true;
                } break;

                case SDL_WINDOWEVENT: {
                    SDL_WindowEvent *window_event = &sdl_event.window;
                    if(window_event->event == SDL_WINDOWEVENT_RESIZED || window_event->event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        window_w = window_event->data1;
                        window_h = window_event->data2;
#ifdef BUILD_EDITOR
                        resize_editor(editor, window_w, window_h);
#else
                        resize_game(game, window_w, window_h);
#endif
                    }
                } break;
            }
            catch_input(input, &sdl_event);
        }

        unbind_framebuffer();
        gl_clear({ 0.1f, 0.1f, 0.1f, 1.0f }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef BUILD_EDITOR
        update_editor(editor, input, delta_time);
        render_editor(editor);
        render_editor_imgui();
#else
        update_game(game, input, target_delta_time);
        
        if(fullscreen != game->request_fullscreen) {
            toggle_fullscreen(sdl_window, &fullscreen, game->request_fullscreen);
        }

        if(game->request_fit_window_to_draw_rect && fullscreen == false) {
            SDL_SetWindowSize(sdl_window, game->draw_rect.w, game->draw_rect.h);
            SDL_SetWindowPosition(sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            game->request_fit_window_to_draw_rect = false;
        }

        render_game(game);
#endif

#if defined(_DEBUG_MODE)
        if(input->keys[key_f11] & input_pressed) {
            toggle_fullscreen(sdl_window, &fullscreen, !fullscreen);
#ifndef BUILD_EDITOR
            game->request_fullscreen = fullscreen;
#endif
        }                
#endif

        SDL_GL_SwapWindow(sdl_window);

#ifndef BUILD_EDITOR
        if(game->should_quit_game) {
            break_loop = true;
            continue;
        }
#endif
    }

    
#ifdef BUILD_EDITOR
    delete_editor(editor);
#else
    free_main_menu();
    delete_game(game);
#endif

    delete_input(input);

    global_data::free();
    audio_player::a_quit();
    render::r_quit();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}