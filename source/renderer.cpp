#include "renderer.h"

#include "../data/preloaded_data.h"

#define MAX_TEXTURES 8
#define QUADS_PER_DRAW_CALL      (8192 * 2)
#define LINES_PER_DRAW_CALL      (8192 * 2 * 2)
#define TEXT_QUADS_PER_DRAW_CALL (8192 * 2)

struct QuadVertex {
    int32_t   a_position [3];
    float32_t a_tex_coord[2];
    float32_t a_color    [4];
    int32_t   a_tex_id;
};

struct LineVertex {
    int32_t   a_position[3];
    float32_t a_color   [4];
};

struct TextVertex {
    float32_t a_position [3];
    float32_t a_tex_coord[2];
    float32_t a_color    [4];
    int32_t   a_tex_id;
};

namespace {
    bool renderer_initialized = false;
    bool scene_began = false;

    RenderStats render_stats;

    Texture *white_texture;
    Font    *default_font;
    Font    *default_font_big;

    // --- scene data --- (configurable)
    RenderSetup render_setup;
    float32_t line_width;
    int32_t quads_x_to_flip;
    int32_t quads_y_to_flip;
    recti32 scissor_rect;
    bool    scissor_active;

    /* textures, used for quads and text */
    int32_t  textures[MAX_TEXTURES];
    uint32_t pushed_textures;

    // --- quad data ---
    Shader *quad_shader;
    VertexArray *quad_va;

    uint32_t max_quads;
    uint32_t max_quad_verts;
    uint32_t max_quad_indices;
    uint32_t *quad_ib_data;
    QuadVertex *quad_vb_data;
    uint32_t pushed_quads;

    /* --- line data --- */
    Shader *line_shader;
    VertexArray *line_va;

    uint32_t max_lines;
    uint32_t max_line_verts;
    LineVertex *line_vb_data;

    uint32_t pushed_lines;

    /* --- text data --- */
    Shader *text_shader;
    VertexArray *text_va;

    uint32_t max_text_quads;
    uint32_t max_text_verts;
    uint32_t max_text_indices;
    uint32_t *text_ib_data;
    TextVertex *text_vb_data;

    uint32_t pushed_text_quads;
};

static
void init_quads(void) {
    max_quads        = QUADS_PER_DRAW_CALL;
    max_quad_verts   = QUADS_PER_DRAW_CALL * 4;
    max_quad_indices = QUADS_PER_DRAW_CALL * 6;

    // Allocate memory for quad vertices and indices
    size_t vb_data_bytes = max_quad_verts * sizeof(QuadVertex);
    size_t ib_data_bytes = max_quad_indices * sizeof(uint32_t);
    quad_vb_data = (QuadVertex *)malloc(vb_data_bytes);
    quad_ib_data = (uint32_t *)malloc(ib_data_bytes);
    assert(quad_vb_data != nullptr && quad_ib_data != nullptr);

    // Setup quad vertex buffer
    BufferLayout quad_vb_layout = { };
    quad_vb_layout.push_element(3, BufferLayout::EType::INT32,   "a_position");
    quad_vb_layout.push_element(2, BufferLayout::EType::FLOAT32, "a_tex_coord");
    quad_vb_layout.push_element(4, BufferLayout::EType::FLOAT32, "a_color");
    quad_vb_layout.push_element(1, BufferLayout::EType::INT32,   "a_tex_id");
    VertexBuffer *quad_vb = create_vertex_buffer(nullptr, vb_data_bytes, GL_DYNAMIC_DRAW, &quad_vb_layout);
    
    // Setup quad indices
    // @todo No need to reset it from the beginning<?>
    for(uint32_t indice = 0, vertice = 0; indice < max_quad_indices; indice += 6, vertice += 4) {
        uint32_t *next_indice = quad_ib_data + indice;
        next_indice[0] = vertice + 0;
        next_indice[1] = vertice + 1;
        next_indice[2] = vertice + 2;
        next_indice[3] = vertice + 0;
        next_indice[4] = vertice + 3;
        next_indice[5] = vertice + 2;
    }
    IndexBuffer *quad_ib = create_index_buffer(quad_ib_data, max_quad_indices);

    // Create vertex array
    quad_va = create_vertex_array();
    quad_va->attach_vertex_buffer(quad_vb);
    quad_va->set_index_buffer(quad_ib);

    // Create quad shader
    const char *quad_vert = quad_vert_shader_string;
    const char *quad_frag = quad_frag_shader_string;
    const size_t quad_vert_len = strlen(quad_vert);
    const size_t quad_frag_len = strlen(quad_frag);
    quad_shader = create_shader(quad_vert, quad_vert_len, quad_frag, quad_frag_len);

    // Set texture array in the shader
    int32_t tex_slots[MAX_TEXTURES];
    for(int32_t idx = 0; idx < MAX_TEXTURES; ++idx) {
        tex_slots[idx] = idx;
    }
    quad_shader->set_int_array("u_textures", (int32_t *)tex_slots, MAX_TEXTURES);
}

