#include "handmade_entity.h"
#include "handmade_tile.h"
#include "handmade_position.h"

internal entity *
GetNewEntity(entity *Entities)
{
    entity *Entity = 0;

    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *CheckEntity = &Entities[EntityIndex];
        if (!CheckEntity->IsActive)
        {
            Entity = CheckEntity;
            break;
        }
    }
    
    return Entity;
}

internal void
SetEntityTypeDefaults(entity *Entity, entity_type Type, tile_room *Room, uint32 FrameCounter)
{
    entity ZeroEntity = {};
    *Entity = ZeroEntity;
    Entity->Type = Type;
    Entity->Room = Room;
    Entity->SpawnFrame = FrameCounter;
    Entity->IsActive = true;
    Entity->Width = 1.0f;
    Entity->Height = 1.0f;
    
    // Set type-specific defaults
    switch (Type)
    {
        case EntityType_Octorok:
        case EntityType_Moblin:
        {
            Entity->Damage = 1;
            Entity->FireFrequency = 60;
            Entity->Health = 3;
        } break;
        
        case EntityType_OctorokRock:
        case EntityType_MoblinArrow:
        {
            Entity->Width = 0.5f;
            Entity->Height = 1.0f;
            Entity->Damage = 1;
            Entity->IsProjectile = true;
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

internal entity *
AllocateNewEntityInRoom(tile_room *Room, entity_type Type, uint32 FrameCounter)
{
    entity *Entity = 0;

    Entity = GetNewEntity(Room->Entities);
    Assert(Entity);
    SetEntityTypeDefaults(Entity, Type, Room, FrameCounter);

    return Entity;
}

internal entity *
AllocateNewEntityInRoom(tile_map *TileMap, room_id RoomID, entity_type Type, uint32 FrameCounter)
{
    entity *Entity = 0;

    tile_room *Room = &TileMap->TileRooms[RoomID];
    Entity = AllocateNewEntityInRoom(Room, Type, FrameCounter);

    return Entity;
}

internal entity_tween *
GetNewEntityTween(entity_tween_queue *TweenQueue)
{
    entity_tween *Result;

    Result = &TweenQueue->Tweens[TweenQueue->TweenIndex++ % MAX_ENTITY_TWEENS];
    entity_tween ZeroInitializedTween = {};
    *Result = ZeroInitializedTween;

    return Result;
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

internal bool32
IsEntityCollidingWithPlayer(entity *Entity, vector2 PlayerRoomPos)
{
    if (!Entity->IsActive) return false;
    
    // Entities are always in the same room as player (we only iterate PlayerRoom entities)
    return IsHitboxPointActive(PlayerRoomPos, Entity->P, Entity->Width, Entity->Height);
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
UpdatePushBlock(game_state *GameState, entity *Entity)
{
    bool32 IsTweenActive = Entity->CurrentTween && Entity->CurrentTween->Active;
    if (Entity->ConsecutiveCollisionCounter >= 30 && !IsTweenActive)
    {
        // Need to see if push block can move to the next space
        vector2 PushDirection = GameState->PlayerDirection;
        vector2 NewPosition = Entity->P + PushDirection;
        tile_map *TileMap = GameState->World->TileMap;
        if (IsTileRoomPointEmpty(TileMap, Entity->Room, NewPosition))
        {
            entity_tween *EntityTween = GetNewEntityTween(&GameState->EntityTweenQueue);
            EntityTween->Active = true;
            EntityTween->Entity = Entity;
            EntityTween->Path = PushDirection;
            EntityTween->Length = 30;
            EntityTween->CurrentFrame = 0;
            Entity->CurrentTween = EntityTween;
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

        uint32 RandomIndex = GameState->FrameCounter % RandomNumberTableLength;
        uint32 RandomNumber = RandomNumberTable[RandomIndex] % DirArraySize;
        Octorok->Direction = *DirectionArray[RandomNumber];
    }

    // Octorok fire projectile
    if (Octorok->Health > 0 &&
        GameState->FrameCounter % Octorok->FireFrequency == 0)
    {
        entity *Projectile = AllocateNewEntityInRoom(Octorok->Room, EntityType_OctorokRock,
                                                     GameState->FrameCounter);
        Projectile->P = Octorok->P;
        Projectile->Direction = {0.0f, -1.0f};
    }

    if (Octorok->Health == 0)
    {
        Octorok->IsActive = false;
    }
}

internal void
UpdateOctorokProjectile(entity *Projectile)
{
    real32 Speed = 0.1f;
    Projectile->P = Projectile->P + Projectile->Direction * Speed;

    if (Projectile->P.Y < 0)
    {
        Projectile->IsActive = false;
    }
}

internal void
UpdateMoblin(game_state *GameState, entity *Moblin)
{
    // Moblin position update
    // TODO: Set moblin default Y to -1
    Moblin->Direction.X = 0.0f;
    if (Moblin->P.Y > 8)
    {
        Moblin->Direction.Y = -1.0f;
    }
    else if (Moblin->P.Y < 3)
    {
        Moblin->Direction.Y = 1.0f;
    }
    Moblin->P.Y += Moblin->Direction.Y * 0.05f;

    // Moblin fire projectile
    if (GameState->FrameCounter % Moblin->FireFrequency == 0)
    {
        entity *Projectile = AllocateNewEntityInRoom(Moblin->Room, EntityType_MoblinArrow, 
                                                     GameState->FrameCounter);
        if (Projectile)
        {
            Projectile->P = Moblin->P;
            Projectile->Direction = Moblin->Direction;
        }
    }

    if (Moblin->Health == 0)
    {
        Moblin->IsActive = false;
    }
}

internal void
UpdateMoblinProjectile(entity *Projectile)
{
    real32 Speed = 0.1f;
    Projectile->P = Projectile->P + Projectile->Direction * Speed;

    if (Projectile->P.Y < 0)
    {
        Projectile->IsActive = false;
    }
}

internal void
UpdateBomb(entity *Entity, uint32 FrameCounter)
{
    if (FrameCounter - Entity->SpawnFrame > 60)
    {
        // Spawn 9 dust particles
    }
}

internal void
UpdateBombDust(entity *Entity)
{
    // Colliding with door

    // Colliding with entity in room
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

internal void
HandleCollisionPushBlock(game_state *GameState, entity *Entity)
{
    // Need to see if push block can move to the next space
    vector2 PushDirection = GameState->PlayerDirection;
    vector2 NewPosition = Entity->P + PushDirection;
    tile_map *TileMap = GameState->World->TileMap;
    if (IsTileRoomPointEmpty(TileMap, Entity->Room, NewPosition))
    {
        Entity->P = NewPosition;
    }
}

internal void
HandleEntityCollision(game_state *GameState, entity *Entity)
{
    if (Entity->Type == EntityType_PushBlock)
    {
        HandleCollisionPushBlock(GameState, Entity);
    }
}
