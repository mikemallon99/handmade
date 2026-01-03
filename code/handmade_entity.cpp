#include "handmade_entity.h"
#include "handmade_tile.h"
#include "handmade_position.h"

internal entity *
GetNewEntity(entity *Entities)
{
    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *Entity = &Entities[EntityIndex];
        if (!Entity->IsActive)
        {
            Entity->IsActive = true;
            return Entity;
        }
    }
    
    return 0; // No inactive entities found
}

internal entity *
GetNewEntityInRoom(tile_room *Room)
{
    entity *Entity = GetNewEntity(Room->Entities);
    if (Entity)
    {
        Entity->Room = Room;  // Set room pointer
    }
    return Entity;
}

internal entity *
GetNewEntityInRoom(tile_map *TileMap, room_id RoomID)
{
    entity *Entity = GetNewEntityInRoom(&TileMap->TileRooms[RoomID]);

    return Entity;
}

internal bool32
IsEntityTypeEnemy(entity_type EntityType)
{
    bool32 Result = false;

    switch (EntityType)
    {
        case EntityType_Octorok:
        case EntityType_Moblin:
            Result = true;
            break;
        default:
            Result = false;
            break;
    }

    return Result;
}

internal void
SetEntityTypeDefaults(entity *Entity, entity_type Type)
{
    Entity->Type = Type;
    Entity->IsActive = true;
    Entity->InvincibilityTimer = 0;
    Entity->IFramesFlicker = false;
    Entity->Health = 0;  // Default, override per-instance if needed
    Entity->Width = 1.0f;
    Entity->Height = 1.0f;
    Entity->Damage = 0;
    Entity->IsProjectile = false;
    Entity->FireFrequency = 0;
    
    // Set type-specific defaults
    switch (Type)
    {
        case EntityType_Octorok:
        case EntityType_Moblin:
        {
            Entity->Damage = 1;
            Entity->FireFrequency = 60;
        } break;
        
        case EntityType_OctorokRock:
        case EntityType_MoblinArrow:
        {
            Entity->Width = 0.5f;
            Entity->Height = 1.0f;
            Entity->Damage = 1;
            Entity->IsProjectile = true;
        } break;
        
        case EntityType_OldMan:
        {
            // Uses default width/height
        } break;
        
        case EntityType_Fire:
        {
            Entity->Damage = 1;
        } break;
        
        case EntityType_Sword:
        {
            Entity->Width = 0.5f;
            Entity->Height = 1.0f;
        } break;
    }
}

internal bool32
IsHitboxPointActive(vector2 PointP, vector2 HitboxP,
                    real32 HitboxWidth, real32 HitboxHeight)
{
    vector2 HitboxBotLeft = HitboxP;
    vector2 HitboxTopRight = HitboxP;
    HitboxTopRight.X += HitboxWidth;
    HitboxTopRight.Y += HitboxHeight;

    // AABB checking
    if (PointP.X >= HitboxBotLeft.X && 
        PointP.X <= HitboxTopRight.X &&
        PointP.Y >= HitboxBotLeft.Y &&
        PointP.Y <= HitboxTopRight.Y)
    {
        return true;
    }

    return false;
}

internal entity *
SpawnOctorokProjectile(game_state *GameState, tile_room *Room, vector2 SpawnPosition)
{
    entity *Projectile = GetNewEntityInRoom(Room);
    if (Projectile)
    {
        SetEntityTypeDefaults(Projectile, EntityType_OctorokRock);
        Projectile->P = SpawnPosition;
        // TODO: Make projectiles just use direction
        Projectile->VelocityX = 0.0;
        Projectile->VelocityY = -5.0;
        Projectile->Direction = {0.0f, -1.0f};
    }
    
    return Projectile;
}

internal bool32
IsEntityCollidingWithPlayer(entity *Entity, vector2 PlayerRoomPos)
{
    if (!Entity->IsActive) return false;
    
    // Entities are always in the same room as player (we only iterate PlayerRoom entities)
    return IsHitboxPointActive(PlayerRoomPos, Entity->P, Entity->Width, Entity->Height);
}

internal void
UpdateOldMan(game_state *GameState, entity *OldMan)
{
    // OldMan doesn't move, just stands there
    // Could add idle animation or dialogue triggers here
}