static
void free_quads(void) {
    delete_shader(quad_shader);
    delete_vertex_array(quad_va);
    free(quad_ib_data);
    free(quad_vb_data);
}

static
void init_lines(void) {
    max_lines      = LINES_PER_DRAW_CALL;
    max_line_verts = LINES_PER_DRAW_CALL * 2;

    // Allocate memory for line vertices
    size_t vb_data_bytes = max_line_verts * sizeof(LineVertex);
    line_vb_data = (LineVertex *)malloc(vb_data_bytes);
    assert(line_vb_data != nullptr);

    // Setup line vertex buffer
    BufferLayout line_vb_layout = { };
    line_vb_layout.push_element(3, BufferLayout::EType::INT32,   "a_position");
    line_vb_layout.push_element(4, BufferLayout::EType::FLOAT32, "a_color");
    VertexBuffer *line_vb = create_vertex_buffer(nullptr, vb_data_bytes, GL_DYNAMIC_DRAW, &line_vb_layout);

    line_va = create_vertex_array();
    line_va->attach_vertex_buffer(line_vb);

    // Create line shader
    const char *line_vert = line_vert_shader_string;
    const char *line_frag = line_frag_shader_string;
    const size_t line_vert_len = strlen(line_vert);
    const size_t line_frag_len = strlen(line_frag);
    line_shader = create_shader(line_vert, line_vert_len, line_frag, line_frag_len);
}

static
void free_lines(void) {
    delete_shader(line_shader);
    delete_vertex_array(line_va);
    free(line_vb_data);
}

static
void init_text(void) {
    max_text_quads   = TEXT_QUADS_PER_DRAW_CALL;
    max_text_verts   = TEXT_QUADS_PER_DRAW_CALL * 4;
    max_text_indices = TEXT_QUADS_PER_DRAW_CALL * 6;

    // Allocate memory for text quads vertices and indices
    size_t vb_data_bytes = max_text_verts * sizeof(TextVertex);
    size_t ib_data_bytes = max_text_indices * sizeof(uint32_t);
    text_vb_data = (TextVertex *)malloc(vb_data_bytes);
    text_ib_data = (uint32_t *)malloc(ib_data_bytes);
    assert(text_vb_data != nullptr && text_ib_data != nullptr);

    // Setup text vertex buffer
    BufferLayout vb_layout = { };
    vb_layout.push_element(3, BufferLayout::EType::FLOAT32, "a_position");
    vb_layout.push_element(2, BufferLayout::EType::FLOAT32, "a_tex_coord");
    vb_layout.push_element(4, BufferLayout::EType::FLOAT32, "a_color");
    vb_layout.push_element(1, BufferLayout::EType::INT32,   "a_tex_id");
    VertexBuffer *text_vb = create_vertex_buffer(nullptr, vb_data_bytes, GL_DYNAMIC_DRAW, &vb_layout);

    // Setup text indices
    for(uint32_t indice = 0, vertice = 0; indice < max_quad_indices; indice += 6, vertice += 4) {
        uint32_t *next_indice = text_ib_data + indice;
        next_indice[0] = vertice + 0;
        next_indice[1] = vertice + 1;
        next_indice[2] = vertice + 2;
        next_indice[3] = vertice + 0;
        next_indice[4] = vertice + 3;
        next_indice[5] = vertice + 2;
    }
    IndexBuffer *text_ib = create_index_buffer(text_ib_data, max_text_indices);

    // Create vertex array
    text_va = create_vertex_array();
    text_va->attach_vertex_buffer(text_vb);
    text_va->set_index_buffer(text_ib);

    // Create text shader
    const char *text_vert = text_vert_shader_string;
    const char *text_frag = text_frag_shader_string;
    const size_t vert_len = strlen(text_vert);
    const size_t frag_len = strlen(text_frag);
    text_shader = create_shader(text_vert, vert_len, text_frag, frag_len);

    // Set texture array in the shader
    int32_t tex_slots[MAX_TEXTURES];
    for(int32_t idx = 0; idx < MAX_TEXTURES; ++idx) {
        tex_slots[idx] = idx;
    }
    text_shader->set_int_array("u_textures", (int32_t *)tex_slots, MAX_TEXTURES);
}

static
void free_text(void) {
    delete_shader(text_shader);
    delete_vertex_array(text_va);
    free(text_vb_data);
    free(text_ib_data);
}

