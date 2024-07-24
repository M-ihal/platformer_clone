#include "save_data.h"
#include "all_entities.h"
#include <fstream>

#define SERIALIZE(Type) if(entity_type_id == entity_type_id(Type)) { return serialize_entity_##Type; }
entity_serialize_proc *get_serialize_proc(int32_t entity_type_id)   { TO_SERIALIZE return NULL; }
#undef SERIALIZE

#define SERIALIZE(Type) if(entity_type_id == entity_type_id(Type)) { return deserialize_entity_##Type; }
entity_deserialize_proc *get_deserialize_proc(int32_t entity_type_id) { TO_SERIALIZE return NULL; }
#undef SERIALIZE

void load_level_from_save_data(Level *level, LevelSaveData *save_data) {
    if(level->entities.count != 0) {
        recreate_empty_level(&level); // @check
    }

    level->disable_level_timer = save_data->disable_level_timer;
    level->level_music_id[(int32_t)ELevelMusic::regular]                       = save_data->level_music_id[(int32_t)ELevelMusic::regular];
    level->level_music_id[(int32_t)ELevelMusic::during_star_power]             = save_data->level_music_id[(int32_t)ELevelMusic::during_star_power];
    level->level_music_id[(int32_t)ELevelMusic::during_player_enter_pipe_anim] = save_data->level_music_id[(int32_t)ELevelMusic::during_player_enter_pipe_anim];
    level->level_music_id[(int32_t)ELevelMusic::on_level_completed]            = save_data->level_music_id[(int32_t)ELevelMusic::on_level_completed];

    for(auto &es_data : save_data->entity_save_data) {
        int32_t type_id;

        /* Get type id */ {
            std::string type_string;
            if(!es_data.try_get_string("type", &type_string)) {
                printf("Error: Couldn't load entity (corrupted).\n");
                continue;
            } else {
                type_id = entity_type_id_from_string(type_string.c_str());
            }
        }

        entity_deserialize_proc *deserialize_proc = get_deserialize_proc(type_id);
        if(deserialize_proc != NULL) {
            Entity *entity = deserialize_proc(level, &es_data);
            if(entity == NULL) {
                fprintf(stderr, "Failed to load entity <%s>! (possibly corrupted)\n", entity_type_string[type_id]);
                // @todo What should happen, for now just continue
            }
        }
    }
}

std::string generate_level_save_data(Level *level) {
    std::string data = { };

    data += "disable_level_timer : " + std::string(BOOL_STRING(level->disable_level_timer)) + "\n";
    data += "music_id_regular : "  + std::string(music_string[level->level_music_id[(int32_t)ELevelMusic::regular]]) + "\n";
    data += "music_id_star : "     + std::string(music_string[level->level_music_id[(int32_t)ELevelMusic::during_star_power]]) + "\n";
    data += "music_id_cutscene : " + std::string(music_string[level->level_music_id[(int32_t)ELevelMusic::during_player_enter_pipe_anim]]) + "\n";
    data += "music_id_complete : " + std::string(music_string[level->level_music_id[(int32_t)ELevelMusic::on_level_completed]]) + "\n";
    data += "@level_params\n";

    for_every_entity(level, entity) {
        entity_serialize_proc *serialize_proc = get_serialize_proc(entity->entity_type_id);
        if(serialize_proc) {
            // Write common data
            data += "type : " + std::string(entity_type_string[entity->entity_type_id]) + "\n";

            EntitySaveData es_data = serialize_proc(entity);
            for(auto save_value : es_data.save_values) {
                // Write token
                data += save_value.first + " : ";

                // Write values
                bool first = true;
                for(auto &value : save_value.second) {
                    if(!first) {
                        data += ", ";
                    }
                    data += value;
                    first = false;
                }
                data += "\n";
            }
            data += "@next\n";
        }
    }
    return data;
}

LevelSaveData parse_level_save_data(const char *filepath) {
    LevelSaveData level_save_data = { };

    std::ifstream in_file;
    in_file.open(filepath);
    if(!in_file.is_open()) {
        // Could not open the file
        //fprintf(stderr, "Couldn't load %s level file.\n", filepath);
        return level_save_data;
    }

    auto trim_string = [] (std::string *string) {
        string->erase(0, string->find_first_not_of(" \t\n\r\f\v"));
        string->erase(string->find_last_not_of(" \t\n\r\f\v") + 1);
    };

    std::string prop_name  = "";
    std::string prop_value = "";

    EntitySaveData es_data = { };
    while(!in_file.eof()) {
        std::string line;
        std::getline(in_file, line);

        trim_string(&line);

        if(line.empty()) {
            continue;
        }

        size_t idx = line.find_first_of(':');
        if(idx != std::string::npos) {
            prop_name = line.substr(0, idx);
            prop_value = line.substr(idx + 1, line.size() - idx - 1);

            trim_string(&prop_name);
            trim_string(&prop_value);

            std::string token;
            for(char _char : prop_value) {
                if(_char == ',') {
                    trim_string(&token);
                    es_data.save_values[prop_name].push_back(token);
                    token.clear();
                } else {
                    token += _char;
                }
            }

            trim_string(&token);

            if(!token.empty()) {
                es_data.save_values[prop_name].push_back(token);
            }
        } else {
            if(line == "@next") {
                level_save_data.entity_save_data.push_back(es_data);
                es_data = { };
            } else if(line == "@level_params") {
                int32_t music_id_regular  = MUSIC_NONE; // Default
                int32_t music_id_star     = MUSIC_NONE; // Default
                int32_t music_id_cutscene = MUSIC_NONE; // Default
                int32_t music_id_complete = MUSIC_NONE; // Default

                std::string music_id_string = "";

                if(!es_data.try_get_bool("disable_level_timer", &level_save_data.disable_level_timer, 1)) level_save_data.disable_level_timer = false;
                if(es_data.try_get_string("music_id_regular",  &music_id_string)) music_id_regular  = music_id_from_string(music_id_string.c_str());
                if(es_data.try_get_string("music_id_star",     &music_id_string)) music_id_star     = music_id_from_string(music_id_string.c_str());
                if(es_data.try_get_string("music_id_cutscene", &music_id_string)) music_id_cutscene = music_id_from_string(music_id_string.c_str());
                if(es_data.try_get_string("music_id_complete", &music_id_string)) music_id_complete = music_id_from_string(music_id_string.c_str());

                level_save_data.level_music_id[(int32_t)ELevelMusic::regular]                       = music_id_regular  != MUSIC__INVALID ? music_id_regular  : MUSIC_NONE;
                level_save_data.level_music_id[(int32_t)ELevelMusic::during_star_power]             = music_id_star     != MUSIC__INVALID ? music_id_star     : MUSIC_NONE;
                level_save_data.level_music_id[(int32_t)ELevelMusic::during_player_enter_pipe_anim] = music_id_cutscene != MUSIC__INVALID ? music_id_cutscene : MUSIC_NONE;
                level_save_data.level_music_id[(int32_t)ELevelMusic::on_level_completed]            = music_id_complete != MUSIC__INVALID ? music_id_complete : MUSIC_NONE;
            }
        }
    }

    return level_save_data;
}

