#include "background.h"
#include "all_entities.h"
#include "data.h"

/* ---  BackgroundPlane --- */

ENTITY_SERIALIZE_PROC(BackgroundPlane) {
    auto bg_plane = self_base->as<BackgroundPlane>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", bg_plane->position.e, 2);
    es_data.add_int32("size",     bg_plane->size.e,     2);
    es_data.add_float32("color",  bg_plane->color.e,    3);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(BackgroundPlane) {
    vec2i position;
    vec2i size;
    vec3  color;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_int32("size",     size.e,     2),
       !es_data->try_get_float32("color",  color.e,    3)) {
        return NULL;
    }

    return spawn_background_plane(level, position, size, color);
}

ENTITY_RENDER_PROC(render_background_plane);

BackgroundPlane *spawn_background_plane(Level *level, vec2i position, vec2i size, vec3 color) {
    auto bg_plane = create_entity_m(level, BackgroundPlane);
    bg_plane->render_proc = render_background_plane;
    bg_plane->position = position;

    bg_plane->size  = size;
    bg_plane->color = color;

    return bg_plane;
}

ENTITY_RENDER_PROC(render_background_plane) {
    auto bg_plane = self_base->as<BackgroundPlane>();
    render::r_quad(bg_plane->position, Z_BACKGROUND_POS, bg_plane->size, vec4::make(bg_plane->color, 1.0f));
}

/* ---  BackgroundSprite --- */

ENTITY_SERIALIZE_PROC(BackgroundSprite) {
    auto bg_sprite = self_base->as<BackgroundSprite>();

    EntitySaveData es_data = { };
    es_data.add_int32("position", bg_sprite->position.e, 2);
    es_data.add_cstring("sprite", sprite_string[bg_sprite->sprite_id]);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(BackgroundSprite) {
    vec2i       position;
    std::string sprite;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_string("sprite", &sprite,     1)) {
        return NULL;
    }

    int32_t sprite_id = sprite_id_from_string(sprite.c_str());
    if(sprite_id == SPRITE__INVALID) {
        return NULL;
    }

    return spawn_background_sprite(level, position, sprite_id);
}

ENTITY_RENDER_PROC(render_background_sprite);

BackgroundSprite *spawn_background_sprite(Level *level, vec2i position, int32_t sprite_id) {
    auto bg_sprite = create_entity_m(level, BackgroundSprite);
    bg_sprite->render_proc = render_background_sprite;
    bg_sprite->position = position;
    bg_sprite->sprite_id = sprite_id;
    return bg_sprite;
}

ENTITY_RENDER_PROC(render_background_sprite) {
    auto bg_sprite = self_base->as<BackgroundSprite>();
    auto sprite = global_data::get_sprite(bg_sprite->sprite_id);
    render::r_sprite(bg_sprite->position, Z_BACKGROUND_SPRITE_POS, { sprite.width, sprite.height }, sprite, color::white);
}

/* --- TimedSprite --- */

ENTITY_UPDATE_PROC(update_timed_sprite);
ENTITY_RENDER_PROC(render_timed_sprite);

TimedSprite *spawn_timed_sprite(Level *level, vec2i position, float32_t duration, Sprite sprite) {
    auto t_sprite = create_entity_m(level, TimedSprite);
    t_sprite->update_proc = update_timed_sprite;
    t_sprite->render_proc = render_timed_sprite;
    t_sprite->position = position;

    assert(sprite.texture != NULL);
    assert(duration > 0.0f);

    t_sprite->sprite     = sprite;
    t_sprite->z_position = 0;
    t_sprite->duration   = duration;
    t_sprite->timer      = 0.0f;

    return t_sprite;
}

ENTITY_UPDATE_PROC(update_timed_sprite) {
    auto t_sprite = self_base->as<TimedSprite>();
    t_sprite->timer += delta_time;
    if(t_sprite->timer >= t_sprite->duration) {
        delete_entity(t_sprite);
        return;
    }
}

ENTITY_RENDER_PROC(render_timed_sprite) {
    auto t_sprite = self_base->as<TimedSprite>();
    render::r_sprite(t_sprite->position, t_sprite->z_position, { t_sprite->sprite.width, t_sprite->sprite.height }, t_sprite->sprite, color::white);
}

/* --- TimedAnim --- */

ENTITY_UPDATE_PROC(update_timed_anim);
ENTITY_RENDER_PROC(render_timed_anim);

