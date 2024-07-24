#ifndef _OPENGL_ABS_H
#define _OPENGL_ABS_H

#include <GL/glew.h>
#include <vector>

#include "common.h"
#include "maths.h"

typedef GLuint gl_id;
typedef GLenum gl_enum;

struct Texture {
    gl_id texture_id;
    int32_t width;
    int32_t height;
    int32_t bytes_per_pixel;
    gl_enum internal_format;
    gl_enum filter_min;
    gl_enum filter_mag;
    gl_enum wrap_s;
    gl_enum wrap_t;

    inline vec2i size(void) { return { this->width, this->height }; }

    void set_pixels(void *pixels, int32_t width, int32_t height, int32_t x_offset, int32_t y_offset, gl_enum data_format, gl_enum data_type);
    void reset_texture(void *pixels, int32_t width, int32_t height, int32_t bpp, gl_enum data_format, gl_enum data_type, gl_enum internal_format);
    void set_filter_min(gl_enum param);
    void set_filter_mag(gl_enum param);
    void set_wrap_s(gl_enum param);
    void set_wrap_t(gl_enum param);
};

Texture *create_texture(void *pixels, int32_t width, int32_t height, int32_t bpp, gl_enum data_format, gl_enum data_type, gl_enum internal_format);
Texture *load_texture(const char *filename, gl_enum data_format, gl_enum data_type, gl_enum internal_format);
void delete_texture(Texture *texture);

struct BufferLayout {
    int32_t stride;
    int32_t combined_count;
    int32_t next_element;

    enum class EType : uint8_t { INT32, FLOAT32 };

    struct Element {
        char name[64];
        int32_t count;
        int32_t offset;
        EType type;
    } elements[16];

    void push_element(int32_t count, EType type, const char *name);
};

struct Shader {
    gl_id program_id;
    gl_id vert_id;
    gl_id frag_id;

    void use(void);
    bool set_float(const char *name, float32_t value);
    bool set_int(const char *name, int32_t value);
    bool set_int_array(const char *name, int32_t *values, int32_t count);
    bool set_vec4(const char *name, vec4 value);
    bool set_mat4x4(const char *name, mat4x4 value);
    void set_texture(gl_id texture_id, uint32_t unit);
};

Shader *create_shader(const char *vertex, size_t vertex_len, const char *fragment, size_t fragment_len);
void delete_shader(Shader *shader);

struct VertexBuffer {
    gl_id   buffer_id;
    size_t  size;
    gl_enum usage;
    BufferLayout layout;

    void set_data(void *data, size_t size, int32_t offset);
};

VertexBuffer *create_vertex_buffer(void *data, size_t size, gl_enum usage, BufferLayout *layout);
void delete_vertex_buffer(VertexBuffer *vb);

struct IndexBuffer {
    gl_id buffer_id;
    int32_t count;

    void set_data(uint32_t *data, int32_t count, int32_t offset);
};

IndexBuffer *create_index_buffer(uint32_t *data, int32_t count);
void delete_index_buffer(IndexBuffer *ib);

struct VertexArray {
    gl_id array_id;
    int32_t next_location;
    int32_t vb_count;
    IndexBuffer *ib;
    VertexBuffer *vbs[8];

    void bind(void);
    
    // VertexArray deletes buffer pointers with itself...
    void attach_vertex_buffer(VertexBuffer *vb);
    void set_index_buffer(IndexBuffer *ib);
};

VertexArray *create_vertex_array(void);
void delete_vertex_array(VertexArray *va);

struct Framebuffer {
    gl_id framebuffer_id;
    int32_t width;
    int32_t height;
    Texture *color;
    Texture *depth;

    void bind(void);
};

Framebuffer *create_framebuffer(int32_t width, int32_t height);
void delete_framebuffer(Framebuffer *fb);
void unbind_framebuffer(void);

void gl_viewport(int32_t x, int32_t y, int32_t w, int32_t h);
void gl_viewport(recti32 rect);
void gl_clear(vec4 color, uint32_t flags);
void gl_draw_elements(gl_enum mode, size_t count, gl_enum type, void *indices);
void gl_draw_arrays(gl_enum mode, int32_t first, size_t count);

#endif /* _OPENGL_ABS_H */