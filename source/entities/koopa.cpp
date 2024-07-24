#include "koopa.h"
#include "all_entities.h"
#include "data.h"

namespace {
    constexpr float32_t koopa_speed   = 40.0f;
    constexpr float32_t koopa_gravity = 120.0f;
    constexpr float32_t koopa_jump_power = 150.0f;

    AnimSet koopa_anim_sets[KOOPA_TYPE__COUNT];
    AnimSet shell_anim_sets[KOOPA_TYPE__COUNT];
}

void init_koopa_common_data(void) {
    AnimSet *set = NULL;

    set = &koopa_anim_sets[KOOPA_TYPE_GREEN];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_2), 0.4f);

    set = &koopa_anim_sets[KOOPA_TYPE_RED];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_2), 0.4f);

    set = &koopa_anim_sets[KOOPA_TYPE_GREEN_FLYING];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_FLYING_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_FLYING_2), 0.4f);

    set = &koopa_anim_sets[KOOPA_TYPE_RED_FLYING];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_FLYING_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_FLYING_2), 0.4f);

    set = &shell_anim_sets[KOOPA_TYPE_GREEN];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_SHELL_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_SHELL_2), 0.4f);

    set = &shell_anim_sets[KOOPA_TYPE_RED];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_SHELL_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_SHELL_2), 0.4f);

    set = &shell_anim_sets[KOOPA_TYPE_GREEN_FLYING];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_SHELL_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_KOOPA_SHELL_2), 0.4f);

    set = &shell_anim_sets[KOOPA_TYPE_RED_FLYING];
    init_anim_set(set);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_SHELL_1), 0.4f);
    next_anim_frame(set, global_data::get_sprite(SPRITE_RED_KOOPA_SHELL_2), 0.4f);
}