TimedAnim *spawn_timed_anim(Level *level, vec2i position, AnimSet *anim_set) {
    assert(anim_set != NULL && anim_set->frame_count > 1);

    auto t_anim = create_entity_m(level, TimedAnim);
    t_anim->update_proc = update_timed_anim;
    t_anim->render_proc = render_timed_anim;
    t_anim->position = position;
    t_anim->anim_player = init_anim_player(anim_set);

    // default
    t_anim->centered = false;
    t_anim->z_pos = 0;
    t_anim->type = TimedAnim::TYPE_LOOPS;
    t_anim->loops = 1;
    t_anim->loops_passed = 0;

    return t_anim;
}

TimedAnim *spawn_timed_anim_loops(Level *level, vec2i position, AnimSet *anim_set, int32_t loops) {
    auto t_anim = spawn_timed_anim(level, position, anim_set);
    t_anim->type = TimedAnim::TYPE_LOOPS;
    t_anim->loops = loops;
    t_anim->loops_passed = 0;
    return t_anim;
}

TimedAnim *spawn_timed_anim_timer(Level *level, vec2i position, AnimSet *anim_set, float32_t life_time) {
    auto t_anim = spawn_timed_anim(level, position, anim_set);
    t_anim->type = TimedAnim::TYPE_TIMER;
    t_anim->life_time = life_time;
    t_anim->timer = 0;
    return t_anim;
}

ENTITY_UPDATE_PROC(update_timed_anim) {
    auto t_anim = self_base->as<TimedAnim>();
    switch(t_anim->type) {
        case TimedAnim::TYPE_LOOPS: {
            int32_t last_frame_id = t_anim->anim_player.frame_id;
            update_anim(&t_anim->anim_player, delta_time);
            if(last_frame_id > 0 && t_anim->anim_player.frame_id == 0) { // looped
                t_anim->loops_passed += 1;
                if(t_anim->loops_passed >= t_anim->loops) {
                    delete_entity(t_anim);
                    return;
                }
            }
        } break;

        case TimedAnim::TYPE_TIMER: {
            update_anim(&t_anim->anim_player, delta_time);
            t_anim->timer += delta_time;
            if(t_anim->timer >= t_anim->life_time) {
                delete_entity(t_anim);
                return;
            }
        } break;
    }
}

ENTITY_RENDER_PROC(render_timed_anim) {
    auto t_anim = self_base->as<TimedAnim>();
    auto frame = get_current_frame(&t_anim->anim_player);

    vec2i position;
    if(t_anim->centered) {
        position = t_anim->position - frame->size() / 2;
    } else {
        position = t_anim->position;
    }
    render::r_sprite(position, t_anim->z_pos, frame->size(), frame->sprite, color::white);
}

/* --- EnemyFallAnim --- */

ENTITY_UPDATE_PROC(update_enemy_fall_anim);
ENTITY_RENDER_PROC(render_enemy_fall_anim);

constexpr float32_t enemy_fall_anim_time    = 3.0f;
constexpr float32_t enemy_fall_anim_x_speed = 50.0f;
constexpr float32_t enemy_fall_anim_y_speed = 120.0f;
constexpr float32_t enemy_fall_anim_gravity = 400.0f;

EnemyFallAnim *spawn_enemy_fall_anim(Level *level, vec2i position, int32_t dir, Sprite sprite) {
    auto fall_anim = create_entity_m(level, EnemyFallAnim);
    fall_anim->update_proc = update_enemy_fall_anim;
    fall_anim->render_proc = render_enemy_fall_anim;
    fall_anim->position = position;

    fall_anim->sprite = sprite;
    fall_anim->timer = 0.0f;

    fall_anim->speed_x = enemy_fall_anim_x_speed * sign(dir);
    fall_anim->speed_y = enemy_fall_anim_y_speed;
    fall_anim->offset_x = 0.0f;
    fall_anim->offset_y = 0.0f;

    return fall_anim;
}

ENTITY_UPDATE_PROC(update_enemy_fall_anim) {
    auto fall_anim = self_base->as<EnemyFallAnim>();
    fall_anim->timer += delta_time;

    fall_anim->speed_y -= enemy_fall_anim_gravity * delta_time;
    fall_anim->offset_x += fall_anim->speed_x * delta_time;
    fall_anim->offset_y += fall_anim->speed_y * delta_time;

    if(fall_anim->timer >= enemy_fall_anim_time) {
        delete_entity(fall_anim);
        return;
    }
}

