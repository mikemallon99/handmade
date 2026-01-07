#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#include "handmade_position.h"

// Forward declaration to avoid circular dependency
struct tile_room;

enum direction 
{
    Direction_Up, 
    Direction_Down, 
    Direction_Left, 
    Direction_Right,
};

enum entity_type
{
    EntityType_Null,

    // Enemies & Projectiles
    EntityType_Octorok,
    EntityType_Moblin,
    EntityType_OctorokRock,
    EntityType_MoblinArrow,

    // Npcs
    EntityType_OldMan,

    // Random stuff
    EntityType_Fire,
    EntityType_PushBlock,

    // Items
    EntityType_Sword,
    EntityType_BasicKey,

    EntityType_Total,
};

// NOTE: I dont like having to do type stubs, remove if possible
struct entity_tween;

// Unified entity structure - used by all enemies and projectiles
struct entity
{
    bool32 IsActive;
    entity_type Type;
    
    // Position (all entities have this) - local to room, no room coordinates needed
    vector2 P;
    real32 Height;
    real32 Width;
    
    // Room parent - which room this entity belongs to
    tile_room *Room;
    
    // Health system (enemies only, projectiles use IsActive instead)
    uint32 Health;
    uint32 InvincibilityTimer;
    bool32 IFramesFlicker;
    
    // Damage system
    uint32 Damage;  // Amount of damage this entity does on collision (0 = no damage)
    
    // Movement (projectiles use velocity, enemies use direction for AI)
    real32 VelocityX;
    real32 VelocityY;
    vector2 Direction;
    
    // Projectile system
    bool32 IsProjectile;
    uint32 FireFrequency;

    // For Push block
    bool32 IsSolid;
    int32 ConsecutiveCollisionCounter;
    entity_tween *CurrentTween;
};

struct entity_tween
{
    entity *Entity;
    bool32 Active;
    vector2 Path;
    uint32 Length;
    uint32 CurrentFrame;
};

struct entity_tween_queue
{
    entity_tween Tweens[MAX_ENTITY_TWEENS];
    uint32 TweenIndex;
};

#endif

