#include "editor.h"
#include "all_entities.h"
#include "renderer.h"
#include "save_data.h"

#include "data.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

namespace {
    constexpr int32_t panel_l_width = 320;
    constexpr int32_t panel_r_width = 200;
    
    constexpr int32_t EDITOR_Z_NEAR = -100;
    constexpr int32_t EDITOR_Z_FAR  =  100;
    
    constexpr vec4 tilemap_mesh_color    = { 1.0f, 1.0f, 1.0f, 0.2f };
    constexpr vec4 cregion_outline_color = color::cyan;
}

static void set_imgui_style_for_editor(void);

// @temp @platform @todo
#include <SDL_syswm.h>
#include <Windows.h>
#include <commdlg.h>
#undef near
#undef far
static bool win32_file_dialog_open(char *file_buffer, size_t file_buffer_size, SDL_Window *sdl_window, char *filter = NULL);
static bool win32_file_dialog_save(char *file_buffer, size_t file_buffer_size, SDL_Window *sdl_window, char *filter = NULL, char *def_ext = NULL);

inline // lil helper @move
void get_min_and_max(TileInfo *out_min, TileInfo *out_max, TileInfo ti1, TileInfo ti2) {
    if(out_min) { *out_min = tile_info({ min_value(ti1.tile.x, ti2.tile.x), min_value(ti1.tile.y, ti2.tile.y) }); }
    if(out_max) { *out_max = tile_info({ max_value(ti1.tile.x, ti2.tile.x), max_value(ti1.tile.y, ti2.tile.y) }); }
}

static void init_spawn_mode(SpawnModeData *spawn_data, Editor *editor);
static void update_spawn_mode(SpawnModeData *spawn_data, Editor *editor, Input *input);
static void render_spawn_mode(SpawnModeData *spawn_data, Editor *editor);
static void imgui_spawn_mode(SpawnModeData *spawn_data, Editor *editor, int32_t max_item_width);

static void init_modify_mode(ModifyModeData *modify_data, Editor *editor);
static void update_modify_mode(ModifyModeData *modify_data, Editor *editor, Input *input);
static void render_modify_mode(ModifyModeData *modify_data, Editor *editor);
static void imgui_modify_mode(ModifyModeData *modify_data, Editor *editor, int32_t max_item_width);

static void init_paint_mode(PaintModeData *paint_data, Editor *editor);
static void update_paint_mode(PaintModeData *paint_data, Editor *editor, Input *input);
static void render_paint_mode(PaintModeData *paint_data, Editor *editor);
static void imgui_paint_mode(PaintModeData *paint_data, Editor *editor, int32_t max_item_width);

inline static // Sets size and position of entity for selecting with mouse etc... / Returns selection importance or zero on fail
int32_t get_entity_size_and_pos_for_editor(Entity *entity, vec2i *out_pos, vec2i *out_size) {
    if(entity->has_collider) {
        *out_pos  = entity->position + entity->has_collider->offset;
        *out_size = entity->has_collider->size;
        return 100;
    }

    switch(entity->entity_type_id) {
        case entity_type_id(Tilemap): {
            auto tilemap = entity->as<Tilemap>();
            *out_pos = tilemap->position;
            *out_size = vec2i{ tilemap->x_tiles, tilemap->y_tiles } * TILE_SIZE_2;
            return 50;
        } break;

        case entity_type_id(CameraRegion): {
            auto region = entity->as<CameraRegion>();
            *out_pos = region->position;
            *out_size = { region->x_width, region->const_height_in_pixels };
            return 60;
        } break;

        case entity_type_id(BackgroundPlane): {
            auto bg_plane = entity->as<BackgroundPlane>();
            *out_pos = bg_plane->position;
            *out_size = bg_plane->size;
            return 20;
        } break;

        case entity_type_id(BackgroundSprite): {
            auto bg_sprite = entity->as<BackgroundSprite>();
            auto sprite = global_data::get_sprite(bg_sprite->sprite_id);
            *out_pos = bg_sprite->position;
            *out_size = { sprite.width, sprite.height };
            return 70;
        } break;

        case entity_type_id(FireBar): {
            auto fire_bar = entity->as<FireBar>();
            *out_pos = fire_bar->position - fire_bar->fire_size / 2;
            *out_size = fire_bar->fire_size;
            return 80;
        } break;

        case entity_type_id(Portal): {
            auto portal = entity->as<Portal>();
            *out_pos = portal->origin_tile.position;
            *out_size = portal->get_size_for_editor();
            return 85;
        } break;

        case entity_type_id(TriggerableText): {
            auto font = global_data::get_small_font();
            auto tt   = entity->as<TriggerableText>();
            *out_pos  = tt->position - vec2i{ 0, (int32_t)roundf(-font->descent * font->scale_for_pixel_height) };
            *out_size = vec2i{ (int32_t)roundf(font->calc_string_width(tt->text)), (int32_t)roundf((font->ascent - font->descent) * font->scale_for_pixel_height) };
            return 80;
        } break;

        case entity_type_id(BackgroundImage): {
            auto bg_image = entity->as<BackgroundImage>();
            auto image = global_data::get_image(bg_image->image_id);
            *out_pos = bg_image->position;
            *out_size = image->size();
            return 65;
        } break;
    }

    return 0;
}

void reset_editor_modes(Editor *editor) {
    init_spawn_mode(&editor->spawn_mode_data, editor);
    init_modify_mode(&editor->modify_mode_data, editor);
    init_paint_mode(&editor->paint_mode_data, editor);

    editor->spritesheet_cell_w = TILE_SIZE;
    editor->spritesheet_cell_h = TILE_SIZE;
}

Editor *create_editor(SDL_Window *sdl_window, SDL_GLContext gl_context) {
    // Init imgui here...
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(sdl_window, gl_context);
    ImGui_ImplOpenGL3_Init();
    set_imgui_style_for_editor();

    Editor *editor = malloc_and_zero_struct(Editor);
    editor->sdl_window = sdl_window;
    editor->gl_context = gl_context;

    editor->level = create_empty_level();
    editor->editor_mode = EDITOR_MODE_SPAWNING;

    reset_editor_modes(editor);
    
    editor->show_cursor_coords = false;
    editor->show_tilemap_mesh  = false;
    editor->tilemap_opacity    = 1.0f;

    return editor;
}

static
void reset_editor_render_view(Editor *editor) {
    auto view = &editor->render_view;
    view->left   = -(editor->draw_rect.w / 2);
    view->bottom = -(editor->draw_rect.h / 2);
    view->right  =   editor->draw_rect.w / 2;
    view->top    =   editor->draw_rect.h / 2;
    view->near   =   EDITOR_Z_NEAR;
    view->far    =   EDITOR_Z_FAR;
    view->scale       = 1.0f;
    view->x_translate = 0.0f;
    view->y_translate = 0.0f;
}

void resize_editor(Editor *editor, int32_t window_w, int32_t window_h) {
    editor->window_w = window_w;
    editor->window_h = window_h;

    editor->draw_rect = {
        panel_l_width, 
        0,
        window_w - panel_l_width - panel_r_width,
        window_h
    };

    reset_editor_render_view(editor);

    editor->framebuffer = create_framebuffer(editor->draw_rect.w, editor->draw_rect.h);
}

void delete_editor(Editor *editor) {
    assert(editor != NULL);
    free(editor);
}

void update_editor(Editor *editor, Input *input, float64_t delta_time) {
    editor->time_elapsed += delta_time;
    editor->delta_time = delta_time;
    editor->mouse_pos = { input->x_mouse, editor->window_h - input->y_mouse };
    editor->mouse_is_down_this_frame = input->btns[btn_left] & input_is_down;
    editor->cursor_in_editor_bounds = input->x_mouse >= editor->draw_rect.x && input->x_mouse <= editor->draw_rect.x + editor->draw_rect.w
                                      && input->y_mouse >= editor->draw_rect.y && input->y_mouse <= editor->draw_rect.y + editor->draw_rect.h;

    /* Change the render view with the mouse */ {
        const vec2 mouse_in_view_space_prev = convert_to_view_space({ input->x_mouse_prev, editor->window_h - input->y_mouse_prev }, editor->draw_rect, &editor->render_view);
        const vec2 mouse_in_view_space      = convert_to_view_space({ input->x_mouse,      editor->window_h - input->y_mouse },      editor->draw_rect, &editor->render_view);
        
        if(input->btns[btn_right] & input_is_down) {
            vec2 diff = mouse_in_view_space - mouse_in_view_space_prev;
            editor->render_view.x_translate -= diff.x;
            editor->render_view.y_translate -= diff.y;
        }

        // Zoom only if cursor in editor bounds
        if(editor->cursor_in_editor_bounds && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            const float32_t zoom_factor = 1.2f;
            int32_t zoom_dir = input->scroll_move;
            float32_t prev_scale = editor->render_view.scale;
            if(zoom_dir > 0) {
                editor->render_view.scale *= zoom_factor;
            } else if(zoom_dir < 0) {
                editor->render_view.scale /= zoom_factor;
            }

            if(editor->render_view.scale < 0.2f || editor->render_view.scale > 22.2f) {
                editor->render_view.scale = prev_scale;
            }
        }
    }

    for(int32_t mode_idx = 0; mode_idx < EDITOR_MODE__COUNT; ++mode_idx) {
        if(input->keys[editor_mode_key_bind[mode_idx]] & input_pressed) {
            editor->editor_mode = (EEditorMode)mode_idx;
        }
    }

    editor->mouse_pos_in_game_space = pixel_from_real(convert_to_view_space({ input->x_mouse, editor->window_h - input->y_mouse }, editor->draw_rect, &editor->render_view));
    editor->ti_cursor = tile_info_at(editor->mouse_pos_in_game_space);

    if(input->btns[btn_left] & input_pressed) {
        editor->mouse_pos_in_game_space_last_press = editor->mouse_pos_in_game_space;
        editor->ti_cursor_last_press = editor->ti_cursor;
    }

    switch(editor->editor_mode) {
        case EDITOR_MODE_SPAWNING: {
            update_spawn_mode(&editor->spawn_mode_data, editor, input);
        } break;

        case EDITOR_MODE_MODYFING: {
            update_modify_mode(&editor->modify_mode_data, editor, input);
        } break;

        case EDITOR_MODE_PAINTING: {
            update_paint_mode(&editor->paint_mode_data, editor, input);
        } break;
    }

    std::string quick_save_level_path = global_data::get_data_path() + "levels//_quick_saved.level";
    if(input->keys[key_f05] & input_pressed) {
        std::string level_data = generate_level_save_data(editor->level);
        if(save_file(quick_save_level_path.c_str(), (char *)level_data.c_str(), level_data.size())) {
            printf("Quick saved level saved to    %s (%d bytes)\n", quick_save_level_path.c_str(), level_data.size());
        }
    } else if(input->keys[key_f09] & input_pressed) {
        reset_editor_modes(editor);
        auto level_save_data = parse_level_save_data(quick_save_level_path.c_str());
        recreate_empty_level(&editor->level);
        load_level_from_save_data(editor->level, &level_save_data);
        printf("Quick saved level loaded from %s\n", quick_save_level_path.c_str());
    }
}

