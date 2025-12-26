#include "handmade.h"

#include "handmade_random.h"
#include "handmade_overworld.h"
#include "handmade_dungeon.h"
#include "handmade_tile.cpp"
#include "handmade_entity.cpp"
#include "handmade_sprite.cpp"
#include "handmade_position.cpp"


struct game_offscreen_buffer;


internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;

    for (int Y=0; Y < Buffer->Height; Y++) {
        uint32 *Pixel = (uint32 *)Row;
        for (int X=0; X < Buffer->Width; X++) {
            /*
             * Pixel in memory: 00 00 00 00
             * LITTLE ENDIAN ARCHITECTURE
             * LSB APPEARS AT THE END
             * 0x xxRRGGBB
             */

            uint8 Blue = (uint8)(X + XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            uint8 Red = 0;
            uint8 Padding = 0;
            *Pixel++ = (Red << 16 | ((Green << 16) | Blue));
        }

        Row += Buffer->Pitch;
    }
}

internal direction
Vector2ToDirectionEnum(vector2 *Vector2)
{
    direction DirectionEnum = FRONT;
    
    // Determine primary direction based on which component has larger absolute value
    real32 AbsX = (Vector2->X < 0.0f) ? -Vector2->X : Vector2->X;
    real32 AbsY = (Vector2->Y < 0.0f) ? -Vector2->Y : Vector2->Y;
    
    if (AbsY > AbsX)
    {
        // Vertical direction is primary
        if (Vector2->Y > 0.0f)
        {
            DirectionEnum = BACK;
        }
        else
        {
            DirectionEnum = FRONT;
        }
    }
    else
    {
        // Horizontal direction is primary
        if (Vector2->X > 0.0f)
        {
            DirectionEnum = RIGHT;
        }
        else
        {
            DirectionEnum = LEFT;
        }
    }
    
    return DirectionEnum;
}

internal bool32
IsDirectionOpposite(direction DirA, direction DirB)
{
    bool32 Result = false;
    if (DirA == FRONT && DirB == BACK ||
        DirA == BACK && DirB == FRONT ||
        DirA == LEFT && DirB == RIGHT ||
        DirA == RIGHT && DirB == LEFT)
    {
        Result = true;
    }

    return Result;
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer, 
              real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);
    int32 MaxX = RoundReal32ToInt32(RealMaxX);
    int32 MaxY = RoundReal32ToInt32(RealMaxY);

    uint32 Color = (RoundReal32ToUInt32(R * 255.0f) << 16 |
                    RoundReal32ToUInt32(G * 255.0f) << 8 |
                    RoundReal32ToUInt32(B * 255.0f));

    if (MinX < 0)
    {
        MinX = 0;
    }
    if (MinY < 0)
    {
        MinY = 0;
    }
    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8 *Row = ((uint8 *)Buffer->Memory + 
                    MinX*Buffer->BytesPerPixel + 
                    MinY*Buffer->Pitch);
    for (int Y = MinY;
         Y < MaxY;
         Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = MinX;
            X < MaxX;
            X++)
        {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}

internal vector2
WorldToScreen(tile_map *TileMap, vector2 WorldPos, uint32 CameraTileX, real32 PlayAreaY)
{
    vector2 ScreenPos;
    ScreenPos.X = TileMap->TileSideInPixels * (WorldPos.X - (real32)CameraTileX);
    ScreenPos.Y = PlayAreaY - TileMap->TileSideInPixels * (WorldPos.Y - 11.0f);
    return ScreenPos;
}

internal void
DrawDebugPoint(game_offscreen_buffer *Buffer, tile_map *TileMap, real32 PlayAreaY, world_position Point)
{
    // Debug point doesn't account for camera - uses absolute position
    real32 PointX = TileMap->TileSideInPixels * Point.Pos.X;
    real32 PointY = PlayAreaY - TileMap->TileSideInPixels * (Point.Pos.Y - 11.0f);
    DrawRectangle(Buffer, PointX, PointY-2, PointX+2, PointY, 
                    1.0f, 0.0f, 0.0f);
}

