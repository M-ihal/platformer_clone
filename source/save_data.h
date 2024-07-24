#ifndef _SAVE_DATA_H
#define _SAVE_DATA_H

#include "common.h"
#include "maths.h"
#include "level.h"

// Serialize function declarations
#define _ENTITY_SERIALIZE_PROC(name)   struct EntitySaveData name(struct Entity *self_base)
#define _ENTITY_DESERIALIZE_PROC(name) struct Entity *name(struct Level *level, struct EntitySaveData *es_data)
typedef _ENTITY_SERIALIZE_PROC(entity_serialize_proc);
typedef _ENTITY_DESERIALIZE_PROC(entity_deserialize_proc);

// For declaring serialize procs
#define ENTITY_SERIALIZE_PROC(Type)   _ENTITY_SERIALIZE_PROC(serialize_entity_##Type)
#define ENTITY_DESERIALIZE_PROC(Type) _ENTITY_DESERIALIZE_PROC(deserialize_entity_##Type)

// @todo Including commas in strings is bad... Should make strings be betwen ""

struct EntitySaveData {
    void add_int32(const char *token, int32_t *values, int32_t count);
    void add_float32(const char *token, float32_t *values, int32_t count);
    void add_string(const char *token, std::string *strings, int32_t count);
    void add_cstring(const char *token, const char *cstring);
    void add_cstring(const char *token, const char **cstrings, int32_t count);
    void add_bool(const char *token, bool *values, int32_t count);

    bool try_get_int32(const char *token, int32_t *values, int32_t count = 1);
    bool try_get_float32(const char *token, float32_t *values, int32_t count = 1);
    bool try_get_string(const char *token, std::string *strings, int32_t count = 1);
    bool try_get_cstring(const char *token, const char **cstrings, int32_t *sizes, int32_t count = 1);
    bool try_get_cstring(const char *token, const char *cstring, int32_t size);
    bool try_get_bool(const char *token, bool *values, int32_t count = 1);

    bool try_get_all(const char *token, std::vector<std::string> *values);

    std::unordered_map<std::string, std::vector<std::string>> save_values;
};

struct LevelSaveData {
    bool disable_level_timer;
    int32_t level_music_id[(int32_t)ELevelMusic::__COUNT];
    std::vector<EntitySaveData> entity_save_data;
};

std::string generate_level_save_data(Level *level);
LevelSaveData parse_level_save_data(const char *filepath);
void load_level_from_save_data(Level *level, LevelSaveData *save_data);
void load_level(Level *level, const char *level_path, LevelSaveData *out_save_data = NULL);

#endif /* _SAVE_DATA_H */