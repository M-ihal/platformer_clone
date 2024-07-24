#include "level_entities.h"
#include "all_entities.h"
#include "data.h"

/* --- KillRegion --- */

ENTITY_SERIALIZE_PROC(KillRegion) {
    EntitySaveData es_data;
    es_data.add_int32("position", self_base->position.e, 2);
    es_data.add_int32("size",     self_base->has_collider->size.e, 2);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(KillRegion) {
    vec2i position;
    vec2i size;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_int32("size",     size.e,     2)) {
        return NULL;
    }

    return spawn_kill_region(level, position, size);
}

ENTITY_UPDATE_PROC(update_kill_region);

KillRegion *spawn_kill_region(Level *level, vec2i position, vec2i size) {
    auto kill_region = create_entity_m(level, KillRegion);
    kill_region->update_proc = update_kill_region;
    kill_region->position = position;

    auto collider = &kill_region->collider;
    collider->size   = size;
    collider->offset = { 0, 0 };
    kill_region->has_collider = collider;

    set_entity_flag(kill_region, E_FLAG_IS_GAMEPLAY_ENTITY);

    return kill_region;
}

ENTITY_UPDATE_PROC(update_kill_region) {
    auto kill_region = self_base->as<KillRegion>();
    auto level = kill_region->level;
 
#define maybe_delete_entity_if_collides(Type) for_entity_type(level, Type, _entity) { if(intersect(kill_region, _entity)) { delete_entity(_entity); continue; } }

    // Just delete entities that collide with kill rect
    // @todo : could do it differently
    // maybe_delete_entity_if_collides(Fireball);
    maybe_delete_entity_if_collides(Goomba);
    maybe_delete_entity_if_collides(Koopa);
    maybe_delete_entity_if_collides(KoopaShell);
    maybe_delete_entity_if_collides(Mushroom);
    maybe_delete_entity_if_collides(Star);
}

/* --- FlagPole --- */