void render_editor(Editor *editor) {
    editor->framebuffer->bind();
    gl_clear({ 0.05f, 0.05f, 0.05f, 1.0f }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    RenderSetup setup;
    setup.proj_m = editor->render_view.calc_proj();
    setup.view_m = editor->render_view.calc_view();
    setup.viewport = { 0, 0, editor->framebuffer->width, editor->framebuffer->height };
    setup.framebuffer = editor->framebuffer;
    render::r_begin(setup);
    
    switch(editor->editor_mode) {
        case EDITOR_MODE_SPAWNING: {
            render_spawn_mode(&editor->spawn_mode_data, editor);
        } break;

        case EDITOR_MODE_MODYFING: {
            render_modify_mode(&editor->modify_mode_data, editor);
        } break;

        case EDITOR_MODE_PAINTING: {
            render_paint_mode(&editor->paint_mode_data, editor);
        } break;

        case EDITOR_MODE_VIEW_SPRITESHEET: {
            auto ss = global_data::get_spritesheet();
            int32_t cell_w = editor->spritesheet_cell_w;
            int32_t cell_h = editor->spritesheet_cell_h;
            int32_t cell_x = ss->width / cell_w;
            int32_t cell_y = ss->height / cell_h;

            render::r_texture({ 0, 0 }, 0, ss->size(), ss, { 1.0f, 1.0f, 1.0f, editor->tilemap_opacity });
            // const vec4 _color = { 0.15f, 0.15f, 0.15f, 1.0f };
            const vec4 _color = { 0.8f, 0.8f, 0.8f, 0.1f };

            for(int32_t x = 0; x < cell_x; ++x) {
                for(int32_t y = 0; y < cell_y; ++y) {
                    vec2i pos = { x * cell_w, y * cell_h };
                    render::r_set_line_width(1.0f);
                    render::r_line_quad(pos, -1, { cell_w, cell_h }, _color);
                }
            }

            int32_t cursor_cell_x = editor->mouse_pos_in_game_space.x / editor->spritesheet_cell_w;
            int32_t cursor_cell_y = editor->mouse_pos_in_game_space.y / editor->spritesheet_cell_h;

            if(cursor_cell_x >= 0 && cursor_cell_x < cell_x && cursor_cell_y >= 0 && cursor_cell_y < cell_y) {
                render::r_set_line_width(2.0f);
                render::r_line_quad({ cursor_cell_x * cell_w, cursor_cell_y * cell_h }, -2, { cell_w, cell_h }, { 0.8f, 0.4f, 0.8f, 1.0f });
            }
        } break;
    }
    
    // Draw X and Y axis
    render::r_set_line_width(2.0f);
#if 0
    render::r_line({ -100000, 0 }, { 1000000, 0 }, 10, { 1.0f, 1.0f, 1.0f, 0.15f });
    render::r_line({ 0, -100000 }, { 0, 1000000 }, 10, { 1.0f, 1.0f, 1.0f, 0.15f });
#else
    render::r_line({ -100000, 0 }, { 1000000, 0 }, 10, { 0.7f, 0.3f, 0.3f, 0.3f });
    render::r_line({ 0, -100000 }, { 0, 1000000 }, 10, { 0.3f, 0.3f, 0.7f, 0.3f });
#endif

    render::r_end();

    // Render framebuffer to the screen
    RenderSetup bsetup;
    bsetup.proj_m = mat4x4::orthographic(0, 0, editor->window_w, editor->window_h, 0, 100);
    bsetup.view_m = mat4x4::identity();
    bsetup.viewport = { 0, 0, editor->window_w, editor->window_h };
    bsetup.framebuffer = NULL;
    render::r_begin(bsetup);
    render::r_texture(editor->draw_rect.xy, 0, editor->draw_rect.wh, editor->framebuffer->color);

    /* Draw cursor coords */ 
    if(editor->show_cursor_coords) {
        char buffer[32];
        sprintf_s(buffer, array_count(buffer), "%d, %d", editor->mouse_pos_in_game_space.x, editor->mouse_pos_in_game_space.y);
    
        const vec2i offset = { 8, 8 };
        render::r_text(editor->mouse_pos + offset, 0, buffer, render::def_font());
    }

    render::r_end();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    auto button_centered = [&] (const char *cstr, float32_t width, float32_t height = 0.0f) -> bool {
        float32_t window_w = ImGui::GetWindowSize().x;
        ImGui::SetCursorPosX((window_w - width) * 0.5f);
        return ImGui::Button(cstr, { width, height });
    };

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    /* Left panel */ {
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ (float32_t)panel_l_width, (float32_t)editor->window_h });
        ImGui::Begin("panel_l", NULL, flags);

        const int32_t max_item_width = panel_l_width - ImGui::GetStyle().WindowPadding.x * 2;

        ImGui::Text("est.fps: %.1f", (1.0 / editor->delta_time));
        ImGui::Text("Used entities: %d", editor->level->entities.count);
        ImGui::Text("memory size: %db", editor->level->memory_size);
        ImGui::Text("memory used: %db, %.1f%%", editor->level->memory_used, ((float64_t)editor->level->memory_used / (float64_t)editor->level->memory_size) * 100.0);
        ImGui::Text("next_entity_unique_id: %d", editor->level->next_entity_id);
        ImGui::NewLine();

        ImGui::Checkbox("Disable level timer", &editor->level->disable_level_timer);

        if(ImGui::Button(editor->show_level_music_window ? "Hide level music window" : "Set level music", { (float32_t)max_item_width, 0 })) {
            editor->show_level_music_window = !editor->show_level_music_window;
        }

        if(editor->show_level_music_window) {
            ImGui::Begin("Level music", &editor->show_level_music_window, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse); {
                ImGui::SetWindowSize({(float32_t)panel_l_width, 0}); // Make it same size as left editor panel

                int32_t x = 0;

                ImGui::Text("Regular music");
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::Combo("##choose_level_music_1", &editor->level->level_music_id[(int32_t)ELevelMusic::regular], music_string, MUSIC__COUNT);

                ImGui::Text("Star powerup music");
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::Combo("##choose_level_music_2", &editor->level->level_music_id[(int32_t)ELevelMusic::during_star_power], music_string, MUSIC__COUNT);

                ImGui::Text("Level complete music");
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::Combo("##choose_level_music_3", &editor->level->level_music_id[(int32_t)ELevelMusic::on_level_completed], music_string, MUSIC__COUNT);

                ImGui::Text("Pipe cutscene music");
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::Combo("##choose_level_music_4", &editor->level->level_music_id[(int32_t)ELevelMusic::during_player_enter_pipe_anim], music_string, MUSIC__COUNT);

            } ImGui::End();
        }

        ImGui::NewLine();
        ImGui::SetNextItemWidth(max_item_width);
        ImGui::ListBox("##choose_editor_mode", (int32_t *)&editor->editor_mode, editor_mode_string, EDITOR_MODE__COUNT, EDITOR_MODE__COUNT);
        ImGui::NewLine();

        switch(editor->editor_mode) {
            case EDITOR_MODE_SPAWNING: {
                imgui_spawn_mode(&editor->spawn_mode_data, editor, max_item_width);
            } break;

            case EDITOR_MODE_MODYFING: {
                imgui_modify_mode(&editor->modify_mode_data, editor, max_item_width);
            } break;

            case EDITOR_MODE_PAINTING: {
                imgui_paint_mode(&editor->paint_mode_data, editor, max_item_width);
            } break;

            case EDITOR_MODE_VIEW_SPRITESHEET: {
                ImGui::InputInt("##spritesheet_cell_w", &editor->spritesheet_cell_w, 1, 1);
                ImGui::InputInt("##spritesheet_cell_h", &editor->spritesheet_cell_h, 1, 1);

                int32_t cursor_cell_x = editor->mouse_pos_in_game_space.x / editor->spritesheet_cell_w;
                int32_t cursor_cell_y = editor->mouse_pos_in_game_space.y / editor->spritesheet_cell_h;

                ImGui::Text("cell_x: %d", cursor_cell_x);
                ImGui::Text("cell_y: %d", cursor_cell_y);
            } break;
        }
               
        ImGui::End();
    }

    /* Right panel */ {
        ImGui::SetNextWindowPos({ (float32_t)(editor->window_w - panel_r_width), 0 });
        ImGui::SetNextWindowSize({ (float32_t)panel_r_width, (float32_t)editor->window_h });
        ImGui::Begin("panel_r", NULL, flags);
        const float32_t button_w = (float32_t)panel_r_width - ImGui::GetStyle().FramePadding.x * 2;

        if(button_centered("Load level", button_w)) {
            char level_path[256] = ".level";

            if(win32_file_dialog_open(level_path, sizeof(level_path), editor->sdl_window, ".level\0*.level\0\0")) {
                reset_editor_modes(editor);

                LevelSaveData level_save_data = parse_level_save_data(level_path);
                recreate_empty_level(&editor->level);
                load_level_from_save_data(editor->level, &level_save_data);
            }
        }

        if(button_centered("Save level", button_w)) {
            char level_path[256] = ".level";
            if(win32_file_dialog_save(level_path, sizeof(level_path), editor->sdl_window, ".level\0*.level\0\0", ".level")) {
                std::string level_data = generate_level_save_data(editor->level);
                if(!save_file(level_path, (char *)level_data.c_str(), level_data.size())) {
                    printf("Failed to save level %s\n", level_path);
                }
            }
        }

        if(button_centered("Reset level", button_w)) {
            reset_editor_modes(editor);
            recreate_empty_level(&editor->level);
        }

        if(button_centered(editor->show_cursor_coords ? "Hide cursor coords" : "Show cursor coords", button_w)) {
            editor->show_cursor_coords = !editor->show_cursor_coords;
        }

        if(button_centered(editor->show_tilemap_mesh ? "Hide tilemap mesh" : "Show tilemap mesh", button_w)) {
            editor->show_tilemap_mesh = !editor->show_tilemap_mesh;
        }

        ImGui::Text("Tilemap opacity:");
        ImGui::SliderFloat("##tilemap_opacity", &editor->tilemap_opacity, 0.0f, 1.0f);

        if(button_centered("Reset render view", button_w)) {
            reset_editor_render_view(editor);
        }

        ImGui::End();
    }

    ImGui::Render();
}

void process_events_for_editor_imgui(SDL_Event *sdl_event) {
    ImGui_ImplSDL2_ProcessEvent(sdl_event);
}

