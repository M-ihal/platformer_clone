#ifndef _ENTITY_TYPES_DEFINED
#define _ENTITY_TYPES_DEFINED

// Entity declarations
#define ENTITY_TYPE(...)
#define ENTITY_TYPES\
    ENTITY_TYPE(Player)\
    ENTITY_TYPE(Goomba)\
    ENTITY_TYPE(Koopa)\
    ENTITY_TYPE(KoopaShell)\
    ENTITY_TYPE(Piranha)\
    ENTITY_TYPE(Tilemap)\
    ENTITY_TYPE(Mushroom)\
    ENTITY_TYPE(FirePlant)\
    ENTITY_TYPE(Star)\
    ENTITY_TYPE(Fireball)\
    ENTITY_TYPE(TileBreakAnim)\
    ENTITY_TYPE(EnemyFallAnim)\
    ENTITY_TYPE(CameraRegion)\
    ENTITY_TYPE(TimedSprite)\
    ENTITY_TYPE(TimedAnim)\
    ENTITY_TYPE(CoinDropAnim)\
    ENTITY_TYPE(BackgroundPlane)\
    ENTITY_TYPE(BackgroundSprite)\
    ENTITY_TYPE(FireBar)\
    ENTITY_TYPE(KillRegion)\
    ENTITY_TYPE(FlagPole)\
    ENTITY_TYPE(Coin)\
    ENTITY_TYPE(Portal)\
    ENTITY_TYPE(MovingPlatform)\
    ENTITY_TYPE(FloatingText)\
    ENTITY_TYPE(Trigger)\
    ENTITY_TYPE(TriggerableText)\
    ENTITY_TYPE(BackgroundImage)\
    ENTITY_TYPE(Bowser)\
    ENTITY_TYPE(BowserFire)\
    ENTITY_TYPE(CastleLever)\
    ENTITY_TYPE(SideMovingPlatform)
#undef ENTITY_TYPE

#endif /* _ENTITY_TYPES_DEFINED */

#ifndef ONLY_ENTITY_TYPES
#ifndef _ALL_ENTITIES_H
#define _ALL_ENTITIES_H

// Entities that will be serialized, need to have defined ENTITY_SERIALIZE_PROC(Type) and ENTITY_DESERIALIZE_PROC(Type)
#define TO_SERIALIZE\
    SERIALIZE(Player)\
    SERIALIZE(Tilemap)\
    SERIALIZE(Goomba)\
    SERIALIZE(Koopa)\
    SERIALIZE(Piranha)\
    SERIALIZE(CameraRegion)\
    SERIALIZE(BackgroundPlane)\
    SERIALIZE(BackgroundSprite)\
    SERIALIZE(FireBar)\
    SERIALIZE(KillRegion)\
    SERIALIZE(FlagPole)\
    SERIALIZE(Coin)\
    SERIALIZE(Portal)\
    SERIALIZE(MovingPlatform)\
    SERIALIZE(Trigger)\
    SERIALIZE(TriggerableText)\
    SERIALIZE(BackgroundImage)\
    SERIALIZE(Bowser)\
    SERIALIZE(CastleLever)\
    SERIALIZE(Mushroom)\
    SERIALIZE(SideMovingPlatform)

#define Z_PLAYER_POS    0
#define Z_GOOMBA_POS    1
#define Z_KOOPA_POS     1
#define Z_TILEMAP_POS   5
#define Z_MUSHROOM_POS  3
#define Z_FIREPLANT_POS 3
#define Z_STAR_POS      3
#define Z_PIRANHA_POS   8
#define Z_POWERUP_SPAWN_POS 6 // Power-ups during spawn animation
#define Z_BACKGROUND_POS 10
#define Z_BACKGROUND_SPRITE_POS 8
#define Z_FIRE_BAR_POS   5
#define Z_FLAG_POLE_POS  4
#define Z_COIN_POS       2
#define Z_MOVING_PLATFORM_POS 3
#define Z_FLOATING_TEXT_POS -5
#define Z_BACKGROUND_IMAGE_POS  9
#define Z_BOWSER_POS 1
#define Z_BOWSER_FIRE_POS -1
#define Z_CASTLE_LEVER_POS 4

#define Z_FAR_POS  +10
#define Z_NEAR_POS -10

#define POINTS_FOR_PIRANHA           400
#define POINTS_FOR_GOOMBA            300
#define POINTS_FOR_KOOPA             800
#define POINTS_FOR_KOOPA_STOMP       300
#define POINTS_FOR_KOOPA_SHELL       600
#define POINTS_FOR_KOOPA_SHELL_PUSH  100
#define POINTS_FOR_BOWSER            5000
#define POINTS_FOR_POWERUP           500
#define POINTS_PER_FLAG_POLE_TILE    750

#include "entities/player.h"
#include "entities/goomba.h"
#include "entities/koopa.h"
#include "entities/piranha.h"
#include "entities/tilemap.h"
#include "entities/power_up.h"
#include "entities/camera_region.h"
#include "entities/background.h"
#include "entities/fire_bar.h"
#include "entities/level_entities.h"
#include "entities/moving_platform.h"
#include "entities/bowser.h"

// Procs that will be called to initialize file internal variables
#define INIT_FILE_DATA_PROCS\
    IFD_PROC(init_player_common_data)\
    IFD_PROC(init_goomba_common_data)\
    IFD_PROC(init_koopa_common_data)\
    IFD_PROC(init_piranha_common_data)\
    IFD_PROC(init_tilemap_common_data)\
    IFD_PROC(init_power_up_common_data)\
    IFD_PROC(init_fire_bar_common_data)\
    IFD_PROC(init_level_entities_common_data)\
    IFD_PROC(init_bowser_common_data)

#define IFD_PROC(proc_name) void proc_name(void);
INIT_FILE_DATA_PROCS;
#undef IFD_PROC

#define IFD_PROC(proc_name) proc_name();
inline void init_entities_data(void) { INIT_FILE_DATA_PROCS; };
#undef IFD_PROC

#endif /* _ALL_ENTITIES_H */
#endif /* ONLY_ENTITY_TYPES */