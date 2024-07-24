#ifndef _RENDERER_H
#define _RENDERER_H

#include "common.h"
#include "maths.h"
#include "opengl_abs.h"

#include <unordered_map>

// #define R_TEXT_DEBUG_DRAW
#define R_DEF_FONT_HEIGHT     18
#define R_DEF_FONT_BIG_HEIGHT 64

struct RenderStats {
    uint32_t draw_calls;
    uint32_t quads;
    uint32_t lines;
    uint32_t text_quads;
};

struct RenderSetup {
    mat4x4 proj_m;
    mat4x4 view_m;
    recti32 viewport;
    Framebuffer *framebuffer; // Needs to stay valid until render::r_end()
};

struct Sprite;
struct Font;

namespace render {
    void r_init(void);
    void r_quit(void);
    void r_begin(RenderSetup setup);
    void r_end(void);
    void r_scissor_begin(recti32 rect); // Call after  render::r_begin
    void r_scissor_end(void);           // Call before render::r_end
    void r_flush(void);                 // Quads are rendered first, so sometimes flush for correct blending

    void get_current_setup(RenderSetup *out_setup);

    void r_set_line_width(float32_t width);    // If called after r_begin and before r_end -> flushes
    void r_set_flip_x_quads(int32_t num = 1);
    void r_set_flip_y_quads(int32_t num = 1);
    RenderStats r_reset_stats(void);
    
    /* --- Quads --- */
    void r_quad(vec2i position, int32_t z_pos, vec2i size, vec4 color);
    void r_quad_outline(vec2i position, int32_t z_pos, vec2i size, int32_t width, vec4 color);
    void r_texture(vec2i position, int32_t z_pos, vec2i size, Texture *texture, vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
    void r_sprite(vec2i position, int32_t z_pos, vec2i size, Sprite sprite, vec4 color);

    // Perc is ant offset <0.0, 1.0>
    void r_quad_marching_ants(vec2i pos, int32_t z_pos, vec2i size, int32_t ant_length, vec4 color, float32_t perc, bool backwards = false);

    /* --- Lines --- */
    void r_line(vec2i point_a, vec2i point_b, int32_t z_pos, vec4 color);
    void r_line_quad(vec2i position, int32_t z_pos, vec2i size, vec4 color);

    /* --- Text --- */
    void r_text(vec2i position, int32_t z_pos, const char *text, Font *font, vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
    void r_text_formatted(vec2i position, int32_t z_pos, Font *font, vec4 color, const char *text, ...);
    void r_text_scaled(vec2i position, int32_t z_pos, const char *string, Font *font, float32_t scale, vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }); // p.bad

    /* Default font stuff */
    Font *def_font(void);
    Font *def_font_big(void);
};

struct Sprite {
    Texture *texture; // The texture needs to stay valid when using defined sprites
    int32_t width;
    int32_t height;
    vec2 tex_coords[4];

    inline vec2i size(void) {
        return { this->width, this->height };
    }
};

Sprite define_sprite(Texture *texture, int32_t cell_w, int32_t cell_h, int32_t cell_x, int32_t cell_y, int32_t sprite_w, int32_t sprite_h);

struct AnimSet {
    struct Frame {
        Sprite    sprite;
        float32_t duration;

        vec2i size(void) { 
            return { this->sprite.width, this->sprite.height }; 
        }
    };
    
    static constexpr int32_t max_frames = 32;

    Frame   frames[max_frames];
    int32_t frame_count;
};

AnimSet init_anim_set(AnimSet *set = NULL);
bool next_anim_frame(AnimSet *set, Sprite sprite, float32_t duration);

struct AnimPlayer {
    AnimSet  *set;
    int32_t   frame_id;
    float32_t timer;
};

AnimPlayer init_anim_player(AnimSet *set = NULL);
void set_anim(AnimPlayer *anim, AnimSet *set);
void advance_anim_frame(AnimPlayer *anim);
AnimSet::Frame *get_current_frame(AnimPlayer *anim);
void update_anim(AnimPlayer *anim, float32_t delta_time);
void reset_anim_player(AnimPlayer *anim);

struct Font {
    void *stbtt_font_info;
    void *font_data;

    int32_t height;
    float32_t scale_for_pixel_height;

    int32_t ascent;
    int32_t descent;
    int32_t line_gap;

    struct Glyph {
        int32_t width;
        int32_t height;
        int32_t x_offset;
        int32_t y_offset;
        int32_t advance;
        int32_t left_side_bearing;