static
void setup_opengl(void) {
    glEnable(GL_TEXTURE_2D);
    // glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LINE_SMOOTH);
}

static void _reset_renderer(void);

void render::r_init(void) {
    setup_opengl();
    init_quads();
    init_lines();
    init_text();
    line_width = 1.0f;

    const uint32_t white_pixel = 0xFFFFFFFF;
    white_texture = create_texture((void *)&white_pixel, 1, 1, 4, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA);
    assert(white_texture != NULL);

    default_font     = load_ttf_font_from_memory(preloaded::liberation_mono_regular_ttf_data, array_count(preloaded::liberation_mono_regular_ttf_data), R_DEF_FONT_HEIGHT,     1024, 1024);
    default_font_big = load_ttf_font_from_memory(preloaded::liberation_mono_regular_ttf_data, array_count(preloaded::liberation_mono_regular_ttf_data), R_DEF_FONT_BIG_HEIGHT, 1024, 1024);
    assert(default_font     != NULL);
    assert(default_font_big != NULL);

    renderer_initialized = true;
    _reset_renderer();
}

void render::r_quit(void) {
    if(!renderer_initialized) {
        return;
    }

    delete_texture(white_texture);
    delete_font(default_font);

    free_quads();
    free_lines();
    free_text();
    renderer_initialized = false;
}

static
int32_t push_texture(gl_id texture_id) {
    for(int32_t texture_slot = 0; texture_slot < pushed_textures; ++texture_slot) {
        if(textures[texture_slot] == texture_id) {
            return texture_slot;
        }
    }

    if((pushed_textures + 1) > MAX_TEXTURES) {
        return -1;
    }

    int32_t texture_slot = pushed_textures++;
    textures[texture_slot] = texture_id;
    return texture_slot;
}

static
void _reset_renderer(void) {
    pushed_textures   = 0;
    pushed_quads      = 0;
    pushed_lines      = 0;
    pushed_text_quads = 0;
}

void render::r_begin(RenderSetup setup) {
    assert(scene_began == false);
    scene_began = true;
    render_setup = setup;
    line_width = 1.0f;
    quads_x_to_flip = 0;
    quads_y_to_flip = 0;
    scissor_active = false;
    _reset_renderer();
}

static
void flush(void) {
    if(!pushed_quads && !pushed_lines && !pushed_text_quads) {
        return;
    }

    if(render_setup.framebuffer) {
        render_setup.framebuffer->bind();
    } else {
        unbind_framebuffer();
    }

    gl_viewport(render_setup.viewport);

    if(scissor_active) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissor_rect.x, scissor_rect.y, scissor_rect.w, scissor_rect.h);
    }

    if(pushed_quads) {
        quad_shader->use();
        for(int32_t slot = 0; slot < pushed_textures; ++slot) { quad_shader->set_texture(textures[slot], slot); }
        quad_shader->set_mat4x4("u_proj", render_setup.proj_m);
        quad_shader->set_mat4x4("u_view", render_setup.view_m);

        const uint32_t index_count   = pushed_quads * 6;
        const uint32_t vb_data_bytes = pushed_quads * 4 * sizeof(QuadVertex);

        // Upload data
        quad_va->bind();
        quad_va->vbs[0]->set_data(quad_vb_data, vb_data_bytes, 0);
        quad_va->ib->set_data(quad_ib_data, index_count, 0);

        gl_draw_elements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, NULL);
    }

    if(pushed_lines) {
        // Set line width
        glLineWidth(line_width);

        // Set uniforms
        line_shader->use();
        line_shader->set_mat4x4("u_proj", render_setup.proj_m);
        line_shader->set_mat4x4("u_view", render_setup.view_m);

        const uint32_t vb_data_bytes = pushed_lines * 2 * sizeof(LineVertex);

        // Upload data
        line_va->bind();
        line_va->vbs[0]->set_data(line_vb_data, vb_data_bytes, 0);

        gl_draw_arrays(GL_LINES, 0, pushed_lines * 2);
    }

    if(pushed_text_quads) {
        text_shader->use();
        for(int32_t slot = 0; slot < pushed_textures; ++slot) { text_shader->set_texture(textures[slot], slot); }
        text_shader->set_mat4x4("u_proj", render_setup.proj_m);
        text_shader->set_mat4x4("u_view", render_setup.view_m);

        const uint32_t index_count   = pushed_text_quads * 6;
        const uint32_t vb_data_bytes = pushed_text_quads * 4 * sizeof(TextVertex);

        text_va->bind();
        text_va->vbs[0]->set_data(text_vb_data, vb_data_bytes, 0);
        text_va->ib->set_data(text_ib_data, index_count, 0);

        gl_draw_elements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, NULL);
    }

    if(scissor_active) {
        glDisable(GL_SCISSOR_TEST);
    }

    render_stats.draw_calls += 1;
    render_stats.quads      += pushed_quads;
    render_stats.lines      += pushed_lines;
    render_stats.text_quads += pushed_text_quads;
    
    _reset_renderer();
}

