#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#include "handmade_position.h"

enum direction {FRONT, BACK, LEFT, RIGHT};

enum entity_type
{
    EntityType_Octorok,
    EntityType_Moblin,
    EntityType_OctorokRock,
    EntityType_MoblinArrow
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
    
    // Movement (projectiles use velocity, enemies use direction for AI)
    real32 VelocityX;
    real32 VelocityY;
    vector2 Direction;
    
    // Projectile system
    uint32 FireFrequency;
};

#endif