internal void
UpdateSword(game_state *GameState, entity *Sword)
{
    vector2 PlayerRoomPos;
    PlayerRoomPos.X = GameState->PlayerP.Pos.X;
    PlayerRoomPos.Y = GameState->PlayerP.Pos.Y;
    bool32 IsColliding = IsEntityCollidingWithPlayer(Sword, PlayerRoomPos);
    if (IsColliding && !GameState->PlayerPickingUpThing)
    {
        // Don't deactivate yet - keep it active for animation
        GameState->HasSword = true;
        GameState->PlayerPickingUpThing = true;
        GameState->PickUpEntity = Sword;
        GameState->PickupFrame = 0;
    }
}

internal void
UpdateBasicKey(game_state *GameState, entity *BasicKey)
{
    vector2 PlayerRoomPos;
    PlayerRoomPos.X = GameState->PlayerP.Pos.X;
    PlayerRoomPos.Y = GameState->PlayerP.Pos.Y;
    bool32 IsColliding = IsEntityCollidingWithPlayer(BasicKey, PlayerRoomPos);
    if (IsColliding && !GameState->PlayerPickingUpThing)
    {
        // Don't deactivate yet - keep it active for animation
        GameState->KeyInventory += 1;
        GameState->PlayerPickingUpThing = true;
        GameState->PickUpEntity = BasicKey;
        GameState->PickupFrame = 0;
    }
}

internal void
UpdatePushBlock(game_state *GameState, entity *PushBlock)
{
    vector2 PlayerRoomPos;
    PlayerRoomPos.X = GameState->PlayerP.Pos.X;
    PlayerRoomPos.Y = GameState->PlayerP.Pos.Y;
    bool32 IsColliding = IsEntityCollidingWithPlayer(PushBlock, PlayerRoomPos);
    if (IsColliding)
    {
        // Need to see if push block can move to the next space
        vector2 PushDirection = GameState->PlayerDirection;
        vector2 NewPosition = PushBlock->P + PushDirection;
        tile_map *TileMap = GameState->World->TileMap;
        if (IsTileRoomPointEmpty(TileMap, PushBlock->Room, NewPosition))
        {
            PushBlock->P = NewPosition;
        }
    }
}

internal void
UpdateOctorok(game_state *GameState, entity *Octorok)
{
    // New algo: go forward until hit a wall, then turn a random direction
    // Octorok position update
    real32 Speed = 0.05f;
    vector2 PositionDelta = Octorok->Direction * Speed;
    
    vector2 NewPosition;
    NewPosition.X = Octorok->P.X + PositionDelta.X;
    NewPosition.Y = Octorok->P.Y + PositionDelta.Y;
    vector2 NewPositionUp = NewPosition;
    NewPositionUp.Y += Octorok->Height;
    vector2 NewPositionRight = NewPosition;
    NewPositionRight.X += Octorok->Width;
    vector2 NewPositionUpRight = NewPosition;
    NewPositionUpRight.X += Octorok->Width;
    NewPositionUpRight.Y += Octorok->Height;

    tile_map *TileMap = GameState->World->TileMap;
    if (IsTileRoomPointEmpty(TileMap, Octorok->Room, NewPosition) &&
        IsTileRoomPointEmpty(TileMap, Octorok->Room, NewPositionUp) &&
        IsTileRoomPointEmpty(TileMap, Octorok->Room, NewPositionRight) &&
        IsTileRoomPointEmpty(TileMap, Octorok->Room, NewPositionUpRight))
    {
        Octorok->P = NewPosition;
    }
    else
    {
        // What directions are available?
        vector2 UpDir = {0.0f, 1.0f};
        vector2 DownDir = {0.0f, -1.0f};
        vector2 LeftDir = {-1.0f, 0.0f};
        vector2 RightDir = {1.0f, 0.0f};

        vector2 UpPosition;
        UpPosition.X = Octorok->P.X + UpDir.X * Speed;
        UpPosition.Y = Octorok->P.Y + UpDir.Y * Speed;
        vector2 DownPosition;
        DownPosition.X = Octorok->P.X + DownDir.X * Speed;
        DownPosition.Y = Octorok->P.Y + DownDir.Y * Speed;
        vector2 LeftPosition;
        LeftPosition.X = Octorok->P.X + LeftDir.X * Speed;
        LeftPosition.Y = Octorok->P.Y + LeftDir.Y * Speed;
        vector2 RightPosition;
        RightPosition.X = Octorok->P.X + RightDir.X * Speed;
        RightPosition.Y = Octorok->P.Y + RightDir.Y * Speed;

        uint32 DirArraySize = 0;
        vector2 *DirectionArray[4];
        if (IsTileRoomPointEmpty(TileMap, Octorok->Room, UpPosition))
        {
            DirectionArray[DirArraySize++] = &UpDir;
        }
        if (IsTileRoomPointEmpty(TileMap, Octorok->Room, DownPosition))
        {
            DirectionArray[DirArraySize++] = &DownDir;
        }
        if (IsTileRoomPointEmpty(TileMap, Octorok->Room, LeftPosition))
        {
            DirectionArray[DirArraySize++] = &LeftDir;
        }
        if (IsTileRoomPointEmpty(TileMap, Octorok->Room, RightPosition))
        {
            DirectionArray[DirArraySize++] = &RightDir;
        }

        local_persist uint32 RandomIdx = 0;
        uint32 RandomNum = RandomNumberTable[RandomIdx++] % DirArraySize;
        Octorok->Direction = *DirectionArray[RandomNum];
    }

    // Octorok fire projectile
    if (Octorok->Health > 0 &&
        GameState->FrameCounter % Octorok->FireFrequency == 0)
    {
        SpawnOctorokProjectile(GameState, Octorok->Room, Octorok->P);
    }

    if (Octorok->Health == 0)
    {
        Octorok->IsActive = false;
    }
}

