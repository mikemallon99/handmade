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
    direction DirectionEnum = Direction_Up;
    
    // Determine primary direction based on which component has larger absolute value
    real32 AbsX = (Vector2->X < 0.0f) ? -Vector2->X : Vector2->X;
    real32 AbsY = (Vector2->Y < 0.0f) ? -Vector2->Y : Vector2->Y;
    
    if (AbsY > AbsX)
    {
        // Vertical direction is primary
        if (Vector2->Y > 0.0f)
        {
            DirectionEnum = Direction_Up;
        }
        else
        {
            DirectionEnum = Direction_Down;
        }
    }
    else
    {
        // Horizontal direction is primary
        if (Vector2->X > 0.0f)
        {
            DirectionEnum = Direction_Right;
        }
        else
        {
            DirectionEnum = Direction_Left;
        }
    }
    
    return DirectionEnum;
}

internal bool32
IsDirectionOpposite(direction DirA, direction DirB)
{
    bool32 Result = false;
    if (DirA == Direction_Up && DirB == Direction_Down ||
        DirA == Direction_Down && DirB == Direction_Up ||
        DirA == Direction_Left && DirB == Direction_Right ||
        DirA == Direction_Right && DirB == Direction_Left)
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

internal void
DrawRectangleHollow(game_offscreen_buffer *Buffer, 
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
        // QUESTION: Are we expecting MaxY to be inclusive in other code?
        if (Y == MinY || Y == MaxY - 1)
        {
            // Line straight across
            uint32 *Pixel = (uint32 *)Row;
            for(int X = MinX;
                X < MaxX;
                X++)
            {
                *Pixel++ = Color;
            }
        }
        else
        {
            // Just 2 pixels on the vertical line
            uint32 *Pixel = (uint32 *)Row;
            *Pixel = Color;
            Pixel += MaxX - MinX - 1;
            *Pixel = Color;
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
DrawDebugPoint(game_offscreen_buffer *Buffer, tile_map *TileMap, real32 PlayAreaY, tile_map_position Point)
{
    // Debug point doesn't account for camera - uses absolute position
    real32 PointX = TileMap->TileSideInPixels * Point.Pos.X;
    real32 PointY = PlayAreaY - TileMap->TileSideInPixels * (Point.Pos.Y - 11.0f);

    DrawRectangle(Buffer, PointX, PointY-2, PointX+2, PointY, 
                    1.0f, 0.0f, 0.0f);
}

internal void
DrawDebugArea2D(game_offscreen_buffer *Buffer, tile_map *TileMap, real32 PlayAreaY, area2d Area)
{
    // Debug point doesn't account for camera - uses absolute position
    real32 TopLeftX = TileMap->TileSideInPixels * Area.BottomLeft.X;
    real32 TopLeftY = PlayAreaY - TileMap->TileSideInPixels * (Area.TopRight.Y - 11.0f);
    real32 BottomRightX = TileMap->TileSideInPixels * Area.TopRight.X;
    real32 BottomRightY = PlayAreaY - TileMap->TileSideInPixels * (Area.BottomLeft.Y - 11.0f);

    // Problem is our areas exist from bottom left to top right
    // But drawing rectangle goes from top left to bottom right
    DrawRectangleHollow(Buffer, TopLeftX, TopLeftY, BottomRightX, BottomRightY, 
                        1.0f, 0.0f, 0.0f);
}

internal area2d
GetArea2D(vector2 Origin, real32 Width, real32 Height)
{
    area2d Result = {};

    Result.BottomLeft = Origin;
    Result.TopRight = Origin;
    Result.TopRight.X += Width;
    Result.TopRight.Y += Height;

    return Result;
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
            uint32 TileID = GetTileValue(TileMap, Room->RoomID, Column, Row);
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
                tile_room *Room, uint32 CameraTileX, uint32 CameraTileY,
                uint32 ScreenTilesWidth, uint32 ScreenTilesHeight, real32 PlayAreaY)
{
    // Layer 1: Draw regular tiles 
    for (uint32 RelRow = 0; RelRow < TileMap->RoomHeight; RelRow++)
    {
        for (uint32 RelColumn = 0; RelColumn < TileMap->RoomWidth; RelColumn++)
        {
            uint32 Column = CameraTileX + RelColumn;
            uint32 Row = CameraTileY - RelRow;
            uint32 TileID = GetTileValue(TileMap, Room->RoomID, Column, Row);
            if (TileID > 0 && TileID < DN_TileCount)
            {
                bmp_tile *TileSprite = &GameState->DungeonTileset.Tiles[TileID];
                // Offset tiles to center them in the 16x11 screen
                real32 MinX = (real32)(TileMap->TileSideInPixels * (RelColumn));
                real32 MinY = PlayAreaY + (real32)(TileMap->TileSideInPixels * (RelRow));
                DrawBMPTile(TileSprite, Buffer, MinX, MinY);
            }
        }
    }

    // Layer 2: Draw room border (full screen background)
    // Border is 256x176 pixels, which matches 16x11 tiles at 16px per tile
    DrawBMPTile(&GameState->DungeonTileset.RoomBorder, Buffer, 0.0f, PlayAreaY);
    
    // Layer 3: Draw doors based on room connections
    // Doors are 32x32 pixels (2 tiles x 2 tiles)
    real32 DoorX = 0;
    real32 DoorY = 0;
    
    // Top door 
    DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) / 2.0f - 16.0f;  // Center - half door width
    DoorY = PlayAreaY;
    DrawBMPTile(&GameState->DungeonTileset.DoorsTop[Room->DungeonDoors[Direction_Up].DoorState], 
                Buffer, DoorX, DoorY);
    
    // Bottom door 
    DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) / 2.0f - 16.0f;
    DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) - 32.0f;  // Bottom - door height
    DrawBMPTile(&GameState->DungeonTileset.DoorsBottom[Room->DungeonDoors[Direction_Down].DoorState], 
                Buffer, DoorX, DoorY);
    
    // Left door 
    DoorX = 0.0f;
    DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) / 2.0f - 16.0f;  // Center vertically - half door height
    DrawBMPTile(&GameState->DungeonTileset.DoorsLeft[Room->DungeonDoors[Direction_Left].DoorState], 
                Buffer, DoorX, DoorY);
    
    // Right door 
    DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) - 32.0f;  // Right edge - door width
    DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) / 2.0f - 16.0f;
    DrawBMPTile(&GameState->DungeonTileset.DoorsRight[Room->DungeonDoors[Direction_Right].DoorState], 
                Buffer, DoorX, DoorY);
}

