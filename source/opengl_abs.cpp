#include "opengl_abs.h"

#define gl_log(...) printf(__VA_ARGS__)
#define gl_check(...) glGetError(); __VA_ARGS__; { auto CONCAT(error_code, __LINE__) = glGetError(); if(CONCAT(error_code, __LINE__)) { gl_log("OpenGL: line:%d code:%d exp:%s!\n", __LINE__, CONCAT(error_code, __LINE__), #__VA_ARGS__); } }

Texture *create_texture(void *pixels, int32_t width, int32_t height, int32_t bpp, gl_enum data_format, gl_enum data_type, gl_enum internal_format) {
    // @check Call everytime?
    gl_check(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    gl_id texture_id = 0;
    gl_check(glCreateTextures(GL_TEXTURE_2D, 1, &texture_id));
    if(texture_id == 0) {
        return NULL;
    }

    gl_check(glBindTexture(GL_TEXTURE_2D, texture_id));
    gl_check(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, data_type, pixels));

    Texture *texture = malloc_and_zero_struct(Texture);
    texture->texture_id = texture_id;
    texture->width = width;
    texture->height = height;
    texture->bytes_per_pixel = bpp;
    texture->internal_format = internal_format;
    texture->set_filter_min(GL_NEAREST);
    texture->set_filter_mag(GL_NEAREST);
    texture->set_wrap_s(GL_CLAMP_TO_BORDER);
    texture->set_wrap_t(GL_CLAMP_TO_BORDER);
    return texture;
}

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture *load_texture(const char *filename, gl_enum data_format, gl_enum data_type, gl_enum format) {
    stbi_set_flip_vertically_on_load(true);

    int32_t width;
    int32_t height;
    int32_t bpp;
    uint8_t *pixels = stbi_load(filename, &width, &height, &bpp, 0);
    if(!pixels) {
        return NULL;
    }

    Texture *texture = create_texture(pixels, width, height, bpp, data_format, data_type, format);

    free(pixels);
    return texture;
}

void delete_texture(Texture *texture) {
    if(texture == NULL) return;

    gl_check(glDeleteTextures(1, &texture->texture_id));
    free(texture);
}

void Texture::set_pixels(void *pixels, int32_t width, int32_t height, int32_t x_offset, int32_t y_offset, gl_enum data_format, gl_enum data_type) {
    gl_check(glBindTexture(GL_TEXTURE_2D, this->texture_id));
    gl_check(glTexSubImage2D(GL_TEXTURE_2D, 0, x_offset, y_offset, width, height, data_format, data_type, pixels));
}

void Texture::reset_texture(void *pixels, int32_t width, int32_t height, int32_t bpp, gl_enum data_format, gl_enum data_type, gl_enum internal_format) {
    gl_check(glBindTexture(GL_TEXTURE_2D, this->texture_id));
    gl_check(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, data_type, pixels));
}

void Texture::set_filter_min(gl_enum param) {
    this->filter_min = param;
    gl_check(glBindTexture(GL_TEXTURE_2D, this->texture_id));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param));
}

void Texture::set_filter_mag(gl_enum param) {
    this->filter_mag = param;
    gl_check(glBindTexture(GL_TEXTURE_2D, this->texture_id));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param));
}

void Texture::set_wrap_s(gl_enum param) {
    this->wrap_s = param;
    gl_check(glBindTexture(GL_TEXTURE_2D, this->texture_id));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param));
}

void Texture::set_wrap_t(gl_enum param) {
    this->wrap_t = param;
    gl_check(glBindTexture(GL_TEXTURE_2D, this->texture_id));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param));
}

void BufferLayout::push_element(int32_t count, EType type, const char *name) {
    Element e = { };
    e.offset = this->stride;
    e.count = count;
    e.type = type;
    memcpy_s(e.name, array_count(e.name), name, strlen(name)); // @todo 

    int32_t type_size = 0;
    switch(type) {
        case EType::INT32:
        case EType::FLOAT32: {
            type_size = 4;
        } break;
    };
    assert(type_size != 0);

    this->elements[this->next_element++] = e;
    this->stride += count * type_size;
    this->combined_count += count;
}

static gl_id compile_shader_source(const char *source, size_t source_len, GLenum shader_type) {
    gl_check(gl_id shader_id = glCreateShader(shader_type));
    gl_check(glShaderSource(shader_id, 1, &source, (const GLint *)&source_len));
    gl_check(glCompileShader(shader_id));

    GLint success = 0;
    gl_check(glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success));
    if(success != GL_TRUE) {
        char buffer[512];
        gl_check(glGetShaderInfoLog(shader_id, array_count(buffer), 0, buffer));
        if(shader_type == GL_VERTEX_SHADER) {
            gl_log("vertex shader: \n");
        } else if(shader_type == GL_FRAGMENT_SHADER) {
            gl_log("fragment shader: \n");
        } else {
            gl_log("unknown shader: \n");
        }
        gl_log(buffer);
        gl_log("\n");
    }
    return shader_id;
}