internal world_position
GetNewPlayerPos(game_state *GameState, world_position PlayerP, real32 dtForFrame)
{
    // Player movement stuff
    // NOTE: Translate origin to the center of player cuz of legacy calculations
    tile_map_position NewPlayerOrigin = {};
    NewPlayerOrigin.Pos.X = PlayerP.Pos.X;
    NewPlayerOrigin.Pos.Y = PlayerP.Pos.Y;
    tile_map_index TileMapIndex = RoomIDToTileMapIndex(PlayerP.RoomID);
    NewPlayerOrigin.RoomIDX = TileMapIndex.X;
    NewPlayerOrigin.RoomIDY = TileMapIndex.Y;

    tile_map_position NewPlayerP = NewPlayerOrigin;
    NewPlayerP.Pos.X += 0.5f;
    tile_map_position InitialPlayerP = NewPlayerP;

    if (!GameState->PlayerUsingSword && !GameState->PlayerUsingBoomerang)
    {
        vector2 MovementDelta = dtForFrame * GameState->PlayerSpeed * GameState->PlayerDirection;
        NewPlayerP.Pos.X += MovementDelta.X;
        NewPlayerP.Pos.Y += MovementDelta.Y;
    }

    tile_map_position NewPlayerUp = NewPlayerP;
    NewPlayerUp.Pos.Y += 0.1f*GameState->PlayerHeight;
    tile_map_position NewPlayerLeft = NewPlayerP;
    NewPlayerLeft.Pos.X -= 0.5f*GameState->PlayerWidth;
    tile_map_position NewPlayerRight = NewPlayerP;
    NewPlayerRight.Pos.X += 0.5f*GameState->PlayerWidth;

    NewPlayerUp = NewPlayerP;
    NewPlayerUp.Pos.Y += 0.1f*GameState->PlayerHeight;
    NewPlayerLeft = NewPlayerP;
    NewPlayerLeft.Pos.X -= 0.5f*GameState->PlayerWidth;
    NewPlayerRight = NewPlayerP;
    NewPlayerRight.Pos.X += 0.5f*GameState->PlayerWidth;

    // Room transitions
    tile_map *TileMap = GameState->World->TileMap;
    tile_room *TileRoom = GetTileRoom(TileMap, NewPlayerOrigin.RoomIDX, NewPlayerOrigin.RoomIDY);
    bool32 SkipCollisions = false;
    if (IsPointOffscreen(TileMap, NewPlayerUp))
    {
        SkipCollisions = true;
        if (TileRoom->Up)
        {
            NewPlayerP = *TileRoom->Up;
        }
        else
        {
            NewPlayerP.Pos.Y = 0.1f*GameState->PlayerHeight + 0.0001f;
            NewPlayerP.RoomIDY += 1;
        }
    }
    // NOTE: Regular center point is player down
    else if (IsPointOffscreen(TileMap, NewPlayerP))
    {
        SkipCollisions = true;
        if (TileRoom->Down)
        {
            NewPlayerP = *TileRoom->Down;
        }
        else
        {
            NewPlayerP.Pos.Y = (real32)TileMap->RoomHeight - 0.1f*GameState->PlayerHeight - 0.0001f;
            NewPlayerP.RoomIDY -= 1;
        }
    }
    else if (IsPointOffscreen(TileMap, NewPlayerLeft))
    {
        SkipCollisions = true;
        if (TileRoom->Left)
        {
            NewPlayerP = *TileRoom->Left;
        }
        else
        {
            NewPlayerP.Pos.X = (real32)TileMap->RoomWidth - 0.5f*GameState->PlayerWidth - 0.0001f;
            NewPlayerP.RoomIDX -= 1;
        }
    }
    else if (IsPointOffscreen(TileMap, NewPlayerRight))
    {
        SkipCollisions = true;
        if (TileRoom->Right)
        {
            NewPlayerP = *TileRoom->Right;
        }
        else
        {
            NewPlayerP.Pos.X = 0.5f*GameState->PlayerWidth + 0.0001f;
            NewPlayerP.RoomIDX += 1;
        }
    }

    // Collisions
    if (!SkipCollisions)
    {
        // Wall Collisions
        if (IsTileMapPointEmpty(TileMap, NewPlayerUp) &&
            IsTileMapPointEmpty(TileMap, NewPlayerP) &&
            IsTileMapPointEmpty(TileMap, NewPlayerLeft) &&
            IsTileMapPointEmpty(TileMap, NewPlayerRight))
        {
            if (!IsOnSameTile(InitialPlayerP, NewPlayerP))
            {
                uint32 TileValue = GetTileValue(TileMap, NewPlayerP);
                if (TileValue == OW_Entrance)
                {
                    NewPlayerP = GetDoorDestination(TileMap, NewPlayerP);
                }
            }
        }
        else
        {
            // NOTE: Dont change players position
            NewPlayerP = NewPlayerOrigin;
            NewPlayerP.Pos.X += 0.5f;
        }

        if (GameState->PlayerUsingSword)
        {
            tile_map_position SwordPoint = NewPlayerOrigin;
            int32 PixelOffsetX = 0;
            int32 PixelOffsetY = 0;
            direction PlayerDir = Vector2ToDirectionEnum(&GameState->PlayerDirection);
            if (PlayerDir == FRONT)
            {
                // NOTE: XY values taken from sprite sheet
                PixelOffsetX = 26 - 18;
                PixelOffsetY = 73 - 62;
            }
            else if (PlayerDir == RIGHT)
            {
                PixelOffsetX = 44 - 18;
                PixelOffsetY = 86 - 92;
            }
            else if (PlayerDir == BACK)
            {
                PixelOffsetX = 24 - 18;
                PixelOffsetY = 97 - 124;
            }
            else if (PlayerDir == LEFT)
            {
                // This math here is weird cuz of the flippy
                PixelOffsetX = -1*(44 - 18) + 16;
                PixelOffsetY = 86 - 92;
            }
            SwordPoint.Pos.X += (real32)PixelOffsetX / TileMap->MetersToPixels;
            SwordPoint.Pos.Y -= (real32)PixelOffsetY / TileMap->MetersToPixels;
            GameState->SwordPoint = SwordPoint;
        }
    }

    // Lock in player position
    NewPlayerOrigin = NewPlayerP;
    NewPlayerOrigin.Pos.X -= 0.5f;

    // Convert backend position to world_position
    world_position Result = {};
    Result.Pos.X = NewPlayerOrigin.Pos.X;
    Result.Pos.Y = NewPlayerOrigin.Pos.Y;
    TileMapIndex.X = NewPlayerOrigin.RoomIDX;
    TileMapIndex.Y = NewPlayerOrigin.RoomIDY;
    Result.RoomID = TileMapIndexToRoomID(TileMapIndex);

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
    if (IsColliding && !GameState->PlayerPickingUpSword)
    {
        // Don't deactivate yet - keep it active for animation
        GameState->HasSword = true;
        GameState->PlayerPickingUpSword = true;
        GameState->PickupFrame = 0;
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
}

internal void
DrawOctorok(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
            real32 PlayAreaY, uint32 CameraTileX, entity *Octorok, octorok_sprites *OctorokSprites)
{
    vector2 OctoOrigin = WorldToScreen(TileMap, Octorok->P, CameraTileX, PlayAreaY);
    real32 OctoScreenX = OctoOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 OctoScreenY = OctoOrigin.Y - TileMap->TileSideInPixels*1.0f;
    // Origin
    DrawRectangle(Buffer, OctoOrigin.X, OctoOrigin.Y-2, OctoOrigin.X+2, OctoOrigin.Y, 
                    1.0f, 0.0f, 0.0f);

    // Hurt/Invincible rendering
    if (Octorok->InvincibilityTimer > 0)
    {
        if (Octorok->InvincibilityTimer % 6 == 0)
        {
            if (Octorok->IFramesFlicker)
            {
                Octorok->IFramesFlicker = false;
            }
            else
            {
                Octorok->IFramesFlicker = true;
            }
        }
    }
    else 
    {
        Octorok->IFramesFlicker = false;
    }

    if (!Octorok->IFramesFlicker)
    {
        uint32 OctoSpriteIndex = (GameState->FrameCounter & 0x10) == 0x10;
        DrawBMPTile(&OctorokSprites->Front[OctoSpriteIndex], Buffer, OctoScreenX, OctoScreenY);
    }
}

internal void
DrawOverworldRoom(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                 tile_room *Room, uint32 CameraTileX, uint32 CameraTileY,
                 uint32 ScreenTilesWidth, uint32 ScreenTilesHeight, real32 PlayAreaY)
{
    for (uint32 RelRow = 0;
         RelRow < ScreenTilesHeight;
         RelRow++)
    {
        for (uint32 RelColumn = 0;
             RelColumn < ScreenTilesWidth;
             RelColumn++)
        {
            uint32 Column = CameraTileX + RelColumn;
            uint32 Row = CameraTileY - RelRow;
            uint32 TileID = GetTileValue(TileMap, Room->RoomIDX, Room->RoomIDY, Column, Row);
            if (TileID > 0)
            {
                bmp_tile *TileSprite = &GameState->OverworldTileset.Tiles[TileID];
                real32 MinX = (real32)(TileMap->TileSideInPixels * RelColumn);
                real32 MinY = PlayAreaY + (real32)(TileMap->TileSideInPixels * RelRow);
                DrawBMPTile(TileSprite, Buffer, MinX, MinY);
            }
        }
    }
}

internal void
DrawDungeonRoom(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                tile_room *Room, real32 PlayAreaY)
{
    // Layer 1: Draw room border (full screen background)
    // Border is 256x176 pixels, which matches 16x11 tiles at 16px per tile
    DrawBMPTile(&GameState->DungeonTileset.RoomBorder, Buffer, 0.0f, PlayAreaY);
    
    // Layer 2: Draw doors based on room connections
    // Doors are 32x32 pixels (2 tiles x 2 tiles)
    uint32 DoorIndex = 0;  // Could vary based on door state (open/closed/locked)
    
    // Top door (if Up connection exists)
    if (Room->Up)
    {
        real32 DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) / 2.0f - 16.0f;  // Center - half door width
        real32 DoorY = PlayAreaY;
        DrawBMPTile(&GameState->DungeonTileset.DoorsTop[DoorIndex], Buffer, DoorX, DoorY);
    }
    
    // Bottom door (if Down connection exists)
    if (Room->Down)
    {
        real32 DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) / 2.0f - 16.0f;
        real32 DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) - 32.0f;  // Bottom - door height
        DrawBMPTile(&GameState->DungeonTileset.DoorsBottom[DoorIndex], Buffer, DoorX, DoorY);
    }
    
    // Left door (if Left connection exists)
    if (Room->Left)
    {
        real32 DoorX = 0.0f;
        real32 DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) / 2.0f - 16.0f;  // Center vertically - half door height
        DrawBMPTile(&GameState->DungeonTileset.DoorsLeft[DoorIndex], Buffer, DoorX, DoorY);
    }
    
    // Right door (if Right connection exists)
    if (Room->Right)
    {
        real32 DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) - 32.0f;  // Right edge - door width
        real32 DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) / 2.0f - 16.0f;
        DrawBMPTile(&GameState->DungeonTileset.DoorsRight[DoorIndex], Buffer, DoorX, DoorY);
    }
    
    // Layer 3: Draw regular tiles (12x7 dungeon room tiles)
    // Dungeon rooms are 12x7, but screen is 16x11, so we need to center them
    uint32 DungeonRoomWidth = 12;
    uint32 DungeonRoomHeight = 7;
    uint32 TileOffsetX = (TileMap->RoomWidth - DungeonRoomWidth) / 2;   // Center horizontally: (16-12)/2 = 2
    uint32 TileOffsetY = (TileMap->RoomHeight - DungeonRoomHeight) / 2; // Center vertically: (11-7)/2 = 2
    
    for (uint32 RelRow = 0; RelRow < DungeonRoomHeight; RelRow++)
    {
        for (uint32 RelColumn = 0; RelColumn < DungeonRoomWidth; RelColumn++)
        {
            uint32 TileID = GetTileValue(TileMap, Room->RoomIDX, Room->RoomIDY, RelColumn, RelRow);
            if (TileID > 0 && TileID < DN_TileCount)
            {
                bmp_tile *TileSprite = &GameState->DungeonTileset.Tiles[TileID];
                // Offset tiles to center them in the 16x11 screen
                real32 MinX = (real32)(TileMap->TileSideInPixels * (TileOffsetX + RelColumn));
                real32 MinY = PlayAreaY + (real32)(TileMap->TileSideInPixels * (TileOffsetY + RelRow));
                DrawBMPTile(TileSprite, Buffer, MinX, MinY);
            }
        }
    }
}