void render::r_end(void) {
    assert(scene_began == true);
    flush();
    scene_began = false;
}

void render::r_scissor_begin(recti32 rect) {
    assert(scene_began && !scissor_active);
    flush();
    scissor_active = true;
    scissor_rect = rect;
}

void render::r_scissor_end(void) {
    assert(scene_began && scissor_active);
    flush();
    scissor_active = false;
}

void render::r_flush(void) {
    flush();
}

void render::get_current_setup(RenderSetup *out_setup) {
    *out_setup = render_setup;
}

static
void quad_base(vec2i positions[4], int32_t z_pos, vec2 tex_coords[4], vec4 color, gl_id texture_id) {
    assert(scene_began == true);

    if((pushed_quads + 1) > max_quads) {
        flush();
    }

    int32_t texture_slot = push_texture(texture_id);
    if(texture_slot == -1) {
        flush();
        texture_slot = push_texture(texture_id);
        assert(texture_slot != -1);
    }

    if(quads_x_to_flip) {
        quads_x_to_flip -= 1;
        swap_2(tex_coords[0], tex_coords[1]);
        swap_2(tex_coords[3], tex_coords[2]);
    }

    if(quads_y_to_flip) {
        quads_y_to_flip -= 1;
        swap_2(tex_coords[0], tex_coords[3]);
        swap_2(tex_coords[1], tex_coords[2]);
    }

    QuadVertex *next_vertex = quad_vb_data + pushed_quads * 4;
    for(int32_t idx = 0; idx < 4; ++idx) {
        next_vertex->a_position[0] = positions[idx].x;
        next_vertex->a_position[1] = positions[idx].y;
        next_vertex->a_position[2] = z_pos;
        next_vertex->a_tex_coord[0] = tex_coords[idx].x;
        next_vertex->a_tex_coord[1] = tex_coords[idx].y;
        next_vertex->a_color[0] = color.r;
        next_vertex->a_color[1] = color.g;
        next_vertex->a_color[2] = color.b;
        next_vertex->a_color[3] = color.a;
        next_vertex->a_tex_id = texture_slot;
        next_vertex += 1;
    }
    pushed_quads += 1;
}

inline static
void calc_vertex_positions(vec2i position, vec2i size, vec2i out_positions[4]) {
    out_positions[0] = { position.x,          position.y };
    out_positions[1] = { position.x + size.x, position.y };
    out_positions[2] = { position.x + size.x, position.y + size.y };
    out_positions[3] = { position.x,          position.y + size.y };
}

void render::r_quad(vec2i position, int32_t z_pos, vec2i size, vec4 color) {
    vec2i positions[4];
    calc_vertex_positions(position, size, positions);

    vec2 tex_coords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    quad_base(positions, z_pos, tex_coords, color, white_texture->texture_id);
}

void render::r_quad_outline(vec2i position, int32_t z_pos, vec2i size, int32_t width, vec4 color) {
    // assert(width < size.x / 2 && width < size.y / 2); // @todo
    render::r_quad(position,                                   z_pos, { size.x, width },         color);
    render::r_quad(position + vec2i { 0, width },              z_pos, { width, size.y - width }, color);
    render::r_quad(position + vec2i { size.x - width, width }, z_pos, { width, size.y - width }, color);
    render::r_quad(position + vec2i { width, size.y - width }, z_pos, { size.x - width * 2, width }, color);
}

void render::r_texture(vec2i position, int32_t z_pos, vec2i size, Texture *texture, vec4 color) {
    vec2i positions[4];
    calc_vertex_positions(position, size, positions);

    vec2 tex_coords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    quad_base(positions, z_pos, tex_coords, color, texture->texture_id);
}

void render::r_sprite(vec2i position, int32_t z_pos, vec2i size, Sprite sprite, vec4 color) {
    assert(sprite.texture != NULL, "Invalid sprite.");

    vec2i positions[4];
    calc_vertex_positions(position, size, positions);

    quad_base(positions, z_pos, sprite.tex_coords, color, sprite.texture->texture_id);
}