Shader *create_shader(const char *vertex, size_t vertex_len, const char *fragment, size_t fragment_len) {
    gl_check(gl_id program_id = glCreateProgram());
    gl_id vert_id = compile_shader_source(vertex, vertex_len, GL_VERTEX_SHADER);
    gl_id frag_id = compile_shader_source(fragment, fragment_len, GL_FRAGMENT_SHADER);

    if(glIsShader(vert_id) == GL_FALSE || glIsShader(frag_id) == GL_FALSE || program_id == 0) {
        goto create_shader_fail;
    }
   
    gl_check(glAttachShader(program_id, vert_id));
    gl_check(glAttachShader(program_id, frag_id));
    gl_check(glLinkProgram(program_id));

    GLint success = 0;
    gl_check(glGetProgramiv(program_id, GL_LINK_STATUS, &success));
    if(success != GL_TRUE) {
        char buffer[512];
        gl_check(glGetProgramInfoLog(program_id, array_count(buffer), 0, buffer));
        gl_log("Failed to link program!\n");
        gl_log(buffer);
        gl_log("\n");
        goto create_shader_fail;
    }

    Shader *shader = malloc_and_zero_struct(Shader);
    shader->program_id = program_id;
    shader->vert_id = vert_id;
    shader->frag_id = frag_id;
    return shader;

    // Fail case
create_shader_fail:
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    glDeleteProgram(program_id);
    return NULL;
}

void delete_shader(Shader *shader) {
    if(shader == NULL) return;

    gl_check(glDeleteShader(shader->vert_id));
    gl_check(glDeleteShader(shader->frag_id));
    gl_check(glDeleteProgram(shader->program_id));
    free(shader);
}

void Shader::use(void) {
    gl_check(glUseProgram(this->program_id));
}

bool Shader::set_float(const char *name, float32_t value) {
    this->use();
    gl_check(GLint location = glGetUniformLocation(this->program_id, name));
    if(location == -1) {
        return false;
    }
    gl_check(glUniform1f(location, value));
    return true;
}

bool Shader::set_int(const char *name, int32_t value) {
    this->use();
    gl_check(GLint location = glGetUniformLocation(this->program_id, name));
    if(location == -1) {
        return false;
    }
    gl_check(glUniform1i(location, value));
    return true;
}

bool Shader::set_int_array(const char *name, int32_t *values, int32_t count) {
    this->use();
    gl_check(GLint location = glGetUniformLocation(this->program_id, name));
    if(location == -1) {
        return false;
    }
    gl_check(glUniform1iv(location, count, values));
    return true;
}

bool Shader::set_vec4(const char *name, vec4 value) {
    this->use();
    gl_check(GLint location = glGetUniformLocation(this->program_id, name));
    if(location == -1) {
        return false;
    }
    gl_check(glUniform4f(location, value.x, value.y, value.z, value.w));
    return true;
}

bool Shader::set_mat4x4(const char *name, mat4x4 value) {
    this->use();
    gl_check(GLint location = glGetUniformLocation(this->program_id, name));
    if(location == -1) {
        return false;
    }
    gl_check(glUniformMatrix4fv(location, 1, false, value.e));
    return true;
}

void Shader::set_texture(gl_id texture_id, uint32_t unit) {
    this->use();
    gl_check(glBindTextureUnit(unit, texture_id));
}

VertexBuffer *create_vertex_buffer(void *data, size_t size, gl_enum usage, BufferLayout *layout) {
    gl_id buffer_id = 0;
    gl_check(glGenBuffers(1, &buffer_id));
    if(buffer_id == 0) {
        return NULL;
    }

    gl_check(glBindBuffer(GL_ARRAY_BUFFER, buffer_id));
    gl_check(glBufferData(GL_ARRAY_BUFFER, size, data, usage));

    auto vb = malloc_struct(VertexBuffer);
    assert(vb);
    vb->buffer_id = buffer_id;
    vb->size = size;
    vb->usage = usage;
    vb->layout = *layout;
    return vb;
}

void delete_vertex_buffer(VertexBuffer *vb) {
    if(vb == NULL) return;

    gl_check(glDeleteBuffers(1, &vb->buffer_id));
    free(vb);
}

void VertexBuffer::set_data(void *data, size_t size, int32_t offset) {
    gl_check(glBindBuffer(GL_ARRAY_BUFFER, this->buffer_id));
    gl_check(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

IndexBuffer *create_index_buffer(uint32_t *data, int32_t count) {
    gl_id buffer_id = 0;
    gl_check(glGenBuffers(1, &buffer_id));
    if(buffer_id == 0) {
        return NULL;
    }

    gl_check(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id));
    gl_check(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), data, GL_DYNAMIC_DRAW));

    auto ib = malloc_struct(IndexBuffer);
    assert(ib);
    ib->buffer_id = buffer_id;
    ib->count = count;
    return ib;
}

