#ifndef _TILEMAP_H
#define _TILEMAP_H

#include "common.h"
#include "level.h"
#include "renderer.h"
#include "save_data.h"

#include "data.h"

/*
      enum ETileDrop
*/
#define TILE_DROPS\
    TILE_DROP(TILE_DROP_NONE)\
    TILE_DROP(TILE_DROP_POWERUP)\
    TILE_DROP(TILE_DROP_STAR)\
    TILE_DROP(TILE_DROP_COINS)

#define TILE_DROP(e_drop) e_drop,
enum ETileDrop : int32_t { TILE_DROPS TILE_DROP__COUNT, TILE_DROP__INVALID };
#undef TILE_DROP

#define TILE_DROP(e_drop) #e_drop,
inline const char *tile_drop_cstr[] = { TILE_DROPS };
#undef TILE_DROP

#define TILE_DROP(e_drop) if(_str == #e_drop) return ETileDrop::e_drop;
inline ETileDrop tile_drop_from_cstr(const char *cstr) { auto _str = std::string(cstr); TILE_DROPS return TILE_DROP__INVALID; }
#undef TILE_DROP

/*
      enum ETileSprite
*/
#define TILE_SPRITES\
    TILE_SPRITE(TILE_SPRITE_BRICK_BIG,             SPRITE_BLOCK_BRICK_BIG)\
    TILE_SPRITE(TILE_SPRITE_SOLID,                 SPRITE_BLOCK_SOLID)\
    TILE_SPRITE(TILE_SPRITE_BRICK,                 SPRITE_BLOCK_BRICK)\
    TILE_SPRITE(TILE_SPRITE_BRICK_2,               SPRITE_BLOCK_BRICK_2)\
    TILE_SPRITE(TILE_SPRITE_COBBLESTONE,           SPRITE_BLOCK_COBBLESTONE)\
    TILE_SPRITE(TILE_SPRITE_QUESTION_EMPTY,        SPRITE_BLOCK_QUESTION_EMPTY)\
    TILE_SPRITE(TILE_SPRITE_UG_BRICK_BIG,          SPRITE_BLOCK_UG_BRICK_BIG)\
    TILE_SPRITE(TILE_SPRITE_UG_SOLID,              SPRITE_BLOCK_UG_SOLID)\
    TILE_SPRITE(TILE_SPRITE_UG_BRICK,              SPRITE_BLOCK_UG_BRICK)\
    TILE_SPRITE(TILE_SPRITE_UG_BRICK_2,            SPRITE_BLOCK_UG_BRICK_2)\
    TILE_SPRITE(TILE_SPRITE_UG_COBBLESTONE,        SPRITE_BLOCK_UG_COBBLESTONE)\
    TILE_SPRITE(TILE_SPRITE_UG_QUESTION_EMPTY,     SPRITE_BLOCK_UG_QUESTION_EMPTY)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_BRICK_BIG,      SPRITE_BLOCK_CASTLE_BRICK_BIG)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_SOLID,          SPRITE_BLOCK_CASTLE_SOLID)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_BRICK,          SPRITE_BLOCK_CASTLE_BRICK)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_BRICK_2,        SPRITE_BLOCK_CASTLE_BRICK_2)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_COBBLESTONE,    SPRITE_BLOCK_CASTLE_COBBLESTONE)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_QUESTION_EMPTY, SPRITE_BLOCK_CASTLE_QUESTION_EMPTY)\
    TILE_SPRITE(TILE_SPRITE_CASTLE_BRIDGE,         SPRITE_BLOCK_CASTLE_BRIDGE)\
    TILE_SPRITE(TILE_SPRITE_PIPE_V_L,              SPRITE_PIPE_V_L)\
    TILE_SPRITE(TILE_SPRITE_PIPE_V_R,              SPRITE_PIPE_V_R)\
    TILE_SPRITE(TILE_SPRITE_PIPE_V_L_ENTRY,        SPRITE_PIPE_V_L_ENTRY)\
    TILE_SPRITE(TILE_SPRITE_PIPE_V_R_ENTRY,        SPRITE_PIPE_V_R_ENTRY)\
    TILE_SPRITE(TILE_SPRITE_PIPE_H_T,              SPRITE_PIPE_H_T)\
    TILE_SPRITE(TILE_SPRITE_PIPE_H_B,              SPRITE_PIPE_H_B)\
    TILE_SPRITE(TILE_SPRITE_PIPE_H_T_ENTRY,        SPRITE_PIPE_H_T_ENTRY)\
    TILE_SPRITE(TILE_SPRITE_PIPE_H_B_ENTRY,        SPRITE_PIPE_H_B_ENTRY)\
    TILE_SPRITE(TILE_SPRITE_PIPE_T_CONNECT,        SPRITE_PIPE_T_CONNECT)\
    TILE_SPRITE(TILE_SPRITE_PIPE_B_CONNECT,        SPRITE_PIPE_B_CONNECT)\
    TILE_SPRITE(TILE_SPRITE_MUSHROOM_CAP,          SPRITE_MUSHROOM_CAP)\
    TILE_SPRITE(TILE_SPRITE_MUSHROOM_CAP_L,        SPRITE_MUSHROOM_CAP_L)\
    TILE_SPRITE(TILE_SPRITE_MUSHROOM_CAP_R,        SPRITE_MUSHROOM_CAP_R)\