void render::r_quad_marching_ants(vec2i pos, int32_t z_pos, vec2i size, int32_t ant_length, vec4 color, float32_t perc, bool backwards) {
    if(size.x < 0 || size.y < 0) {
        return; // size cant be negative
    }

    auto marching_line = [=] (int32_t x, int32_t y, int32_t length, int32_t ant_length, float32_t counter, bool vertical) {
        const int32_t num = (int32_t)roundf(ceilf((float32_t)length / (float32_t)(ant_length * 2.0f)));
        for(int32_t idx = 0; idx < (num + 1); ++idx) {
            const int32_t v = vertical ? y : x;
            int32_t from = clamp_min((idx - 1) * ant_length * 2 + (int32_t)roundf(counter * (ant_length * 2.0f)) + v, v);
            int32_t to   = clamp_max((idx - 1) * ant_length * 2 + (int32_t)roundf(counter * (ant_length * 2.0f)) + ant_length + v, v + length);
            if((to - from) > 0.0f) {
                if(vertical) {
                    render::r_line({ x,  from }, { x, to }, z_pos, color);
                } else {
                    render::r_line({ from, y }, { to, y }, z_pos, color);
                }
            }
        }
    };

    perc = perc - floorf(perc);

    if(backwards) {
        perc = 1.0f - perc;
    }

    marching_line(pos.x, pos.y,           size.x, ant_length, perc, false);
    marching_line(pos.x, pos.y + size.y,  size.x, ant_length, perc, false);
    marching_line(pos.x, pos.y,           size.y, ant_length, perc, true);
    marching_line(pos.x + size.x, pos.y,  size.y, ant_length, perc, true);
}

static
void line_base(vec2i points[2], int32_t z_pos, vec4 colors[2]) {
    assert(scene_began == true);

    if((pushed_lines + 1) > max_lines) {
        flush();
    }

    LineVertex *next_vertex = line_vb_data + pushed_lines * 2;
    for(int32_t idx = 0; idx < 2; ++idx) {
        next_vertex->a_position[0] = points[idx].x;
        next_vertex->a_position[1] = points[idx].y;
        next_vertex->a_position[2] = z_pos;
        next_vertex->a_color[0] = colors[idx].r;
        next_vertex->a_color[1] = colors[idx].g;
        next_vertex->a_color[2] = colors[idx].b;
        next_vertex->a_color[3] = colors[idx].a;
        next_vertex += 1;
    }
    pushed_lines += 1;
}

void render::r_line(vec2i point_a, vec2i point_b, int32_t z_pos, vec4 color) {
    vec2i points[2] = { point_a, point_b };
    vec4  colors[2] = { color, color };
    line_base(points, z_pos, colors);
}

void render::r_line_quad(vec2i position, int32_t z_pos, vec2i size, vec4 color) {
    vec2i positions[4] = {
        { position.x,          position.y,         },
        { position.x + size.x, position.y,         },
        { position.x + size.x, position.y + size.y },
        { position.x,          position.y + size.y },
    };

    r_line(positions[0], positions[1], z_pos, color);
    r_line(positions[1], positions[2], z_pos, color);
    r_line(positions[2], positions[3], z_pos, color);
    r_line(positions[3], positions[0], z_pos, color);
}

static
void text_quad_base(vec2 positions[4], int32_t z_pos, vec2 tex_coords[4], vec4 color, gl_id atlas_tex_id) {
    assert(scene_began == true);

    if((pushed_text_quads + 1) > max_text_quads) {
        flush();
    }

    int32_t texture_slot = push_texture(atlas_tex_id);
    if(texture_slot == -1) {
        flush();
        texture_slot = push_texture(atlas_tex_id);
        assert(texture_slot != -1);
    }

    TextVertex *next_vertex = text_vb_data + pushed_text_quads * 4;
    for(int32_t idx = 0; idx < 4; ++idx) {
        next_vertex->a_position[0] = positions[idx].e[0];
        next_vertex->a_position[1] = positions[idx].e[1];
        next_vertex->a_position[2] = z_pos;
        next_vertex->a_tex_coord[0] = tex_coords[idx].e[0];
        next_vertex->a_tex_coord[1] = tex_coords[idx].e[1];
        next_vertex->a_color[0] = color.e[0];
        next_vertex->a_color[1] = color.e[1];
        next_vertex->a_color[2] = color.e[2];
        next_vertex->a_color[3] = color.e[3];
        next_vertex->a_tex_id = texture_slot;
        next_vertex += 1;
    }
    pushed_text_quads += 1;
}