void render_editor_imgui(void) {
    unbind_framebuffer();
    gl_viewport({ 0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y });
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static 
void init_spawn_mode(SpawnModeData *spawn_data, Editor *editor) {
    spawn_data->spawn_type = SPAWN_PLAYER;
    spawn_data->fire_num = 8;
    spawn_data->sprite_id = 0;

    spawn_data->portal_vertical_enter = true;
    spawn_data->portal_vertical_exit = true;
}

static
void update_spawn_mode(SpawnModeData *spawn_data, Editor *editor, Input *input) {
    // By default draw only the tile where the cursor is
    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SINGLE;
    spawn_data->draw_cursor_mark_red  = false;
    spawn_data->draw_cursor_tile_a   = editor->ti_cursor;
    spawn_data->draw_cursor_tile_b   = editor->ti_cursor;
    spawn_data->draw_cursor_position = editor->ti_cursor.position;
    spawn_data->draw_cursor_size     = TILE_SIZE_2;
    spawn_data->draw_cursor_a_to_b_pos_1 = editor->ti_cursor.position;
    spawn_data->draw_cursor_a_to_b_pos_2 = editor->ti_cursor.position;
    spawn_data->draw_cursor_a_to_b_size_1 = TILE_SIZE_2;
    spawn_data->draw_cursor_a_to_b_size_2 = TILE_SIZE_2;

    const bool lpm_pressed   = input->btns[btn_left] & input_pressed;
    const bool lpm_is_down   = input->btns[btn_left] & input_is_down;
    const bool lpm_released  = input->btns[btn_left] & input_released;
    const bool rpm_is_down   = input->btns[btn_right] & input_is_down;
    const bool shift_pressed = input->keys[key_left_shift] & input_is_down;

    // Prevent spawning same entities on "top" of each other
#define CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_proc, Type) {\
        auto _entity = spawn_proc;\
        assert(_entity->has_collider != NULL);\
        FindCollisionsOpts opts;\
        opts.entity_type_id = entity_type_id(Type);\
        opts.id_to_ignore = _entity->unique_id;\
        if(is_colliding(editor->level, _entity->position, _entity->has_collider, opts)) {\
            delete_entity_imm(_entity);\
        }\
    }

    if(editor->cursor_in_editor_bounds && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemHovered() && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup)) {
        switch(spawn_data->spawn_type) {
            case SPAWN_PLAYER: {
                if(lpm_pressed) {
                    if(editor->level->entities.first_of_type[entity_type_id(Player)]) {
                        delete_entity_imm(editor->level->entities.first_of_type[entity_type_id(Player)]);
                    }
                    spawn_player(editor->level, editor->ti_cursor.position - vec2i{ 0, 1 });
                }
            } break;
    
            case SPAWN_GOOMBA: {
                if(lpm_pressed) {
                    CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_goomba(editor->level, editor->ti_cursor.position - vec2i{ 0, 1 }, (EGoombaType)spawn_data->goomba_type), Goomba);
                }
            } break;

            case SPAWN_KOOPA: {
                if(lpm_pressed) {
                    CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_koopa(editor->level, editor->ti_cursor.position - vec2i{ 0, 1 }, (EKoopaType)spawn_data->koopa_type), Koopa);
                }
            } break;

            case SPAWN_PIRANHA: {
                spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SINGLE;
                spawn_data->draw_cursor_position = editor->ti_cursor.position;
                spawn_data->draw_cursor_size = { TILE_SIZE * 2, TILE_SIZE };

                if(lpm_pressed) {
                    vec2i position = editor->ti_cursor.position + vec2i{ TILE_SIZE / 2, -8 };
                    CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_piranha(editor->level, position), Piranha);
                }
            } break;

            case SPAWN_BOWSER: {
                if(lpm_pressed) {
                    CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_bowser(editor->level, editor->ti_cursor.position - vec2i{ 0, 1 }), Bowser);
                }
                spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SINGLE;
                spawn_data->draw_cursor_position = editor->ti_cursor.position;
                spawn_data->draw_cursor_size = TILE_SIZE_2 * 2;
            } break;

            case SPAWN_TILEMAP: {
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = editor->ti_cursor;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, editor->ti_cursor);
                    const int32_t x_size = ti_max.tile.x - ti_min.tile.x + 1;
                    const int32_t y_size = ti_max.tile.y - ti_min.tile.y + 1;
                    spawn_tilemap(editor->level, ti_min.tile, x_size, y_size);
                }
            } break;
    
            case SPAWN_CAMERA_REGION: {
                TileInfo ti_to = tile_info({ editor->ti_cursor.tile.x, editor->ti_cursor_last_press.tile.y + CameraRegion::const_height_in_tiles - 1 });
    
                spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                if(lpm_is_down) {
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = ti_to;
                } else {
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor;
                    spawn_data->draw_cursor_tile_b = tile_info({ editor->ti_cursor.tile.x, editor->ti_cursor.tile.y + CameraRegion::const_height_in_tiles - 1 });
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, ti_to);
                    const int32_t x_size = ti_max.tile.x - ti_min.tile.x + 1;
                    spawn_camera_region(editor->level, ti_min.position.x, ti_min.position.y, x_size);
                }
            } break;

            case SPAWN_BACKGROUND_PLANE: {
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = editor->ti_cursor;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, editor->ti_cursor);
                    const int32_t x_size = ti_max.tile.x - ti_min.tile.x + 1;
                    const int32_t y_size = ti_max.tile.y - ti_min.tile.y + 1;
                    spawn_background_plane(editor->level, ti_min.position, vec2i{ x_size, y_size } * TILE_SIZE_2, spawn_data->bg_plane_color);
                }
            } break;

            case SPAWN_BACKGROUND_SPRITE: {
                if(lpm_pressed) {
                    if(shift_pressed) {
                        for_entity_type(editor->level, BackgroundSprite, bg_sprite) {
                            auto sprite = global_data::get_sprite(bg_sprite->sprite_id);
                            if(aabb(editor->mouse_pos_in_game_space, { 1, 1 }, bg_sprite->position, { sprite.width, sprite.height })) {
                                delete_entity_imm(bg_sprite);
                                break;
                            }
                        }
                    } else {
                        spawn_background_sprite(editor->level, editor->ti_cursor.position, spawn_data->sprite_id);
                    }
                }

                if(spawn_data->sprite_id != SPRITE__INVALID) {
                    auto sprite = global_data::get_sprite(spawn_data->sprite_id);
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SINGLE;
                    spawn_data->draw_cursor_position = editor->ti_cursor.position;
                    spawn_data->draw_cursor_size = { sprite.width, sprite.height };
                }
            } break;

            case SPAWN_FIRE_BAR: {
                if(lpm_pressed) {
                    spawn_fire_bar(editor->level, editor->ti_cursor.position + (TILE_SIZE_2 / 2), spawn_data->fire_num);
                }
            } break;

            case SPAWN_KILL_REGION: {
                spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                if(lpm_is_down) {
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = editor->ti_cursor;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, editor->ti_cursor);
                    spawn_kill_region(editor->level, ti_min.position, ti_max.position - ti_min.position + TILE_SIZE_2);
                }
            } break;

            case SPAWN_FLAG_POLE: {
                TileInfo ti_to = tile_info({ editor->ti_cursor_last_press.tile.x, editor->ti_cursor.tile.y });
    
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = ti_to;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, ti_to);
                    spawn_flag_pole(editor->level, ti_min.position, ti_max.tile.y - ti_min.tile.y + 1);
                }
            } break;

            case SPAWN_COIN: {
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = editor->ti_cursor;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, editor->ti_cursor);
                    
                    for(int32_t y_tile = ti_min.tile.y; y_tile <= ti_max.tile.y; ++y_tile) {
                        for(int32_t x_tile = ti_min.tile.x; x_tile <= ti_max.tile.x; ++x_tile) {
                            TileInfo ti = tile_info({ x_tile, y_tile });
                            CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_coin(editor->level, ti.position), Coin);
                        }
                    }
                }
            } break;

            case SPAWN_PORTAL: {
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_A_TO_B;
                    spawn_data->draw_cursor_a_to_b_pos_1 = editor->ti_cursor_last_press.position;
                    spawn_data->draw_cursor_a_to_b_pos_2 = editor->ti_cursor.position;

                    if(spawn_data->portal_vertical_exit) {
                        if(spawn_data->portal_offset_x_by_half_a_tile) {
                            spawn_data->draw_cursor_a_to_b_size_2 = { TILE_SIZE * 2, TILE_SIZE };
                        } else {
                            spawn_data->draw_cursor_a_to_b_size_2 = TILE_SIZE_2;
                        }
                    } else {
                        spawn_data->draw_cursor_a_to_b_size_2 = { TILE_SIZE, TILE_SIZE * 2 } ;
                    }

                    if(spawn_data->portal_vertical_enter) {
                        spawn_data->draw_cursor_a_to_b_size_1 = { TILE_SIZE * 2, TILE_SIZE };
                    } else {
                        spawn_data->draw_cursor_a_to_b_size_1 = { TILE_SIZE, TILE_SIZE * 2 };
                    }
                } else {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor;
                    if(spawn_data->portal_vertical_enter) {
                        spawn_data->draw_cursor_tile_b = tile_info(editor->ti_cursor.tile + vec2i{ 1, 0 });
                    } else {
                        spawn_data->draw_cursor_tile_b = tile_info(editor->ti_cursor.tile + vec2i{ 0, 1 });
                    }
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    PortalInfo info;
                    vec2i position;

                    info.vertical_enter = spawn_data->portal_vertical_enter;
                    info.vertical_exit  = spawn_data->portal_vertical_exit;
                    info.do_exit_anim   = spawn_data->portal_do_exit_anim;

                    if(info.vertical_enter) {
                        position = editor->ti_cursor_last_press.position;
                    } else {
                        position = editor->ti_cursor_last_press.position;
                    }

                    if(info.vertical_exit) {
                        info.dest = editor->ti_cursor.position;
                        if(spawn_data->portal_offset_x_by_half_a_tile) {
                            info.dest.x += TILE_SIZE / 2;
                        }
                    } else {
                        info.dest = editor->ti_cursor.position;
                    }

                    spawn_portal(editor->level, position, info);
                }
            } break;

            case SPAWN_MOVING_PLATFORM: {
                TileInfo ti_to = tile_info({ editor->ti_cursor_last_press.tile.x, editor->ti_cursor.tile.y });
    
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = ti_to;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                const int32_t y_distance = absolute_value(ti_to.tile.y - editor->ti_cursor_last_press.tile.y) + 1;

                if(lpm_released && !rpm_is_down && y_distance >= 3) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, ti_to);
                    spawn_moving_platform(editor->level, ti_min.position + vec2i{ TILE_SIZE / 2, 0 }, ti_min.position.y, ti_max.position.y + TILE_SIZE);
                }
            } break;

            case SPAWN_SIDE_MOVING_PLATFORM: {
                TileInfo ti_to = tile_info({ editor->ti_cursor.tile.x, editor->ti_cursor_last_press.tile.y });
    
                if(lpm_is_down) {
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = ti_to;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                const int32_t x_distance = absolute_value(ti_to.tile.x - editor->ti_cursor_last_press.tile.x) + 1;

                if(lpm_released && !rpm_is_down /*&& x_distance >= 3*/) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, ti_to);
                    int32_t distance = ti_max.position.x - ti_min.position.x + TILE_SIZE - moving_platform_x_segments * moving_platform_segment_size;
                    spawn_side_moving_platform(editor->level, ti_min.position, max_value(distance, 0));
                }
            } break;

            case SPAWN_TRIGGER: {
                spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SELECTION;
                if(lpm_is_down) {
                    spawn_data->draw_cursor_tile_a = editor->ti_cursor_last_press;
                    spawn_data->draw_cursor_tile_b = editor->ti_cursor;
                }
    
                if(rpm_is_down) {
                    spawn_data->draw_cursor_mark_red = true;
                }
    
                if(lpm_released && !rpm_is_down) {
                    TileInfo ti_min, ti_max;
                    get_min_and_max(&ti_min, &ti_max, editor->ti_cursor_last_press, editor->ti_cursor);
                    spawn_trigger(editor->level, ti_min.position, ti_max.position - ti_min.position + TILE_SIZE_2, ETriggerType::TRIGGER_NOT_SET);
                }
            } break;

            case SPAWN_TRIGGABLE_TEXT: {
                if(lpm_pressed) {
                    spawn_triggerable_text(editor->level, editor->ti_cursor.position, "TriggerableText");
                }
            } break;

            case SPAWN_BACKGROUND_IMAGE: {
                if(lpm_pressed) {
                    spawn_background_image(editor->level, editor->ti_cursor.position, spawn_data->image_id);
                }

                if(spawn_data->image_id != IMAGE__INVALID) {
                    auto image = global_data::get_image(spawn_data->image_id);
                    spawn_data->draw_cursor_mode = SpawnModeData::DRAW_CURSOR_SINGLE;
                    spawn_data->draw_cursor_position = editor->ti_cursor.position;
                    spawn_data->draw_cursor_size = image->size();
                }
            } break;

            case SPAWN_CASTLE_LEVER: {
                if(lpm_pressed) {
                    CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_castle_lever(editor->level, editor->ti_cursor.position), CastleLever);
                }
            } break;
            
            case SPAWN_MUSHROOM: {
                if(lpm_pressed) {
                    CREATE_AND_MAYBE_DELETE_IF_COLLIDES_WITH_SAME_TYPE(spawn_mushroom(editor->level, editor->ti_cursor.position), Mushroom);
                }
            } break;
        }
    }
}

