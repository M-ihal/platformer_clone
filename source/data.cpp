#include "data.h"

bool read_file(const char *filepath, void **out_file_data, size_t *out_size, bool null_terminated) {
    assert(out_file_data && out_size);

    FILE *file = NULL;
    if(fopen_s(&file, filepath, "rb") != 0) {
        *out_file_data = NULL;
        *out_size = 0;
        return false;
    }

    size_t file_size = 0;
    void *file_data = NULL;

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if(null_terminated) {
        file_data = malloc(file_size + 1);
        assert(file_data);
        fread(file_data, 1, file_size, file);
        ((char *)file_data)[file_size] = '\0';
    } else {
        file_data = malloc(file_size);
        assert(file_data);
        fread(file_data, 1, file_size, file);
    }

    *out_size = file_size;
    *out_file_data = file_data;

    fclose(file);
    return true;
}

void free_file(void *file_data) {
    free(file_data);
}

bool save_file(const char *filepath, void *buffer, size_t buffer_size) {
    assert(buffer && buffer_size);

    FILE *file = NULL;
    if(fopen_s(&file, filepath, "w") != 0) {
        return false;
    }

    if(fprintf(file, "%*s", buffer_size, buffer) < 0) {
        return false;
    }

    fclose(file);
    return true;
}

#include <filesystem>
typedef std::filesystem::path std_path;

namespace global_data {
    bool      _initialized = false;
    std_path  _data_path   = std_path();
    Texture  *_spritesheet = NULL;
    Sprite    _sprites[SPRITE__COUNT];
    Texture  *_images[IMAGE__COUNT];
    Sound    *_sounds[SOUND__COUNT];
    Music    *_music [MUSIC__COUNT];
    Font     *_small_font;


    void init(void) {
        // Find data directory
        auto search_dir = std::filesystem::current_path();
        for(int32_t idx = 0; idx < 3; ++idx) { // Go 3 levels deep
            for(const auto &directory_entry : std::filesystem::directory_iterator(search_dir)) {
                if(directory_entry.is_directory() && directory_entry.path().filename() == DATA_DIRECTORY_NAME) {
                    _data_path = directory_entry.path();
                    _data_path += std::filesystem::path::preferred_separator;
                    break;
                }
            }
            search_dir = search_dir.parent_path();
        }
        assert(_data_path.empty() == false);
        
        // Load spritesheet
        const std::string spritesheet_path = _data_path.string() + SPRITESHEET_FILENAME;
        _spritesheet = load_texture(spritesheet_path.c_str(), GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA);
        assert(_spritesheet != NULL);

        // Setup defined sprites
#define SPRITE(sprite_name, cell_w, cell_h, cell_x, cell_y, sprite_w, sprite_h) _sprites[sprite_name] = define_sprite(_spritesheet, cell_w, cell_h, cell_x, cell_y, sprite_w, sprite_h);
        SPRITES
#undef SPRITE

        // Load images
#define IMAGE(image_name, image_filepath, data_format, data_type, internal_format) _images[image_name] = load_texture((_data_path.string() + image_filepath).c_str(), data_format, data_type, internal_format);
            ALL_IMAGES;
#undef IMAGE

        // Load sounds
#define SOUND(sound_name, filename) _sounds[sound_name] = load_sound_wav((_data_path.string().append(filename)).c_str());
        ALL_SOUNDS;
#undef SOUND

        // Load music
#define MUSIC(music_name, filename) _music[music_name] = load_music((_data_path.string().append(filename)).c_str());
        ALL_MUSIC;
#undef MUSIC
        _music[MUSIC_NONE] = NULL; // Just to make sure

        _small_font = load_ttf_font((_data_path.string() + SMALL_FONT_FILENAME).c_str(), 8, 256, 256);
    }

    void free(void) {
        _data_path.clear();

        delete_texture(_spritesheet);
        _spritesheet = NULL;
        _initialized = false;
    }

    std::string get_data_path(void) {
        return _data_path.string();
    }

    Texture *get_spritesheet(void) {
        return _spritesheet;
    }

    Sprite get_sprite(int32_t sprite_enum) {
        if(sprite_enum < 0 || sprite_enum >= SPRITE__COUNT) {
            return { };
        }
        return _sprites[sprite_enum];
    }

    Font *get_small_font(void) {
        return _small_font;
    }

    Texture *get_image(int32_t image_enum) {
        if(image_enum < 0 || image_enum >= IMAGE__COUNT) {
            return NULL;
        }
        return _images[image_enum];
    }

    Sound *get_sound(int32_t sound_enum) {
        if(sound_enum < 0 || sound_enum >= SOUND__COUNT) {
            return NULL;
        }
        return _sounds[sound_enum];
    }
    
    Music *get_music(int32_t music_enum) {
        if(music_enum < 0 || music_enum >= MUSIC__COUNT) {
            return NULL;
        }
        return _music[music_enum];
    }
};