// Special characters are not handled -> maybe @todo support '\n' and stuff
void render::r_text(vec2i position, int32_t z_pos, const char *string, Font *font, vec4 color) {
    vec2 cursor = { (float32_t)position.x, (float32_t)position.y };

    const size_t len = strlen(string);
    for(int32_t idx = 0; idx < len; ++idx) {
        const uint8_t _char = string[idx];

        Font::Glyph *glyph = font->get_glyph(_char);
        assert(glyph != NULL);

        if(glyph->has_glyph) {
            vec2 glyph_size = { (float32_t)glyph->width, (float32_t)glyph->height };
            vec2 glyph_p = { 
                cursor.x + glyph->left_side_bearing * font->scale_for_pixel_height, 
                cursor.y - glyph->height - glyph->y_offset
            };

            vec2 positions[4] = {
                { glyph_p.x,                glyph_p.y },
                { glyph_p.x + glyph_size.x, glyph_p.y },
                { glyph_p.x + glyph_size.x, glyph_p.y + glyph_size.y },
                { glyph_p.x,                glyph_p.y + glyph_size.y }
            };

            text_quad_base(positions, z_pos, glyph->tex_coords, color, font->texture->texture_id);

#ifdef R_TEXT_DEBUG_DRAW
            r_set_line_width(1.0f);
            r_line_quad(vec2i::make(glyph_p), z_pos, { glyph->width, glyph->height }, color::red);
#endif
        }

        const int32_t adv = glyph->advance + font->kerning_advance(string[idx], string[idx + 1]);
        cursor.x += adv * font->scale_for_pixel_height;
    }

#ifdef R_TEXT_DEBUG_DRAW
    r_line_quad(position, z_pos, { (int32_t)roundf(font->calc_string_width(string)), font->height }, color::green);
    r_line(position, { position.x + (int32_t)roundf(font->calc_string_width(string)), position.y }, z_pos, color::green);
#endif
}

#include <cstdarg>
void render::r_text_formatted(vec2i position, int32_t z_pos, Font *font, vec4 color, const char *text, ...) {
    va_list list;
    va_start(list, text);
    char buffer[1024]; // @todo
    vsprintf_s(buffer, array_count(buffer), text, list);
    render::r_text(position, z_pos, buffer, font, color);
    va_end(list);
}

// @copied
void render::r_text_scaled(vec2i position, int32_t z_pos, const char *string, Font *font, float32_t scale, vec4 color) {
    vec2 cursor = { (float32_t)position.x, (float32_t)position.y };

    const size_t len = strlen(string);
    for(int32_t idx = 0; idx < len; ++idx) {
        const uint8_t _char = string[idx];

        Font::Glyph *glyph = font->get_glyph(_char);
        assert(glyph != NULL);

        if(glyph->has_glyph) {
            vec2 glyph_size = vec2{ (float32_t)glyph->width, (float32_t)glyph->height } * scale;
            vec2 glyph_p = { 
                cursor.x + glyph->left_side_bearing * font->scale_for_pixel_height * scale, 
                cursor.y - glyph->height * scale - glyph->y_offset * scale
            };

            vec2 positions[4] = {
                { glyph_p.x,                glyph_p.y },
                { glyph_p.x + glyph_size.x, glyph_p.y },
                { glyph_p.x + glyph_size.x, glyph_p.y + glyph_size.y },
                { glyph_p.x,                glyph_p.y + glyph_size.y }
            };

            text_quad_base(positions, z_pos, glyph->tex_coords, color, font->texture->texture_id);

#ifdef R_TEXT_DEBUG_DRAW
            r_set_line_width(1.0f);
            r_line_quad(vec2i::make(glyph_p), z_pos, { (int32_t)roundf(glyph->width * scale), (int32_t)roundf(glyph->height * scale) }, color::red);
#endif
        }

        const int32_t adv = glyph->advance + font->kerning_advance(string[idx], string[idx + 1]);
        cursor.x += adv * scale * font->scale_for_pixel_height;
    }

#ifdef R_TEXT_DEBUG_DRAW
    r_line_quad(position, z_pos, { (int32_t)roundf(font->calc_string_width(string) * scale), (int32_t)roundf(font->height * scale) }, color::green);
    r_line(position, { position.x + (int32_t)roundf(font->calc_string_width(string) * scale), position.y }, z_pos, color::green);
#endif
}

void render::r_set_line_width(float32_t width) {
    if(width == line_width) { return; }
    if(scene_began) { flush(); }
    line_width = width;
}

void render::r_set_flip_x_quads(int32_t num) {
    quads_x_to_flip = clamp_min(num, 0);
}

void render::r_set_flip_y_quads(int32_t num) {
    quads_y_to_flip = clamp_min(num, 0);
}

RenderStats render::r_reset_stats(void) {
    RenderStats stats = render_stats;
    zero_struct(&render_stats);
    return stats;
}

Font *render::def_font(void) {
    return default_font;
}

Font *render::def_font_big(void) {
    return default_font_big;
}