void render_portals_connections(Level *level) {
    for_entity_type(level, Portal, portal) {
        render::r_set_line_width(2.5f);
        render_connected_quads(portal->position, portal->get_size_for_editor(), portal->info.dest, TILE_SIZE_2, -1, color::yellow, { 0.0f, 1.0f, 0.0f, 0.75f });
        render::r_line_quad(portal->get_enter_pos(), -3, portal->get_enter_size(), color::green);

        render::r_text({ portal->position.x, portal->position.y + render::def_font()->height }, -2, portal->info.vertical_enter ? "vertical" : "horizontal", global_data::get_small_font(), color::yellow);
        render::r_text({ portal->info.dest.x, portal->info.dest.y + render::def_font()->height }, -2, 
                       !portal->info.do_exit_anim ? "no exit anim" : (portal->info.vertical_exit ? "vertical" : "horizontal"), 
                       global_data::get_small_font(), color::yellow);
    }
}

static
void render_transparent_blocks_drops(Tilemap *tilemap) {
    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            auto tile = tilemap->get_tile(x_tile, y_tile);
            if(tile == NULL || !tile->is_not_empty || tile->tile_desc.tile_drop == TILE_DROP_NONE) {
                continue;
            }

            int32_t sprite_id = SPRITE__INVALID;
            switch(tile->tile_desc.tile_drop) {
                case TILE_DROP_COINS:   { sprite_id = SPRITE_COIN_ENTITY_1; } break;
                case TILE_DROP_POWERUP: { sprite_id = SPRITE_MUSHROOM; } break;
                case TILE_DROP_STAR:    { sprite_id = SPRITE_STAR_1; } break;
            }

            if(sprite_id != SPRITE__INVALID) {
                render::r_sprite(tile->get_ti().position, Z_TILEMAP_POS - 1, TILE_SIZE_2, global_data::get_sprite(sprite_id), { 1.0f, 1.0f, 1.0f, 0.4f });
            }
        }
    }
}