internal void
DrawOctorokProjectile(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                      real32 PlayAreaY, uint32 CameraTileX, entity *Projectile, octorok_sprites *OctorokSprites)
{
    vector2 ProjOrigin = WorldToScreen(TileMap, Projectile->P, CameraTileX, PlayAreaY);
    real32 ProjScreenX = ProjOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ProjScreenY = ProjOrigin.Y - TileMap->TileSideInPixels*1.0f;
    DrawBMPTile(&OctorokSprites->Projectile, Buffer, ProjScreenX, ProjScreenY);
}

internal void
DrawOldMan(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *OldMan, npc_sprites *NPCSprites)
{
    vector2 OldManOrigin = WorldToScreen(TileMap, OldMan->P, CameraTileX, PlayAreaY);
    real32 OldManScreenX = OldManOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 OldManScreenY = OldManOrigin.Y - TileMap->TileSideInPixels*1.0f;
    
    // NOTE: Old man is not animated
    // uint32 OldManSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&NPCSprites->OldMan[1], Buffer, OldManScreenX, OldManScreenY);
}

internal void
DrawSword(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
          real32 PlayAreaY, uint32 CameraTileX, entity *Sword, link_sprites *LinkSprites)
{
    vector2 SwordOrigin = WorldToScreen(TileMap, Sword->P, CameraTileX, PlayAreaY);
    real32 SwordScreenX = SwordOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 SwordScreenY = SwordOrigin.Y - TileMap->TileSideInPixels*1.0f;
    
    // NOTE: Old man is not animated
    // uint32 OldManSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&LinkSprites->Sword[0], Buffer, SwordScreenX, SwordScreenY);
}

internal void
DrawFire(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *Fire, npc_sprites *NPCSprites)
{
    vector2 FireOrigin = WorldToScreen(TileMap, Fire->P, CameraTileX, PlayAreaY);
    real32 FireScreenX = FireOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 FireScreenY = FireOrigin.Y - TileMap->TileSideInPixels*1.0f;
    
    // Animate between two sprites (idle animation)
    uint32 FireSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&NPCSprites->Fire[FireSpriteIndex], Buffer, FireScreenX, FireScreenY);
}

internal void
DrawMoblin(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *Moblin, moblin_sprites *MoblinSprites)
{
    vector2 MoblinOrigin = WorldToScreen(TileMap, Moblin->P, CameraTileX, PlayAreaY);
    real32 MoblinScreenX = MoblinOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 MoblinScreenY = MoblinOrigin.Y - TileMap->TileSideInPixels*1.0f;
    // Origin
    DrawRectangle(Buffer, MoblinOrigin.X, MoblinOrigin.Y-2, MoblinOrigin.X+2, MoblinOrigin.Y, 
                    1.0f, 0.0f, 0.0f);

    // Hurt/Invincible rendering
    if (Moblin->InvincibilityTimer > 0)
    {
        if (Moblin->InvincibilityTimer % 6 == 0)
        {
            if (Moblin->IFramesFlicker)
            {
                Moblin->IFramesFlicker = false;
            }
            else
            {
                Moblin->IFramesFlicker = true;
            }
        }
    }
    else 
    {
        Moblin->IFramesFlicker = false;
    }

    if (!Moblin->IFramesFlicker)
    {
        direction MoblinDir = Vector2ToDirectionEnum(&Moblin->Direction);
        bmp_tile *MoblinDirSprite;
        if (MoblinDir == FRONT)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Front;
        }
        else if (MoblinDir == BACK)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Back;
        }
        else if (MoblinDir == LEFT)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Left;
        }
        else if (MoblinDir == RIGHT)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Right;
        }
        else
        {
            MoblinDirSprite = 0;
            Assert(0);
        }

        uint32 MoblinSpriteIndex = (GameState->FrameCounter & 0x10) == 0x10;

        DrawBMPTile(&MoblinDirSprite[MoblinSpriteIndex], Buffer, MoblinScreenX, MoblinScreenY);
    }
}

