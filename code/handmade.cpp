#include "handmade.h"

#include "handmade_random.h"
#include "handmade_tile.cpp"


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
        GameState->PlayerP.AbsTileX = 1;
        GameState->PlayerP.AbsTileY = 3;
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

        TileMap->TileSideInMeters = 1.4f;
        TileMap->TileSideInPixels = 60;
        TileMap->MetersToPixels = (real32)TileMap->TileSideInPixels/(real32)TileMap->TileSideInMeters;

        real32 LowerLeftX = (real32)-TileMap->TileSideInPixels/2;
        real32 LowerLeftY = (real32)Buffer->Height;

        uint32 RandomNumberIndex = 0;
        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        uint32 ScreenX = 0;
        uint32 ScreenY = 0;

        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorUp = false;
        bool32 DoorDown = false;
        uint32 AbsTileZ = 0;
        for (uint32 ScreenIndex = 0;
             ScreenIndex < 32;
             ScreenIndex++)
        {
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
            uint32 RandomChoice;
            if (DoorUp || DoorDown)
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            }
            else
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            }

            bool32 CreatedZDoor = false;
            if (RandomChoice == 2)
            {
                CreatedZDoor = true;
                if (AbsTileZ == 0)
                {
                    DoorUp = true;
                }
                else 
                {
                    DoorDown = true;
                }
            }
            else if (RandomChoice == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

            for (uint32 TileY = 0;
                 TileY < TilesPerHeight;
                 TileY++)
            {
                for (uint32 TileX = 0;
                     TileX < TilesPerWidth;
                     TileX++)
                {
                    uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;

                    uint32 TileValue = 1;
                    if ((TileX == 0) && !(DoorLeft && (TileY == TilesPerHeight / 2)))
                    {
                        TileValue = 2;
                    }
                    if ((TileX == TilesPerWidth - 1) && !(DoorRight && (TileY == TilesPerHeight / 2)))
                    {
                        TileValue = 2;
                    }

                    if ((TileY == 0) && !(DoorBottom && (TileX == TilesPerWidth / 2)))
                    {
                        TileValue = 2;
                    }
                    if ((TileY == TilesPerHeight - 1) && !(DoorTop && (TileX == TilesPerWidth / 2)))
                    {
                        TileValue = 2;
                    }

                    if (DoorUp && (TileX == TilesPerWidth/2) && (TileY == TilesPerHeight/2))
                    {
                        TileValue = 3;
                    }
                    if (DoorDown && (TileX == TilesPerWidth/2) && (TileY == TilesPerHeight/2))
                    {
                        TileValue = 4;
                    }

                    SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ,
                                 TileValue);
                }
            }

            if (RandomChoice == 2)
            {
                if (AbsTileZ == 0)
                {
                    AbsTileZ = 1;
                }
                else
                {
                    AbsTileZ = 0;
                }
            }
            else if (RandomChoice == 1)
            {
                ScreenX += 1;
            }
            else
            {
                ScreenY += 1;
            }

            if (CreatedZDoor)
            {
                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else
            {
                DoorDown = false;
                DoorUp = false;
            }

            DoorLeft = DoorRight;
            DoorRight = false;
            DoorBottom = DoorTop;
            DoorTop = false;
        }

        // NOTE: maybe move this to platform layer
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f*PlayerHeight;

    for (int ControllerIndex = 0;
         ControllerIndex < ArrayCount(Input->Controllers);
         ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
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
            }
            if (Controller->MoveDown.EndedDown)
            {
                dPlayerY = -1.0f;
            }
            if (Controller->MoveLeft.EndedDown)
            {
                dPlayerX = -1.0f;
            }
            if (Controller->MoveRight.EndedDown)
            {
                dPlayerX = 1.0f;
            }

            real32 PlayerSpeed = 2.0f;
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

                    if (TileValue == 3)
                    {
                        NewPlayerP.AbsTileZ++;
                    }
                    else if (TileValue == 4)
                    {
                        NewPlayerP.AbsTileZ--;
                    }
                }
                GameState->PlayerP = NewPlayerP;
            }
        }
    }

    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 
                  1.0f, 0.0f, 0.0f);

    real32 ScreenCenterX = 0.5f*(real32)Buffer->Width;
    real32 ScreenCenterY = 0.5f*(real32)Buffer->Height;

    for (int32 RelRow = -10;
         RelRow < 10;
         RelRow++)
    {
        for (int32 RelColumn = -20;
             RelColumn < 20;
             RelColumn++)
        {
            uint32 Row = GameState->PlayerP.AbsTileY + RelRow;
            uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
            real32 Gray = 0.5f;
            if (TileID > 0)
            {
                if (TileID == 2)
                {
                    Gray = 1.0f;
                }
                if (TileID > 2)
                {
                    Gray = 0.25f;
                }
                if (Column == GameState->PlayerP.AbsTileX && Row == GameState->PlayerP.AbsTileY)
                {
                    Gray = 0.0f;
                }
                real32 CenX = ScreenCenterX - TileMap->MetersToPixels*GameState->PlayerP.TileRelX + ((real32)RelColumn)*TileMap->TileSideInPixels;
                real32 CenY = ScreenCenterY + TileMap->MetersToPixels*GameState->PlayerP.TileRelY - ((real32)RelRow)*TileMap->TileSideInPixels;
                real32 MinX = CenX - 0.5f*TileMap->TileSideInPixels;
                real32 MinY = CenY - 0.5f*TileMap->TileSideInPixels;
                real32 MaxX = CenX + 0.5f*TileMap->TileSideInPixels;
                real32 MaxY = CenY + 0.5f*TileMap->TileSideInPixels;
                DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY,
                              Gray, Gray, Gray);
            }
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerLeft = ScreenCenterX - TileMap->MetersToPixels*0.5f*PlayerWidth;
    real32 PlayerTop = ScreenCenterY - TileMap->MetersToPixels*PlayerHeight;
    DrawRectangle(Buffer, 
                  PlayerLeft, PlayerTop, 
                  PlayerLeft + TileMap->MetersToPixels*PlayerWidth, 
                  PlayerTop + TileMap->MetersToPixels*PlayerHeight,
                  PlayerR, PlayerG, PlayerB);
}
