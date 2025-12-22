#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#include "handmade_position.h"

enum direction {FRONT, BACK, LEFT, RIGHT};

enum entity_type
{
    // Enemies & Projectiles
    EntityType_Octorok,
    EntityType_Moblin,
    EntityType_OctorokRock,
    EntityType_MoblinArrow,

    // Npcs
    EntityType_OldMan,

    // Random stuff
    EntityType_Fire,

    // Items
    EntityType_Sword,
};

// Unified entity structure - used by all enemies and projectiles
struct entity
{
    bool32 IsActive;
    entity_type Type;
    
    // Position (all entities have this)
    tile_map_position P;
    real32 Height;
    real32 Width;
    
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
};

#endif