static
void render_editor_level(Editor *editor) {
    auto level = editor->level;

    for_every_entity(level, entity) {
        if(entity->render_proc == NULL) {
            continue;
        }

        // Do not render entities that will be rendered customly
        switch(entity->entity_type_id) {
            case entity_type_id(TriggerableText):
            case entity_type_id(Tilemap): {
                continue;
            }
        }

        // Render entity
        entity->render_proc(entity);
    }

    for_entity_type(editor->level, MovingPlatform, platform) {
        _render_debug_moving_platform_metrics(platform, 1);
    }

    for_entity_type(editor->level, SideMovingPlatform, platform) {
        _render_debug_side_moving_platform_metrics(platform, 1);
    }

    const vec3 trigger_color_inactive = { 0.8f, 0.8f, 0.2f };
    const vec3 trigger_color_active   = { 0.4f, 0.8f, 0.2f };

    /* Render triggers */
    for_entity_type(level, Trigger, trigger) {
        bool matching_key_exists = false;
        for_entity_type(editor->level, TriggerableText, tt) {
            if(trigger->trigger_type == TRIGGER_TEXT_VISIBILITY && !strcmp(trigger->trigger_key, tt->trigger_key)) {
                matching_key_exists = true;
            }
        }

        const vec3 color = matching_key_exists ? trigger_color_active : trigger_color_inactive;

        render::r_quad(trigger->position + trigger->collider.offset, -1, trigger->collider.size, vec4::make(color, 0.25f));
        render::r_set_line_width(2.0f);
        render::r_line_quad(trigger->position + trigger->collider.offset, -1, trigger->collider.size, vec4::make(color, 1.0f));
       
        /* render trigger key */ {
            float32_t scale = 12.0f / (float32_t)R_DEF_FONT_BIG_HEIGHT;

            Font   *font = render::def_font_big();
            vec2i   position = trigger->position + trigger->collider.offset + vec2i{ trigger->collider.size.x / 2, trigger->collider.size.y + 3 };
            int32_t string_width = font->calc_string_width(trigger->trigger_key) * scale;

            render::r_text_scaled(position - vec2i{ string_width / 2, 0 }, -1, trigger->trigger_key, font, scale, vec4::make(color, 1.0f));
        }
    }

    for_entity_type(level, TriggerableText, tt) {
        Font   *font = global_data::get_small_font();
        render::r_text(tt->position, -1, tt->text, font);

         /* render trigger key */ {
            bool matching_key = false;
            for_entity_type(editor->level, Trigger, trigger) {
                if(trigger->trigger_type == TRIGGER_TEXT_VISIBILITY && !strcmp(trigger->trigger_key, tt->trigger_key)) {
                    matching_key = true;
                    break;
                }
            }

            const vec3 color = matching_key ? trigger_color_active : trigger_color_inactive;

            vec2i tt_position;
            vec2i tt_size;
            get_entity_size_and_pos_for_editor(tt, &tt_position, &tt_size);

            vec2i padding = vec2i::make(2);
            render::r_quad(tt_position - padding, -1, tt_size + padding * 2, vec4::make(color, 0.25f));
            render::r_set_line_width(2.0f);
            render::r_line_quad(tt_position - padding, -1, tt_size + padding * 2, vec4::make(color, 1.0f));
            
            float32_t scale = 12.0f / (float32_t)R_DEF_FONT_BIG_HEIGHT;

            Font   *font = render::def_font_big();
            vec2i   position = tt_position + vec2i{ tt_size.x / 2, tt_size.y + 3 };
            int32_t string_width = font->calc_string_width(tt->trigger_key) * scale;

            render::r_text_scaled(position - vec2i{ string_width / 2, 0 }, -1, tt->trigger_key, font, scale, vec4::make(color, 1.0f));
        }
    }

    /* Render tilemaps with different alpha */ {
        RenderSetup _setup;
        render::get_current_setup(&_setup);
        render::r_end();

        for_entity_type(level, Tilemap, tilemap) {
            Framebuffer *framebuffer = create_framebuffer(tilemap->x_tiles * TILE_SIZE, tilemap->y_tiles * TILE_SIZE);
            framebuffer->bind();
            gl_clear({ 0, 0, 0, 0 }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderSetup setup = _setup;
            setup.framebuffer = framebuffer;
            setup.proj_m = mat4x4::orthographic(tilemap->position.x, tilemap->position.y, tilemap->position.x + tilemap->x_tiles * TILE_SIZE, tilemap->position.y + tilemap->y_tiles * TILE_SIZE, -100.0f, 100.0f);
            setup.view_m = mat4x4::identity();
            setup.viewport = { 0, 0, framebuffer->width, framebuffer->height };
            render::r_begin(setup);
            tilemap->render_proc(tilemap);
            render::r_end();

            render::r_begin(_setup);
            render::r_texture(tilemap->position, Z_TILEMAP_POS, { framebuffer->width, framebuffer->height }, framebuffer->color, { 1.0f, 1.0f, 1.0f, editor->tilemap_opacity });
            render::r_end();

            delete_framebuffer(framebuffer);
        }

        render::r_begin(_setup);
    }

    for_entity_type(level, Bowser, bowser) {
        _render_debug_bowser_info(bowser);
    }
}

static
void render_spawn_mode(SpawnModeData *spawn_data, Editor *editor) {
    render_editor_level(editor);

    // Render level tilemap meshes
    render::r_set_line_width(1.0f);
    for_entity_type(editor->level, Tilemap, tilemap) {
        render_transparent_blocks_drops(tilemap);
        if(editor->show_tilemap_mesh) {
            render_tilemap_mesh(tilemap, 8, tilemap_mesh_color);
        } else {
            render::r_line_quad(tilemap->position, 0, vec2i{ tilemap->x_tiles, tilemap->y_tiles } * TILE_SIZE_2, tilemap_mesh_color);
        }
    }

    // Render camera region outlines
    render::r_set_line_width(2.5f);
    for_entity_type(editor->level, CameraRegion, region) {
        render::r_quad_marching_ants(region->position, -9, { region->x_width, region->const_height_in_pixels }, 2, cregion_outline_color, 0.0f);
    }

    // Render kill regions
    render::r_set_line_width(2.0f);
    for_entity_type(editor->level, KillRegion, kill_region) {
        render::r_quad(kill_region->position, -1, kill_region->collider.size, { 1.0f, 0.0f, 0.0f, 0.45f });
        render::r_line_quad(kill_region->position, -1, kill_region->collider.size, color::red);
    }

    render_portals_connections(editor->level);

    const vec4 cursor_color = { 0.55f, 0.55f, 0.55f, 1.0f };
    const vec4 cursor_mark_red_color = color::red;
    render::r_set_line_width(2.0f);

    switch(spawn_data->draw_cursor_mode) {
        default: assert(false); break;

        case SpawnModeData::DRAW_CURSOR_SELECTION: {
            TileInfo ti_min, ti_max;
            get_min_and_max(&ti_min, &ti_max, spawn_data->draw_cursor_tile_a, spawn_data->draw_cursor_tile_b);
            render::r_line_quad(ti_min.position, -8, ti_max.position - ti_min.position + TILE_SIZE_2, spawn_data->draw_cursor_mark_red ? cursor_mark_red_color : cursor_color);
        } break;

        case SpawnModeData::DRAW_CURSOR_SINGLE: {
            render::r_line_quad(spawn_data->draw_cursor_position, -8, spawn_data->draw_cursor_size, spawn_data->draw_cursor_mark_red ? cursor_mark_red_color : cursor_color);
        } break;

        case SpawnModeData::DRAW_CURSOR_A_TO_B: {
            const vec4 color = spawn_data->draw_cursor_mark_red ? cursor_mark_red_color : cursor_color;
            render_connected_quads(spawn_data->draw_cursor_a_to_b_pos_1, spawn_data->draw_cursor_a_to_b_size_1,
                                   spawn_data->draw_cursor_a_to_b_pos_2, spawn_data->draw_cursor_a_to_b_size_2,
                                   -8, color, color);
        } break;
    }

    // Draw cursor "pixel"
    render::r_line_quad(editor->mouse_pos_in_game_space, 8, { 1, 1 }, color::white);

    // Draw tiles around the cursor
    int32_t half_size = 5;
    vec2i from = editor->ti_cursor.tile - vec2i::make(half_size);
    vec2i to   = editor->ti_cursor.tile + vec2i::make(half_size);
    for(int32_t y_tile = from.y; y_tile <= to.y; ++y_tile) {
        for(int32_t x_tile = from.x; x_tile <= to.x; ++x_tile) {
            float32_t radius = vec2::length((vec2i { x_tile, y_tile } - editor->ti_cursor.tile).to_vec2());
            if((x_tile == 0 && y_tile == 0) || radius >= half_size) {
                continue;
            }

            float32_t alpha = (1.0f - (radius / (float32_t)half_size)) * 0.05f;

            auto _ti = tile_info({ x_tile, y_tile });
            render::r_line_quad(_ti.position, 10, TILE_SIZE_2, { 1.0f, 1.0f, 1.0f, alpha });
        }
    }
}

static
void imgui_spawn_mode(SpawnModeData *spawn_data, Editor *editor, int32_t max_item_width) {
    ImGui::SetNextItemWidth(max_item_width);
    if(ImGui::BeginListBox("##choose_spawn")) {
        for(int32_t idx = 0; idx < SPAWN_TYPE__COUNT; ++idx) {
            char buffer[64];
            sprintf_s(buffer, array_count(buffer), "%s", spawn_type_string[idx]);
            if(ImGui::Selectable(buffer, spawn_data->spawn_type == idx)) {
                spawn_data->spawn_type = (ESpawnType)idx;
            }
        }
        ImGui::EndListBox();
    }

    TileInfo ti_min, ti_max;
    get_min_and_max(&ti_min, &ti_max, spawn_data->draw_cursor_tile_a, spawn_data->draw_cursor_tile_b);
    ImGui::Text("Selection: %d %d", ti_max.tile.x - ti_min.tile.x + 1, ti_max.tile.y - ti_min.tile.y + 1);

    switch(spawn_data->spawn_type) {
        case SPAWN_GOOMBA: {
            ImGui::Text("Goomba type:");
            ImGui::SetNextItemWidth(max_item_width);
            ImGui::Combo("##choose_goomba_type", &spawn_data->goomba_type, goomba_type_cstr, GOOMBA_TYPE__COUNT);
        } break;

        case SPAWN_KOOPA: {
            ImGui::Text("Koopa type:");
            ImGui::SetNextItemWidth(max_item_width);
            ImGui::Combo("##choose_koopa_type", &spawn_data->koopa_type, koopa_type_cstr, KOOPA_TYPE__COUNT);
        } break;

        case SPAWN_FIRE_BAR: {
            ImGui::Text("Number of fires: "); ImGui::SameLine();
            ImGui::InputInt("##specify_fire_bar_fire_num", &spawn_data->fire_num, 1, 1);
        } break;

        case SPAWN_BACKGROUND_PLANE: {
           ImGui::ColorPicker3("##choose_bg_color", spawn_data->bg_plane_color.e);
        } break;

        case SPAWN_BACKGROUND_SPRITE: {
            ImGui::SetNextItemWidth(max_item_width);
            ImGui::Combo("##choose_background_sprite", &spawn_data->sprite_id, sprite_string, SPRITE__COUNT);

            const float32_t window_padding = ImGui::GetStyle().WindowPadding.x;
            const float32_t item_spacing   = ImGui::GetStyle().ItemSpacing.x;
            const float32_t frame_padding  = ImGui::GetStyle().FramePadding.x;

            //ImGui::SetNextWindowSize({ 256.0f, 512.0f });
            ImGui::Begin("Select background sprite."); {
                float32_t comb_line_width = window_padding;
                for(int32_t sprite_id = 0; sprite_id < SPRITE__COUNT; ++sprite_id) {
                    auto sprite = global_data::get_sprite(sprite_id);
                    ImTextureID im_tex_id = (void *)sprite.texture->texture_id;
                    ImVec2 im_tex_size = { (float32_t)sprite.width * 2, (float32_t)sprite.height * 2 };
                    ImVec2 im_tex_uv_1 = { sprite.tex_coords[0].x, sprite.tex_coords[2].y };
                    ImVec2 im_tex_uv_2 = { sprite.tex_coords[2].x, sprite.tex_coords[0].y };

                    comb_line_width += im_tex_size.x + item_spacing;
                    if(comb_line_width >= ImGui::GetWindowContentRegionMax().x) {
                        comb_line_width = window_padding;
                        ImGui::NewLine();
                        comb_line_width += im_tex_size.x + item_spacing;
                    } else {
                        if(sprite_id != 0) {
                            ImGui::SameLine();
                        }
                    }
                    comb_line_width += frame_padding * 2.0f;

                    char buffer[32] = { };
                    sprintf_s(buffer, array_count(buffer), "_sprite_idx:%d", sprite_id);
                    if(ImGui::ImageButton(buffer, im_tex_id, im_tex_size, im_tex_uv_1, im_tex_uv_2)) {
                        spawn_data->sprite_id = sprite_id;
                    }
                }
            } ImGui::End();
        } break;

        case SPAWN_PORTAL: {
            ImGui::Text("--- choose axis ---");
            ImGui::SetNextItemWidth(max_item_width);
            ImGui::Checkbox("Vertical Enter", &spawn_data->portal_vertical_enter);
            ImGui::Checkbox("Do exit anim", &spawn_data->portal_do_exit_anim);
            if(spawn_data->portal_do_exit_anim) {
                ImGui::Checkbox("Vertical Exit", &spawn_data->portal_vertical_exit);
            }

            if(spawn_data->portal_vertical_exit || !spawn_data->portal_do_exit_anim) {
                ImGui::Checkbox("Offset x by tile/2", &spawn_data->portal_offset_x_by_half_a_tile);
            } else {
                spawn_data->portal_offset_x_by_half_a_tile = false;
            }
        } break;

        case SPAWN_BACKGROUND_IMAGE: {
            ImGui::SetNextItemWidth(max_item_width);
            ImGui::Combo("##choose_background_image", &spawn_data->image_id, image_string, IMAGE__COUNT);

            const float32_t window_padding = ImGui::GetStyle().WindowPadding.x;
            const float32_t item_spacing   = ImGui::GetStyle().ItemSpacing.x;
            const float32_t frame_padding  = ImGui::GetStyle().FramePadding.x;

            ImGui::Begin("Select background image."); {
                float32_t comb_line_width = window_padding;
                for(int32_t image_id = 0; image_id < IMAGE__COUNT; ++image_id) {
                    auto image = global_data::get_image(image_id);
                    ImTextureID im_tex_id = (void *)image->texture_id;
                    ImVec2 im_tex_size = { (float32_t)image->width * 2, (float32_t)image->height * 2 };
                    ImVec2 im_tex_uv_1 = { 0, 1 };
                    ImVec2 im_tex_uv_2 = { 1, 0 };

                    comb_line_width += im_tex_size.x + item_spacing;
                    if(comb_line_width >= ImGui::GetWindowContentRegionMax().x) {
                        comb_line_width = window_padding;
                        ImGui::NewLine();
                        comb_line_width += im_tex_size.x + item_spacing;
                    } else {
                        if(image_id != 0) {
                            ImGui::SameLine();
                        }
                    }
                    comb_line_width += frame_padding * 2.0f;

                    char buffer[32] = { };
                    sprintf_s(buffer, array_count(buffer), "_image_id:%d", image_id);
                    if(ImGui::ImageButton(buffer, im_tex_id, im_tex_size, im_tex_uv_1, im_tex_uv_2)) {
                        spawn_data->image_id = image_id;
                    }
                }
            } ImGui::End();
        } break;
    }
}

static 
void init_modify_mode(ModifyModeData *modify_data, Editor *editor) {
    modify_data->filter_by_type = true;
    modify_data->entity_type_id_filter = entity_type_id(Player);
    modify_data->selected_entity = NULL;
    modify_data->hovered_entity  = NULL;
    modify_data->drag_selected_entity = false;
}

Entity *find_prev_of_type(Entity *entity) {
    for(Entity *entity_b = entity->level->entities.first_of_type[entity->entity_type_id]; entity_b != NULL; entity_b = entity_b->next_of_type) {
        if(entity_b != entity && entity_b->next_of_type && entity_b->next_of_type == entity) {
            return entity_b;
        }
    }
    return NULL;
}

static
void modify_mode_delete_selected_entity(ModifyModeData *modify_data) {
    auto entity = modify_data->selected_entity;
    if(entity->next_of_type != NULL) {
        modify_data->selected_entity = entity->next_of_type;
    } else {
        auto prev = find_prev_of_type(entity);
        if(prev != NULL) {
            modify_data->selected_entity = prev;
        } else {
            modify_data->selected_entity = NULL;
        }
    }
    delete_entity_imm(entity);
}

static
void update_modify_mode(ModifyModeData *modify_data, Editor *editor, Input *input) {
    std::pair<Entity *, int32_t> hovered = { NULL, 0 };

    modify_data->selected_entity_changed_this_frame = false;

    if(editor->cursor_in_editor_bounds && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        for_every_entity(editor->level, entity) {
            vec2i e_pos, e_size;

            int32_t importance = get_entity_size_and_pos_for_editor(entity, &e_pos, &e_size);
            if(importance == 0) {
                continue;
            }

            if(aabb(editor->mouse_pos_in_game_space, { 1, 1 }, e_pos, e_size)) {
                if(importance > hovered.second) {
                    hovered = { entity, importance };
                }
            }
        }
    }

    if(input->btns[btn_left] & input_released || modify_data->selected_entity == NULL || !editor->cursor_in_editor_bounds) {
        modify_data->drag_selected_entity = false;
    }

    //  Quickly added moving entities with mouse @check

    if(modify_data->drag_selected_entity) {
        if(modify_data->selected_entity) {
            modify_data->selected_entity->position = editor->mouse_pos_in_game_space - modify_data->offset_of_selected_entity;
        }
        modify_data->hovered_entity = NULL;
    } else {
        if(hovered.first != NULL && hovered.first != modify_data->selected_entity) {
            if(input->btns[btn_left] & input_pressed) {
                modify_data->selected_entity = hovered.first;
                modify_data->entity_type_id_filter = hovered.first->entity_type_id;
                modify_data->selected_entity_changed_this_frame = true;
            } else {
                modify_data->hovered_entity = hovered.first;
            }
        } else if(hovered.first != NULL && hovered.first == modify_data->selected_entity) {
            if(input->btns[btn_left] & input_pressed) {
                modify_data->drag_selected_entity = true;
                modify_data->offset_of_selected_entity = editor->mouse_pos_in_game_space - hovered.first->position;
            }
            modify_data->hovered_entity = NULL;
        } else {
            modify_data->hovered_entity = NULL;
        }

        if(modify_data->selected_entity != NULL && input->keys[key_delete] & input_pressed) {
            modify_mode_delete_selected_entity(modify_data);
        }
    }
}

static
void render_modify_mode(ModifyModeData *modify_data, Editor *editor) {
    render_editor_level(editor);

    const vec4 ants_color = { 0.8f, 0.8f, 0.8f, 1.0f };
    const int32_t ant_length = 8;

    // Render level tilemap meshes
    for_entity_type(editor->level, Tilemap, tilemap) {
        render_transparent_blocks_drops(tilemap);

        if(tilemap == modify_data->selected_entity) {
            continue;
        }

        render::r_set_line_width(1.0f);
        if(editor->show_tilemap_mesh) {
            render_tilemap_mesh(tilemap, -3, tilemap_mesh_color);
        } else {
            render::r_line_quad(tilemap->position, 0, vec2i{ tilemap->x_tiles, tilemap->y_tiles } * TILE_SIZE_2, tilemap_mesh_color);
        }
    }

    // Render camera regions
    for_entity_type(editor->level, CameraRegion, region) {
        if(region == modify_data->selected_entity) {
            continue;
        }
        render::r_set_line_width(1.5f);
        render::r_quad_marching_ants(region->position, -9, { region->x_width, region->const_height_in_pixels }, 2, cregion_outline_color, 0.0f);
    }

    // Render kill regions
    render::r_set_line_width(2.0f);
    for_entity_type(editor->level, KillRegion, kill_region) {
        render::r_quad(kill_region->position, -1, kill_region->collider.size, { 1.0f, 0.0f, 0.0f, 0.45f });
        render::r_line_quad(kill_region->position, -1, kill_region->collider.size, color::red);
    }

    render_portals_connections(editor->level);
    
    if(modify_data->selected_entity != NULL) {
        vec2i pos  = { };
        vec2i size = { };
        bool has_size_and_pos = get_entity_size_and_pos_for_editor(modify_data->selected_entity, &pos, &size);
        if(has_size_and_pos) {
            render::r_set_line_width(2.0f);
            render::r_quad_marching_ants(pos, -10, size, ant_length, color::white, editor->time_elapsed, true);
        }
    }

    if(modify_data->hovered_entity != NULL) {
        vec2i pos  = { };
        vec2i size = { };
        bool has_size_and_pos = get_entity_size_and_pos_for_editor(modify_data->hovered_entity, &pos, &size);
        if(has_size_and_pos) {
            render::r_set_line_width(2.0f);
            render::r_quad_marching_ants(pos, -10, size, ant_length, { 1.0f, 1.0f, 1.0f, 0.5f }, editor->time_elapsed); // @todo
        }
    }
}

static
void imgui_modify_mode(ModifyModeData *modify_data, Editor *editor, int32_t max_item_width) {
    ImGui::Checkbox("Filter by type", &modify_data->filter_by_type);

    if(modify_data->filter_by_type) {
        ImGui::SetNextItemWidth(max_item_width);
        if(ImGui::Combo("##choose_entity_type_id_filter", &modify_data->entity_type_id_filter, entity_type_string, entity_type_count())) {
            modify_data->selected_entity = NULL;
        }
    
        ImGui::SetNextItemWidth(max_item_width);
        if(ImGui::BeginListBox("##entitites")) {
            for(Entity *entity = editor->level->entities.first_of_type[modify_data->entity_type_id_filter]; entity != NULL; entity = entity->next_of_type) {
                char buffer[64];
                sprintf_s(buffer, array_count(buffer), "ID: %d", entity->unique_id);
                if(ImGui::Selectable(buffer, entity == modify_data->selected_entity)) {
                    modify_data->selected_entity = entity;
                }
            }
            ImGui::EndListBox();
        }
    } else {
        ImGui::SetNextItemWidth(max_item_width);
        if(ImGui::BeginListBox("##entitites")) {
            for_every_entity(editor->level, entity) {
                char buffer[64];
                sprintf_s(buffer, array_count(buffer), "ID: %d | %s", entity->unique_id, entity_type_string[entity->entity_type_id]);
                if(ImGui::Selectable(buffer, entity == modify_data->selected_entity)) {
                    modify_data->selected_entity = entity;
                }
            }
            ImGui::EndListBox();
        }
    }

    // skip frame if just changed selected entity - hacky
    if(modify_data->selected_entity_changed_this_frame) {
        return;
    }
    
    if(modify_data->selected_entity) {
        const float32_t half_padding = ImGui::GetStyle().ItemSpacing.x * 0.5f;
        
        if(ImGui::Button("Delete all", { (float32_t)max_item_width * 0.5f - half_padding, 0.0f })) {
            while(editor->level->entities.first_of_type[modify_data->entity_type_id_filter] != NULL) {
                modify_mode_delete_selected_entity(modify_data);
            }
            return;
        }

        ImGui::SameLine();

        if(ImGui::Button("Delete entity", { (float32_t)max_item_width * 0.5f - half_padding, 0.0f })) {
            modify_mode_delete_selected_entity(modify_data);
        }
    }
    
    if(modify_data->selected_entity) {
        ImGui::SetNextItemWidth(max_item_width);
        ImGui::Text("Entity position:");
        ImGui::SetNextItemWidth(max_item_width);
        ImGui::InputInt("##position_x", &modify_data->selected_entity->position.x);
        ImGui::SetNextItemWidth(max_item_width);
        ImGui::InputInt("##position_y", &modify_data->selected_entity->position.y);
        ImGui::NewLine();

        switch(modify_data->selected_entity->entity_type_id) {
            case entity_type_id(Player): {
                auto player = modify_data->selected_entity->as<Player>();
                ImGui::Checkbox("Starts by entering pipe cutscene", &player->player_starts_by_entering_pipe_cutscene);
            } break;

            case entity_type_id(Goomba): {
                auto goomba = modify_data->selected_entity->as<Goomba>();
                ImGui::Text("Move direction");
                ImGui::SliderInt("##goomba_move_dir_slider", &goomba->move_dir, -1, 1);
            } break;

            case entity_type_id(Koopa): {
                auto koopa = modify_data->selected_entity->as<Koopa>();
                ImGui::Text("Move direction");
                ImGui::SliderInt("##koopa_move_dir_slider", &koopa->move_dir, -1, 1);
            } break;
            
            case entity_type_id(Bowser): {
                auto bowser = modify_data->selected_entity->as<Bowser>();
                ImGui::InputInt("##bowser_max_moves_in_any_direction", &bowser->max_moves_in_any_direction, 1, 1); ImGui::SameLine(); ImGui::Text("Move area");
                clamp_min(&bowser->max_moves_in_any_direction, 0);

                ImGui::InputInt("##bowser_vision_range", &bowser->vision_range, 1, 2); ImGui::SameLine(); ImGui::Text("vision range");
                clamp_min(&bowser->vision_range, 0);
            } break;

            case entity_type_id(Tilemap): {
                auto tilemap = modify_data->selected_entity->as<Tilemap>();
            } break;

            case entity_type_id(CameraRegion): {
                auto region = modify_data->selected_entity->as<CameraRegion>();
                
                int32_t x_tiles_width = region->x_tiles_width;
                ImGui::InputInt("##region_x_tiles_width", &x_tiles_width, 1, 1); ImGui::SameLine(); ImGui::Text("x tiles width");
                if(x_tiles_width != region->x_tiles_width && x_tiles_width > 0) {
                    region->x_tiles_width = x_tiles_width;
                    region->x_width = x_tiles_width * TILE_SIZE;
                }
            } break;

            case entity_type_id(BackgroundPlane): {
                auto bg_plane = modify_data->selected_entity->as<BackgroundPlane>();
                ImGui::InputInt("##bg_plane_w", &bg_plane->size.x, 1, 8); ImGui::SameLine(); ImGui::Text("width");
                ImGui::InputInt("##bg_plane_h", &bg_plane->size.y, 1, 8); ImGui::SameLine(); ImGui::Text("height");
                ImGui::ColorPicker3("##choose_bg_color", bg_plane->color.e);
            } break;

            case entity_type_id(BackgroundImage): {
                auto bg_image = modify_data->selected_entity->as<BackgroundImage>();
                ImGui::InputFloat("##bg_image_opacity", &bg_image->opacity, 0.0f, 1.0f); ImGui::SameLine(); ImGui::Text("opacity");
                clamp(&bg_image->opacity, 0.0f, 1.0f);
            } break;

            case entity_type_id(FireBar): {
                auto fire_bar = modify_data->selected_entity->as<FireBar>();

                ImGui::Text("Number of fires: "); ImGui::SameLine();
                ImGui::InputInt("##specify_fire_bar_fire_num", &fire_bar->fire_num, 1, 1);

                ImGui::NewLine();
                int32_t angle_idx = fire_bar->angle_idx;
                ImGui::Text("--- Starting angle ---");
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::SliderInt("##specify_fire_bar_start_angle", &angle_idx, 0, array_count(fire_bar_angles) - 1);
                fire_bar->angle_idx = angle_idx;
                fire_bar->angle = calc_fire_bar_angle(angle_idx);
            } break;

            case entity_type_id(MovingPlatform): {
                auto platform = modify_data->selected_entity->as<MovingPlatform>();

                int32_t y_start = platform->y_start;
                int32_t y_end   = platform->y_end;
                ImGui::InputInt("##platform_y_start", &y_start, 1, 8); ImGui::SameLine(); ImGui::Text("y start");
                ImGui::InputInt("##platform_y_end",   &y_end,   1, 8); ImGui::SameLine(); ImGui::Text("y end");

                if(y_start <= y_end) {
                    platform->y_start = y_start;
                    platform->y_end= y_end;

                    if(platform->position.y < platform->y_start) {
                        platform->position.y = platform->y_start;
                    } else if(platform->position.y > platform->y_end) {
                        platform->position.y = platform->y_end;
                    }
                }

                ImGui::SliderInt("Starting position", &platform->position.y, platform->y_start, platform->y_end);

                int32_t num_of_segments = platform->num_of_segments;
                ImGui::SliderInt("Num of segments", &num_of_segments, 1, 32);
                if(num_of_segments != platform->num_of_segments) {
                    platform->set_num_of_segments(num_of_segments);
                }
                
                ImGui::InputFloat("##platform_seconds_per_distance", &platform->seconds_per_distance, 0.1f, 0.5f); ImGui::SameLine(); ImGui::Text("seconds per distance");
                clamp_min(&platform->seconds_per_distance, 0.1f);
            } break;

            case entity_type_id(SideMovingPlatform): {
                auto platform = modify_data->selected_entity->as<SideMovingPlatform>();
                int32_t num_of_segments = platform->num_of_segments;
                ImGui::SliderInt("Num of segments", &num_of_segments, 1, 32); 
                if(num_of_segments != platform->num_of_segments) {
                    platform->set_num_of_segments(num_of_segments);
                }

                float32_t old_duration = platform->seconds_per_distance;

                bool changed_distance = ImGui::InputInt("##platform_x_distance", &platform->x_distance, 1, 2); ImGui::SameLine(); ImGui::Text("x distance");
                bool changed_duration = ImGui::InputFloat("##platform_seconds_per_distance", &platform->seconds_per_distance, 0.1f, 0.5f); ImGui::SameLine(); ImGui::Text("seconds per distance");
                if(changed_distance || changed_duration) {
                    platform->recalculate();
                }
                clamp_min(&platform->seconds_per_distance, 0.5f);

                // Recalc starting time accumulator
                if(changed_duration) {
                    float32_t perc_old = platform->time_accumulator / (old_duration * 2.0f);
                    platform->time_accumulator = perc_old * platform->seconds_per_distance * 2.0f;
                }

                ImGui::SetNextItemWidth(max_item_width);
                ImGui::SliderFloat("##platform_time_accumulator_start", &platform->time_accumulator, 0.0f, platform->seconds_per_distance * 2.0f);
                platform->collider.offset.x = calc_side_moving_platform_offset(platform);
            } break;

            case entity_type_id(Trigger): {
                auto trigger = modify_data->selected_entity->as<Trigger>();

                int32_t trigger_type = trigger->trigger_type;
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::Combo("##choose_trigger_type", &trigger_type, trigger_type_cstr, TRIGGER_TYPE__COUNT);
                trigger->trigger_type = (ETriggerType)trigger_type;

                ImGui::SetNextItemWidth(max_item_width);
                ImGui::InputText("##trigger_key", trigger->trigger_key, Trigger::trigger_key_size);
            } break;

            case entity_type_id(TriggerableText): {
                auto tt = modify_data->selected_entity->as<TriggerableText>();
                ImGui::SetNextItemWidth(max_item_width);
                ImGui::InputText("##tt_text",     tt->text, TriggerableText::triggerable_text_max_length);
                ImGui::InputText("key", tt->trigger_key, Trigger::trigger_key_size);
            } break;

            case entity_type_id(Mushroom): {
                auto mushroom = modify_data->selected_entity->as<Mushroom>();
                ImGui::Text("Move direction");
                ImGui::SliderInt("##mushroom_move_dir_slider", &mushroom->direction, -1, 1);
            } break;
        }
    }
}

static
void init_paint_mode(PaintModeData *paint_data, Editor *editor) {
    paint_data->paint_by_selecting     = true;
    paint_data->show_tile_flags_window = true;

    paint_data->selected_tilemap = NULL;
    
    paint_data->tile_desc.tile_flags             = TILE_FLAG_BLOCKS_MOVEMENT;
    paint_data->tile_desc.tile_drop              = TILE_DROP_NONE;
    paint_data->tile_desc.drops_left             = 0;
    paint_data->tile_desc.is_animated            = false;
    paint_data->tile_desc.tile_anim              = (ETileAnim)0;
    paint_data->tile_desc.tile_sprite            = (ETileSprite)0;
    paint_data->tile_desc.tile_sprite_after_drop = (ETileSprite)0;
}

static
void update_paint_mode(PaintModeData *paint_data, Editor *editor, Input *input) {
    // Determine if cursor is pointing at any tile in selected tilemap
    paint_data->cursor_tile_in_tilemap_bounds = false;
    if(paint_data->selected_tilemap) {
        const vec2i origin_tile = paint_data->selected_tilemap->get_position_ti().tile;

        auto tilemap = paint_data->selected_tilemap;
        if(editor->ti_cursor.tile.x >= origin_tile.x && editor->ti_cursor.tile.x < origin_tile.x + tilemap->x_tiles
           && editor->ti_cursor.tile.y >= origin_tile.y && editor->ti_cursor.tile.y < origin_tile.y + tilemap->y_tiles) {
            paint_data->cursor_tile_in_tilemap_bounds = true;
        }

        /* Copy tile desc */
        if(editor->cursor_in_editor_bounds && paint_data->cursor_tile_in_tilemap_bounds && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            if(input->keys[key_left_ctrl] & input_is_down && input->btns[btn_left] & input_pressed) {
                vec2i rel_tile_xy = editor->ti_cursor.tile - origin_tile;
                Tile *tile = tilemap->get_tile(rel_tile_xy.x, rel_tile_xy.y);
                if(tile != NULL) {
                    paint_data->tile_desc = tile->tile_desc;
                }
            } else {
                paint_data->clear_selected_tiles = input->keys[key_left_shift] & input_is_down;

                if(paint_data->paint_by_selecting) {
                    if(input->btns[btn_left] & input_pressed) {
                        paint_data->currently_selecting = true;
                        paint_data->selection_ti_a = editor->ti_cursor;
                    }
                    
                    paint_data->cancel_selection_paint = false;
                    if(paint_data->currently_selecting) {
                        paint_data->cancel_selection_paint = input->btns[btn_right] & input_is_down;

                        paint_data->selection_ti_b = editor->ti_cursor;

                        if(input->btns[btn_left] & input_released) {
                            paint_data->currently_selecting = false;

                            if(!paint_data->cancel_selection_paint) {
                                // Paint
                                TileInfo ti_min, ti_max;
                                get_min_and_max(&ti_min, &ti_max, paint_data->selection_ti_a, paint_data->selection_ti_b);
                                for(int32_t tile_y = ti_min.tile.y; tile_y <= ti_max.tile.y; ++tile_y) {
                                    for(int32_t tile_x = ti_min.tile.x; tile_x <= ti_max.tile.x; ++tile_x) {
                                        // Calc tilemap_rel position
                                        int32_t tile_x_relative = tile_x - origin_tile.x;
                                        int32_t tile_y_relative = tile_y - origin_tile.y;
                                        if(tilemap->tile_in_bounds(tile_x_relative, tile_y_relative)) {
                                            if(paint_data->clear_selected_tiles) {
                                                // Clear tile
                                                Tile *tile = tilemap->get_tile(tile_x_relative, tile_y_relative);
                                                if(tile != NULL) {
                                                    clear_tile(tile);
                                                }
                                            } else {
                                                tilemap->set_tile(tile_x_relative, tile_y_relative, paint_data->tile_desc);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    vec2i rel_tile_xy = editor->ti_cursor.tile - origin_tile;
                    if(input->btns[btn_left] & input_is_down) {
                        if(paint_data->clear_selected_tiles) {
                            // Clear tile
                            Tile *tile = tilemap->get_tile(rel_tile_xy.x, rel_tile_xy.y);
                            if(tile != NULL) {
                                clear_tile(tile);
                            }
                        } else {
                            // Set tile
                            tilemap->set_tile(rel_tile_xy.x, rel_tile_xy.y, paint_data->tile_desc);
                        }
                    }
                }
            }
        } else {
            paint_data->currently_selecting = false;
        }
    }
}

static
void render_invisible_blocks_outline(Tilemap *tilemap, float32_t time_elapsed) {
    // Draw "invisible" blocks
    for(int32_t y_tile = 0; y_tile < tilemap->y_tiles; ++y_tile) {
        for(int32_t x_tile = 0; x_tile < tilemap->x_tiles; ++x_tile) {
            Tile *tile = tilemap->get_tile(x_tile, y_tile);
            if(tile != NULL && (tile->tile_desc.tile_flags & TILE_FLAG_IS_INVISIBLE || tile->tile_desc.tile_flags & TILE_FLAG_NO_RENDER)) {
                const auto ti = tile->get_ti();
                render::r_quad_marching_ants(ti.position, -10, TILE_SIZE_2, 8, { 1.0f, 1.0f, 1.0f, 0.8f }, time_elapsed);
                render::r_sprite(ti.position, -8, TILE_SIZE_2, global_data::get_sprite(SPRITE_BLOCK_QUESTION_1), { 1.0f, 1.0f, 1.0f, 0.4f });
            }
        }
    }
}

static
void render_paint_mode(PaintModeData *paint_data, Editor *editor) {
    render_editor_level(editor);

    render::r_set_line_width(0.7f);
    for_entity_type(editor->level, Tilemap, tilemap) {
        render_transparent_blocks_drops(tilemap);

        if(tilemap == paint_data->selected_tilemap) {
            continue;
        }

        if(editor->show_tilemap_mesh) {
            render_tilemap_mesh(tilemap, 0, tilemap_mesh_color);
        } else {
            render::r_line_quad(tilemap->position, 0, vec2i{ tilemap->x_tiles, tilemap->y_tiles } * TILE_SIZE_2, tilemap_mesh_color);
        }
    }

    if(paint_data->selected_tilemap) {
        Tilemap *tilemap = paint_data->selected_tilemap;

        if(editor->show_tilemap_mesh) {
            render::r_set_line_width(1.25f);
            render_tilemap_mesh(tilemap, -5, { 1.0f, 1.0f, 1.0f, 0.1f });
        }

        render_invisible_blocks_outline(tilemap, editor->time_elapsed);

        const vec2i pos = tilemap->position;
        const vec2i size = vec2i { tilemap->x_tiles, tilemap->y_tiles } * TILE_SIZE_2;
        const vec4 ants_color = { 0.7f, 0.7f, 0.8f, 1.0f };
        const int32_t ant_length = 8;
        render::r_set_line_width(2.0f);
        render::r_quad_marching_ants(pos, -10, size, ant_length, ants_color, editor->time_elapsed, true);

        if(paint_data->paint_by_selecting) {

            vec4 color;
            if(paint_data->cancel_selection_paint) {
                color = color::yellow;
            } else if(paint_data->clear_selected_tiles) {
                color = color::red;
            } else {
                color = color::white;
            }

            if(paint_data->currently_selecting) {
                TileInfo ti_min, ti_max;
                get_min_and_max(&ti_min, &ti_max, paint_data->selection_ti_a, paint_data->selection_ti_b);
                render::r_set_line_width(1.25f);
                render::r_quad_marching_ants(ti_min.position, -10, ti_max.position - ti_min.position + TILE_SIZE_2, 6, color, editor->time_elapsed);
            } else {
                render::r_set_line_width(1.25f);
                render::r_quad_marching_ants(editor->ti_cursor.position, -10, TILE_SIZE_2, 6, color, editor->time_elapsed);
            }
        } else {
            if(paint_data->cursor_tile_in_tilemap_bounds) {
                vec4 color = paint_data->clear_selected_tiles ? color::dark_green : color::white;

                render::r_set_line_width(1.5f);
                render::r_line_quad(editor->ti_cursor.position, -10, TILE_SIZE_2, color);
            }
        }
    }
}

static
void imgui_paint_mode(PaintModeData *paint_data, Editor *editor, int32_t max_item_width) {
    ImGui::SetNextItemWidth(max_item_width);
    if(ImGui::BeginListBox("##tilemaps")) {
        for(Entity *entity = editor->level->entities.first_of_type[entity_type_id(Tilemap)]; entity != NULL; entity = entity->next_of_type) {
            char buffer[64];
            sprintf_s(buffer, array_count(buffer), "ID: %d", entity->unique_id);
            if(ImGui::Selectable(buffer, entity == paint_data->selected_tilemap)) {
                paint_data->selected_tilemap = entity->as<Tilemap>();
            }
        }
        ImGui::EndListBox();
    }

    ImGui::Checkbox("Paint by selecting", &paint_data->paint_by_selecting);
    if(paint_data->paint_by_selecting) {
        if(paint_data->currently_selecting) {
            TileInfo ti_min, ti_max;
            get_min_and_max(&ti_min, &ti_max, paint_data->selection_ti_a, paint_data->selection_ti_b);
            ImGui::Text("Selection: %d, %d", ti_max.tile.x - ti_min.tile.x + 1, ti_max.tile.y - ti_min.tile.y + 1);
        } else {
            ImGui::Text("Selection: none");
        }
    } 

    auto tile_flag_checkbox = [=] (const char *label, uint32_t tile_flag) {
        bool is_set = paint_data->tile_desc.tile_flags & tile_flag;
        bool changed = ImGui::Checkbox(label, &is_set);
        if(changed) {
            if(is_set) {
                paint_data->tile_desc.tile_flags |= tile_flag;
            } else {
                paint_data->tile_desc.tile_flags &= ~tile_flag;
            }
        }
    };

    if(ImGui::Button(paint_data->show_tile_flags_window ? "Hide tile flags" : "Change tile flags", { (float32_t)max_item_width, 0 })) {
        paint_data->show_tile_flags_window = !paint_data->show_tile_flags_window;
    }

    if(paint_data->show_tile_flags_window) {
        ImGui::Begin("Tile flags", &paint_data->show_tile_flags_window, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse); {
            ImGui::Text("Tile flags: %d", paint_data->tile_desc.tile_flags);
            tile_flag_checkbox("Blocks movement", TILE_FLAG_BLOCKS_MOVEMENT);
            tile_flag_checkbox("Do anim on hit", TILE_FLAG_DO_ANIM_ON_HIT);
            tile_flag_checkbox("Breakable", TILE_FLAG_BREAKABLE);
            tile_flag_checkbox("Invisible", TILE_FLAG_IS_INVISIBLE);
            tile_flag_checkbox("Becomes visible after hit", TILE_FLAG_BECOMES_VISIBLE_AFTER_HIT);
            tile_flag_checkbox("Changes sprite after drop", TILE_FLAG_CHANGES_SPRITE_AFTER_DROP);
            tile_flag_checkbox("Do not render", TILE_FLAG_NO_RENDER);
        } ImGui::End();
    }
    
    ImGui::NewLine();
    ImGui::Text("--- Specify tile drop/s ---");
    ImGui::SetNextItemWidth(max_item_width);
    ImGui::Combo("##choose_tile_drop", (int32_t *)&paint_data->tile_desc.tile_drop, tile_drop_cstr, TILE_DROP__COUNT);
    if(paint_data->tile_desc.tile_drop != TILE_DROP_NONE) {
        ImGui::SetNextItemWidth(max_item_width);
        ImGui::InputInt("##specify_drop_amount", &paint_data->tile_desc.drops_left, 1, 1);
    }

    auto pick_sprite = [&] (ETileSprite *tile_sprite_ptr, const char *imgui_id_button_base) -> void {
        const float32_t window_padding = ImGui::GetStyle().WindowPadding.x;
        const float32_t frame_padding  = ImGui::GetStyle().FramePadding.x;
        const float32_t sprite_width = 32.0f;
        const float32_t space  = ImGui::GetWindowContentRegionMax().x - window_padding * 2;
        const int32_t   collumns = (int32_t)floorf(space / (sprite_width + frame_padding * 2));

        ImGui::BeginTable("pick_sprite_table", collumns);
        ImGui::TableNextColumn();
        for(int32_t tile_sprite_idx = 0; tile_sprite_idx < TILE_SPRITE__COUNT; ++tile_sprite_idx) {
            auto sprite = get_tile_sprite((ETileSprite)tile_sprite_idx);
            ImTextureID im_tex_id = (void *)sprite.texture->texture_id;
            ImVec2 im_tex_size = { (float32_t)sprite.width, (float32_t)sprite.height };
            ImVec2 im_tex_uv_1 = { sprite.tex_coords[0].x, sprite.tex_coords[2].y };
            ImVec2 im_tex_uv_2 = { sprite.tex_coords[2].x, sprite.tex_coords[0].y };

            char buffer[64] = { };
            sprintf_s(buffer, array_count(buffer), (std::string(imgui_id_button_base) + "##tile_sprite_idx:%d").c_str(), tile_sprite_idx);

            const ImVec4 tint = (ETileSprite)tile_sprite_idx == *tile_sprite_ptr ? ImVec4{ 0, 0, 0, 1 } : ImVec4{ 1, 1, 1, 1 };
            const float32_t height_scale = sprite_width / (float32_t)sprite.width;

            if(ImGui::ImageButton(buffer, im_tex_id, { sprite_width, sprite.height * height_scale }, im_tex_uv_1, im_tex_uv_2, {}, tint)) {
                *tile_sprite_ptr = (ETileSprite)tile_sprite_idx;
            }
            ImGui::TableNextColumn();
        }
        ImGui::EndTable();
    };

    // Don't select visuals if won't be rendered
    if(!(paint_data->tile_desc.tile_flags & TILE_FLAG_NO_RENDER)) {
        ImGui::NewLine();
        ImGui::Text("--- Specify tile visuals ---");
        ImGui::Checkbox("Animated", &paint_data->tile_desc.is_animated);
        ImGui::SetNextItemWidth(max_item_width);
        if(paint_data->tile_desc.is_animated) {
            ImGui::Combo("##choose_tile_anim", (int32_t *)&paint_data->tile_desc.tile_anim, tile_anim_cstr, TILE_ANIM__COUNT);
        } else {
            pick_sprite(&paint_data->tile_desc.tile_sprite, "tile_sprite");
        }

        tile_flag_checkbox("Changes sprite after drop", TILE_FLAG_CHANGES_SPRITE_AFTER_DROP);
        if(paint_data->tile_desc.tile_flags & TILE_FLAG_CHANGES_SPRITE_AFTER_DROP) {
            pick_sprite(&paint_data->tile_desc.tile_sprite_after_drop, "tile_sprite_after_drop");
        }
    }

    // Pick tile break anim
    if(paint_data->tile_desc.tile_flags & TILE_FLAG_BREAKABLE) {
        ImGui::NewLine();
        ImGui::Text("- Pick tile break animation -");
        ImGui::Text("%d", (int32_t)paint_data->tile_desc.tile_break_anim);
        ImGui::SetNextItemWidth(max_item_width);
        if(ImGui::BeginListBox("##tile_break_anims")) {
            for(int32_t idx = 0; idx < TILE_BREAK_ANIM__COUNT; ++idx) {
                if(ImGui::Selectable(tile_break_anim_cstr[idx], paint_data->tile_desc.tile_break_anim == (ETileBreakAnim)idx)) {
                    paint_data->tile_desc.tile_break_anim = (ETileBreakAnim)idx;
                }
            }
            ImGui::EndListBox();
        }
    }
}

static
bool win32_file_dialog_open(char *file_buffer, size_t file_buffer_size, SDL_Window *sdl_window, char *filter) {
    assert(file_buffer != NULL && file_buffer_size > 0);

    SDL_SysWMinfo sys_wm_info;
    SDL_VERSION(&sys_wm_info.version);
    if(SDL_GetWindowWMInfo(sdl_window, &sys_wm_info) != SDL_TRUE) {
        return false;
    }

    OPENFILENAMEA ofna = { };
    ofna.lStructSize = sizeof(OPENFILENAMEA);
    ofna.hwndOwner = sys_wm_info.info.win.window;
    ofna.hInstance = 0;
    ofna.lpstrFilter = filter;
    ofna.lpstrCustomFilter = NULL;
    ofna.nMaxCustFilter = 0;
    ofna.nFilterIndex = 0;
    ofna.lpstrFile = file_buffer;
    ofna.nMaxFile = file_buffer_size;
    ofna.lpstrFileTitle = NULL;
    ofna.nMaxFileTitle = 0;
    ofna.lpstrInitialDir = NULL;
    ofna.lpstrTitle = NULL;
    ofna.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofna) != 0;
}

static
bool win32_file_dialog_save(char *file_buffer, size_t file_buffer_size, SDL_Window *sdl_window, char *filter, char *def_ext) {
    assert(file_buffer != NULL && file_buffer_size > 0);

    SDL_SysWMinfo sys_wm_info;
    SDL_VERSION(&sys_wm_info.version);
    if(SDL_GetWindowWMInfo(sdl_window, &sys_wm_info) != SDL_TRUE) {
        return false;
    }

    OPENFILENAMEA ofna = { };
    ofna.lStructSize = sizeof(OPENFILENAMEA);
    ofna.hwndOwner = sys_wm_info.info.win.window;
    ofna.hInstance = 0;
    ofna.lpstrFilter = filter;
    ofna.lpstrCustomFilter = NULL;
    ofna.nMaxCustFilter = 0;
    ofna.nFilterIndex = 0;
    ofna.lpstrFile = file_buffer;
    ofna.nMaxFile = file_buffer_size;
    ofna.lpstrFileTitle = NULL;
    ofna.nMaxFileTitle = 0;
    ofna.lpstrInitialDir = NULL;
    ofna.lpstrTitle = NULL;
    ofna.lpstrDefExt = def_ext;
    ofna.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    return GetSaveFileNameA(&ofna) != 0;
}

static
void set_imgui_style_for_editor(void) {
    ImGui::GetIO().Fonts->AddFontFromFileTTF((global_data::get_data_path() + "LiberationMono-Regular.ttf").c_str(), 18.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 1.5f; // = 5.3f;
    style.FrameRounding = 2.3f;
    style.ScrollbarRounding = 0;

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.30f, 0.30f, 0.30f, 0.65f); // = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.5f,  0.5f,  0.5f,  0.49f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.60f, 0.60f, 0.6f, 0.68f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}