Sprite define_sprite(Texture *texture, int32_t cell_w, int32_t cell_h, int32_t cell_x, int32_t cell_y, int32_t sprite_w, int32_t sprite_h) {
    Sprite sprite;
    sprite.texture = texture;
    sprite.width  = sprite_w;
    sprite.height = sprite_h;
    
    recti32 rect;
    rect.x = cell_w * cell_x;
    rect.y = cell_h * cell_y;
    rect.w = sprite_w;
    rect.h = sprite_h;

    float64_t texture_w_f64 = (float64_t)texture->width;
    float64_t texture_h_f64 = (float64_t)texture->height;

    sprite.tex_coords[0].x = (float64_t)rect.x / texture_w_f64;
    sprite.tex_coords[0].y = (float64_t)rect.y / texture_h_f64;
    sprite.tex_coords[1].x = ((float64_t)rect.x + (float64_t)rect.w) / texture_w_f64;
    sprite.tex_coords[1].y =  (float64_t)rect.y                      / texture_h_f64;
    sprite.tex_coords[2].x = ((float64_t)rect.x + (float64_t)rect.w) / texture_w_f64;
    sprite.tex_coords[2].y = ((float64_t)rect.y + (float64_t)rect.h) / texture_h_f64;
    sprite.tex_coords[3].x =  (float64_t)rect.x                      / texture_w_f64;
    sprite.tex_coords[3].y = ((float64_t)rect.y + (float64_t)rect.h) / texture_h_f64;

    return sprite;
}

AnimSet init_anim_set(AnimSet *set) {
    AnimSet anim_set = { };
    if(set) {
        *set = anim_set;
    }
    return anim_set;
}

bool next_anim_frame(AnimSet *set, Sprite sprite, float32_t duration) {
    int32_t frame_id = set->frame_count;
    if(set->frame_count > AnimSet::max_frames) {
        // @error : No space for next frame
        return false;
    }
    set->frame_count += 1;

    AnimSet::Frame *frame = &set->frames[frame_id];
    frame->sprite = sprite;
    frame->duration = duration;
    return true;
}

AnimPlayer init_anim_player(AnimSet *set) {
    AnimPlayer anim = { };
    anim.set = set;
    return anim;
}

void set_anim(AnimPlayer *anim, AnimSet *set) {
    anim->set      = set;
    anim->frame_id = 0;
    anim->timer    = 0.0f;
}

void advance_anim_frame(AnimPlayer *anim) {
    assert(anim != NULL);

    anim->frame_id += 1;
    if(anim->frame_id >= anim->set->frame_count) {
        anim->frame_id = 0;
    }
}

AnimSet::Frame *get_current_frame(AnimPlayer *anim) {
    assert(anim->set != NULL);

    if(anim->set->frame_count == 0) {
        return NULL;
    }
    assert(anim->frame_id >= 0 && anim->frame_id < anim->set->frame_count);

    AnimSet::Frame *frame = anim->set->frames + anim->frame_id;
    return frame;
}

void update_anim(AnimPlayer *anim, float32_t delta_time) {
    assert(anim->set != NULL);

    AnimSet::Frame *frame = get_current_frame(anim);
    if(anim->set->frame_count <= 1) {
        return;
    }
    
    anim->timer += delta_time;
    while(anim->timer >= frame->duration) {
        anim->timer -= frame->duration;
        advance_anim_frame(anim);
    }
}

void reset_anim_player(AnimPlayer *anim) {
    anim->timer = 0.0f;
    anim->frame_id = 0;
}

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "data.h"