void load_level(Level *level, const char *level_path, LevelSaveData *out_save_data) {
    LevelSaveData save_data = parse_level_save_data(level_path);
    load_level_from_save_data(level, &save_data);

    if(out_save_data != NULL) {
        *out_save_data = save_data;
    }
}

void EntitySaveData::add_int32(const char *token, int32_t *values, int32_t count) {
    for(int32_t idx = 0; idx < count; ++idx) {
        this->save_values[token].push_back(std::to_string(values[idx]));
    }
}

void EntitySaveData::add_float32(const char *token, float32_t *values, int32_t count) {
    for(int32_t idx = 0; idx < count; ++idx) {
        this->save_values[token].push_back(std::to_string(values[idx]));
    }
}

void EntitySaveData::add_string(const char *token, std::string *strings, int32_t count) {
    for(int32_t idx = 0; idx < count; ++idx) {
        this->save_values[token].push_back(strings[idx]);
    }
}

void EntitySaveData::add_cstring(const char *token, const char *cstring) {
    this->save_values[token].push_back(std::string(cstring));
}

void EntitySaveData::add_cstring(const char *token, const char **cstrings, int32_t count) {
    for(int32_t idx = 0; idx < count; ++idx) {
        this->save_values[token].push_back(std::string(cstrings[idx]));
    }
}

void EntitySaveData::add_bool(const char *token, bool *values, int32_t count) {
    for(int32_t idx = 0; idx < count; ++idx) {
        this->save_values[token].push_back(values[idx] ? "true" : "false");
    }
}

bool EntitySaveData::try_get_int32(const char *token, int32_t *values, int32_t count) {
    auto _int32_values = this->save_values.find(token);
    if(_int32_values == this->save_values.end() || _int32_values->second.size() != count) {
        return false;
    }
    for(int32_t idx = 0; idx < count; ++idx) {
        values[idx] = std::stoi(_int32_values->second[idx]);
    }
    return true;
}

bool EntitySaveData::try_get_float32(const char *token, float32_t *values, int32_t count) {
    auto _float32_values = this->save_values.find(token);
    if(_float32_values == this->save_values.end() || _float32_values->second.size() != count) {
        return false;
    }
    for(int32_t idx = 0; idx < count; ++idx) {
        values[idx] = std::stof(_float32_values->second[idx]);
    }
    return true;
}

bool EntitySaveData::try_get_string(const char *token, std::string *strings, int32_t count) {
    auto _strings = this->save_values.find(token);
    if(_strings == this->save_values.end() || _strings->second.size() != count) {
        return false;
    }
    for(int32_t idx = 0; idx < count; ++idx) {
        strings[idx] = _strings->second[idx];
    }
    return true;
}

bool EntitySaveData::try_get_cstring(const char *token, const char **cstrings, int32_t *sizes, int32_t count) {
    auto _strings = this->save_values.find(token);
    if(_strings == this->save_values.end() || _strings->second.size() != count) {
        return false;
    }
    for(int32_t idx = 0; idx < count; ++idx) {
        strcpy_s((char *)cstrings[idx], sizes[idx], _strings->second[idx].c_str());
    }
    return true;
}

bool EntitySaveData::try_get_cstring(const char *token, const char *cstring, int32_t size) {
    return this->try_get_cstring(token, &cstring, &size, 1);
}

bool EntitySaveData::try_get_bool(const char *token, bool *values, int32_t count) {
    auto _bool_values = this->save_values.find(token);
    if(_bool_values == this->save_values.end() || _bool_values->second.size() != count) {
        return false;
    }
    for(int32_t idx = 0; idx < count; ++idx) {
        if(_bool_values->second[idx] == "true" || _bool_values->second[idx] == "1" || _bool_values->second[idx] == "TRUE" || _bool_values->second[idx] == "True") {
           values[idx] = true;
        } else if(_bool_values->second[idx] == "false" || _bool_values->second[idx] == "0" || _bool_values->second[idx] == "FALSE" || _bool_values->second[idx] == "False") {
            values[idx] = false;
        } else {
            return false;
        }
    }
    return true;
}

bool EntitySaveData::try_get_all(const char *token, std::vector<std::string> *values) {
    auto _values = this->save_values.find(token);
    if(_values == this->save_values.end()) {
        return false;
    }
    *values = _values->second;
    return true;
}