internal void
DrawDungeonRoomLayer2(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                      tile_room *Room, real32 PlayAreaY)
{
    // Draw the top halves of the doors if theyre opened
    // Doors are 32x32 pixels (2 tiles x 2 tiles)
    real32 DoorX = 0;
    real32 DoorY = 0;
    
    // Top door 
    DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) / 2.0f - 16.0f;  // Center - half door width
    DoorY = PlayAreaY;
    DrawBMPTile(&GameState->DungeonTileset.DoorsTop[Dungeon_Door_TopLayer], 
                Buffer, DoorX, DoorY);
    
    // Bottom door 
    DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) / 2.0f - 16.0f;
    DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) - 32.0f;  // Bottom - door height
    DoorY += 16;
    DrawBMPTile(&GameState->DungeonTileset.DoorsBottom[Dungeon_Door_TopLayer], Buffer, DoorX, DoorY);
    
    // Left door 
    DoorX = 0.0f;
    DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) / 2.0f - 16.0f;  // Center vertically - half door height
    DrawBMPTile(&GameState->DungeonTileset.DoorsLeft[Dungeon_Door_TopLayer], Buffer, DoorX, DoorY);
    
    // Right door 
    DoorX = (real32)(TileMap->RoomWidth * TileMap->TileSideInPixels) - 32.0f;  // Right edge - door width
    DoorX += 16;
    DoorY = PlayAreaY + (real32)(TileMap->RoomHeight * TileMap->TileSideInPixels) / 2.0f - 16.0f;
    DrawBMPTile(&GameState->DungeonTileset.DoorsRight[Dungeon_Door_TopLayer], Buffer, DoorX, DoorY);
}

internal void
DrawOctorokProjectile(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                      real32 PlayAreaY, uint32 CameraTileX, entity *Projectile, octorok_sprites *OctorokSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Projectile->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    DrawBMPTile(&OctorokSprites->Projectile, Buffer, ScreenX, ScreenY);
}