internal void
UpdateOctorokProjectile(entity *Projectile)
{
    Projectile->P.X += Projectile->VelocityX/60.0f;
    Projectile->P.Y += Projectile->VelocityY/60.0f;

    if (Projectile->P.Y < 0)
    {
        Projectile->IsActive = false;
    }
}

internal entity *
SpawnMoblinProjectile(game_state *GameState, tile_room *Room, vector2 SpawnPosition, vector2 ArrowDirection)
{
    entity *Projectile = GetNewEntityInRoom(Room);
    if (Projectile)
    {
        SetEntityTypeDefaults(Projectile, EntityType_MoblinArrow);
        Projectile->P = SpawnPosition;
        // TODO: Make projectiles just use direction
        Projectile->VelocityX = 0.0;
        Projectile->VelocityY = -5.0;
        Projectile->Direction = ArrowDirection;
    }
    
    return Projectile;
}

internal void
UpdateMoblin(game_state *GameState, entity *Moblin)
{
    // Moblin position update
    local_persist int32 YDirection = -1;
    if (Moblin->P.Y > 8)
    {
        YDirection = -1;
    }
    else if (Moblin->P.Y < 3)
    {
        YDirection = 1;
    }
    Moblin->Direction.X = 0.0f;
    Moblin->Direction.Y = (real32)YDirection;
    Moblin->P.Y += (real32)YDirection * 0.05f;

    // Moblin fire projectile
    if (GameState->FrameCounter % Moblin->FireFrequency == 0)
    {
        SpawnMoblinProjectile(GameState, Moblin->Room, Moblin->P, Moblin->Direction);
    }

    if (Moblin->Health == 0)
    {
        Moblin->IsActive = false;
    }
}

internal void
UpdateMoblinProjectile(entity *Projectile)
{
    Projectile->P.X += Projectile->VelocityX/60.0f;
    Projectile->P.Y += Projectile->VelocityY/60.0f;

    if (Projectile->P.Y < 0)
    {
        Projectile->IsActive = false;
    }
}

internal void
UpdateEntity(game_state *GameState, entity *Entity)
{
    if (Entity->Type == EntityType_Octorok)
    {
        UpdateOctorok(GameState, Entity);
    }
    else if (Entity->Type == EntityType_Moblin)
    {
        UpdateMoblin(GameState, Entity);
    }
    else if (Entity->Type == EntityType_OctorokRock)
    {
        UpdateOctorokProjectile(Entity);
    }
    else if (Entity->Type == EntityType_MoblinArrow)
    {
        UpdateMoblinProjectile(Entity);
    }
    else if (Entity->Type == EntityType_Sword)
    {
        UpdateSword(GameState, Entity);
    }
    else if (Entity->Type == EntityType_BasicKey)
    {
        UpdateBasicKey(GameState, Entity);
    }
    else if (Entity->Type == EntityType_PushBlock)
    {
        UpdatePushBlock(GameState, Entity);
    }
}
