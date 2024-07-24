#ifndef _LEVEL_ENTITIES_h
#define _LEVEL_ENTITIES_h

#include "common.h"
#include "entity.h"
#include "save_data.h"
#include "renderer.h"

/* --- KillRegion --- */

ENTITY_SERIALIZE_PROC(KillRegion);
ENTITY_DESERIALIZE_PROC(KillRegion);

struct KillRegion : Entity {
    Collider collider;
};

KillRegion *spawn_kill_region(Level *level, vec2i position, vec2i size);

/* --- FlagPole --- */

ENTITY_SERIALIZE_PROC(FlagPole);
ENTITY_DESERIALIZE_PROC(FlagPole);

struct FlagPole : Entity {
    Collider collider;
    int32_t height_in_tiles;

    float32_t flag_t;
    bool move_flag_down;
};

FlagPole *spawn_flag_pole(Level *level, vec2i position, int32_t height_in_tiles);
void      move_the_flag_down(FlagPole *flag_pole);

/* --- Coin --- */

ENTITY_SERIALIZE_PROC(Coin);
ENTITY_DESERIALIZE_PROC(Coin);

struct Coin : Entity {
    Collider collider;
    AnimPlayer anim_player;
};

Coin *spawn_coin(Level *level, vec2i position);
void collect_coin(Coin *coin);

/* --- Portal --- */

ENTITY_SERIALIZE_PROC(Portal);
ENTITY_DESERIALIZE_PROC(Portal);

struct PortalInfo {
    bool vertical_enter = true; // false -> horizontal
    bool vertical_exit  = true; // false -> horizontal
    bool do_exit_anim   = true;
    vec2i dest          = { 0, 0 };
};

struct Portal : Entity {
    PortalInfo info;
    TileInfo origin_tile;

    vec2i get_enter_pos(void);
    vec2i get_enter_size(void);
    vec2i get_size_for_editor(void);
};

Portal *spawn_portal(Level *level, vec2i position, PortalInfo info);

void render_connected_quads(vec2i pos_1, vec2i size_1, vec2i pos_2, vec2i size_2, int32_t z_pos, vec4 color, vec4 line_color);

/* --- Trigger --- */

ENTITY_SERIALIZE_PROC(Trigger);
ENTITY_DESERIALIZE_PROC(Trigger);

enum ETriggerType {
    TRIGGER_NOT_SET         = 0,
    TRIGGER_TEXT_VISIBILITY = 1,
    TRIGGER_TYPE__COUNT
};

inline const char *trigger_type_cstr[] = {
    "Trigger type not set",
    "Trigger text"
};

static_assert(TRIGGER_TYPE__COUNT == array_count(trigger_type_cstr));

struct Trigger : Entity {
    static constexpr int32_t trigger_key_size = 32;
    static constexpr char *trigger_key__invalid = "set the key";

    Collider     collider;
    ETriggerType trigger_type;
    char         trigger_key[trigger_key_size];
    bool         already_triggered;
};

Trigger *spawn_trigger(Level *level, vec2i position, vec2i size, ETriggerType trigger_type, char trigger_key[Trigger::trigger_key_size] = Trigger::trigger_key__invalid);
void trigger_the_trigger(Trigger *trigger);
void reset_trigger(Trigger *trigger);

/* --- TriggerableText --- */

ENTITY_SERIALIZE_PROC(TriggerableText);
ENTITY_DESERIALIZE_PROC(TriggerableText);

struct TriggerableText : Entity {
    static constexpr int32_t triggerable_text_max_length = 64;

    char trigger_key[Trigger::trigger_key_size];
    char text[triggerable_text_max_length];
    bool has_been_triggered;

    void trigger(void);
    void untrigger(void);
};

TriggerableText *spawn_triggerable_text(Level *level, vec2i position, char text[TriggerableText::triggerable_text_max_length], char trigger_key[Trigger::trigger_key_size] = Trigger::trigger_key__invalid);

/* --- CastleLever --- */

ENTITY_SERIALIZE_PROC(CastleLever);
ENTITY_DESERIALIZE_PROC(CastleLever);

struct CastleLever : Entity {
    Collider collider;
};

CastleLever *spawn_castle_lever(Level *level, vec2i position);

#endif /* _LEVEL_ENTITIES_h */