Font *load_ttf_font_from_memory(uint8_t *_font_data, size_t font_data_size, int32_t height, int32_t atlas_width, int32_t atlas_height) {
    uint8_t *font_data = (uint8_t *)malloc(font_data_size);
    assert(font_data);
    memcpy_s(font_data, font_data_size, _font_data, font_data_size);

    stbtt_fontinfo *font_info = malloc_struct(stbtt_fontinfo);
    if(stbtt_InitFont(font_info, font_data, 0) == 0) {
        free_file(font_data);
        return NULL;
    }

    float32_t scale_for_pixel_height = stbtt_ScaleForMappingEmToPixels(font_info, height);

    int32_t ascent;
    int32_t descent;
    int32_t line_gap;
    stbtt_GetFontVMetrics(font_info, &ascent, &descent, &line_gap);

    int32_t atlas_w = atlas_width;
    int32_t atlas_h = atlas_height;
    int32_t atlas_bpp = 1;
    uint8_t *atlas_bitmap = (uint8_t *)malloc_and_zero(atlas_w * atlas_h * atlas_bpp);
    assert(atlas_bitmap);

    const int32_t num_nodes = 4096;
    stbrp_node *rp_nodes = (stbrp_node *)malloc(sizeof(stbrp_node) * num_nodes);
    assert(rp_nodes);

    stbrp_context rect_pack_context;
    stbrp_init_target(&rect_pack_context, atlas_w, atlas_h, rp_nodes, num_nodes);

    const float32_t atlas_w_f32 = (float32_t)atlas_w;
    const float32_t atlas_h_f32 = (float32_t)atlas_h;

    auto glyphs = new std::unordered_map<int32_t, Font::Glyph>();
    for(int32_t codepoint = 0; codepoint < 256; ++codepoint) {
        Font::Glyph glyph = { };

        stbtt_GetCodepointHMetrics(font_info, codepoint, &glyph.advance, &glyph.left_side_bearing);

        // If glyph has bitmap, pack to atlas bitmap and calculate tex coords
        uint8_t *glyph_bitmap = stbtt_GetCodepointBitmap(font_info, 0, scale_for_pixel_height, codepoint, &glyph.width, &glyph.height, &glyph.x_offset, &glyph.y_offset);
        if(glyph_bitmap) {
            stbrp_rect rect;
            rect.w = glyph.width;
            rect.h = glyph.height;
            stbrp_pack_rects(&rect_pack_context, &rect, 1) == 1;
            if(!rect.was_packed) { // Couldn't pack rect
                free(glyph_bitmap);
                continue;
            }

            glyph.has_glyph = true;

            // Copy glyph pixels to assigned atlas locations
            for(int32_t y = 0; y < rect.h; ++y) {
                for(int32_t x = 0; x < rect.w; ++x) {
                    atlas_bitmap[(rect.y + y) * atlas_w + (rect.x + x)] = glyph_bitmap[(rect.h - y - 1) * rect.w + x];
                }
            }

            // Calculate tex coords
            glyph.tex_coords[0] = { rect.x / atlas_w_f32,             rect.y / atlas_h_f32 };
            glyph.tex_coords[1] = { (rect.x + rect.w) / atlas_w_f32,  rect.y / atlas_h_f32 };
            glyph.tex_coords[2] = { (rect.x + rect.w) / atlas_w_f32, (rect.y + rect.h) / atlas_h_f32 };
            glyph.tex_coords[3] = { rect.x / atlas_w_f32,            (rect.y + rect.h) / atlas_h_f32 };

            free(glyph_bitmap);
        }

        glyphs->insert({ codepoint, glyph });
    }

    Texture *atlas_texture = create_texture((void *)atlas_bitmap, atlas_w, atlas_h, atlas_bpp, GL_RED, GL_UNSIGNED_BYTE, GL_RED);
    assert(atlas_texture);

    free(rp_nodes);
    free(atlas_bitmap);

    auto font = malloc_and_zero_struct(Font);
    font->stbtt_font_info = (void *)font_info;
    font->font_data = font_data;
    font->height = height;
    font->scale_for_pixel_height = scale_for_pixel_height;
    font->ascent = ascent;
    font->descent = descent;
    font->line_gap = line_gap;
    font->texture = atlas_texture;
    font->glyphs = glyphs;
    return font;
}

Font *load_ttf_font(const char *filepath, int32_t height, int32_t atlas_width, int32_t atlas_height) {
    void  *font_data;
    size_t font_data_size;
    if(!read_file(filepath, &font_data, &font_data_size, false)) {
        return NULL;
    }

    Font *font = load_ttf_font_from_memory((uint8_t *)font_data, font_data_size, height, atlas_width, atlas_height);
    free_file(font_data);
    return font;
}

void delete_font(Font *font) {
    free(font->stbtt_font_info);
    free(font->font_data);
    delete font->glyphs;
    delete_texture(font->texture);
    free(font);
}

Font::Glyph *Font::get_glyph(int32_t codepoint) {
    auto glyph_slot = this->glyphs->find(codepoint);
    if(glyph_slot == this->glyphs->end()) {
        return NULL;
    }

    Font::Glyph *glyph = &glyph_slot->second;
    return glyph;
}

int32_t Font::kerning_advance(int32_t left, int32_t right) {
    int32_t result = stbtt_GetGlyphKernAdvance((stbtt_fontinfo *)this->stbtt_font_info, left, right);
    return result;
}

// @todo Hacky way of calculating width, ... special characters not supported
float32_t Font::calc_string_width(const char *string) {
    int32_t width = 0;
    size_t len = strlen(string);
    for(int32_t idx = 0; idx < len; ++idx) {
        Font::Glyph *glyph = this->get_glyph(string[idx]);
        assert(glyph != NULL);
        width += glyph->advance + this->kerning_advance(string[idx], string[idx + 1]);
    }
    return width * this->scale_for_pixel_height;
}