        bool has_glyph;
        vec2 tex_coords[4];
    };

    Texture *texture;
    std::unordered_map<int32_t, Glyph> *glyphs; // Pointer because c++ new kw

    Glyph *get_glyph(int32_t codepoint);
    int32_t kerning_advance(int32_t left, int32_t right);
    float32_t calc_string_width(const char *string);
};

Font *load_ttf_font_from_memory(uint8_t *font_data, size_t font_data_size, int32_t height, int32_t atlas_width, int32_t atlas_height);
Font *load_ttf_font(const char *filepath, int32_t height, int32_t atlas_width, int32_t atlas_height);
void delete_font(Font *font);

namespace color {
    constexpr vec4 black       = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr vec4 white       = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr vec4 red         = { 1.0f, 0.0f, 0.0f, 1.0f };
    constexpr vec4 green       = { 0.0f, 1.0f, 0.0f, 1.0f };
    constexpr vec4 blue        = { 0.0f, 0.0f, 1.0f, 1.0f };
    constexpr vec4 yellow      = { 1.0f, 1.0f, 0.0f, 1.0f };
    constexpr vec4 cyan        = { 0.0f, 1.0f, 1.0f, 1.0f };
    constexpr vec4 dark_green  = { 0.00392f, 0.20784f, 0.09803f, 1.0f };
}

/* --- Shaders --- */

inline const char *quad_vert_shader_string = R"(
        #version 330 core
        layout(location = 0) in ivec3 a_position;
        layout(location = 1) in vec2 a_tex_coord;
        layout(location = 2) in vec4 a_color; 
        layout(location = 3) in int a_tex_id;
        
        uniform mat4 u_view;
        uniform mat4 u_proj;
        
        out vec4 v_color;
        out vec2 v_tex_coord;
        flat out int v_tex_id;

        void main() {
            gl_Position = u_proj * u_view * vec4(a_position, 1.0);
        
            v_color = a_color;

            v_tex_coord = a_tex_coord;
            v_tex_id = a_tex_id;
        }
    )";

inline const char *quad_frag_shader_string = R"(
        #version 330 core
        uniform sampler2D u_textures[16];
        
        in vec4 v_color;
        in vec2 v_tex_coord;
        flat in int v_tex_id;

        out vec4 f_color;
        
        void main() {
        #if 1
            f_color = texture(u_textures[v_tex_id], v_tex_coord) * v_color;
            if(f_color.a == 0.0) {
                discard;
            }
            // f_color.r = pow(f_color.r, 2.2);
            // f_color.g = pow(f_color.g, 2.2);
            // f_color.b = pow(f_color.b, 2.2);
        #else  
            f_color = vec4(v_tex_coord.x, v_tex_coord.y, 0.0, 1.0);
        #endif
        }
    )";

inline const char *line_vert_shader_string = R"(
        #version 330 core
        layout(location = 0) in ivec3 a_position;
        layout(location = 1) in vec4  a_color; 
               
        uniform mat4 u_view;
        uniform mat4 u_proj;
        
        out vec4 v_color;
        
        void main() {
            gl_Position = u_proj * u_view * vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

inline const char *line_frag_shader_string = R"(
        #version 330 core
        
        in  vec4 v_color;
        out vec4 f_color;
        
        void main() {
            f_color = v_color;
        }
    )";

inline const char *text_vert_shader_string = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_tex_coord;
        layout(location = 2) in vec4 a_color;
        layout(location = 3) in int a_tex_id;
        
        uniform mat4 u_view;
        uniform mat4 u_proj;
        
        out vec4 v_color;
        out vec2 v_tex_coord;
        flat out int v_tex_id;

        void main() {
            gl_Position = u_proj * u_view * vec4(a_position, 1.0);
            v_color = a_color;
            v_tex_coord = a_tex_coord;
            v_tex_id = a_tex_id;
        }
    )";

inline const char *text_frag_shader_string = R"(
        #version 330 core
        uniform sampler2D u_textures[16];
        
        in vec4 v_color;
        in vec2 v_tex_coord;
        flat in int v_tex_id;
        out vec4 f_color;
        
        void main() {
            #if 1
                f_color = vec4(1.0f, 1.0f, 1.0f, texture(u_textures[v_tex_id], v_tex_coord).r) * v_color;
                if(f_color.a == 0) {
                    //discard;
                }
            #else
                f_color = vec4(v_tex_coord.x, v_tex_coord.y, 0.0, 1.0);
            #endif
        }
    )";

#endif /* _RENDERER_H */