void delete_index_buffer(IndexBuffer *ib) {
    if(ib == NULL) return;

    gl_check(glDeleteBuffers(1, &ib->buffer_id));
    free(ib);
}

void IndexBuffer::set_data(uint32_t *data, int32_t count, int32_t offset) {
    gl_check(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->buffer_id));
    gl_check(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, count * sizeof(uint32_t), data));
}

VertexArray *create_vertex_array(void) {
    gl_id array_id = 0;
    gl_check(glGenVertexArrays(1, &array_id));
    if(array_id == 0) {
        return NULL;
    }
    
    auto va = malloc_struct(VertexArray);
    assert(va);
    va->array_id = array_id;
    va->next_location = 0;
    va->vb_count = 0;
    va->ib = NULL;
    zero_struct(va->vbs);
    return va;
}

void delete_vertex_array(VertexArray *va) {
    if(va == NULL) return;

    if(va->ib) {
        delete_index_buffer(va->ib);
    }
    for(int32_t idx = 0; idx < va->vb_count; ++idx) {
        delete_vertex_buffer(va->vbs[idx]);
    }
    gl_check(glDeleteVertexArrays(1, &va->array_id));
    free(va);
}

void VertexArray::bind(void) {
    gl_check(glBindVertexArray(this->array_id));
}

void VertexArray::attach_vertex_buffer(VertexBuffer *vb) {
    this->vbs[this->vb_count++] = vb;
    gl_check(glBindVertexArray(this->array_id));
    gl_check(glBindBuffer(GL_ARRAY_BUFFER, vb->buffer_id));

    BufferLayout *layout = &vb->layout;
    for(int32_t element_idx = 0; element_idx < layout->next_element; ++element_idx) {
        BufferLayout::Element *e = &layout->elements[element_idx];

        const uint32_t location = this->next_location + element_idx;
        gl_check(glEnableVertexAttribArray(location));

        switch(e->type) {
            default: assert(0); // JIC

            case BufferLayout::EType::INT32: {
                gl_check(glVertexAttribIPointer(location, e->count, GL_INT, layout->stride, (void *)((uint64_t)e->offset)));
            } break;

            case BufferLayout::EType::FLOAT32: {
                gl_check(glVertexAttribPointer(location, e->count, GL_FLOAT, GL_FALSE, layout->stride, (void *)((uint64_t)e->offset)));
            } break;
        }
    }
    this->next_location += layout->next_element;
}

void VertexArray::set_index_buffer(IndexBuffer *ib) {
    this->ib = ib;
    gl_check(glBindVertexArray(this->array_id));
    gl_check(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->buffer_id));
}

Framebuffer *create_framebuffer(int32_t width, int32_t height) {
    gl_id framebuffer_id = 0;
    gl_check(glCreateFramebuffers(1, &framebuffer_id));
    if(framebuffer_id == 0) {
        return NULL;
    }

    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id));

    Texture *color = create_texture(NULL, width, height, 4, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
    Texture *depth = create_texture(NULL, width, height, 4, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8);
    assert(color && depth);

    if(color == NULL || depth == NULL) {
        return NULL;
    }

    gl_check(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,        GL_TEXTURE_2D, color->texture_id, 0));
    gl_check(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth->texture_id, 0));

    assert(glCheckNamedFramebufferStatus(framebuffer_id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if(glCheckNamedFramebufferStatus(framebuffer_id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return NULL;
    }

    auto fb = malloc_struct(Framebuffer);
    assert(fb);
    fb->framebuffer_id = framebuffer_id;
    fb->width  = width;
    fb->height = height;
    fb->color = color;
    fb->depth = depth;
    return fb;
}

void delete_framebuffer(Framebuffer *fb) {
    if(fb == NULL) return;

    delete_texture(fb->depth);
    delete_texture(fb->color);
    gl_check(glDeleteFramebuffers(1, &fb->framebuffer_id));
    free(fb);
}

void Framebuffer::bind(void) {
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer_id));
}

void unbind_framebuffer(void) {
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void gl_viewport(int32_t x, int32_t y, int32_t w, int32_t h) {
    gl_check(glViewport(x, y, w, h));
}

void gl_viewport(recti32 rect) {
    gl_viewport(rect.x, rect.y, rect.w, rect.h);
}

void gl_clear(vec4 color, uint32_t flags) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(flags);
}

void gl_draw_elements(gl_enum mode, size_t count, gl_enum type, void *indices) {
    gl_check(glDrawElements(mode, count, type, indices));
}

void gl_draw_arrays(gl_enum mode, int32_t first, size_t count) {
    gl_check(glDrawArrays(mode, first, count));
}