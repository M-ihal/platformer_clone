#ifndef _ENTITY_H
#define _ENTITY_H

#include "common.h"
#include "maths.h"
#include "input.h"

struct SimInput {
    bool player_move_r;
    bool player_move_l;
    bool player_run;
    bool player_jump;
    bool player_croutch;
    bool player_throw;
    bool player_try_entering_pipe_v;
    bool player_try_entering_pipe_h;
};

// Entity Type declarations
#define ONLY_ENTITY_TYPES
#include "all_entities.h"
#undef  ONLY_ENTITY_TYPES

// Enum of entity types, _EntityTypeID::__COUNT = number of defined game entities
#define ENTITY_TYPE(Type) Type,
enum class _EntityTypeID : int32_t { ENTITY_TYPES __COUNT };
#undef ENTITY_TYPE

#define entity_type_id(Type) (int32_t)(_EntityTypeID::Type)     // Get entity type ID
#define entity_type_count()  (int32_t)(_EntityTypeID::__COUNT)  // Get count of entity types
#define is_entity_type_id_valid(type_id) (type_id >= 0 && type_id < entity_type_count())
#define assert_entity_type(entity_ptr, Type) assert(((Entity *)(entity_ptr))->entity_type_id == entity_type_id(Type))

#define ENTITY_TYPE(Type) #Type,
inline const char *entity_type_string[] = { ENTITY_TYPES };
#undef ENTITY_TYPE

#define ENTITY_TYPE(Type) if(std::string(string) == entity_type_string[entity_type_id(Type)]) return entity_type_id(Type);
inline int32_t entity_type_id_from_string(const char *string) { ENTITY_TYPES return -1; }
#undef ENTITY_TYPE

#define ENTITY_UPDATE_PROC(name) void name(struct Entity *self_base, SimInput input, float64_t delta_time)
#define ENTITY_RENDER_PROC(name) void name(struct Entity *self_base)
#define ENTITY_DELETE_PROC(name) void name(struct Entity *self_base)

typedef ENTITY_UPDATE_PROC(entity_update_proc);
typedef ENTITY_RENDER_PROC(entity_render_proc);
typedef ENTITY_DELETE_PROC(entity_delete_proc);

enum : uint16_t {
    E_FLAG_IS_GAMEPLAY_ENTITY              = 1 << 0, // Is handled by do_move and stuff
    E_FLAG_ALWAYS_BLOCKS_MOVEMENT          = 1 << 1,
    E_FLAG_SLEEPS_UNTIL_AWAKEN             = 1 << 2, // Doesn't get updated while is asleep
    E_FLAG_BLOCKS_MOVEMENT_FROM_ABOVE_ONLY = 1 << 3,
    E_FLAG_DOES_NOT_COLLIDE_WITH_TILES     = 1 << 4,
    E_FLAG_DOES_DAMAGE_TO_PLAYER           = 1 << 5,
    E_FLAG_STOMPABLE                       = 1 << 6,
    E_FLAG_COLLIDES_ONLY_WITH_PLAYER       = 1 << 7 // @todo implement for SideMovingPlatform and stuff maybe
};

struct Entity {
    struct Entity *next;
    struct Entity *next_of_type;
    struct Level  *level;

    bool     deleted; // Will be deleted at the end of update_level procedure
    bool     in_use;  // Is in used entities pool
    int32_t  entity_type_id;
    size_t   size_of_entity;
    uint32_t unique_id;

    template<typename Type> inline constexpr Type *as() { return (Type *)this; }; // @note Cast

    entity_update_proc *update_proc;
    entity_render_proc *render_proc;
    entity_delete_proc *delete_proc;

    struct Collider *has_collider;
    struct MoveData *has_move_data;

    /* Common data */
    bool     is_asleep;
    uint16_t entity_flags;
    vec2i    position;
};

// create_entity returns ptr to new entity with cleared data
#define create_entity_m(level, Type) (Type *)create_entity(level, sizeof(Type), entity_type_id(Type))
Entity *create_entity(struct Level *level, size_t size_of_entity, int32_t entity_type_id);
void delete_entity(Entity *entity);     // Sets entity->deleted to true, the entity is deleted at the end of the update_level procedure
void delete_entity_imm(Entity *entity); // Deletes the entity immediately
#define set_entity_flag(entity_ptr, flag) { (entity_ptr)->entity_flags |= (flag); }

inline 
void make_asleep(Entity *entity) {
    set_entity_flag(entity, E_FLAG_SLEEPS_UNTIL_AWAKEN);
    entity->is_asleep = true;
}

inline
bool is_entity_used(Entity *entity) {
    bool valid = entity->deleted == false && entity->in_use == true;
    return valid;
}

Entity *get_entity_by_unique_id(struct Level *level, uint32_t unique_id);
Entity *get_entity_by_unique_id(struct Level *level, uint32_t unique_id, int32_t entity_type_id);
#define get_entity_by_unique_id_m(level_ptr, unique_id, Type) (Type *)get_entity_by_unique_id(level_ptr, unique_id, entity_type_id(Type))

struct EntityPool {
    uint32_t       count;
    struct Entity *first;
    struct Entity *last;
    uint32_t       count_of_type[entity_type_count()];
    struct Entity *first_of_type[entity_type_count()];
    struct Entity *last_of_type [entity_type_count()];
};

void entity_pool_push  (EntityPool *pool, Entity *entity, int32_t entity_type_id);
void entity_pool_remove(EntityPool *pool, Entity *entity, int32_t entity_type_id);

struct Collider {
    vec2i size;
    vec2i offset;
};

inline vec2i offset_position(Entity *e, Collider *c) {
    return e->position + c->offset;
}

bool aabb(vec2i a_pos, vec2i a_size, vec2i b_pos, vec2i b_size);
bool intersect(Entity *entity_a, Entity *entity_b);

struct FindCollisionsOpts {
    int32_t  entity_type_id = -1;
    uint16_t entity_flags   =  0;
    uint32_t id_to_ignore   =  0;
};

// @todo -> get rid of std::vector
bool is_colliding                    (Level *level, vec2i position, Collider *collider, FindCollisionsOpts opts, vec2i offset = { 0, 0 });
std::vector<Entity *> find_collisions(Level *level, vec2i position, Collider *collider, FindCollisionsOpts opts, vec2i offset = { 0, 0 });
void gather_collisions(std::vector<Entity *> *destination, std::vector<Entity *> found);

struct HitCallbackResult { 
    bool stop_move;
};

HitCallbackResult hit_callback_result(bool stop_move);

enum EAxis {
    NO_AXIS = 0,
    X_AXIS  = 1,
    Y_AXIS  = 2
};

enum ECInfoType {
    NO_MOVE,
    ON_MOVE,
};

struct CollisionInfo {
    ECInfoType type;
    EAxis      axis;
    int32_t    unit; // 1, 0, -1

    // one is NULL
    Entity      *with_entity;
    struct Tile *with_tile;
};

#define HIT_CALLBACK_PROC(name) HitCallbackResult name(Entity *self_base, CollisionInfo collision)
typedef HIT_CALLBACK_PROC(hit_callback_proc);

struct MoveData {
    bool is_grounded;
    bool is_grounded_prev;
    vec2 reminder;
    vec2 speed;
    hit_callback_proc *hit_callback;
};

void zero_move_speed(MoveData *move_data);
void do_move(Entity *entity, MoveData *move_data, float32_t delta_time); // Calls back to entity with hit_callback proc

#endif /* _ENTITY_H */