ENTITY_SERIALIZE_PROC(FlagPole) {
    auto flag_pole = self_base->as<FlagPole>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", flag_pole->position.e,      2);
    es_data.add_int32("height",  &flag_pole->height_in_tiles, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(FlagPole) {
    vec2i   position;
    int32_t height;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_int32("height",   &height,    1)) {
        return NULL;
    }

    return spawn_flag_pole(level, position, height);
}

ENTITY_UPDATE_PROC(update_flag_pole);
ENTITY_RENDER_PROC(render_flag_pole);

FlagPole *spawn_flag_pole(Level *level, vec2i position, int32_t height_in_tiles) {
    if(height_in_tiles <= 1) { // Min. 2
        return NULL;
    }

    auto flag_pole = create_entity_m(level, FlagPole);
    flag_pole->position = position;
    flag_pole->update_proc = update_flag_pole;
    flag_pole->render_proc = render_flag_pole;
    flag_pole->height_in_tiles = height_in_tiles;
    flag_pole->move_flag_down = false;

    auto collider = &flag_pole->collider;
    collider->offset = { 4, 0 };
    collider->size   = { 8, height_in_tiles * TILE_SIZE };
    flag_pole->has_collider = collider;

    return flag_pole;
}

void move_the_flag_down(FlagPole *flag_pole) {
    assert_entity_type(flag_pole, FlagPole);
    flag_pole->move_flag_down = true;
    flag_pole->flag_t = 0.0f;
}

ENTITY_UPDATE_PROC(update_flag_pole) {
    auto flag_pole = self_base->as<FlagPole>();

    if(flag_pole->move_flag_down && flag_pole->flag_t < 1.0f) {
        flag_pole->flag_t += (delta_time * player_consts::sliding_flag_pole_speed) / (flag_pole->height_in_tiles * TILE_SIZE); // @meh
        clamp_max(&flag_pole->flag_t, 1.0f);
    }
}

ENTITY_RENDER_PROC(render_flag_pole) {
    auto flag_pole = self_base->as<FlagPole>();
    
    // @temp
    auto sprite_pole      = global_data::get_sprite(SPRITE_FLAG_POLE);
    auto sprite_pole_top  = global_data::get_sprite(SPRITE_FLAG_POLE_TOP);
    auto sprite_pole_flag = global_data::get_sprite(SPRITE_FLAG_POLE_FLAG);

    /* Flag pole */
    for(int32_t idx = 0; idx < flag_pole->height_in_tiles; ++idx) {
        const vec2i position = { flag_pole->position.x, flag_pole->position.y + idx * TILE_SIZE };
        render::r_sprite(position, Z_FLAG_POLE_POS, TILE_SIZE_2, sprite_pole, color::white);
    }

    /* Flag pole top */ {
        const vec2i position = { flag_pole->position.x, flag_pole->position.y + flag_pole->height_in_tiles * TILE_SIZE };
        render::r_sprite(position, Z_FLAG_POLE_POS, TILE_SIZE_2, sprite_pole_top, color::white);
    }

    /* The flag */ {
        const int32_t y_distance = flag_pole->height_in_tiles * TILE_SIZE - TILE_SIZE;
        const vec2i position = { 
            flag_pole->position.x - sprite_pole_flag.width / 2 - 1, 
            flag_pole->position.y + (int32_t)floorf((1.0f - flag_pole->flag_t) * y_distance)
        };
        render::r_sprite(position, Z_FLAG_POLE_POS, { sprite_pole_flag.width, sprite_pole_flag.height }, sprite_pole_flag, color::white);
    }
}

/* --- Coin --- */

namespace {
    AnimSet anim_set_coin;
}

ENTITY_SERIALIZE_PROC(Coin) {
    EntitySaveData es_data = { };
    es_data.add_int32("position", self_base->position.e, 2);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Coin) {
    vec2i position;

    if(!es_data->try_get_int32("position", position.e, 2)) {
        return NULL;
    }

    return spawn_coin(level, position);
}

ENTITY_UPDATE_PROC(update_coin);
ENTITY_RENDER_PROC(render_coin);

Coin *spawn_coin(Level *level, vec2i position) {
    auto coin = create_entity_m(level, Coin);
    coin->update_proc = update_coin;
    coin->render_proc = render_coin;
    coin->position = position;
    coin->entity_flags = E_FLAG_IS_GAMEPLAY_ENTITY;

    auto collider = &coin->collider;
    collider->offset = { 3, 3 };
    collider->size = { 10, 10 };
    coin->has_collider = collider;

    coin->anim_player = init_anim_player(&anim_set_coin);

    return coin;
}

ENTITY_UPDATE_PROC(update_coin) {
    auto coin = self_base->as<Coin>();
    update_anim(&coin->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_coin) {
    auto coin = self_base->as<Coin>();
    auto frame = get_current_frame(&coin->anim_player);
    render::r_sprite(coin->position, Z_COIN_POS, frame->size(), frame->sprite, color::white);
}

void collect_coin(Coin *coin) {
    add_coins(coin->level);
    delete_entity(coin);
    return;
}

/* --- Portal --- */

ENTITY_SERIALIZE_PROC(Portal) {
    auto portal = self_base->as<Portal>();

    EntitySaveData es_data = { };
    es_data.add_int32("position",  portal->position.e,  2);
    es_data.add_int32("info.dest", portal->info.dest.e, 2);
    es_data.add_bool("info.vertical_enter", &portal->info.vertical_enter, 1);
    es_data.add_bool("info.vertical_exit",  &portal->info.vertical_exit,  1);
    es_data.add_bool("info.do_exit_anim",   &portal->info.do_exit_anim,   1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Portal) {
    int32_t position[2];
    int32_t dest    [2];
    bool vertical_enter;
    bool vertical_exit;
    bool do_exit_anim;

    if(!es_data->try_get_int32("position", position, 2) || 
       !es_data->try_get_int32("info.dest", dest,    2) ||
       !es_data->try_get_bool("info.vertical_enter", &vertical_enter) ||
       !es_data->try_get_bool("info.vertical_exit",  &vertical_exit) ||
       !es_data->try_get_bool("info.do_exit_anim",   &do_exit_anim)) {
        return NULL;
    }

    PortalInfo info;
    info.dest = vec2i::make(dest);
    info.vertical_enter = vertical_enter;
    info.vertical_exit  = vertical_exit;
    info.do_exit_anim   = do_exit_anim;

    return spawn_portal(level, vec2i::make(position), info);
}

Portal *spawn_portal(Level *level, vec2i position, PortalInfo info) {
    auto portal = create_entity_m(level, Portal);
    portal->position = position;
    portal->info = info;
    portal->origin_tile = tile_info_at(portal->position);
    return portal;
}

vec2i Portal::get_enter_pos(void) {
    if(this->info.vertical_enter) {
        return { 
            this->origin_tile.position.x + TILE_SIZE - 1, 
            this->origin_tile.position.y + TILE_SIZE 
        };
    } else {
        return this->origin_tile.position - vec2i{ 1, 0 };
    }
}

vec2i Portal::get_enter_size(void) {
    return this->info.vertical_enter ? vec2i{ 2, 1 } : vec2i{ 1, 4 };
}

vec2i Portal::get_size_for_editor(void) {
    return this->info.vertical_enter ? vec2i{ TILE_SIZE * 2, TILE_SIZE } : vec2i{ TILE_SIZE, TILE_SIZE * 2 };
}

void init_level_entities_common_data(void) {
    anim_set_coin = init_anim_set();
    next_anim_frame(&anim_set_coin, global_data::get_sprite(SPRITE_COIN_ENTITY_1), 0.333f);
    next_anim_frame(&anim_set_coin, global_data::get_sprite(SPRITE_COIN_ENTITY_2), 0.333f);
    next_anim_frame(&anim_set_coin, global_data::get_sprite(SPRITE_COIN_ENTITY_3), 0.333f);
}

void render_connected_quads(vec2i pos_1, vec2i size_1, vec2i pos_2, vec2i size_2, int32_t z_pos, vec4 color, vec4 line_color) {
    vec2i corners_1[] = {
        pos_1,
        pos_1 + vec2i{ size_1.x, 0 },
        pos_1 + vec2i{ size_1.x, size_1.y },
        pos_1 + vec2i{ 0,        size_1.y },
    };

    vec2i corners_2[] = {
        pos_2,
        pos_2 + vec2i{ size_2.x, 0 },
        pos_2 + vec2i{ size_2.x, size_2.y },
        pos_2 + vec2i{ 0,        size_2.y },
    };

    float32_t best = FLT_MAX;
    vec2i corner_1 = corners_1[0];
    vec2i corner_2 = corners_2[0];

    for(int32_t idx_1 = 0; idx_1 < 4; ++idx_1) {
        for(int32_t idx_2 = 0; idx_2 < 4; ++idx_2) {
            vec2 vector = vec2::vector(corners_1[idx_1].to_vec2(), corners_2[idx_2].to_vec2());
            float32_t dist = vec2::length(vector);
            if(dist < best) {
                best = dist;
                corner_1 = corners_1[idx_1];
                corner_2 = corners_2[idx_2];
            }
        }
    }

    render::r_line_quad(pos_1, z_pos, size_1, color);
    render::r_line_quad(pos_2, z_pos, size_2, color);
    render::r_line(corner_1, corner_2, z_pos, line_color);
}

/* --- Trigger --- */

ENTITY_SERIALIZE_PROC(Trigger) {
    auto trigger = self_base->as<Trigger>();
    auto trigger_type = (int32_t)trigger->trigger_type;

    EntitySaveData es_data = { };
    es_data.add_int32("position",  trigger->position.e,  2);
    es_data.add_int32("size", trigger->collider.size.e,  2);
    es_data.add_int32("trigger_type", &trigger_type, 1);
    es_data.add_cstring("trigger_key", trigger->trigger_key);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Trigger) {
    int32_t position[2];
    int32_t size    [2];
    int32_t trigger_type;
    char    trigger_key[Trigger::trigger_key_size];

    if(!es_data->try_get_int32("position", position, 2) || 
       !es_data->try_get_int32("size", size, 2) ||
       !es_data->try_get_int32("trigger_type", &trigger_type, 1) ||
       !es_data->try_get_cstring("trigger_key", trigger_key, Trigger::trigger_key_size)) {
        return NULL;
    }

    return spawn_trigger(level, vec2i::make(position), vec2i::make(size), (ETriggerType)trigger_type, trigger_key);
}

Trigger *spawn_trigger(Level *level, vec2i position, vec2i size, ETriggerType trigger_type, char trigger_key[Trigger::trigger_key_size]) {
    auto trigger = create_entity_m(level, Trigger);
    trigger->position = position;
    trigger->trigger_type = trigger_type;
    trigger->already_triggered = false;

    auto collider = &trigger->collider;
    collider->size = size;
    collider->offset = { };
    trigger->has_collider = collider;

    strcpy_s(trigger->trigger_key, Trigger::trigger_key_size, trigger_key);

    return trigger;
}

void trigger_the_trigger(Trigger *trigger) {
    auto level = trigger->level;

    if(strcmp(trigger->trigger_key, Trigger::trigger_key__invalid) == 0) {
        // trigger key not set
        return;
    }

    if(trigger->already_triggered) {
        return;
    }

    switch(trigger->trigger_type) {
        case TRIGGER_NOT_SET:
        default: {
            // Do nothing
        } break;

        case TRIGGER_TEXT_VISIBILITY: {
            for_entity_type(level, TriggerableText, tt) {
                assert(is_entity_used(tt));
                if(strcmp(trigger->trigger_key, tt->trigger_key) == 0) {
                    tt->trigger();
                }
            }
        } break;
    }

    trigger->already_triggered = true;
}

void reset_trigger(Trigger *trigger) {
    auto level = trigger->level;

    switch(trigger->trigger_type) {
        case TRIGGER_NOT_SET:
        default: {
            // Do nothing
        } break;

        case TRIGGER_TEXT_VISIBILITY: {
            for_entity_type(level, TriggerableText, tt) {
                assert(is_entity_used(tt));
                if(strcmp(trigger->trigger_key, tt->trigger_key) == 0) {
                    tt->untrigger();
                }
            }
        } break;
    }

    trigger->already_triggered = false;
}

/* --- TriggerableText --- */

ENTITY_SERIALIZE_PROC(TriggerableText) {
    auto tt = self_base->as<TriggerableText>();

    EntitySaveData es_data = { };
    es_data.add_int32("position",  tt->position.e,  2);
    es_data.add_cstring("tt_text", tt->text);
    es_data.add_cstring("trigger_key", tt->trigger_key);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(TriggerableText) {
    int32_t position   [2];
    char    text       [TriggerableText::triggerable_text_max_length];
    char    trigger_key[Trigger::trigger_key_size];

    if(!es_data->try_get_int32("position", position, 2) || 
       !es_data->try_get_cstring("tt_text", text, TriggerableText::triggerable_text_max_length) ||
       !es_data->try_get_cstring("trigger_key", trigger_key, Trigger::trigger_key_size)) {
        return NULL;
    }

    return spawn_triggerable_text(level, vec2i::make(position), text, trigger_key);
}

ENTITY_RENDER_PROC(render_triggerable_text);

TriggerableText *spawn_triggerable_text(Level *level, vec2i position, char text[TriggerableText::triggerable_text_max_length], char trigger_key[Trigger::trigger_key_size]) {
    auto tt = create_entity_m(level, TriggerableText);
    tt->render_proc = render_triggerable_text;
    tt->position = position;
    
    strcpy_s(tt->trigger_key, Trigger::trigger_key_size, trigger_key);
    strcpy_s(tt->text, TriggerableText::triggerable_text_max_length, text);

    return tt;
}

ENTITY_RENDER_PROC(render_triggerable_text) {
    auto tt = self_base->as<TriggerableText>();
    if(tt->has_been_triggered == false) {
        return;
    }
    render::r_text(tt->position, -5, tt->text, global_data::get_small_font());
}

void TriggerableText::trigger(void) {
    this->has_been_triggered = true;
}

void TriggerableText::untrigger(void) {
    this->has_been_triggered = false;
}

/* --- CastleLever --- */

ENTITY_SERIALIZE_PROC(CastleLever) {
    EntitySaveData es_data = { };
    es_data.add_int32("position", self_base->position.e, 2);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(CastleLever) {
    vec2i position;
    if(!es_data->try_get_int32("position", position.e, 2)) {
        return NULL;
    }

    return spawn_castle_lever(level, position);
}

ENTITY_RENDER_PROC(render_castle_lever);

CastleLever *spawn_castle_lever(Level *level, vec2i position) {
    auto lever = create_entity_m(level, CastleLever);
    lever->position = position;
    lever->render_proc = render_castle_lever;

    auto collider = &lever->collider;
    collider->offset = { 1, 1 };
    collider->size   = TILE_SIZE_2 - collider->offset * 2;
    lever->has_collider = collider;

    return lever;
}

ENTITY_RENDER_PROC(render_castle_lever) {
    auto lever = self_base->as<CastleLever>();
    auto sprite = global_data::get_sprite(SPRITE_CASTLE_LEVER);
    render::r_sprite(lever->position, Z_CASTLE_LEVER_POS, sprite.size(), sprite, color::white);
}