ENTITY_SERIALIZE_PROC(Koopa) {
    auto koopa = self_base->as<Koopa>();
    int32_t koopa_type = (int32_t)koopa->koopa_type;

    EntitySaveData es_data = { };
    es_data.add_int32("position", koopa->position.e, 2);
    es_data.add_int32("move_dir", &koopa->move_dir, 1);
    es_data.add_int32("koopa_type", &koopa_type, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(Koopa){
    vec2i   position;
    int32_t move_dir;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_int32("move_dir", &move_dir,  1)) {
        return NULL;
    }

    int32_t koopa_type;
    if(!es_data->try_get_int32("koopa_type", &koopa_type, 1)) {
        koopa_type = KOOPA_TYPE_GREEN;
    }

    auto koopa = spawn_koopa(level, position, (EKoopaType)koopa_type);
    koopa->move_dir = move_dir;
    return koopa;
}

ENTITY_UPDATE_PROC(update_koopa);
ENTITY_RENDER_PROC(render_koopa);
HIT_CALLBACK_PROC(koopa_hit_callback);

Koopa *spawn_koopa(Level *level, vec2i position, EKoopaType koopa_type) {
    auto koopa = create_entity_m(level, Koopa);
    koopa->update_proc = update_koopa;
    koopa->render_proc = render_koopa;
    koopa->position = position;

    koopa->koopa_type = koopa_type;

    set_entity_flag(koopa, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(koopa, E_FLAG_DOES_DAMAGE_TO_PLAYER);
    set_entity_flag(koopa, E_FLAG_STOMPABLE);

    auto collider = &koopa->collider;
    collider->size   = { 12, 12 };
    collider->offset = { 2,  1 };
    koopa->has_collider = collider;

    auto move_data = &koopa->move_data;
    move_data->hit_callback = koopa_hit_callback;
    koopa->has_move_data = move_data;

    koopa->move_dir = 1;
    koopa->anim_player = init_anim_player(&koopa_anim_sets[koopa->koopa_type]);

    make_asleep(koopa);

    return koopa;
}

ENTITY_UPDATE_PROC(update_koopa) {
    auto koopa = self_base->as<Koopa>();

    if(koopa->change_move_dir_next_frame) {
        koopa->move_dir *= -1;
        koopa->change_move_dir_next_frame = false;
    }

    if(koopa->bounce_next_frame) {
        koopa->move_data.speed.y = koopa_jump_power;
        koopa->bounce_next_frame = false;
    }

    koopa->move_data.speed.x = koopa_speed * koopa->move_dir;
    koopa->move_data.speed.y -= koopa_gravity * delta_time;
    do_move(koopa, &koopa->move_data, delta_time);

    update_anim(&koopa->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_koopa) {
    auto koopa = self_base->as<Koopa>();
    auto frame = get_current_frame(&koopa->anim_player);
    
    render::r_set_flip_x_quads((int32_t)(koopa->move_dir < 0));
    render::r_sprite(koopa->position, Z_KOOPA_POS, frame->size(), frame->sprite, color::white);
}

HIT_CALLBACK_PROC(koopa_hit_callback) {
    auto koopa = self_base->as<Koopa>();

    if(((collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT) || (collision.with_entity != NULL && collision.with_entity->entity_type_id == entity_type_id(Koopa))) && collision.axis == X_AXIS) {
        koopa->change_move_dir_next_frame = true;
        return hit_callback_result(true);
    }

    if((koopa->koopa_type == KOOPA_TYPE_RED_FLYING || koopa->koopa_type == KOOPA_TYPE_GREEN_FLYING) && collision.with_tile != NULL && collision.axis == Y_AXIS && collision.unit == -1 && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT) {
        koopa->bounce_next_frame = true;
        return hit_callback_result(true);
    }

    return hit_callback_result(false);
}

void kill_koopa(Koopa *koopa, int32_t fall_dir) {
    add_points_with_text_above_entity(koopa->level, POINTS_FOR_KOOPA, koopa);

    AnimSet *shell_anim_set = &shell_anim_sets[koopa->koopa_type];
    assert(shell_anim_set->frame_count >= 2);

    spawn_enemy_fall_anim(koopa->level, koopa->position, fall_dir, shell_anim_set->frames[1].sprite);

    audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));

    delete_entity(koopa);
}

void hit_koopa(Koopa *koopa) {
    audio_player::a_play_sound(global_data::get_sound(SOUND_STOMP));
    add_points_with_text_above_entity(koopa->level, POINTS_FOR_KOOPA_STOMP, koopa);
    spawn_koopa_shell(koopa, koopa->koopa_type);
    delete_entity(koopa);
}

ENTITY_UPDATE_PROC(update_koopa_shell);
ENTITY_RENDER_PROC(render_koopa_shell);
HIT_CALLBACK_PROC(shell_hit_callback);

const float32_t shell_speed   = 240.0f;
const float32_t shell_gravity = 200.0f;

KoopaShell *spawn_koopa_shell(Koopa *koopa, EKoopaType koopa_type) {
    auto level = koopa->level;

    auto shell = create_entity_m(level, KoopaShell);
    shell->update_proc = update_koopa_shell;
    shell->render_proc = render_koopa_shell;
    shell->position = koopa->position;
    shell->koopa_type = koopa_type;

    set_entity_flag(shell, E_FLAG_STOMPABLE);
    set_entity_flag(shell, E_FLAG_IS_GAMEPLAY_ENTITY);
    set_entity_flag(shell, E_FLAG_DOES_DAMAGE_TO_PLAYER);

    auto collider = &shell->collider;
    collider->size = { 12, 12 };
    collider->offset = { 2, 1 };
    shell->has_collider = collider;

    auto move_data = &shell->move_data;
    move_data->hit_callback = shell_hit_callback;
    shell->has_move_data = move_data;

    shell->move_dir = 0;
    shell->anim_player = init_anim_player(&shell_anim_sets[koopa_type]);

    return shell;
}

ENTITY_UPDATE_PROC(update_koopa_shell) {
    auto shell = self_base->as<KoopaShell>();

    if(shell->change_move_dir_next_frame) {
        shell->move_dir *= -1;
        shell->change_move_dir_next_frame = false;
    }

    shell->move_data.speed.x = shell_speed * shell->move_dir;
    shell->move_data.speed.y -= shell_gravity * delta_time;
    do_move(shell, &shell->move_data, delta_time);

    update_anim(&shell->anim_player, delta_time);
}

ENTITY_RENDER_PROC(render_koopa_shell) {
    auto shell = self_base->as<KoopaShell>();

    if(shell->move_dir != 0) {
        shell->anim_player.timer = 0.0f; // @move
        auto sprite = shell->anim_player.set->frames[1].sprite;
        render::r_sprite(shell->position, Z_KOOPA_POS, sprite.size(), sprite, color::white);
    } else {
        auto frame = get_current_frame(&shell->anim_player);
        render::r_sprite(shell->position, Z_KOOPA_POS, frame->size(), frame->sprite, color::white);
    }
}

HIT_CALLBACK_PROC(shell_hit_callback) {
    auto shell = self_base->as<KoopaShell>();

    if(collision.axis == X_AXIS && collision.with_tile != NULL && collision.with_tile->tile_desc.tile_flags & TILE_FLAG_BLOCKS_MOVEMENT) {
        shell->change_move_dir_next_frame = true;
        return hit_callback_result(true);
    }

    if(collision.axis == X_AXIS && collision.with_entity != NULL) {
        switch(collision.with_entity->entity_type_id) {
            case entity_type_id(Goomba): {
                kill_goomba(collision.with_entity->as<Goomba>(), false, collision.unit); 
                return hit_callback_result(false);
            }

            case entity_type_id(Koopa): {
                kill_koopa(collision.with_entity->as<Koopa>(), collision.unit); 
                return hit_callback_result(false);
            }

            case entity_type_id(KoopaShell): {
                kill_koopa_shell(collision.with_entity->as<KoopaShell>(), collision.unit);
                kill_koopa_shell(shell, -collision.unit);
                return hit_callback_result(false);
            }
        }
    }

    return hit_callback_result(false);
}

void kill_koopa_shell(KoopaShell *shell, int32_t fall_dir) {
    add_points_with_text_above_entity(shell->level, POINTS_FOR_KOOPA_SHELL, shell);

    Sprite *sprite = NULL;
    switch(shell->koopa_type) {
        case KOOPA_TYPE_RED:
        case KOOPA_TYPE_RED_FLYING: {
            spawn_enemy_fall_anim(shell->level, shell->position, fall_dir, global_data::get_sprite(SPRITE_RED_KOOPA_SHELL_2));
        } break;

        case KOOPA_TYPE_GREEN:
        case KOOPA_TYPE_GREEN_FLYING: {
            spawn_enemy_fall_anim(shell->level, shell->position, fall_dir, global_data::get_sprite(SPRITE_KOOPA_SHELL_2));
        } break;
        
        default: { } break;
    }

    audio_player::a_play_sound(global_data::get_sound(SOUND_KILL_ENEMY));

    delete_entity(shell);
}

void push_koopa_shell(KoopaShell *shell, int32_t dir) {
    add_points_with_text_above_entity(shell->level, POINTS_FOR_KOOPA_SHELL_PUSH, shell);

    assert(dir == 1 || dir == -1 || dir == 0);
    shell->move_dir = dir;

    audio_player::a_play_sound(global_data::get_sound(SOUND_STOMP));
}