#define TILE_SPRITE(e_sprite, ...) e_sprite,
enum ETileSprite : int32_t { TILE_SPRITES TILE_SPRITE__COUNT, TILE_SPRITE__INVALID };
#undef TILE_SPRITE

#define TILE_SPRITE(e_sprite, ...) #e_sprite,
inline const char *tile_sprite_cstr[] = { TILE_SPRITES };
#undef TILE_SPRITE

#define TILE_SPRITE(e_sprite, ...) if(_str == #e_sprite) return ETileSprite::e_sprite;
inline ETileSprite tile_sprite_from_cstr(const char *cstr) { auto _str = std::string(cstr); TILE_SPRITES return TILE_SPRITE__INVALID; }
#undef TILE_SPRITE

Sprite get_tile_sprite(ETileSprite e_sprite);

/*
      enum ETileAnim
*/
#define TILE_ANIMS\
    TILE_ANIM(TILE_ANIM_QUESTION)

#define TILE_ANIM(e_anim) e_anim,
enum ETileAnim : int32_t { TILE_ANIMS TILE_ANIM__COUNT, TILE_ANIM__INVALID };
#undef TILE_ANIM

#define TILE_ANIM(e_anim) #e_anim,
inline const char *tile_anim_cstr[] = { TILE_ANIMS };
#undef TILE_ANIM

#define TILE_ANIM(e_anim) if(_str == #e_anim) return ETileAnim::e_anim;
inline ETileAnim tile_anim_from_cstr(const char *cstr) { auto _str = std::string(cstr); TILE_ANIMS; return TILE_ANIM__INVALID; }
#undef TILE_ANIM

/*
        enum ETileBreakAnim
*/
#define TILE_BREAK_ANIMS\
    TILE_BREAK_ANIM(TILE_BREAK_ANIM_BRICK)\
    TILE_BREAK_ANIM(TILE_BREAK_ANIM_BRICK_UG)

#define TILE_BREAK_ANIM(e_tb_anim) e_tb_anim,
enum ETileBreakAnim : int32_t { TILE_BREAK_ANIMS TILE_BREAK_ANIM__COUNT, TILE_BREAK_ANIM__INVALID };
#undef TILE_BREAK_ANIM

#define TILE_BREAK_ANIM(e_tb_anim) #e_tb_anim,
inline const char *tile_break_anim_cstr[] = { TILE_BREAK_ANIMS };
#undef TILE_BREAK_ANIM