internal void
DrawMoblinProjectile(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                     real32 PlayAreaY, uint32 CameraTileX, entity *Projectile, moblin_sprites *MoblinSprites)
{
    vector2 ProjOrigin = WorldToScreen(TileMap, Projectile->P, CameraTileX, PlayAreaY);
    real32 ProjScreenX = ProjOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ProjScreenY = ProjOrigin.Y - TileMap->TileSideInPixels*1.0f;

    direction ArrowDir = Vector2ToDirectionEnum(&Projectile->Direction);
    bmp_tile *ArrowSprite;
    if (ArrowDir == FRONT)
    {
        ArrowSprite = &MoblinSprites->ArrowFront;
    }
    else if (ArrowDir == BACK)
    {
        ArrowSprite = &MoblinSprites->ArrowBack;
    }
    else if (ArrowDir == LEFT)
    {
        ArrowSprite = &MoblinSprites->ArrowLeft;
    }
    else if (ArrowDir == RIGHT)
    {
        ArrowSprite = &MoblinSprites->ArrowRight;
    }
    else
    {
        ArrowSprite = 0;
        Assert(0);
    }

    DrawBMPTile(ArrowSprite, Buffer, ProjScreenX, ProjScreenY);
}

internal void
DrawBoomerang(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
              real32 PlayAreaY, uint32 CameraTileX)
{
    vector2 BoomerangOrigin = WorldToScreen(TileMap, GameState->BoomerangP.Pos, CameraTileX, PlayAreaY);
    real32 BoomerangScreenX = BoomerangOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 BoomerangScreenY = BoomerangOrigin.Y - TileMap->TileSideInPixels*0.5f;
    
    uint32 SpriteIndex = (GameState->FrameCounter / 5) % 8;
    DrawBMPTile(&GameState->BoomerangSprites.Sprites[SpriteIndex], Buffer, BoomerangScreenX, BoomerangScreenY);
}

internal void
DrawEntity(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *Entity)
{
    if (Entity->Type == EntityType_Octorok)
    {
        DrawOctorok(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                   Entity, &GameState->OctorokSprites);
    }
    else if (Entity->Type == EntityType_Moblin)
    {
        DrawMoblin(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                  Entity, &GameState->MoblinSprites);
    }
    else if (Entity->Type == EntityType_OldMan)
    {
        DrawOldMan(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                  Entity, &GameState->NPCSprites);
    }
    else if (Entity->Type == EntityType_Fire)
    {
        DrawFire(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                  Entity, &GameState->NPCSprites);
    }
    else if (Entity->Type == EntityType_Sword)
    {
        DrawSword(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                  Entity, &GameState->LinkSprites);
    }
    else if (Entity->Type == EntityType_OctorokRock)
    {
        DrawOctorokProjectile(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                             Entity, &GameState->OctorokSprites);
    }
    else if (Entity->Type == EntityType_MoblinArrow)
    {
        DrawMoblinProjectile(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                            Entity, &GameState->MoblinSprites);
    }
}

