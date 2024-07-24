#ifndef _EDITOR_H
#define _EDITOR_H

#include "common.h"
#include "maths.h"
#include "input.h"
#include "opengl_abs.h"
#include "level.h"
#include "all_entities.h"

#include <SDL.h>
#include <SDL_opengl.h>

enum EEditorMode : int32_t {
    EDITOR_MODE_SPAWNING,
    EDITOR_MODE_MODYFING,
    EDITOR_MODE_PAINTING,
    EDITOR_MODE_VIEW_SPRITESHEET,
    EDITOR_MODE__COUNT
};

inline const char *editor_mode_string[] = {
    "[F1] Spawn entities",
    "[F2] Modify entities",
    "[F3] Place tiles",
    "[F4] View Spritesheet"
};

inline const int32_t editor_mode_key_bind[] = {
    key_f01, 
    key_f02, 
    key_f03,
    key_f04
};

static_assert(array_count(editor_mode_key_bind) == EDITOR_MODE__COUNT && array_count(editor_mode_string) == EDITOR_MODE__COUNT);

enum ESpawnType : int32_t {
    SPAWN_PLAYER,
    SPAWN_GOOMBA,
    SPAWN_KOOPA,
    SPAWN_PIRANHA,
    SPAWN_TILEMAP,
    SPAWN_BOWSER,
    SPAWN_CAMERA_REGION,
    SPAWN_BACKGROUND_PLANE,
    SPAWN_BACKGROUND_SPRITE,
    SPAWN_FIRE_BAR,
    SPAWN_KILL_REGION,
    SPAWN_FLAG_POLE,
    SPAWN_COIN,
    SPAWN_PORTAL,
    SPAWN_MOVING_PLATFORM,
    SPAWN_SIDE_MOVING_PLATFORM,
    SPAWN_TRIGGER,
    SPAWN_TRIGGABLE_TEXT,
    SPAWN_BACKGROUND_IMAGE,
    SPAWN_CASTLE_LEVER,
    SPAWN_MUSHROOM,
    SPAWN_TYPE__COUNT
};

inline const char *spawn_type_string[] = {
    "Spawn player",
    "Spawn goomba",
    "Spawn koopa",
    "Spawn piranha",
    "Spawn tilemap",
    "Spawn Bowser",
    "Spawn camera region",
    "Spawn background plane",
    "Spawn background sprite",
    "Spawn fire bar",
    "Spawn kill region",
    "Spawn flag pole",
    "Spawn coin",
    "Spawn portal",
    "Spawn moving platform",
    "Spawn side moving platform",
    "Spawn trigger",
    "Spawn triggable text",
    "Spawn background image",
    "Spawn castle lever",
    "Spawn mushroom",
};

struct SpawnModeData {
    ESpawnType spawn_type;

    // SPAWN_GOOMBA
    int32_t goomba_type;

    // SPAWN_KOOPA
    int32_t koopa_type;

    // SPAWN_BACKGROUND_PLANE
    vec3 bg_plane_color;

    // SPAWN_FIRE_BAR
    int32_t fire_num;

    // SPAWN_BACKGROUND_SPRITE
    int32_t sprite_id;

    // SPAWN_BACKGROUND_IMAGE
    int32_t image_id;

    // SPAWN_PORTAL
    bool portal_vertical_enter;
    bool portal_do_exit_anim;
    bool portal_vertical_exit;
    bool portal_offset_x_by_half_a_tile;

    // SPAWN_TRIGGER
    // int32_t trigger_type;

    enum {
        DRAW_CURSOR_SELECTION,
        DRAW_CURSOR_SINGLE,
        DRAW_CURSOR_A_TO_B
    }    draw_cursor_mode;
    bool draw_cursor_mark_red;

    // DRAW_CURSOR_SELECTION
    TileInfo draw_cursor_tile_a;
    TileInfo draw_cursor_tile_b;
    // DRAW_CURSOR_SINGLE
    vec2i    draw_cursor_position;
    vec2i    draw_cursor_size;
    // DRAW_CURSOR_A_TO_B
    vec2i    draw_cursor_a_to_b_pos_1;
    vec2i    draw_cursor_a_to_b_pos_2;
    vec2i    draw_cursor_a_to_b_size_1;
    vec2i    draw_cursor_a_to_b_size_2;
};

struct ModifyModeData {
    bool    filter_by_type;
    int32_t entity_type_id_filter;
    Entity *selected_entity;
    Entity *hovered_entity;
    bool  selected_entity_changed_this_frame;
    bool  drag_selected_entity;
    vec2i offset_of_selected_entity;
};

struct PaintModeData {
    bool paint_by_selecting;
    bool currently_selecting;
    bool cancel_selection_paint;
    bool clear_selected_tiles;
    TileInfo selection_ti_a; // not relative to tilemap...
    TileInfo selection_ti_b; // not relative to tilemap...

    bool show_tile_flags_window;
    bool cursor_tile_in_tilemap_bounds;
    Tilemap *selected_tilemap;
    TileDesc tile_desc;
};

static_assert(SPAWN_TYPE__COUNT == array_count(spawn_type_string));
static_assert(EDITOR_MODE__COUNT == array_count(editor_mode_string));

struct Editor {
    SDL_Window   *sdl_window;
    SDL_GLContext gl_context;
    int32_t window_w;
    int32_t window_h;

    float64_t delta_time;
    float64_t time_elapsed;

    vec2i mouse_pos;
    vec2i mouse_pos_in_game_space;
    vec2i mouse_pos_in_game_space_last_press;
    bool  mouse_is_down_this_frame;
    bool  cursor_in_editor_bounds;

    TileInfo ti_cursor;            // TileInfo of tile on which the cursor is
    TileInfo ti_cursor_last_press; // TileInfo of tile on which the cursor was when left mouse button was pressed last time

    Level *level;
    EEditorMode editor_mode;

    SpawnModeData  spawn_mode_data;
    ModifyModeData modify_mode_data;
    PaintModeData  paint_mode_data;
    
    bool show_level_music_window;

    int32_t spritesheet_cell_w;
    int32_t spritesheet_cell_h;

    /* rendering */
    recti32      draw_rect;
    RenderView   render_view;
    Framebuffer *framebuffer;

    bool show_cursor_coords;
    bool show_tilemap_mesh;
    float32_t tilemap_opacity;
};

Editor *create_editor(SDL_Window *sdl_window, SDL_GLContext gl_context);
void  resize_editor(Editor *editor, int32_t window_w, int32_t window_h);
void  delete_editor(Editor *editor);
void  update_editor(Editor *editor, Input *input, float64_t delta_time);
void  render_editor(Editor *editor);

void process_events_for_editor_imgui(SDL_Event *sdl_event);
void render_editor_imgui(void);

#endif /* _EDITOR_H */