ENTITY_RENDER_PROC(render_enemy_fall_anim) {
    auto fall_anim = self_base->as<EnemyFallAnim>();
    const vec2i position = { fall_anim->position.x + (int32_t)roundf(fall_anim->offset_x), fall_anim->position.y + (int32_t)roundf(fall_anim->offset_y) };

    render::r_set_flip_y_quads(1);
    render::r_sprite(position, 0, { fall_anim->sprite.width, fall_anim->sprite.height }, fall_anim->sprite, color::white);
}

/* --- FloatingText --- */

static constexpr float32_t f_text_duration = 4.0f;
static constexpr int32_t   f_text_distance = 64.0f; // 32;

ENTITY_UPDATE_PROC(update_floating_text);
ENTITY_RENDER_PROC(render_floating_text);

FloatingText *spawn_floating_text(Level *level, vec2i position, char text[FloatingText::text_max_length]) {
    auto f_text = create_entity_m(level, FloatingText);
    f_text->update_proc = update_floating_text;
    f_text->render_proc = render_floating_text;
    f_text->position = position;
    f_text->render_centered = false;

    strcpy_s(f_text->text, FloatingText::text_max_length * sizeof(char), text);

    const auto font = global_data::get_small_font();
    const int32_t max_loops = 6;
    int32_t loops = 0;
    do {
        bool collides = false;

        const vec2i f_text_size = { (int32_t)font->calc_string_width(f_text->text), font->height };
        for_entity_type(level, FloatingText, other) {
            if(other == f_text) continue;

            const vec2i other_size = { (int32_t)font->calc_string_width(other->text), font->height };
            if(aabb(f_text->position, f_text_size, other->position, other_size)) {
                collides = true;
                break;
            }
        }
        
        if(collides) {
            f_text->position.y -= global_data::get_small_font()->height;
        } else {
            break;
        }

        loops += 1;
    } while(loops < max_loops);

    return f_text;
}

FloatingText *spawn_floating_text_number(Level *level, vec2i position, int32_t number) {
    char text[FloatingText::text_max_length];
    sprintf_s(text, array_count(text), "%d", number);
    return spawn_floating_text(level, position, text);
}

ENTITY_UPDATE_PROC(update_floating_text) {
    auto f_text = self_base->as<FloatingText>();
    
    if((f_text->timer += delta_time) >= f_text_duration) {
        delete_entity(f_text);
        return;
    }
}

ENTITY_RENDER_PROC(render_floating_text) {
    auto f_text = self_base->as<FloatingText>();
    auto font = global_data::get_small_font();

    const float32_t perc = f_text->timer / f_text_duration;
    const vec2i position = {
        f_text->position.x - (f_text->render_centered ? (int32_t)roundf(font->calc_string_width(f_text->text)) / 2 : 0),
        f_text->position.y + (int32_t)roundf(perc * f_text_distance)
    };

    render::r_text(position, Z_FLOATING_TEXT_POS, f_text->text, font);
}

/* --- Image --- */

ENTITY_SERIALIZE_PROC(BackgroundImage) {
    auto bg_image = self_base->as<BackgroundImage>();

    EntitySaveData es_data = { };
    es_data.add_int32("position",   bg_image->position.e, 2);
    es_data.add_cstring("image_id", image_string[bg_image->image_id]);
    es_data.add_float32("opacity",  &bg_image->opacity, 1);
    return es_data;
}

ENTITY_DESERIALIZE_PROC(BackgroundImage) {
    vec2i       position;
    std::string image;

    if(!es_data->try_get_int32("position", position.e, 2),
       !es_data->try_get_string("image_id",  &image,   1)) {
        return NULL;
    }

    int32_t image_id = image_id_from_string(image.c_str());
    if(image_id == SPRITE__INVALID) {
        return NULL;
    }

    auto bg_image = spawn_background_image(level, position, image_id);
    es_data->try_get_float32("opacity", &bg_image->opacity, 1);
    return bg_image;
}

ENTITY_RENDER_PROC(render_background_image);

BackgroundImage *spawn_background_image(Level *level, vec2i position, int32_t image_id) {
    auto bg_image = create_entity_m(level, BackgroundImage);
    bg_image->render_proc = render_background_image;
    bg_image->position = position;
    bg_image->image_id = image_id;
    bg_image->opacity = 1.0f;
    return bg_image;
}

ENTITY_RENDER_PROC(render_background_image) {
    auto bg_image = self_base->as<BackgroundImage>();
    auto image = global_data::get_image(bg_image->image_id);
    render::r_texture(bg_image->position, Z_BACKGROUND_IMAGE_POS, image->size(), image, { 1.0f, 1.0f, 1.0f, bg_image->opacity });
}