internal void 
GameOutputSound(game_sound_output_buffer *SoundBuffer, game_state *GameState, int ToneHz)
{
    int16 ToneVolume = 3000;
    int WavePeriod = (int)((real32)(SoundBuffer->SamplesPerSecond)/(real32)(ToneHz));

    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0;
         SampleIndex < SoundBuffer->SampleCount;
         SampleIndex++)
    {
#if 0
        real32 SineValue = Sin(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif

        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

#if 0
        GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
#endif
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(SoundBuffer, GameState, 3000);
}

// NOTE: Assume 60fps for now until we start having problems with that assumption
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;
        TileMap->MapWidth = 16;
        TileMap->MapHeight = 16;
        TileMap->TileRooms = PushArray(&GameState->WorldArena, 
                                       TileMap->MapWidth*TileMap->MapHeight,
                                       tile_room);
        TileMap->RoomWidth = 16;
        TileMap->RoomHeight = 11;

        TileMap->TileSideInMeters = 1.0f;
        TileMap->TileSideInPixels = 16;
        TileMap->MetersToPixels = (real32)TileMap->TileSideInPixels/(real32)TileMap->TileSideInMeters;

        uint32 SpawnRoomX = 7;
        uint32 SpawnRoomY = 0;
        tile_room *TileRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, (char *)HardcodedMap, SpawnRoomX, SpawnRoomY);
        GameState->RoomDebug1 = TileRoom;
        GameState->RoomDebug2 = LoadOverworldRoom(&GameState->WorldArena, TileMap, (char *)HardcodedMap2, SpawnRoomX+1, SpawnRoomY);

        // NOTE: Overworld is 16x8 but we allocate 16x16, so we store extra rooms in the top 16x8 half
        uint32 CaveRoomX = 0;
        uint32 CaveRoomY = 8;
        TileRoom->Door.Pos.X = (real32)TileMap->RoomWidth / 2.0f;
        TileRoom->Door.Pos.Y = 0.5f;
        TileRoom->Door.RoomIDX = CaveRoomX;
        TileRoom->Door.RoomIDY = CaveRoomY;

        tile_room *CaveRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, (char *)CaveMap, CaveRoomX, CaveRoomY);
        CaveRoom->Down = PushStruct(&GameState->WorldArena, tile_map_position);
        CaveRoom->Down->RoomIDX = SpawnRoomX;
        CaveRoom->Down->RoomIDY = SpawnRoomY;
        CaveRoom->Down->Pos.X = 4.5f;
        CaveRoom->Down->Pos.Y = 8.5f;

        // Also whats the point of the Pos.X and Pos.Y stuff?
        uint32 DungeonRoomX = 0;
        uint32 DungeonRoomY = 9;

        // TODO: This seems complicated
        tile_room *DungeonRoom = LoadDungeonRoom(&GameState->WorldArena, TileMap, (char *)DungeonRoom1, DungeonRoomX, DungeonRoomY);
        DungeonRoom->Down = PushStruct(&GameState->WorldArena, tile_map_position);
        DungeonRoom->Down->RoomIDX = SpawnRoomX;
        DungeonRoom->Down->RoomIDY = SpawnRoomY;
        DungeonRoom->Down->Pos.X = 9.0f;
        DungeonRoom->Down->Pos.Y = 10.0f;

        TileRoom->Up = PushStruct(&GameState->WorldArena, tile_map_position);
        TileRoom->Up->RoomIDX = DungeonRoomX;
        TileRoom->Up->RoomIDY = DungeonRoomY;
        TileRoom->Up->Pos.X = 8.0f;
        TileRoom->Up->Pos.Y = 1.0f;

        GameState->PlayerHealth = 6;
        GameState->MaxHealth = 6;
        GameState->HasSword = false;
        GameState->PlayerPickingUpSword = false;
        GameState->PickupFrame = 0;
        GameState->TotalPickupFrames = 30 * 4;
        GameState->CaveTextCharIndex = 0;
        GameState->PlayerDirection = {0.0f, -1.0f}; // FRONT
        GameState->BoomerangMaxDistance = 5.0f;
        GameState->BoomerangSpeed = 8.0f;

        GameState->PlayerP.Pos.X = 5.0f;
        GameState->PlayerP.Pos.Y = 5.0f;
        // NOTE: How should the programmer get the room IDs? When were not a cohesive overworld
        tile_map_index SpawnIndex = {SpawnRoomX, SpawnRoomY};
        GameState->PlayerP.RoomID = TileMapIndexToRoomID(SpawnIndex);

        tile_map_index TileMapIndex = RoomIDToTileMapIndex(GameState->PlayerP.RoomID);

        // Add Octorok1 to entity array
        GameState->Octorok1 = GetNewEntityInRoom(GameState->RoomDebug1);
        SetEntityTypeDefaults(GameState->Octorok1, EntityType_Octorok);
        GameState->Octorok1->Health = 3;
        GameState->Octorok1->P.X = 8;
        GameState->Octorok1->P.Y = 5.0f;
        GameState->Octorok1->Direction.X = -1.0f;
        GameState->Octorok1->Direction.Y = 0.0f;

        // Add Octorok2 to entity array
        GameState->Octorok2 = GetNewEntityInRoom(GameState->RoomDebug1);
        SetEntityTypeDefaults(GameState->Octorok2, EntityType_Octorok);
        GameState->Octorok2->Health = 3;
        GameState->Octorok2->P.X = 9;
        GameState->Octorok2->P.Y = 5.0f;
        GameState->Octorok2->Direction.X = -1.0f;
        GameState->Octorok2->Direction.Y = 0.0f;

        // Add Octorok3 to entity array
        GameState->Octorok3 = GetNewEntityInRoom(GameState->RoomDebug1);
        SetEntityTypeDefaults(GameState->Octorok3, EntityType_Octorok);
        GameState->Octorok3->Health = 3;
        GameState->Octorok3->P.X = 4;
        GameState->Octorok3->P.Y = 5.0f;
        GameState->Octorok3->Direction.X = -1.0f;
        GameState->Octorok3->Direction.Y = 0.0f;

        entity *OldMan = GetNewEntityInRoom(CaveRoom);
        if (OldMan)
        {
            SetEntityTypeDefaults(OldMan, EntityType_OldMan);
            OldMan->P.X = 7.5f;
            OldMan->P.Y = 5.0f;
        }

        entity *Fire1 = GetNewEntityInRoom(CaveRoom);
        if (Fire1)
        {
            SetEntityTypeDefaults(Fire1, EntityType_Fire);
            Fire1->P.X = 5.5f;
            Fire1->P.Y = 5.0f;
        }

        entity *Fire2 = GetNewEntityInRoom(CaveRoom);
        if (Fire2)
        {
            SetEntityTypeDefaults(Fire2, EntityType_Fire);
            Fire2->P.X = 9.5f;
            Fire2->P.Y = 5.0f;
        }

        entity *Sword = GetNewEntityInRoom(CaveRoom);
        if (Sword)
        {
            SetEntityTypeDefaults(Sword, EntityType_Sword);
            Sword->P.X = 7.5f;
            Sword->P.Y = 3.0f;
            GameState->Sword = Sword;
        }

        // Add Moblin to entity array
        GameState->Moblin1 = GetNewEntityInRoom(GameState->RoomDebug1);
        SetEntityTypeDefaults(GameState->Moblin1, EntityType_Moblin);
        GameState->Moblin1->Health = 3;
        GameState->Moblin1->P.X = 6;
        GameState->Moblin1->P.Y = 5.0f;
        GameState->Moblin1->Direction.X = -1.0f;
        GameState->Moblin1->Direction.Y = 0.0f;

        // Add Moblin to entity array
        GameState->Moblin2 = GetNewEntityInRoom(GameState->RoomDebug1);
        SetEntityTypeDefaults(GameState->Moblin2, EntityType_Moblin);
        GameState->Moblin2->Health = 3;
        GameState->Moblin2->P.X = 7;
        GameState->Moblin2->P.Y = 5.0f;
        GameState->Moblin2->Direction.X = -1.0f;
        GameState->Moblin2->Direction.Y = 0.0f;

        // Add Octorok1 to entity array
        GameState->Octorok4 = GetNewEntityInRoom(GameState->RoomDebug2);
        SetEntityTypeDefaults(GameState->Octorok4, EntityType_Octorok);
        GameState->Octorok4->Health = 3;
        GameState->Octorok4->P.X = 8;
        GameState->Octorok4->P.Y = 5.0f;
        GameState->Octorok4->Direction.X = -1.0f;
        GameState->Octorok4->Direction.Y = 0.0f;

        // NOTE: We should probably start a new arena for this image? 
        // Memory->Background = LoadBMPFile(&GameState->WorldArena, Memory, Thread, "test/test_background.bmp");
        // GameState->Background = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
        //                                     "backgrounds_processed/kitchen.bmp");

        GameState->LinkBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "tiles/link.bmp");
        LoadLinkSprites(&GameState->LinkSprites, &GameState->LinkBMP);
        LoadBoomerangSprites(&GameState->BoomerangSprites, &GameState->LinkBMP);

        GameState->NPCBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "tiles/npcs.bmp");
        LoadNPCSprites(&GameState->NPCSprites, &GameState->NPCBMP);

        GameState->OverworldBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/overworld_tileset.bmp");
        LoadOverworldTileset(&GameState->OverworldTileset, &GameState->OverworldBMP);

        GameState->DungeonBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/dungeon_tileset.bmp");
        LoadDungeonTileset(&GameState->DungeonTileset, &GameState->DungeonBMP);

        LoadTextTileset(&GameState->TextTileset, &GameState->OverworldBMP);

        GameState->HudBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/hud_tileset.bmp");
        LoadHudTileset(&GameState->HudTileset, &GameState->HudBMP);

        GameState->OWEnemiesBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/overworld_enemies.bmp");
        
        LoadOctorokSprites(&GameState->OctorokSprites, &GameState->OWEnemiesBMP);

        LoadMoblinSprites(&GameState->MoblinSprites, &GameState->OWEnemiesBMP);

        // NOTE: maybe move this to platform layer
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    GameState->PlayerWidth = 0.75f;
    GameState->PlayerHeight = 1.0f;

    GameState->PlayerSpeed = 0.0f;
    for (int ControllerIndex = 0;
         ControllerIndex < ArrayCount(Input->Controllers);
         ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (!Controller->IsConnected)
        {
            continue;
        }

        if (Controller->IsAnalog)
        {
            // NOTE: Use analog movement tuning
        }
        else
        {
            // Prevent movement during animations
            if (GameState->PlayerPickingUpSword || GameState->PlayerUsingSword || GameState->PlayerUsingBoomerang)
            {
                GameState->PlayerSpeed = 0.0f;
            }
            else
            {
                // NOTE: Use digital movement tuning
                if (Controller->MoveUp.EndedDown)
                {
                    GameState->PlayerDirection = {0.0f, 1.0f}; // BACK
                    GameState->PlayerSpeed = 5.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    GameState->PlayerDirection = {0.0f, -1.0f}; // FRONT
                    GameState->PlayerSpeed = 5.0f;
                }
                if (Controller->MoveLeft.EndedDown)
                {
                    GameState->PlayerDirection = {-1.0f, 0.0f}; // LEFT
                    GameState->PlayerSpeed = 5.0f;
                }
                if (Controller->MoveRight.EndedDown)
                {
                    GameState->PlayerDirection = {1.0f, 0.0f}; // RIGHT
                    GameState->PlayerSpeed = 5.0f;
                }

                if (Controller->ActionUp.EndedDown)
                {
                    GameState->PlayerSpeed = 10.0f;
                }
            }

            // B
            if (Controller->ActionLeft.EndedDown)
            {
                if (GameState->HasSword && !GameState->PlayerUsingSword)
                {
                    GameState->PlayerUsingSword = true;
                    GameState->SwordUsageFrame = -1;
                }
            }
            // A
            if (Controller->ActionDown.EndedDown)
            {
                if (!GameState->PlayerUsingBoomerang)
                {
                    GameState->PlayerUsingBoomerang = true;
                    GameState->BoomerangUsageFrame = 0;
                }
            }
        }

        if (Controller->MoveUp.EndedDown || 
            Controller->MoveDown.EndedDown || 
            Controller->MoveLeft.EndedDown || 
            Controller->MoveRight.EndedDown)
        {
            if (GameState->FrameCounter % 5 == 0)
            {
                if (GameState->WalkStep == 0) 
                {
                    GameState->WalkStep = 1;
                }
                else
                {
                    GameState->WalkStep = 0;
                }
            }
        }
        else
        {
            GameState->WalkStep = 0;
        }
    }

    // Player logic stuff
    if (GameState->PlayerUsingSword)
    {
        GameState->SwordUsageFrame += 1;

        if (GameState->SwordUsageFrame > 30)
        {
            GameState->PlayerUsingSword = false;
        }
    }

    // Update pickup animation
    if (GameState->PlayerPickingUpSword)
    {
        GameState->PickupFrame += 1;
        
        // Update sword entity position using pointer
        if (GameState->Sword && GameState->Sword->IsActive)
        {
            // Calculate animation progress (0.0 to 1.0)
            real32 t = (real32)GameState->PickupFrame / ((real32)GameState->TotalPickupFrames / 4);
            // Dont let it get larger than 1, cuz we want it to freeze above links head
            if (t > 1.0f) t = 1.0f;
            
            // Start position: Link's position (at arm level)
            real32 StartY = GameState->PlayerP.Pos.Y + 0.75f;  // Slightly above Link's feet
            real32 EndY = GameState->PlayerP.Pos.Y + 1.25f;    // Above Link's head
            
            // Interpolate Y position (sword moves upward)
            GameState->Sword->P.Y = StartY + (EndY - StartY) * t;
            
            // Keep X position aligned with Link (centered)
            GameState->Sword->P.X = GameState->PlayerP.Pos.X + 0.25f;  // Center on Link
        }
        
        // Animation duration
        if (GameState->PickupFrame > GameState->TotalPickupFrames)
        {
            // Now deactivate the sword entity
            if (GameState->Sword)
            {
                GameState->Sword->IsActive = false;
            }
            
            GameState->PlayerPickingUpSword = false;
            GameState->PickupFrame = 0;
        }
    }

    if (GameState->PlayerUsingBoomerang)
    {
        // Set position on first frame
        if (GameState->BoomerangUsageFrame == 0)
        {
            GameState->BoomerangStartP = GameState->PlayerP;
            GameState->BoomerangReturning = false;
            GameState->BoomerangP = GameState->PlayerP;
            GameState->BoomerangDirection = GameState->PlayerDirection;
        }

        // Keep boomerang in same room as player
        GameState->BoomerangP.RoomID = GameState->PlayerP.RoomID;

        // Calculate squared distance from start (avoid sqrt for comparison)
        real32 DistanceX = GameState->BoomerangP.Pos.X - GameState->BoomerangStartP.Pos.X;
        real32 DistanceY = GameState->BoomerangP.Pos.Y - GameState->BoomerangStartP.Pos.Y;
        real32 DistanceSq = DistanceX*DistanceX + DistanceY*DistanceY;
        real32 MaxDistanceSq = GameState->BoomerangMaxDistance * GameState->BoomerangMaxDistance;

        if (!GameState->BoomerangReturning)
        {
            // Move boomerang away from player
            if (DistanceSq < MaxDistanceSq)
            {
                GameState->BoomerangP.Pos = GameState->BoomerangP.Pos + Input->dtForFrame * GameState->BoomerangSpeed * GameState->BoomerangDirection;
            }
            else
            {
                // Reached max distance, start returning
                GameState->BoomerangReturning = true;
            }
        }
        else
        {
            // Returning to player
            vector2 ToPlayer;
            ToPlayer.X = GameState->PlayerP.Pos.X - GameState->BoomerangP.Pos.X;
            ToPlayer.Y = GameState->PlayerP.Pos.Y - GameState->BoomerangP.Pos.Y;
            real32 DistToPlayer = Length(ToPlayer);

            if (DistToPlayer > 0.01f)
            {
                // Normalize direction
                ToPlayer.X /= DistToPlayer;
                ToPlayer.Y /= DistToPlayer;
                
                // Move toward player
                GameState->BoomerangP.Pos = GameState->BoomerangP.Pos + Input->dtForFrame * GameState->BoomerangSpeed * ToPlayer;
            }

            // Check if close to player
            if (DistToPlayer < 0.5f)
            {
                // Deactivate boomerang
                GameState->PlayerUsingBoomerang = false;
                GameState->BoomerangUsageFrame = 0;
            }
        }

        GameState->BoomerangUsageFrame += 1;
    }

    // Movement
    GameState->PlayerP = GetNewPlayerPos(GameState, GameState->PlayerP, Input->dtForFrame);

    // Update all entities
    // NOTE: This kinda uses the tile map system, investigate
    tile_map_index TileMapIndex = RoomIDToTileMapIndex(GameState->PlayerP.RoomID);
    tile_room *PlayerRoom = GetTileRoom(TileMap, TileMapIndex.X, TileMapIndex.Y);
    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *Entity = &PlayerRoom->Entities[EntityIndex];
        if (Entity->IsActive)
        {
            UpdateEntity(GameState, Entity);
        }
    }

    // Check entity collisions with player
    // NOTE: I chose to do this after the entity position update, so were not using pos from last frame
    if (GameState->InvincibilityTimer == 0)
    {
        // Convert player position to room position for collision checks
        vector2 PlayerRoomPos;
        PlayerRoomPos.X = GameState->PlayerP.Pos.X;
        PlayerRoomPos.Y = GameState->PlayerP.Pos.Y;
        
        for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
        {
            entity *Entity = &PlayerRoom->Entities[EntityIndex];
            if (Entity->IsActive && IsEntityCollidingWithPlayer(Entity, PlayerRoomPos))
            {
                // Skip if player blocks the projectile
                if (Entity->IsProjectile && !GameState->PlayerUsingSword &&
                    IsDirectionOpposite(Vector2ToDirectionEnum(&GameState->PlayerDirection), Vector2ToDirectionEnum(&Entity->Direction)))
                {
                    // Do nothing
                }
                else if (Entity->Damage > 0)
                {
                    // Apply damage if entity has damage value
                    GameState->InvincibilityTimer = 60;
                    GameState->PlayerHealth -= Entity->Damage;
                }

                // Deactivate projectiles on hit
                if (Entity->Type == EntityType_OctorokRock || Entity->Type == EntityType_MoblinArrow)
                {
                    Entity->IsActive = false;
                }
            }
        }
    }
    else
    {
        GameState->InvincibilityTimer -= 1;
    }

    // Player sword hitting entities
    if (GameState->PlayerUsingSword)
    {
        for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
        {
            entity *Entity = &PlayerRoom->Entities[EntityIndex];
            
            // Only check enemy types (not projectiles)
            if (Entity->IsActive && Entity->Type == EntityType_Octorok || Entity->Type == EntityType_Moblin)
            {
                if (Entity->InvincibilityTimer == 0 && Entity->Health > 0)
                {
                    // Convert sword point to room position for comparison
                    vector2 SwordRoomPos;
                    SwordRoomPos.X = GameState->SwordPoint.Pos.X;
                    SwordRoomPos.Y = GameState->SwordPoint.Pos.Y;
                    
                    bool32 IsEnemyHit = IsHitboxPointActive(SwordRoomPos, 
                                                             Entity->P, 
                                                             Entity->Width, 
                                                             Entity->Height);
                    if (IsEnemyHit)
                    {
                        Entity->InvincibilityTimer = 60;
                        Entity->Health -= 1;
                    }
                }
            }
        }
    }

    // Update invincibility timers
    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *Entity = &PlayerRoom->Entities[EntityIndex];
        if (Entity->IsActive && Entity->InvincibilityTimer > 0)
        {
            Entity->InvincibilityTimer -= 1;
        }
    }

    // RENDERING

    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 
                  0.0f, 0.0f, 0.0f);

    // NOTE: Camera coord is the tile which will be placed in the top left 
    //       coordinate of screen
    uint32 ScreenTilesWidth = 16;
    uint32 ScreenTilesHeight = 11;

    uint32 CameraTileX = 0;
    uint32 CameraTileY = (ScreenTilesHeight - 1);

    real32 PlayAreaY = (real32)Buffer->Height - 11.0f * TileMap->TileSideInPixels;

    // Draw room tiles based on room type
    TileMapIndex = RoomIDToTileMapIndex(GameState->PlayerP.RoomID);
    tile_room *CurrentRoom = GetTileRoom(TileMap, TileMapIndex.X, TileMapIndex.Y);
    if (CurrentRoom && CurrentRoom->Type == RoomType_Overworld)
    {
        DrawOverworldRoom(GameState, Buffer, TileMap, CurrentRoom, 
                         CameraTileX, CameraTileY, ScreenTilesWidth, ScreenTilesHeight, PlayAreaY);
    }
    else if (CurrentRoom && CurrentRoom->Type == RoomType_Dungeon)
    {
        DrawDungeonRoom(GameState, Buffer, TileMap, CurrentRoom, PlayAreaY);
    }

    vector2 HeroOrigin = WorldToScreen(TileMap, GameState->PlayerP.Pos, CameraTileX, PlayAreaY);
    real32 PlayerScreenX = HeroOrigin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 PlayerScreenY = HeroOrigin.Y - TileMap->TileSideInPixels*1.0f;

    // real32 HeroCenterX = 8.0f;
    // real32 HeroCenterY = 16.0f;
    real32 SpriteMinX = PlayerScreenX;
    real32 SpriteMinY = PlayerScreenY;

    bmp_tile *LinkSprite;
    direction PlayerDir = Vector2ToDirectionEnum(&GameState->PlayerDirection);
    
    // Priority: Pickup animation > Sword usage > Normal walking
    if (GameState->PlayerPickingUpSword)
    {
        // Animate between the 2 pickup sprites (15 frames per sprite)
        uint32 PickupSpriteIndex = 1;
        LinkSprite = &GameState->LinkSprites.PickUp[PickupSpriteIndex];
    }
    else if (GameState->PlayerUsingSword)
    {
        uint32 SwordSpriteIndex;
        if (GameState->SwordUsageFrame < 10)
        {
            SwordSpriteIndex = 0;
        }
        else if (GameState->SwordUsageFrame < 20)
        {
            SwordSpriteIndex = 1;
        }
        else if (GameState->SwordUsageFrame <= 30)
        {
            // SwordSpriteIndex = (uint32)GameState->SwordUsageFrame - 4 + 2;
            SwordSpriteIndex = 3;
        }
        else
        {
            SwordSpriteIndex = 0;
            Assert(0);
        }

        if (PlayerDir == FRONT)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordFront[SwordSpriteIndex];
        }
        else if (PlayerDir == BACK)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordBack[SwordSpriteIndex];
        }
        else if (PlayerDir == LEFT)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordLeft[SwordSpriteIndex];
        }
        else if (PlayerDir == RIGHT)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordRight[SwordSpriteIndex];
        }
        else
        {
            LinkSprite = 0;
            Assert(0);
        }
    }
    else
    {
        if (PlayerDir == FRONT)
        {
            LinkSprite = &GameState->LinkSprites.Front[GameState->WalkStep];
        }
        else if (PlayerDir == BACK)
        {
            LinkSprite = &GameState->LinkSprites.Back[GameState->WalkStep];
        }
        else if (PlayerDir == LEFT)
        {
            LinkSprite = &GameState->LinkSprites.Left[GameState->WalkStep];
        }
        else if (PlayerDir == RIGHT)
        {
            LinkSprite = &GameState->LinkSprites.Right[GameState->WalkStep];
        }
        else
        {
            LinkSprite = 0;
            Assert(0);
        }
    }

    // Invincibility rendering
    if (GameState->InvincibilityTimer > 0)
    {
        if (GameState->InvincibilityTimer % 6 == 0)
        {
            if (GameState->IFramesFlicker)
            {
                GameState->IFramesFlicker = false;
            }
            else
            {
                GameState->IFramesFlicker = true;
            }
        }
    }
    else 
    {
        GameState->IFramesFlicker = false;
    }

    if (!GameState->IFramesFlicker)
    {
        DrawBMPTile(LinkSprite, Buffer, SpriteMinX, SpriteMinY);
    }

    DrawDebugPoint(Buffer, TileMap, PlayAreaY, GameState->PlayerP);

    // if (GameState->PlayerUsingSword)
    // {
    //     DrawDebugPoint(Buffer, TileMap, PlayAreaY, GameState->SwordPoint);
    // }

    // Draw Coordinates UI
    // TODO: Make this into a sprintf thing
    uint32 XDigit0 = 0;
    uint32 XDigit1 = 0;
    if (GameState->PlayerP.Pos.X >= 0)
    {
        XDigit0 = FloorReal32ToUInt32(GameState->PlayerP.Pos.X) / 10;
        XDigit1 = FloorReal32ToUInt32(GameState->PlayerP.Pos.X) % 10;
    }
    uint32 XDigit0ASCII = XDigit0;
    if (XDigit0 == 0)
    {
        // Blank space instead of 0
        XDigit0ASCII = 36;
    }
    uint32 XDigit1ASCII = XDigit1;
    DrawBMPTile(&GameState->TextTileset.Tiles[XDigit0ASCII], Buffer, 0, 0);
    DrawBMPTile(&GameState->TextTileset.Tiles[XDigit1ASCII], Buffer, 8, 0);

    // Comma
    DrawBMPTile(&GameState->TextTileset.Tiles[40], Buffer, 16, 0);

    uint32 YDigit0 = 0;
    uint32 YDigit1 = 0;
    if (GameState->PlayerP.Pos.Y >= 0)
    {
        YDigit0 = FloorReal32ToUInt32(GameState->PlayerP.Pos.Y) / 10;
        YDigit1 = FloorReal32ToUInt32(GameState->PlayerP.Pos.Y) % 10;
    }
    uint32 YDigit0ASCII = YDigit0;
    if (YDigit0 == 0)
    {
        // Blank space instead of 0
        YDigit0ASCII = 36;
    }
    uint32 YDigit1ASCII = YDigit1;
    DrawBMPTile(&GameState->TextTileset.Tiles[YDigit0ASCII], Buffer, 24, 0);
    DrawBMPTile(&GameState->TextTileset.Tiles[YDigit1ASCII], Buffer, 32, 0);

    uint8 TestString[] = "IT'S DANGEROUS TO GO ALONE, 420";
    DrawString(Buffer, &GameState->TextTileset, TestString, 0.0f, 8.0f);

    DrawBMPTile(&GameState->TextTileset.Tiles[GameState->PlayerHealth], Buffer, 0, 16);
    // Debug code
    DrawBMPTile(&GameState->TextTileset.Tiles[GameState->Octorok1->Health], Buffer, 32, 16);

    // Draw all entities
    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *Entity = &PlayerRoom->Entities[EntityIndex];
        if (Entity->IsActive)
        {
            DrawEntity(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, Entity);
        }
    }

    // Draw boomerang
    if (GameState->PlayerUsingBoomerang)
    {
        DrawBoomerang(GameState, Buffer, TileMap, PlayAreaY, CameraTileX);
    }

    // Draw cave room text
    if (GameState->PlayerP.RoomID == Room_Overworld_SwordCave)
    {
        // Increment character index every 2 frames (adjust speed here)
        // Only increment when actually in the cave room
        if (GameState->FrameCounter % 2 == 0)
        {
            GameState->CaveTextCharIndex++;
        }
        
        // Position text in world space (tile coordinates)
        vector2 TextWorldPos;
        TextWorldPos.X = 2.0f;  // 2 tiles from left
        TextWorldPos.Y = 8.0f;   // 8 tiles from bottom
        
        // Convert to screen coordinates
        vector2 TextScreenPos = WorldToScreen(TileMap, TextWorldPos, CameraTileX, PlayAreaY);
        
        uint8 Line1[] = "IT'S DANGEROUS TO GO";
        uint8 Line2[] = "ALONE! TAKE THIS.";
        
        int32 Line1Length = StringLength(Line1);
        int32 Line2Length = StringLength(Line2);
        int32 TotalLength = Line1Length + Line2Length;
        
        // Draw text character by character (typewriter effect)
        if (GameState->CaveTextCharIndex <= Line1Length)
        {
            // Still drawing first line
            DrawStringPartial(Buffer, &GameState->TextTileset, Line1, 
                             GameState->CaveTextCharIndex, 
                             TextScreenPos.X, TextScreenPos.Y);
        }
        else
        {
            // First line complete, draw it and start second line
            DrawString(Buffer, &GameState->TextTileset, Line1, TextScreenPos.X, TextScreenPos.Y);
            
            int32 Line2Chars = GameState->CaveTextCharIndex - Line1Length;
            if (Line2Chars > 0)
            {
                DrawStringPartial(Buffer, &GameState->TextTileset, Line2, Line2Chars,
                                 TextScreenPos.X, TextScreenPos.Y + 8.0f);
            }
        }
        
        // Cap at total length (keep text fully displayed once complete)
        if (GameState->CaveTextCharIndex > TotalLength)
        {
            GameState->CaveTextCharIndex = TotalLength;
        }
    }
    else
    {
        // Reset animation when not in cave room (so it restarts when re-entering)
        GameState->CaveTextCharIndex = 0;
    }

    // Draw HUD

    // Draw health: max health, current health
    int32 CurrentHealth = GameState->PlayerHealth;
    int32 MaxHealth = GameState->MaxHealth;
    // Hearts have halves, so health should be multiples of 2
    Assert((MaxHealth % 2) == 0);
    for (int32 i = 0; i < MaxHealth/2; i++)
    {
        int32 LeftoverHealth = CurrentHealth - i*2;
        uint32 HeartIdx = 0;
        // Full Health
        if (LeftoverHealth > 1)
        {
            HeartIdx = 2;
        }
        // Half Health
        else if (LeftoverHealth == 1)
        {
            HeartIdx = 1;
        }
        // Empty Health
        else 
        {
            HeartIdx = 0;
        }

        real32 HeartX = 8.0f * (real32)i;
        DrawBMPTile(&GameState->HudTileset.Hearts[HeartIdx], Buffer, HeartX, 0);
    }

    GameState->FrameCounter++;
}
