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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerP.AbsTileX = 4;
        GameState->PlayerP.AbsTileY = 5;
        GameState->PlayerP.TileRelX = 0.0f;
        GameState->PlayerP.TileRelY = 0.0f;

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;
        // NOTE: set to use 256x256 tile chunks
        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;
        TileMap->TileChunkCountZ = 128;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, 
                                        TileMap->TileChunkCountX*
                                        TileMap->TileChunkCountY*
                                        TileMap->TileChunkCountZ,
                                        tile_chunk);

        TileMap->TileSideInMeters = 1.0f;
        TileMap->TileSideInPixels = 16;
        TileMap->MetersToPixels = (real32)TileMap->TileSideInPixels/(real32)TileMap->TileSideInMeters;

        // GenerateTileMap(GameState, TileMap);
        int32 HardcodedMapWidth = 16;
        int32 HardcodedMapHeight = 11;
        uint32 AbsTileZ = 0;
        for (int32 SourceY = HardcodedMapHeight-1;
             SourceY >= 0;
             SourceY--)
        {
            for (int32 SourceX = 0;
                SourceX < HardcodedMapWidth;
                SourceX++)
            {
                uint32 SourceIndex = SourceY*HardcodedMapWidth + SourceX;
                uint32 TileValue = HardcodedMap[SourceIndex];
                uint32 AbsTileX = SourceX;
                uint32 AbsTileY = (HardcodedMapHeight-1) - SourceY;
                SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ,
                                TileValue);
            }
        }

        // NOTE: We should probably start a new arena for this image? 
        // Memory->Background = LoadBMPFile(&GameState->WorldArena, Memory, Thread, "test/test_background.bmp");
        GameState->Background = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                            "backgrounds_processed/kitchen.bmp");

        GameState->LinkFront = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "sprites/link_front.bmp");
        GameState->LinkBack = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                          "sprites/link_back.bmp");
        GameState->LinkLeft = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                          "sprites/link_left.bmp");
        GameState->LinkRight = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                           "sprites/link_right.bmp");

        GameState->OverworldBMP = LoadBMPFile(&GameState->WorldArena, Memory, Thread, 
                                                "tiles/overworld_tileset.bmp");
        LoadOverworldTileset(&GameState->OverworldTileset, &GameState->OverworldBMP);
        
        // NOTE: maybe move this to platform layer
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    real32 PlayerWidth = 0.75f;
    real32 PlayerHeight = 1.0f;

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
            real32 dPlayerX = 0.0f;
            real32 dPlayerY = 0.0f;
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

            real32 PlayerSpeed = 5.0f;
            if (Controller->ActionUp.EndedDown)
            {
                PlayerSpeed = 10.0f;
            }
            dPlayerX *= PlayerSpeed;
            dPlayerY *= PlayerSpeed;

            tile_map_position NewPlayerP = GameState->PlayerP;
            NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
            NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

            tile_map_position NewPlayerLeft = NewPlayerP;
            NewPlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            NewPlayerLeft = RecanonicalizePosition(TileMap, NewPlayerLeft);
            tile_map_position NewPlayerRight = NewPlayerP;
            NewPlayerRight.TileRelX += 0.5f*PlayerWidth;
            NewPlayerRight = RecanonicalizePosition(TileMap, NewPlayerRight);

            if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
                IsTileMapPointEmpty(TileMap, NewPlayerLeft) &&
                IsTileMapPointEmpty(TileMap, NewPlayerRight))
            {
                if (!IsOnSameTile(GameState->PlayerP, NewPlayerP))
                {
                    uint32 TileValue = GetTileValue(TileMap, NewPlayerP);
                }
                GameState->PlayerP = NewPlayerP;
            }
        }
    }

    // Start drawing process

    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 
                  1.0f, 0.0f, 0.0f);

    real32 OffsetX = 0.5f*TileMap->TileSideInPixels;
    real32 OffsetY = 0.5f*TileMap->TileSideInPixels;

    // NOTE: Camera coord is the tile which will be placed in the top left 
    //       coordinate of screen
    uint32 CameraTileX = 0;
    uint32 CameraTileY = 10;

    uint32 ScreenTilesWidth = 16;
    uint32 ScreenTilesHeight = 11;

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
            uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
            if (TileID > 0)
            {
                bmp_tile *TileSprite = &GameState->OverworldTileset.Tiles[TileID];
                real32 MinX = (real32)(TileMap->TileSideInPixels * RelColumn);
                real32 MinY = PlayAreaY + (real32)(TileMap->TileSideInPixels * RelRow);
                DrawBMPTile(TileSprite, Buffer, MinX, MinY);
            }
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerScreenX = 0.5f*TileMap->TileSideInPixels + TileMap->TileSideInPixels*((real32)(int32)(GameState->PlayerP.AbsTileX - CameraTileX) + GameState->PlayerP.TileRelX);
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 PlayerScreenY = PlayAreaY + 0.5f*TileMap->TileSideInPixels - TileMap->TileSideInPixels*((real32)(int32)(GameState->PlayerP.AbsTileY - CameraTileY) + GameState->PlayerP.TileRelY);
    real32 PlayerLeft = PlayerScreenX - TileMap->MetersToPixels*0.5f*PlayerWidth;
    real32 PlayerTop = PlayerScreenY - TileMap->MetersToPixels*PlayerHeight;
    DrawRectangle(Buffer, 
                  PlayerLeft, PlayerTop, 
                  PlayerLeft + TileMap->MetersToPixels*PlayerWidth, 
                  PlayerTop + TileMap->MetersToPixels*PlayerHeight,
                  PlayerR, PlayerG, PlayerB);

    real32 HeroCenterX = 8.0f;
    real32 HeroCenterY = 16.0f;
    real32 SpriteMinX = PlayerScreenX - HeroCenterX;
    real32 SpriteMinY = PlayerScreenY - HeroCenterY;
    if (GameState->HeroDirection == FRONT)
    {
        DrawBMPFile(&GameState->LinkFront, Buffer, SpriteMinX, SpriteMinY);
    }
    else if (GameState->HeroDirection == BACK)
    {
        DrawBMPFile(&GameState->LinkBack, Buffer, SpriteMinX, SpriteMinY);
    }
    else if (GameState->HeroDirection == LEFT)
    {
        DrawBMPFile(&GameState->LinkLeft, Buffer, SpriteMinX, SpriteMinY);
    }
    else if (GameState->HeroDirection == RIGHT)
    {
        DrawBMPFile(&GameState->LinkRight, Buffer, SpriteMinX, SpriteMinY);
    }
}