#define TILE_BREAK_ANIM(e_tb_anim) if(strcmp(cstr, #e_tb_anim) == 0) { return e_tb_anim; } else // That 'else' is scary
inline ETileBreakAnim tile_break_anim_from_cstr(const char *cstr) { TILE_BREAK_ANIMS; return TILE_BREAK_ANIM__INVALID; }
#undef TILE_BREAK_ANIM

// Tile flags
enum : uint32_t {
    TILE_FLAG_BLOCKS_MOVEMENT           = 1 << 0,
    TILE_FLAG_DO_ANIM_ON_HIT            = 1 << 1,
    TILE_FLAG_BREAKABLE                 = 1 << 2,
    TILE_FLAG_IS_INVISIBLE              = 1 << 3,
    TILE_FLAG_CHANGES_SPRITE_AFTER_DROP = 1 << 4,
    TILE_FLAG_BECOMES_VISIBLE_AFTER_HIT = 1 << 5,
    TILE_FLAG_NO_RENDER                 = 1 << 16
};

// Tile data that describes the tile, gets serialized
struct TileDesc {
    uint32_t    tile_flags;
    ETileDrop   tile_drop;
    int32_t     drops_left;
    bool        is_animated;
    ETileAnim   tile_anim;
    ETileSprite tile_sprite;
    ETileSprite tile_sprite_after_drop;
    ETileBreakAnim tile_break_anim; // if TILE_FLAG_BREAKABLE is set
};

struct Tilemap;
struct Tile {
    bool is_not_empty;
    Tilemap *tilemap;
    int32_t  tile_x; // Relative to the tilemap
    int32_t  tile_y; // Relative to the tilemap
    
    TileDesc   tile_desc;
    AnimPlayer anim_player;

    // Hit animation
    float32_t hit_anim_counter;
    float32_t hit_anim_perc;
    enum { 
        HIT_ANIM_NO, 
        HIT_ANIM_UP, 
        HIT_ANIM_DOWN 
    } hit_anim;

    TileInfo get_ti(void);
};

void clear_tile(Tile *tile);
void tile_hit(Tile *tile, struct Player *player);

ENTITY_SERIALIZE_PROC(Tilemap);
ENTITY_DESERIALIZE_PROC(Tilemap);

struct Tilemap : Entity {
    Tile *tiles;
    int32_t x_tiles;
    int32_t y_tiles;

    TileInfo get_position_ti(void) {
        return tile_info_at(this->position);
    }

    Tile *get_tile(int32_t x, int32_t y);
    Tile *set_tile(int32_t x, int32_t y, TileDesc tile_desc);

    inline bool tile_in_bounds(int32_t x, int32_t y) {
        return x >= 0 && y >= 0 && x < this->x_tiles && y < this->y_tiles;
    }
};

Tilemap *spawn_tilemap(Level *level, vec2i tile, int32_t x_tiles, int32_t y_tiles);
void render_tilemap_mesh(Tilemap *tilemap, int32_t z_pos, vec4 color);

struct FindTileCollisionOpts {
    uint32_t tile_flags_required;
    uint32_t tile_flags_forbidden;
};

bool is_colliding_with_any_tile(vec2i position, Collider *collider, Tilemap *tilemap, vec2i offset = { }, FindTileCollisionOpts opts = { });
std::vector<Tile *> find_collisions(vec2i position, Collider *collider, Tilemap *tilemap, vec2i offset = { }, FindTileCollisionOpts opts = { });

struct TileBreakAnim : Entity {
    float32_t  timer;
    AnimPlayer anim_player;
    vec2 speed_top;
    vec2 speed_bot;
    vec2 offset_top;
    vec2 offset_bot;
};

TileBreakAnim *spawn_tile_break_anim(Level *level, TileInfo ti, ETileBreakAnim tile_break_anim);

struct CoinDropAnim : Entity {
    float32_t  timer;
    float32_t  timer_perc;
    AnimPlayer anim_player;
};

CoinDropAnim *spawn_coin_drop_anim(Level *level, TileInfo ti);

#endif /* _TILEMAP_H */