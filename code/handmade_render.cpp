
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
DrawBomb(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
         real32 PlayAreaY, uint32 CameraTileX, entity *Entity, link_sprites *LinkSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Entity->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    DrawBMPTile(&LinkSprites->Bomb, Buffer, ScreenX, ScreenY);
}

internal void
DrawBombDust(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
             real32 PlayAreaY, uint32 CameraTileX, entity *Entity, link_sprites *LinkSprites)
{
    vector2 Origin = WorldToScreen(TileMap, Entity->P, CameraTileX, PlayAreaY);
    real32 ScreenX = Origin.X;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ScreenY = Origin.Y - TileMap->TileSideInPixels*1.0f;
    
    uint32 DustFrame = (GameState->FrameCounter - Entity->SpawnFrame) / 5;
    DrawBMPTile(&LinkSprites->BombDust[DustFrame], Buffer, ScreenX, ScreenY);
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
    DrawBMPTile(&GameState->LinkSprites.Boomerang[SpriteIndex], Buffer, ScreenX, ScreenY);
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
    else if (Entity->Type == EntityType_Bomb)
    {
        DrawBomb(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
                 Entity, &GameState->LinkSprites);
    }
    else if (Entity->Type == EntityType_BombDust)
    {
        DrawBombDust(GameState, Buffer, TileMap, PlayAreaY, CameraTileX, 
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