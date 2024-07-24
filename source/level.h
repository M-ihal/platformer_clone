#ifndef _LEVEL_H
#define _LEVEL_H

#include "common.h"
#include "entity.h"

#define LEVEL_MEMORY_TO_ALLOC MB(8)

#define for_every_entity(level, var_name)      for(Entity *var_name = (level)->entities.first; var_name != NULL; (var_name) = (var_name)->next)
#define for_entity_type(level, Type, var_name) for(Type *var_name = (level)->entities.first_of_type[entity_type_id(Type)]->as<Type>(); var_name != NULL; (var_name) = (var_name)->next_of_type->as<Type>())
#define for_entity_type_id(level, type_id, var_name) for(Entity *var_name = (level)->entities.first_of_type[type_id]; var_name != NULL; (var_name) = (var_name)->next_of_type)

#define TILE_SIZE 16
#define TILE_SIZE_2 (vec2i{ TILE_SIZE, TILE_SIZE })

enum EPauseState {
    UNPAUSED,
    PAUSED,
    TO_BE_PAUSED
};

struct RenderView {
    float32_t left;
    float32_t bottom;
    float32_t right;
    float32_t top;
    float32_t near;
    float32_t far;
    float32_t scale;
    float32_t x_translate;
    float32_t y_translate;

    mat4x4 calc_proj(void);
    mat4x4 calc_view(void);
    vec2   calc_dims(void);
    vec2   calc_dims_scaled(void);
};

struct CameraRegion;

enum ELevelState {
    PLAYING,         // Gameplay
    FINISHING_LEVEL, // Counting score and stuff after entering the castle
    FINISHED         // Level finished, do nothing
};

enum class ELevelMusic : int32_t {
    regular = 0,
    during_star_power,
    during_player_enter_pipe_anim,
    on_level_completed,
    __COUNT,
    __NOT_SET
};

struct Level {
    uint8_t *memory;
    size_t   memory_size;
    size_t   memory_used;

    int32_t next_entity_id;
    EntityPool entities;
    EntityPool entities_unused;
    std::vector<Entity *> *to_be_deleted; // Entities that will be deleted at the end of frame, ptr because of new kw

#define MAX_NO_PAUSED_ENTITIES 32
    EPauseState pause_state;
    int32_t no_paused_entities_count;
    Entity *no_paused_entities[MAX_NO_PAUSED_ENTITIES];
    
    RenderView render_view;
    uint32_t   current_region_unique_id;

    // Level music
    ELevelMusic current_level_music;
    int32_t level_music_id[(int32_t)ELevelMusic::__COUNT];

    // Level states
    bool player_has_died;
    bool player_has_reached_the_end;
    ELevelState level_state;

    // ELevelState::FINISHING_LEVEL
    float32_t finishing_level_timer;
    int32_t   finishing_level_stage;

    // Frame data
    int32_t   frames_elapsed;
    float64_t delta_time;
    float64_t elapsed_time;

    int32_t coins_collected_this_frame;
    int32_t points_aquired_this_frame;

    // Wake up rect
    vec2i wake_up_rect_position;
    vec2i wake_up_rect_size;
    
    // Level score
    bool      disable_level_timer;
    bool      update_level_time;
    float64_t level_time_accumulator;
    int32_t   level_time;
};

Level *create_empty_level(void);
void recreate_empty_level(Level **level);
void delete_level(Level *level);
void update_level(Level *level, SimInput input, struct Game *game, float64_t delta_time);
void render_level(Level *level);

void set_level_music_and_play(Level *level, ELevelMusic music = ELevelMusic::__NOT_SET); // __NOT_SET = ensure correct music is playing
void set_starting_level_music(Level *level);

void pause_level(Level *level, Entity *exception = NULL);
void unpause_level(Level *level);


void add_points(Level *level, int32_t points);
void add_coins(Level *level, int32_t coins = 1);
void add_points_with_text_above_entity(Level *level, int32_t points, Entity *entity);

void render_level_debug_stuff(Level *level);

struct TileInfo {
    vec2i tile;
    vec2i position; // "bottom-left" pixel of the tile
};

vec2i    pixel_from_real(vec2 real);
TileInfo tile_info(vec2i tile);
TileInfo tile_info_at(vec2i p);
vec2     convert_to_view_space(vec2i viewport_position, recti32 viewport, RenderView *view);

#endif /* _LEVEL_H */