internal void
DrawOldMan(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *OldMan, npc_sprites *NPCSprites)
{
    vector2 Origin = WorldToScreen(TileMap, OldMan->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    // NOTE: Old man is not animated
    // uint32 OldManSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&NPCSprites->OldMan[1], Buffer, ScreenX, ScreenY);
}

internal void
DrawSword(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
          real32 PlayAreaY, uint32 CameraTileX, entity *Sword, link_sprites *LinkSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Sword->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    DrawBMPTile(&LinkSprites->Sword[0], Buffer, ScreenX, ScreenY);
}

internal void
DrawBasicKey(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
             real32 PlayAreaY, uint32 CameraTileX, entity *BasicKey, item_sprites *ItemSprites)
{
    // TODO: Something isnt right about this world to screen stuff
    vector2 Origin = WorldToScreen(TileMap, BasicKey->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    DrawBMPTile(&ItemSprites->BasicKey, Buffer, ScreenX, ScreenY);
}

internal void
DrawPushBlock(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
              real32 PlayAreaY, uint32 CameraTileX, entity *BasicKey, dungeon_tileset *DungeonTileset)
{
    // TODO: Something isnt right about this world to screen stuff
    vector2 Origin = WorldToScreen(TileMap, BasicKey->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    DrawBMPTile(&DungeonTileset->Tiles[DN_Block], Buffer, ScreenX, ScreenY);
}

internal void
DrawFire(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
         real32 PlayAreaY, uint32 CameraTileX, entity *Fire, npc_sprites *NPCSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Fire->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    // Animate between two sprites (idle animation)
    uint32 FireSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&NPCSprites->Fire[FireSpriteIndex], Buffer, ScreenX, ScreenY);
}

internal void
DrawMoblin(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *Moblin, moblin_sprites *MoblinSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Moblin->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    // Origin
    DrawRectangle(Buffer, Origin.X, Origin.Y-2, Origin.X+2, Origin.Y, 
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
        if (MoblinDir == Direction_Up)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Back;
        }
        else if (MoblinDir == Direction_Down)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Front;
        }
        else if (MoblinDir == Direction_Left)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Left;
        }
        else if (MoblinDir == Direction_Right)
        {
            MoblinDirSprite = (bmp_tile *)&MoblinSprites->Right;
        }
        else
        {
            MoblinDirSprite = 0;
            Assert(0);
        }

        uint32 MoblinSpriteIndex = (GameState->FrameCounter & 0x10) == 0x10;

        DrawBMPTile(&MoblinDirSprite[MoblinSpriteIndex], Buffer, ScreenX, ScreenY);
    }
}

internal void
DrawMoblinProjectile(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                     real32 PlayAreaY, uint32 CameraTileX, entity *Projectile, moblin_sprites *MoblinSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Projectile->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;

    direction ArrowDir = Vector2ToDirectionEnum(&Projectile->Direction);
    bmp_tile *ArrowSprite;
    if (ArrowDir == Direction_Up)
    {
        ArrowSprite = &MoblinSprites->ArrowBack;
    }
    else if (ArrowDir == Direction_Down)
    {
        ArrowSprite = &MoblinSprites->ArrowFront;
    }
    else if (ArrowDir == Direction_Left)
    {
        ArrowSprite = &MoblinSprites->ArrowLeft;
    }
    else if (ArrowDir == Direction_Right)
    {
        ArrowSprite = &MoblinSprites->ArrowRight;
    }
    else
    {
        ArrowSprite = 0;
        Assert(0);
    }

    DrawBMPTile(ArrowSprite, Buffer, ScreenX, ScreenY);
}

