#include "handmade.h"

#include "handmade_random.h"
#include "handmade_map.h"
#include "handmade_tile.cpp"
#include "handmade_sprite.cpp"


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
DrawDebugPoint(game_offscreen_buffer *Buffer, tile_map *TileMap, real32 PlayAreaY, tile_map_position Point)
{
    real32 PointX = TileMap->TileSideInPixels*Point.Pos.X;
    real32 PointY = PlayAreaY - TileMap->TileSideInPixels*(Point.Pos.Y - 11.0f);
    DrawRectangle(Buffer, PointX, PointY-2, PointX+2, PointY, 
                    1.0f, 0.0f, 0.0f);
}

internal bool32
IsHitboxPointActive(tile_map_position PlayerP, tile_map_position HitboxP, 
                    real32 HitboxWidth, real32 HitboxHeight)
{
    bool32 Active = false;

    tile_map_position HitboxBotLeft = HitboxP;
    tile_map_position HitboxTopRight = HitboxP;
    HitboxTopRight.Pos.X += HitboxWidth;
    HitboxTopRight.Pos.Y += HitboxHeight;

    // AABB checking
    if (PlayerP.Pos.X >= HitboxBotLeft.Pos.X && 
        PlayerP.Pos.X <= HitboxTopRight.Pos.X &&
        PlayerP.Pos.Y >= HitboxBotLeft.Pos.Y &&
        PlayerP.Pos.Y <= HitboxTopRight.Pos.Y)
    {
        Active = true;
    }

    return Active;
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
        tile_room *TileRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, HardcodedMap, SpawnRoomX, SpawnRoomY);
        LoadOverworldRoom(&GameState->WorldArena, TileMap, HardcodedMap2, SpawnRoomX+1, SpawnRoomY);

        // NOTE: Overworld is 16x8 but we allocate 16x16, so we store extra rooms in the top 16x8 half
        uint32 CaveRoomX = 0;
        uint32 CaveRoomY = 8;
        TileRoom->Door.Pos.X = (real32)TileMap->RoomWidth / 2.0f;
        TileRoom->Door.Pos.Y = 0.5f;
        TileRoom->Door.RoomIDX = CaveRoomX;
        TileRoom->Door.RoomIDY = CaveRoomY;
        tile_room *CaveRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, CaveMap, CaveRoomX, CaveRoomY);
        CaveRoom->Down = PushStruct(&GameState->WorldArena, tile_map_position);
        CaveRoom->Down->RoomIDX = SpawnRoomX;
        CaveRoom->Down->RoomIDY = SpawnRoomY;
        CaveRoom->Down->Pos.X = 4.5f;
        CaveRoom->Down->Pos.Y = 8.5f;

        GameState->PlayerHealth = 9;
        GameState->PlayerP.RoomIDX = SpawnRoomX;
        GameState->PlayerP.RoomIDY = SpawnRoomY;
        GameState->PlayerP.Pos.X = 5.0f;
        GameState->PlayerP.Pos.Y = 5.0f;

        GameState->Octorok.Base.Health = 3;
        GameState->Octorok.Base.P.Pos.X = 8;
        GameState->Octorok.Base.P.Pos.Y = 5.0f;
        GameState->Octorok.Base.P.RoomIDX = SpawnRoomX;
        GameState->Octorok.Base.P.RoomIDY = SpawnRoomY;
        GameState->OctorokProjectile.Base.FireFrequency = 60;

        GameState->Moblin.Base.Health = 3;
        GameState->Moblin.Base.P.Pos.X = 6;
        GameState->Moblin.Base.P.Pos.Y = 5.0f;
        GameState->Moblin.Base.P.RoomIDX = SpawnRoomX;
        GameState->Moblin.Base.P.RoomIDY = SpawnRoomY;
        GameState->Moblin.Base.Direction = FRONT;
        GameState->MoblinProjectile.Base.FireFrequency = 60;

        // NOTE: We should probably start a new arena for this image? 
        // Memory->Background = LoadBMPFile(&GameState->WorldArena, Memory, Thread, "test/test_background.bmp");
        GameState->Background = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                            "backgrounds_processed/kitchen.bmp");

        GameState->LinkBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "tiles/link.bmp");
        LoadLinkSprites(&GameState->LinkSprites, &GameState->LinkBMP);

        GameState->OverworldBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/overworld_tileset.bmp");
        LoadOverworldTileset(&GameState->OverworldTileset, &GameState->OverworldBMP);

        LoadTextTileset(&GameState->TextTileset, &GameState->OverworldBMP);

        GameState->OWEnemiesBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/overworld_enemies.bmp");
        
        LoadOctorokSprites(&GameState->Octorok.Sprites, &GameState->OWEnemiesBMP);

        LoadMoblinSprites(&GameState->Moblin.Sprites, &GameState->OWEnemiesBMP);

        GameState->OctorokProjectile.Sprite.Tileset = &GameState->OWEnemiesBMP;
        GameState->OctorokProjectile.Sprite.X = 69;
        GameState->OctorokProjectile.Sprite.Y = 11;
        GameState->OctorokProjectile.Sprite.Width = 8;
        GameState->OctorokProjectile.Sprite.Height = 16;

        // NOTE: maybe move this to platform layer
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    real32 PlayerWidth = 0.75f;
    real32 PlayerHeight = 1.0f;

    real32 dPlayerX = 0.0f;
    real32 dPlayerY = 0.0f;
    real32 PlayerSpeed = 5.0f;
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
            // NOTE: Use digital movement tuning
            if (Controller->MoveUp.EndedDown)
            {
                dPlayerY = 1.0f;
                GameState->HeroDirection = BACK;
            }
            if (Controller->MoveDown.EndedDown)
            {
                dPlayerY = -1.0f;
                GameState->HeroDirection = FRONT;
            }
            if (Controller->MoveLeft.EndedDown)
            {
                dPlayerX = -1.0f;
                GameState->HeroDirection = LEFT;
            }
            if (Controller->MoveRight.EndedDown)
            {
                dPlayerX = 1.0f;
                GameState->HeroDirection = RIGHT;
            }

            if (Controller->ActionUp.EndedDown)
            {
                PlayerSpeed = 10.0f;
            }

            // B
            if (Controller->ActionLeft.EndedDown)
            {
                if (!GameState->PlayerUsingSword)
                {
                    GameState->PlayerUsingSword = true;
                    GameState->SwordUsageFrame = -1;
                }
            }
            // A
            if (Controller->ActionDown.EndedDown)
            {
                // PlayerSpeed = 10.0f;
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

    // Player movement stuff
    dPlayerX *= PlayerSpeed;
    dPlayerY *= PlayerSpeed;

    // NOTE: Translate origin to the center of player cuz of legacy calculations
    tile_map_position NewPlayerOrigin = GameState->PlayerP;
    tile_map_position NewPlayerP = NewPlayerOrigin;
    NewPlayerP.Pos.X += 0.5f;

    if (!GameState->PlayerUsingSword)
    {
        NewPlayerP.Pos.X += Input->dtForFrame*dPlayerX;
        NewPlayerP.Pos.Y += Input->dtForFrame*dPlayerY;
    }

    tile_map_position NewPlayerUp = NewPlayerP;
    NewPlayerUp.Pos.Y += 0.1f*PlayerHeight;
    tile_map_position NewPlayerLeft = NewPlayerP;
    NewPlayerLeft.Pos.X -= 0.5f*PlayerWidth;
    tile_map_position NewPlayerRight = NewPlayerP;
    NewPlayerRight.Pos.X += 0.5f*PlayerWidth;

    NewPlayerUp = NewPlayerP;
    NewPlayerUp.Pos.Y += 0.1f*PlayerHeight;
    NewPlayerLeft = NewPlayerP;
    NewPlayerLeft.Pos.X -= 0.5f*PlayerWidth;
    NewPlayerRight = NewPlayerP;
    NewPlayerRight.Pos.X += 0.5f*PlayerWidth;

    // Room transitions
    tile_room *TileRoom = GetTileRoom(TileMap, GameState->PlayerP.RoomIDX, GameState->PlayerP.RoomIDY);
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
            NewPlayerP.Pos.Y = 0.1f*PlayerHeight + 0.0001f;
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
            NewPlayerP.Pos.Y = (real32)TileMap->RoomHeight - 0.1f*PlayerHeight - 0.0001f;
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
            NewPlayerP.Pos.X = (real32)TileMap->RoomWidth - 0.5f*PlayerWidth - 0.0001f;
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
            NewPlayerP.Pos.X = 0.5f*PlayerWidth + 0.0001f;
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
            if (!IsOnSameTile(GameState->PlayerP, NewPlayerP))
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
            if (GameState->HeroDirection == FRONT)
            {
                // NOTE: XY values taken from sprite sheet
                PixelOffsetX = 26 - 18;
                PixelOffsetY = 73 - 62;
            }
            else if (GameState->HeroDirection == RIGHT)
            {
                PixelOffsetX = 44 - 18;
                PixelOffsetY = 86 - 92;
            }
            else if (GameState->HeroDirection == BACK)
            {
                PixelOffsetX = 24 - 18;
                PixelOffsetY = 97 - 124;
            }
            else if (GameState->HeroDirection == LEFT)
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
    GameState->PlayerP = NewPlayerOrigin;

    // Octorok position update
    if (IsInSameTileRoom(GameState->PlayerP, GameState->Octorok.Base.P))
    {
        local_persist int32 YDirection = -1;
        if (GameState->Octorok.Base.P.Pos.Y > 8)
        {
            YDirection = -1;
        }
        else if (GameState->Octorok.Base.P.Pos.Y < 3)
        {
            YDirection = 1;
        }
        GameState->Octorok.Base.P.Pos.Y += (real32)YDirection * 0.05f;
    }

    // Octorok fire projectile
    if (!GameState->OctorokProjectile.Base.IsActive && 
        GameState->Octorok.Base.Health > 0 &&
        GameState->FrameCounter % GameState->OctorokProjectile.Base.FireFrequency == 0)
    {
        GameState->OctorokProjectile.Base.IsActive = true;
        GameState->OctorokProjectile.Base.VelocityX = 0.0;
        GameState->OctorokProjectile.Base.VelocityY = -5.0;
        GameState->OctorokProjectile.Base.P = GameState->Octorok.Base.P;
    }

    if (GameState->OctorokProjectile.Base.IsActive)
    {
        GameState->OctorokProjectile.Base.P.Pos.X += GameState->OctorokProjectile.Base.VelocityX/60.0f;
        GameState->OctorokProjectile.Base.P.Pos.Y += GameState->OctorokProjectile.Base.VelocityY/60.0f;

        if (GameState->OctorokProjectile.Base.P.Pos.Y < 0)
        {
            GameState->OctorokProjectile.Base.IsActive = false;
        }
    }

    // Checking if octorok hit player
    // NOTE: I chose to do this after the octo position update, so were not using pos from last frame
    if (GameState->InvincibilityTimer == 0)
    {
        // TODO: Make it so i dont have to check this everywhere
        if (GameState->Octorok.Base.Health)
        {
            bool32 IsHit = IsHitboxPointActive(NewPlayerP, GameState->Octorok.Base.P, 1.0f, 1.0f);
            if (IsHit)
            {
                GameState->InvincibilityTimer = 60;
                GameState->PlayerHealth -= 1;
            }
        }

        // Is projectile hitting the player?
        if (GameState->OctorokProjectile.Base.IsActive)
        {
            bool32 IsHit = IsHitboxPointActive(NewPlayerP, GameState->OctorokProjectile.Base.P, 0.5f, 1.0f);
            if (IsHit)
            {
                GameState->InvincibilityTimer = 60;
                GameState->PlayerHealth -= 1;
                GameState->OctorokProjectile.Base.IsActive = false;
            }
        }
    }
    else
    {
        GameState->InvincibilityTimer -= 1;
    }

    // Player sword hitting Octorok checking
    if (GameState->Octorok.Base.InvincibilityTimer == 0 && GameState->Octorok.Base.Health > 0)
    {
        bool32 IsEnemyHit = IsHitboxPointActive(GameState->SwordPoint, GameState->Octorok.Base.P, 1.0f, 1.0f);
        if (IsEnemyHit)
        {
            GameState->Octorok.Base.InvincibilityTimer = 60;
            GameState->Octorok.Base.Health -= 1;
        }
    }
    if (GameState->Octorok.Base.InvincibilityTimer > 0)
    {
        GameState->Octorok.Base.InvincibilityTimer -= 1;
    }

    // Moblin position update
    if (IsInSameTileRoom(GameState->PlayerP, GameState->Moblin.Base.P))
    {
        local_persist int32 YDirection = -1;
        if (GameState->Moblin.Base.P.Pos.Y > 8)
        {
            YDirection = -1;
            GameState->Moblin.Base.Direction = FRONT;
        }
        else if (GameState->Moblin.Base.P.Pos.Y < 3)
        {
            YDirection = 1;
            GameState->Moblin.Base.Direction = BACK;
        }
        GameState->Moblin.Base.P.Pos.Y += (real32)YDirection * 0.05f;
    }

    // Moblin fire projectile
    if (!GameState->MoblinProjectile.Base.IsActive && 
        GameState->Moblin.Base.Health > 0 &&
        GameState->FrameCounter % GameState->MoblinProjectile.Base.FireFrequency == 0)
    {
        GameState->MoblinProjectile.Base.IsActive = true;
        GameState->MoblinProjectile.Base.VelocityX = 0.0;
        GameState->MoblinProjectile.Base.VelocityY = -5.0;
        GameState->MoblinProjectile.Base.P = GameState->Moblin.Base.P;
        GameState->MoblinProjectile.Direction = GameState->Moblin.Base.Direction;
    }

    if (GameState->MoblinProjectile.Base.IsActive)
    {
        GameState->MoblinProjectile.Base.P.Pos.X += GameState->MoblinProjectile.Base.VelocityX/60.0f;
        GameState->MoblinProjectile.Base.P.Pos.Y += GameState->MoblinProjectile.Base.VelocityY/60.0f;

        if (GameState->MoblinProjectile.Base.P.Pos.Y < 0)
        {
            GameState->MoblinProjectile.Base.IsActive = false;
        }
    }

    // Checking if octorok hit player
    // NOTE: I chose to do this after the octo position update, so were not using pos from last frame
    if (GameState->InvincibilityTimer == 0)
    {
        // TODO: Make it so i dont have to check this everywhere
        if (GameState->Moblin.Base.Health)
        {
            bool32 IsHit = IsHitboxPointActive(NewPlayerP, GameState->Moblin.Base.P, 1.0f, 1.0f);
            if (IsHit)
            {
                GameState->InvincibilityTimer = 60;
                GameState->PlayerHealth -= 1;
            }
        }

        // Is projectile hitting the player?
        if (GameState->MoblinProjectile.Base.IsActive)
        {
            bool32 IsHit = IsHitboxPointActive(NewPlayerP, GameState->MoblinProjectile.Base.P, 0.5f, 1.0f);
            if (IsHit)
            {
                GameState->InvincibilityTimer = 60;
                GameState->PlayerHealth -= 1;
                GameState->MoblinProjectile.Base.IsActive = false;
            }
        }
    }
    // TODO: Normally this would be uncommented but if we kept it then we would go double speed
    // Cuz we already do this for octorok timer
    // else
    // {
    //     GameState->InvincibilityTimer -= 1;
    // }

    // Player sword hitting Moblin checking
    if (GameState->Moblin.Base.InvincibilityTimer == 0 && GameState->Moblin.Base.Health > 0)
    {
        bool32 IsEnemyHit = IsHitboxPointActive(GameState->SwordPoint, GameState->Moblin.Base.P, 1.0f, 1.0f);
        if (IsEnemyHit)
        {
            GameState->Moblin.Base.InvincibilityTimer = 60;
            GameState->Moblin.Base.Health -= 1;
        }
    }
    if (GameState->Moblin.Base.InvincibilityTimer > 0)
    {
        GameState->Moblin.Base.InvincibilityTimer -= 1;
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

    // NOTE: Maybe this should be its own isolated buffer or something so we dont 
    //       draw the player into the UI
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
            uint32 TileID = GetTileValue(TileMap, 
                                         GameState->PlayerP.RoomIDX,
                                         GameState->PlayerP.RoomIDY, 
                                         Column, Row);
            if (TileID > 0)
            {
                bmp_tile *TileSprite = &GameState->OverworldTileset.Tiles[TileID];
                real32 MinX = (real32)(TileMap->TileSideInPixels * RelColumn);
                real32 MinY = PlayAreaY + (real32)(TileMap->TileSideInPixels * RelRow);
                DrawBMPTile(TileSprite, Buffer, MinX, MinY);
            }
        }
    }

    real32 HeroOriginX = TileMap->TileSideInPixels*(GameState->PlayerP.Pos.X - (real32)CameraTileX);
    real32 HeroOriginY = PlayAreaY - TileMap->TileSideInPixels*(GameState->PlayerP.Pos.Y - 11.0f);
    real32 PlayerScreenX = HeroOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 PlayerScreenY = HeroOriginY - TileMap->TileSideInPixels*1.0f;

    // real32 HeroCenterX = 8.0f;
    // real32 HeroCenterY = 16.0f;
    real32 SpriteMinX = PlayerScreenX;
    real32 SpriteMinY = PlayerScreenY;

    bmp_tile *LinkSprite;
    if (GameState->PlayerUsingSword)
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

        if (GameState->HeroDirection == FRONT)
        {
            LinkSprite = &GameState->LinkSprites.SwordFront[SwordSpriteIndex];
        }
        else if (GameState->HeroDirection == BACK)
        {
            LinkSprite = &GameState->LinkSprites.SwordBack[SwordSpriteIndex];
        }
        else if (GameState->HeroDirection == LEFT)
        {
            LinkSprite = &GameState->LinkSprites.SwordLeft[SwordSpriteIndex];
        }
        else if (GameState->HeroDirection == RIGHT)
        {
            LinkSprite = &GameState->LinkSprites.SwordRight[SwordSpriteIndex];
        }
        else
        {
            LinkSprite = 0;
            Assert(0);
        }
    }
    else
    {
        if (GameState->HeroDirection == FRONT)
        {
            LinkSprite = &GameState->LinkSprites.Front[GameState->WalkStep];
        }
        else if (GameState->HeroDirection == BACK)
        {
            LinkSprite = &GameState->LinkSprites.Back[GameState->WalkStep];
        }
        else if (GameState->HeroDirection == LEFT)
        {
            LinkSprite = &GameState->LinkSprites.Left[GameState->WalkStep];
        }
        else if (GameState->HeroDirection == RIGHT)
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
    DrawBMPTile(&GameState->TextTileset.Tiles[GameState->Octorok.Base.Health], Buffer, 32, 16);

    if (IsInSameTileRoom(GameState->PlayerP, GameState->Octorok.Base.P) && GameState->Octorok.Base.Health > 0)
    {
        real32 OctoOriginX = TileMap->TileSideInPixels*(GameState->Octorok.Base.P.Pos.X - (real32)CameraTileX);
        real32 OctoOriginY = PlayAreaY - TileMap->TileSideInPixels*(GameState->Octorok.Base.P.Pos.Y - 11.0f);
        real32 OctoScreenX = OctoOriginX;
        // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
        real32 OctoScreenY = OctoOriginY - TileMap->TileSideInPixels*1.0f;
        // Origin
        DrawRectangle(Buffer, OctoOriginX, OctoOriginY-2, OctoOriginX+2, OctoOriginY, 
                      1.0f, 0.0f, 0.0f);

        // Hurt/Invincible rendering
        if (GameState->Octorok.Base.InvincibilityTimer > 0)
        {
            if (GameState->Octorok.Base.InvincibilityTimer % 6 == 0)
            {
                if (GameState->Octorok.Base.IFramesFlicker)
                {
                    GameState->Octorok.Base.IFramesFlicker = false;
                }
                else
                {
                    GameState->Octorok.Base.IFramesFlicker = true;
                }
            }
        }
        else 
        {
            GameState->Octorok.Base.IFramesFlicker = false;
        }

        if (!GameState->Octorok.Base.IFramesFlicker)
        {
            uint32 OctoSpriteIndex = (GameState->FrameCounter & 0x10) == 0x10;
            DrawBMPTile(&GameState->Octorok.Sprites.Front[OctoSpriteIndex], Buffer, OctoScreenX, OctoScreenY);
        }
    }

    if (IsInSameTileRoom(GameState->PlayerP, GameState->OctorokProjectile.Base.P) && 
        GameState->OctorokProjectile.Base.IsActive)
    {
        real32 ProjOriginX = TileMap->TileSideInPixels*(GameState->OctorokProjectile.Base.P.Pos.X - (real32)CameraTileX);
        real32 ProjOriginY = PlayAreaY - TileMap->TileSideInPixels*(GameState->OctorokProjectile.Base.P.Pos.Y - 11.0f);
        real32 ProjScreenX = ProjOriginX;
        // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
        real32 ProjScreenY = ProjOriginY - TileMap->TileSideInPixels*1.0f;
        DrawBMPTile(&GameState->OctorokProjectile.Sprite, Buffer, ProjScreenX, ProjScreenY);
    }

    if (IsInSameTileRoom(GameState->PlayerP, GameState->Moblin.Base.P) && GameState->Moblin.Base.Health > 0)
    {
        real32 MoblinOriginX = TileMap->TileSideInPixels*(GameState->Moblin.Base.P.Pos.X - (real32)CameraTileX);
        real32 MoblinOriginY = PlayAreaY - TileMap->TileSideInPixels*(GameState->Moblin.Base.P.Pos.Y - 11.0f);
        real32 MoblinScreenX = MoblinOriginX;
        // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
        real32 MoblinScreenY = MoblinOriginY - TileMap->TileSideInPixels*1.0f;
        // Origin
        DrawRectangle(Buffer, MoblinOriginX, MoblinOriginY-2, MoblinOriginX+2, MoblinOriginY, 
                      1.0f, 0.0f, 0.0f);

        // Hurt/Invincible rendering
        if (GameState->Moblin.Base.InvincibilityTimer > 0)
        {
            if (GameState->Moblin.Base.InvincibilityTimer % 6 == 0)
            {
                if (GameState->Moblin.Base.IFramesFlicker)
                {
                    GameState->Moblin.Base.IFramesFlicker = false;
                }
                else
                {
                    GameState->Moblin.Base.IFramesFlicker = true;
                }
            }
        }
        else 
        {
            GameState->Moblin.Base.IFramesFlicker = false;
        }

        if (!GameState->Moblin.Base.IFramesFlicker)
        {
            direction MoblinDir = GameState->Moblin.Base.Direction;
            bmp_tile *MoblinDirSprite;
            if (MoblinDir == FRONT)
            {
                MoblinDirSprite = (bmp_tile *)&GameState->Moblin.Sprites.Front;
            }
            else if (MoblinDir == BACK)
            {
                MoblinDirSprite = (bmp_tile *)&GameState->Moblin.Sprites.Back;
            }
            else if (MoblinDir == LEFT)
            {
                MoblinDirSprite = (bmp_tile *)&GameState->Moblin.Sprites.Left;
            }
            else if (MoblinDir == RIGHT)
            {
                MoblinDirSprite = (bmp_tile *)&GameState->Moblin.Sprites.Right;
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

    if (IsInSameTileRoom(GameState->PlayerP, GameState->MoblinProjectile.Base.P) && 
        GameState->MoblinProjectile.Base.IsActive)
    {
        real32 ProjOriginX = TileMap->TileSideInPixels*(GameState->MoblinProjectile.Base.P.Pos.X - (real32)CameraTileX);
        real32 ProjOriginY = PlayAreaY - TileMap->TileSideInPixels*(GameState->MoblinProjectile.Base.P.Pos.Y - 11.0f);
        real32 ProjScreenX = ProjOriginX;
        // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
        real32 ProjScreenY = ProjOriginY - TileMap->TileSideInPixels*1.0f;

        direction ArrowDir = GameState->MoblinProjectile.Direction;
        bmp_tile *ArrowSprite;
        if (ArrowDir == FRONT)
        {
            ArrowSprite = &GameState->Moblin.Sprites.ArrowFront;
        }
        else if (ArrowDir == BACK)
        {
            ArrowSprite = &GameState->Moblin.Sprites.ArrowBack;
        }
        else if (ArrowDir == LEFT)
        {
            ArrowSprite = &GameState->Moblin.Sprites.ArrowLeft;
        }
        else if (ArrowDir == RIGHT)
        {
            ArrowSprite = &GameState->Moblin.Sprites.ArrowRight;
        }
        else
        {
            ArrowSprite = 0;
            Assert(0);
        }

        DrawBMPTile(ArrowSprite, Buffer, ProjScreenX, ProjScreenY);
    }

    GameState->FrameCounter++;
}
