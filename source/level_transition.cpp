#include "level_transition.h"
#include "data.h"

namespace {
    constexpr float64_t level_transition_time = 2.5;

    int32_t   lt_world = 0;
    int32_t   lt_level = 0;
    float64_t lt_timer = 0.0;
}

void setup_level_transition(int32_t world, int32_t level) {
    lt_timer = 0.0;
    lt_world = world;
    lt_level = level;

    // No music during transition
    audio_player::a_stop_music();
}

void update_level_transition(float64_t delta_time) {
    lt_timer += delta_time;
}

void render_level_transition(int32_t width, int32_t height) {
    Font *font = global_data::get_small_font();

    render::r_quad({ 0, 0 }, 0, { width, height }, color::black);

    const float64_t perc = lt_timer / level_transition_time;
    if(perc <= 0.025 || perc >= 0.8 ) {
        return;
    }

    const vec2i center = vec2i{ width, height } / 2;
    const int32_t z_text = -1;

    int32_t y_text = center.y + 32;

    /* WORLD */ {
        char buffer[64];
        if(lt_world == 0 && lt_level == 0) {
            sprintf_s(buffer, array_count(buffer), "WORLD CUSTOM");
        } else {
            sprintf_s(buffer, array_count(buffer), "WORLD %d-%d", lt_world, lt_level);
        }
        const float32_t text_width = font->calc_string_width(buffer);
        const vec2i position = { center.x - (int32_t)roundf(text_width * 0.5f), y_text };
        render::r_text(position, z_text, buffer, font);
    }

    y_text = center.y;

    Sprite sprite_mario = global_data::get_sprite(SPRITE_MARIO_IDLE);
    render::r_sprite({ center.x - 24, y_text - 4 }, -1, { sprite_mario.width, sprite_mario.height }, sprite_mario, color::white);

    Sprite sprite_ui_x = global_data::get_sprite(SPRITE_UI_X);
    render::r_sprite({ center.x, y_text }, -1, { sprite_ui_x.width, sprite_ui_x.height }, sprite_ui_x, color::white);

    render::r_text({ center.x + 14, y_text }, z_text, "1", font);

}

bool level_transition_finished(void) {
    return lt_timer >= level_transition_time;
}