internal void
DrawBoomerang(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
              real32 PlayAreaY, uint32 CameraTileX)
{
    vector2 Origin = WorldToScreen(TileMap, GameState->BoomerangP.Pos, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*0.5f;
    
    uint32 SpriteIndex = (GameState->FrameCounter / 5) % 8;
    DrawBMPTile(&GameState->BoomerangSprites.Sprites[SpriteIndex], Buffer, ScreenX, ScreenY);
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
    else if (Entity->Type == EntityType_PushBlock)
    {
        DrawPushBlock(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                      Entity, &GameState->DungeonTileset);
    }
    else if (Entity->Type == EntityType_BasicKey)
    {
        DrawBasicKey(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                     Entity, &GameState->ItemSprites);
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
ProcessPlayerInput(game_state *GameState, game_controller_input *Controller)
{
    if (Controller->IsAnalog)
    {
        // NOTE: Use analog movement tuning
    }
    else
    {
        // Prevent movement during animations
        if (GameState->PlayerPickingUpThing || GameState->PlayerUsingSword || GameState->PlayerUsingBoomerang)
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
        // QUESTION: Should we be using a separate arena for sprite data?
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        // PLAYER INIT

        GameState->PlayerHealth = 6;
        GameState->MaxHealth = 6;
        GameState->HasSword = false;
        GameState->PlayerPickingUpThing = false;
        GameState->PickupFrame = 0;
        GameState->TotalPickupFrames = 30 * 4;
        GameState->CaveTextCharIndex = 0;
        GameState->PlayerDirection = {0.0f, -1.0f}; // FRONT
        GameState->BoomerangMaxDistance = 5.0f;
        GameState->BoomerangSpeed = 8.0f;
        GameState->PlayerP.Pos.X = 5.0f;
        GameState->PlayerP.Pos.Y = 5.0f;
        GameState->PlayerP.RoomID = Room_Overworld_Spawn;
        real32 HitboxWidth = 1.0f;
        real32 HitboxHeight = 0.5f;
        GameState->PlayerHitbox = {};
        GameState->PlayerHitbox.BottomLeft.X = 0.1f;
        GameState->PlayerHitbox.TopRight.X = HitboxWidth - 0.1f;
        GameState->PlayerHitbox.TopRight.Y = HitboxHeight;
        GameState->PlayerHitboxCache.BottomLeft = GameState->PlayerHitbox.BottomLeft + 
                                                    GameState->PlayerP.Pos;
        GameState->PlayerHitboxCache.TopRight = GameState->PlayerHitbox.TopRight + 
                                                    GameState->PlayerP.Pos;

        // SPRITE DATA LOADING

        GameState->LinkBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "tiles/link.bmp");
        LoadLinkSprites(&GameState->LinkSprites, &GameState->LinkBMP);
        LoadBoomerangSprites(&GameState->BoomerangSprites, &GameState->LinkBMP);

        GameState->ItemBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "tiles/items.bmp");
        LoadItemSprites(&GameState->ItemSprites, &GameState->ItemBMP);

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

        // WORLD INIT

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;
        TileMap->TileRooms = PushArray(&GameState->WorldArena, 
                                       (int32)Room_Size,
                                       tile_room);
        TileMap->RoomWidth = 16;
        TileMap->RoomHeight = 11;

        TileMap->TileSideInMeters = 1.0f;
        TileMap->TileSideInPixels = 16;
        TileMap->MetersToPixels = (real32)TileMap->TileSideInPixels/(real32)TileMap->TileSideInMeters;

        // SPAWN ROOM INIT

        tile_room *TileRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, 
                                                (char *)HardcodedMap, Room_Overworld_Spawn);
        TileRoom->IsConnectorActive[Direction_Right] = true;
        TileRoom->RoomConnector[Direction_Right].RoomID = Room_Overworld_Bushes;
        TileRoom->RoomConnector[Direction_Right].Pos.X = 0.5f;
        // TODO: Make it so when u enter the room next door u keep the same xy
        TileRoom->RoomConnector[Direction_Right].Pos.Y = 5.0f;

        TileRoom->IsConnectorActive[Direction_Up] = true;
        TileRoom->RoomConnector[Direction_Up].RoomID = Room_Dungeon1_Entrance;
        TileRoom->RoomConnector[Direction_Up].Pos.X = 8.0f;
        TileRoom->RoomConnector[Direction_Up].Pos.Y = 2.5f;

        TileRoom->Door.Pos.X = (real32)TileMap->RoomWidth / 2.0f;
        TileRoom->Door.Pos.Y = 0.5f;
        TileRoom->Door.RoomID = Room_Overworld_SwordCave;

        entity *Octorok1 = GetNewEntityInRoom(TileMap, Room_Overworld_Spawn);
        SetEntityTypeDefaults(Octorok1, EntityType_Octorok);
        Octorok1->Health = 3;
        Octorok1->P.X = 8;
        Octorok1->P.Y = 5.0f;
        Octorok1->Direction.X = -1.0f;
        Octorok1->Direction.Y = 0.0f;

        entity *Octorok2 = GetNewEntityInRoom(TileMap, Room_Overworld_Spawn);
        SetEntityTypeDefaults(Octorok2, EntityType_Octorok);
        Octorok2->Health = 3;
        Octorok2->P.X = 9;
        Octorok2->P.Y = 5.0f;
        Octorok2->Direction.X = -1.0f;
        Octorok2->Direction.Y = 0.0f;

        entity *Octorok3 = GetNewEntityInRoom(TileMap, Room_Overworld_Spawn);
        SetEntityTypeDefaults(Octorok3, EntityType_Octorok);
        Octorok3->Health = 3;
        Octorok3->P.X = 4;
        Octorok3->P.Y = 5.0f;
        Octorok3->Direction.X = -1.0f;
        Octorok3->Direction.Y = 0.0f;

        entity *Moblin1 = GetNewEntityInRoom(TileMap, Room_Overworld_Spawn);
        SetEntityTypeDefaults(Moblin1, EntityType_Moblin);
        Moblin1->Health = 3;
        Moblin1->P.X = 6;
        Moblin1->P.Y = 5.0f;
        Moblin1->Direction.X = -1.0f;
        Moblin1->Direction.Y = 0.0f;

        entity *Moblin2 = GetNewEntityInRoom(TileMap, Room_Overworld_Spawn);
        SetEntityTypeDefaults(Moblin2, EntityType_Moblin);
        Moblin2->Health = 3;
        Moblin2->P.X = 7;
        Moblin2->P.Y = 5.0f;
        Moblin2->Direction.X = -1.0f;
        Moblin2->Direction.Y = 0.0f;

        // BUSH ROOM INIT

        TileRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, 
                                     (char *)HardcodedMap2, Room_Overworld_Bushes);

        TileRoom->IsConnectorActive[Direction_Left] = true;
        TileRoom->RoomConnector[Direction_Left].RoomID = Room_Overworld_Spawn;
        TileRoom->RoomConnector[Direction_Left].Pos.X = 15.0f;
        TileRoom->RoomConnector[Direction_Left].Pos.Y = 5.0f;

        entity *Octorok4 = GetNewEntityInRoom(TileMap, Room_Overworld_Bushes);
        SetEntityTypeDefaults(Octorok4, EntityType_Octorok);
        Octorok4->Health = 3;
        Octorok4->P.X = 8;
        Octorok4->P.Y = 5.0f;
        Octorok4->Direction.X = -1.0f;
        Octorok4->Direction.Y = 0.0f;

        // OLD MAN CAVE ROOM INIT

        tile_room *CaveRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, 
                                                (char *)CaveMap, Room_Overworld_SwordCave);
        CaveRoom->IsConnectorActive[Direction_Down] = true;
        CaveRoom->RoomConnector[Direction_Down].RoomID = Room_Overworld_Spawn;
        CaveRoom->RoomConnector[Direction_Down].Pos.X = 4.5f;
        CaveRoom->RoomConnector[Direction_Down].Pos.Y = 8.5f;

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

        // DUNGEON 1 ENTRANCE INIT

        tile_room *DungeonRoom = LoadDungeonRoom(&GameState->WorldArena, TileMap, (char *)DungeonRoom1, Room_Dungeon1_Entrance);

        DungeonRoom->IsConnectorActive[Direction_Down] = true;
        DungeonRoom->RoomConnector[Direction_Down].RoomID = Room_Overworld_Spawn;
        DungeonRoom->RoomConnector[Direction_Down].Pos.X = 9.0f;
        DungeonRoom->RoomConnector[Direction_Down].Pos.Y = 10.0f;
        DungeonRoom->DungeonDoors[Direction_Down].DoorState = Dungeon_Door_Open;

        DungeonRoom->IsConnectorActive[Direction_Up] = true;
        DungeonRoom->RoomConnector[Direction_Up].RoomID = Room_Dungeon1_Two;
        DungeonRoom->RoomConnector[Direction_Up].Pos.X = 8.0f;
        DungeonRoom->RoomConnector[Direction_Up].Pos.Y = 2.5f;
        DungeonRoom->DungeonDoors[Direction_Up].DoorState = Dungeon_Door_Locked;

        entity *BasicKey = GetNewEntityInRoom(DungeonRoom);
        if (BasicKey)
        {
            SetEntityTypeDefaults(BasicKey, EntityType_BasicKey);
            BasicKey->P.X = 9.5f;
            BasicKey->P.Y = 3.0f;
        }

        entity *PushBlock = GetNewEntityInRoom(DungeonRoom);
        if (PushBlock)
        {
            SetEntityTypeDefaults(PushBlock, EntityType_PushBlock);
            PushBlock->P.X = 8.0f;
            PushBlock->P.Y = 5.0f;
            PushBlock->IsSolid = true;
        }

        // DUNGEON 2 TEST ROOM INIT

        DungeonRoom = LoadDungeonRoom(&GameState->WorldArena, TileMap, (char *)DungeonRoom1, Room_Dungeon1_Two);
        DungeonRoom->IsConnectorActive[Direction_Down] = true;
        DungeonRoom->RoomConnector[Direction_Down].RoomID = Room_Dungeon1_Entrance;
        DungeonRoom->RoomConnector[Direction_Down].Pos.X = 8.0f;
        DungeonRoom->RoomConnector[Direction_Down].Pos.Y = 8.5f;
        DungeonRoom->DungeonDoors[Direction_Down].DoorState = Dungeon_Door_Shut;

        entity *DungeonOctorok = GetNewEntityInRoom(TileMap, Room_Dungeon1_Two);
        SetEntityTypeDefaults(DungeonOctorok, EntityType_Octorok);
        DungeonOctorok->Health = 3;
        DungeonOctorok->P.X = 8;
        DungeonOctorok->P.Y = 5.0f;
        DungeonOctorok->Direction.X = -1.0f;
        DungeonOctorok->Direction.Y = 0.0f;

        // NOTE: maybe move this to platform layer
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    // Update all entities
    tile_room *PlayerRoom = GetTileRoom(TileMap, GameState->PlayerP.RoomID);
    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *Entity = &PlayerRoom->Entities[EntityIndex];
        // Re-initialize colliding flag
        if (Entity->IsActive)
        {
            UpdateEntity(GameState, Entity);
        }
    }

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

        if (!GameState->BlockPlayerInput)
        {
            ProcessPlayerInput(GameState, Controller);
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

    // Walking animation update
    if (GameState->PlayerSpeed > 0)
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

    // Update pickup animation
    if (GameState->PlayerPickingUpThing)
    {
        GameState->PickupFrame += 1;
        
        // Update entity position using pointer
        if (GameState->PickUpEntity && GameState->PickUpEntity->IsActive)
        {
            // Calculate animation progress (0.0 to 1.0)
            real32 t = (real32)GameState->PickupFrame / ((real32)GameState->TotalPickupFrames / 4);
            // Dont let it get larger than 1, cuz we want it to freeze above links head
            if (t > 1.0f) t = 1.0f;
            
            // Start position: Link's position (at arm level)
            real32 StartY = GameState->PlayerP.Pos.Y + 0.75f;  // Slightly above Link's feet
            real32 EndY = GameState->PlayerP.Pos.Y + 1.25f;    // Above Link's head
            
            // Interpolate Y position (item moves upward)
            GameState->PickUpEntity->P.Y = StartY + (EndY - StartY) * t;
            
            // Keep X position aligned with Link (centered)
            GameState->PickUpEntity->P.X = GameState->PlayerP.Pos.X + 0.25f;  // Center on Link
        }
        
        // Animation duration
        if (GameState->PickupFrame > GameState->TotalPickupFrames)
        {
            // Now deactivate the entity
            if (GameState->PickUpEntity)
            {
                GameState->PickUpEntity->IsActive = false;
            }
            
            GameState->PlayerPickingUpThing = false;
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
    // NOTE: Translate origin to the center of player cuz of legacy calculations
    tile_map_position InitialPlayerOrigin = GameState->PlayerP;
    tile_map_position NewPlayerOrigin = GameState->PlayerP;

    if (!GameState->PlayerUsingSword && !GameState->PlayerUsingBoomerang)
    {
        vector2 MovementDelta = Input->dtForFrame * GameState->PlayerSpeed * GameState->PlayerDirection;
        NewPlayerOrigin.Pos.X += MovementDelta.X;
        NewPlayerOrigin.Pos.Y += MovementDelta.Y;
    }

    // Room transitions
    TileMap = GameState->World->TileMap;
    tile_room *TileRoom = GetTileRoom(TileMap, NewPlayerOrigin.RoomID);
    bool32 SkipCollisions = false;
    bool32 UpdatePosition = true;

    // Take the original hitbox and offset it by position
    area2d NewPlayerHitbox = GameState->PlayerHitbox;
    NewPlayerHitbox.BottomLeft = NewPlayerHitbox.BottomLeft + NewPlayerOrigin.Pos;
    NewPlayerHitbox.TopRight = NewPlayerHitbox.TopRight + NewPlayerOrigin.Pos;

    if (IsAreaOffscreen(TileMap, NewPlayerHitbox))
    {
        direction DirectionIndex = GetOffscreenDirection(TileMap, NewPlayerHitbox);
        if (TileRoom->IsConnectorActive[DirectionIndex])
        {
            SkipCollisions = true;
            NewPlayerOrigin = TileRoom->RoomConnector[DirectionIndex];
        }
        else
        {
            UpdatePosition = false;
        }
    }

    if (TileRoom->Type == RoomType_Dungeon)
    {
        for (int32 DoorIndex = 0;
             DoorIndex < 4;
             DoorIndex++)
        {
            dungeon_door *DungeonDoor = &TileRoom->DungeonDoors[DoorIndex];
            if (IsAreaInArea(NewPlayerHitbox, DungeonDoor->DoorArea))
            {
                if (DungeonDoor->DoorState == Dungeon_Door_Open)
                {
                    SkipCollisions = true;
                }
                else if (DungeonDoor->DoorState == Dungeon_Door_Locked && 
                         GameState->KeyInventory > 0)
                {
                    GameState->KeyInventory -= 1;
                    DungeonDoor->DoorState = Dungeon_Door_Open;
                }
            }
        }
    }

    // Collisions
    if (!SkipCollisions)
    {
        vector2 BottomLeft = NewPlayerHitbox.BottomLeft;
        vector2 BottomRight = {};
        BottomRight.X = NewPlayerHitbox.TopRight.X;
        BottomRight.Y = NewPlayerHitbox.BottomLeft.Y;
        vector2 TopLeft = {};
        TopLeft.X = NewPlayerHitbox.BottomLeft.X;
        TopLeft.Y = NewPlayerHitbox.TopRight.Y;
        vector2 TopRight = NewPlayerHitbox.TopRight;

        // Wall Collisions
        if (IsTileRoomPointEmpty(TileMap, TileRoom, BottomLeft) &&
            IsTileRoomPointEmpty(TileMap, TileRoom, BottomRight) &&
            IsTileRoomPointEmpty(TileMap, TileRoom, TopLeft) &&
            IsTileRoomPointEmpty(TileMap, TileRoom, TopRight))
        {
            uint32 TileValueBottomLeft = GetTileValue(TileMap, TileRoom->RoomID, BottomLeft);
            uint32 TileValueBottomRight = GetTileValue(TileMap, TileRoom->RoomID, BottomRight);
            uint32 TileValueTopLeft = GetTileValue(TileMap, TileRoom->RoomID, TopLeft);
            uint32 TileValueTopRight = GetTileValue(TileMap, TileRoom->RoomID, TopRight);
            if (TileValueBottomLeft == OW_Entrance)
            {
                NewPlayerOrigin = GetDoorDestination(TileMap, TileRoom, BottomLeft);
            }
            else if (TileValueBottomRight == OW_Entrance)
            {
                NewPlayerOrigin = GetDoorDestination(TileMap, TileRoom, BottomRight);
            }
            else if (TileValueTopLeft == OW_Entrance)
            {
                NewPlayerOrigin = GetDoorDestination(TileMap, TileRoom, TopLeft);
            }
            else if (TileValueTopRight == OW_Entrance)
            {
                NewPlayerOrigin = GetDoorDestination(TileMap, TileRoom, TopRight);
            }
            // NOTE: Commenting this out cuz i feel like ill understand why it was like this in the future
            // if (!IsOnSameTile(InitialPlayerOrigin, NewPlayerOrigin))
            // {
            //     uint32 TileValue = GetTileValue(TileMap, NewPlayerOrigin);
            //     if (TileValue == OW_Entrance)
            //     {
            //         NewPlayerOrigin = GetDoorDestination(TileMap, NewPlayerOrigin);
            //     }
            // }
        }
        else
        {
            // Check if theres a gap between area2d and the wall
            // Assumption is that this behavior only happens on tile map collisions
            bool32 FoundGap = false;
            direction PlayerDir = Vector2ToDirectionEnum(&GameState->PlayerDirection);
            real32 Tolerance = 0.01f;
            if (PlayerDir == Direction_Up)
            {
                real32 DifferenceY = FloorReal32ToUInt32(NewPlayerHitbox.TopRight.Y) - 
                                        GameState->PlayerHitboxCache.TopRight.Y;
                if (DifferenceY > Tolerance)
                {
                    NewPlayerOrigin = GameState->PlayerP;
                    NewPlayerOrigin.Pos.Y += DifferenceY - Tolerance;
                    FoundGap = true;
                }
            }
            else if (PlayerDir == Direction_Down)
            {
                real32 DifferenceY = GameState->PlayerHitboxCache.BottomLeft.Y - 
                                        FloorReal32ToUInt32(GameState->PlayerHitboxCache.BottomLeft.Y);
                if (DifferenceY > Tolerance)
                {
                    NewPlayerOrigin = GameState->PlayerP;
                    NewPlayerOrigin.Pos.Y -= DifferenceY - Tolerance;
                    FoundGap = true;
                }
            }
            else if (PlayerDir == Direction_Right)
            {
                real32 DifferenceX = FloorReal32ToUInt32(NewPlayerHitbox.TopRight.X) - 
                                        GameState->PlayerHitboxCache.TopRight.X;
                if (DifferenceX > Tolerance)
                {
                    NewPlayerOrigin = GameState->PlayerP;
                    NewPlayerOrigin.Pos.X += DifferenceX - Tolerance;
                    FoundGap = true;
                }
            }
            else if (PlayerDir == Direction_Left)
            {
                real32 DifferenceX = GameState->PlayerHitboxCache.BottomLeft.X - 
                                        FloorReal32ToUInt32(GameState->PlayerHitboxCache.BottomLeft.X);
                if (DifferenceX > Tolerance)
                {
                    NewPlayerOrigin = GameState->PlayerP;
                    NewPlayerOrigin.Pos.X -= DifferenceX - Tolerance;
                    FoundGap = true;
                }
            }

            if (!FoundGap)
            {
                UpdatePosition = false;
            }
        }

        // Check if player is colliding with any entities
        for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
        {
            entity *Entity = &TileRoom->Entities[EntityIndex];
            if (Entity->IsActive)
            {
                area2d EntityHitbox = GetArea2D(Entity->P, Entity->Width, Entity->Height);
                bool32 IsColliding = IsAreaInArea(NewPlayerHitbox, EntityHitbox);
                // NOTE: Combining these just an optimization, can also nest
                if (IsColliding && Entity->IsSolid)
                {
                    // NOTE: Might wanna handle player enemy collisions here
                    // HandleEntityCollision(GameState, Entity);
                    Entity->ConsecutiveCollisionCounter += 1;
                    UpdatePosition = false;
                }
                else
                {
                    Entity->ConsecutiveCollisionCounter = 0;
                }
            }
        }
    }

    if (UpdatePosition)
    {
        GameState->PlayerP = NewPlayerOrigin;
        // NOTE: Need to be careful about this PlayerHitbox guy being outdated
        GameState->PlayerHitboxCache.BottomLeft = GameState->PlayerHitbox.BottomLeft + GameState->PlayerP.Pos;
        GameState->PlayerHitboxCache.TopRight = GameState->PlayerHitbox.TopRight + GameState->PlayerP.Pos;
    }

    if (GameState->PlayerUsingSword)
    {
        tile_map_position SwordPoint = GameState->PlayerP;
        int32 PixelOffsetX = 0;
        int32 PixelOffsetY = 0;
        direction PlayerDir = Vector2ToDirectionEnum(&GameState->PlayerDirection);
        if (PlayerDir == Direction_Up)
        {
            // NOTE: XY values taken from sprite sheet
            // TODO: Make sure this works, messed up cuz direction bug
            PixelOffsetX = 24 - 18;
            PixelOffsetY = 97 - 124;
        }
        else if (PlayerDir == Direction_Right)
        {
            PixelOffsetX = 44 - 18;
            PixelOffsetY = 86 - 92;
        }
        else if (PlayerDir == Direction_Down)
        {
            PixelOffsetX = 26 - 18;
            PixelOffsetY = 73 - 62;
        }
        else if (PlayerDir == Direction_Left)
        {
            // This math here is weird cuz of the flippy
            PixelOffsetX = -1*(44 - 18) + 16;
            PixelOffsetY = 86 - 92;
        }
        SwordPoint.Pos.X += (real32)PixelOffsetX / TileMap->MetersToPixels;
        SwordPoint.Pos.Y -= (real32)PixelOffsetY / TileMap->MetersToPixels;
        GameState->SwordPoint = SwordPoint;
    }

    // Dungeon room completion check
    if (PlayerRoom->Type == RoomType_Dungeon)
    {
        bool32 AreEnemiesDefeated = true;
        for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
        {
            entity *Entity = &PlayerRoom->Entities[EntityIndex];
            if (IsEntityTypeEnemy(Entity->Type) && Entity->IsActive)
            {
                AreEnemiesDefeated = false;
            }
        }

        if (AreEnemiesDefeated)
        {
            for (int32 DoorIndex = 0;
                 DoorIndex < 4;
                 DoorIndex++)
            {
                if (PlayerRoom->DungeonDoors[DoorIndex].DoorState == Dungeon_Door_Shut)
                {
                    PlayerRoom->DungeonDoors[DoorIndex].DoorState = Dungeon_Door_Open;
                }
            }
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
    tile_room *CurrentRoom = GetTileRoom(TileMap, GameState->PlayerP.RoomID);
    if (CurrentRoom && CurrentRoom->Type == RoomType_Overworld)
    {
        DrawOverworldRoom(GameState, Buffer, TileMap, CurrentRoom, 
                          CameraTileX, CameraTileY, ScreenTilesWidth, ScreenTilesHeight, PlayAreaY);
    }
    else if (CurrentRoom && CurrentRoom->Type == RoomType_Dungeon)
    {
        DrawDungeonRoom(GameState, Buffer, TileMap, CurrentRoom, 
                        CameraTileX, CameraTileY, ScreenTilesWidth, ScreenTilesHeight, PlayAreaY);
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
    if (GameState->PlayerPickingUpThing)
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

        if (PlayerDir == Direction_Up)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordBack[SwordSpriteIndex];
        }
        else if (PlayerDir == Direction_Down)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordFront[SwordSpriteIndex];
        }
        else if (PlayerDir == Direction_Left)
        {
            LinkSprite = &GameState->LinkSprites.UseSwordLeft[SwordSpriteIndex];
        }
        else if (PlayerDir == Direction_Right)
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
        if (PlayerDir == Direction_Up)
        {
            LinkSprite = &GameState->LinkSprites.Back[GameState->WalkStep];
        }
        else if (PlayerDir == Direction_Down)
        {
            LinkSprite = &GameState->LinkSprites.Front[GameState->WalkStep];
        }
        else if (PlayerDir == Direction_Left)
        {
            LinkSprite = &GameState->LinkSprites.Left[GameState->WalkStep];
        }
        else if (PlayerDir == Direction_Right)
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

    // DrawDebugPoint(Buffer, TileMap, PlayAreaY, GameState->PlayerP);
    DrawDebugArea2D(Buffer, TileMap, PlayAreaY, GameState->PlayerHitboxCache);

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

    if (GameState->PlayerHealth >= 0)
    {
        DrawBMPTile(&GameState->TextTileset.Tiles[GameState->PlayerHealth], Buffer, 0, 16);
    }

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

    // Dungeon Room Layer 2
    if (CurrentRoom && CurrentRoom->Type == RoomType_Dungeon)
    {
        DrawDungeonRoomLayer2(GameState, Buffer, TileMap, CurrentRoom, PlayAreaY);
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

        // NOTE: Offset just to make space for other shit
        real32 OffsetX = 8.0f * 6;
        real32 HeartX = 8.0f * (real32)i + OffsetX;
        DrawBMPTile(&GameState->HudTileset.Hearts[HeartIdx], Buffer, HeartX, 0);
    }

    GameState->FrameCounter++;
}
