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

// NOTE: This is in screen space coordinates, where Y=0 is at the top
// NOTE: Opposed to world space coords, where Y=0 is at the bottom
internal void
DrawRectangle(game_offscreen_buffer *Buffer, 
              v2 vMin, v2 vMax,
              real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(vMin.X);
    int32 MinY = RoundReal32ToInt32(vMin.Y);
    int32 MaxX = RoundReal32ToInt32(vMax.X);
    int32 MaxY = RoundReal32ToInt32(vMax.Y);

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

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
    loaded_bitmap Result = {};

    // 0 x AA RR GG BB
    // Little endian = BB, GG, RR, AA
    debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
    if (ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;

        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);

        uint32 *FixedPixels = Pixels;
        for (int32 Y = 0;
             Y < Header->Height;
             Y++)
        {
            for (int32 X = 0;
                 X < Header->Width;
                 X++)
            {
                *FixedPixels = (
                    (((*FixedPixels & AlphaMask) >> AlphaShift.Index) << 24) |
                    (((*FixedPixels & RedMask) >> RedShift.Index) << 16) |
                    (((*FixedPixels & GreenMask) >> GreenShift.Index) << 8) |
                    (((*FixedPixels & BlueMask) >> BlueShift.Index) << 0)
                );
                FixedPixels++;
            }
        }
    }

    return Result;
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

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, 
           real32 RealX, real32 RealY,
           uint32 AlignX = 0, uint32 AlignY = 0)
{
    RealX -= AlignX;
    RealY -= AlignY;
    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = RoundReal32ToInt32(RealX + (real32)Bitmap->Width);
    int32 MaxY = RoundReal32ToInt32(RealY + (real32)Bitmap->Height);

    int32 SourceOffsetX = 0;
    int32 SourceOffsetY = 0;
    if (MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    if (MinY < 0)
    {
        SourceOffsetY = -MinY;
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

    uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
    SourceRow += -SourceOffsetY*Bitmap->Width + SourceOffsetX;
    uint8 *DestRow = ((uint8 *)Buffer->Memory +
                        MinX*Buffer->BytesPerPixel +
                        MinY*Buffer->Pitch);
    for (int32 BlitY = MinY;
         BlitY < MaxY;
         BlitY++)
    {
        uint32 *Dest = (uint32 *)DestRow;
        uint32 *Source = (uint32 *)SourceRow;
        for (int32 BlitX = MinX;
             BlitX < MaxX;
             BlitX++)
        {
            real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
            real32 SR = (real32)((*Source >> 16) & 0xFF);
            real32 SG = (real32)((*Source >> 8) & 0xFF);
            real32 SB = (real32)((*Source >> 0) & 0xFF);

            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >> 8) & 0xFF);
            real32 DB = (real32)((*Dest >> 0) & 0xFF);

            real32 R = (1.0f-A)*DR + A*SR;
            real32 G = (1.0f-A)*DG + A*SG;
            real32 B = (1.0f-A)*DB + A*SB;

            *Dest = (((uint32)(R + 0.5f) << 16) | 
                    ((uint32)(G + 0.5f) << 8) |
                    ((uint32)(B + 0.5f) << 0));

            Dest++;
            Source++;
        }

        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}

internal void
InitializePlayer(entity *Entity)
{
    Entity->Exists = true;
    Entity->P.AbsTileX = 1;
    Entity->P.AbsTileY = 1;
    Entity->P.Offset.X = 0.0f;
    Entity->P.Offset.Y = 0.0f;
    Entity->Height = 1.4f;
    Entity->Width = 0.75f*Entity->Height;
}

internal void
MovePlayer(game_state* GameState, entity *Entity, real32 dt, v2 ddP)
{
    tile_map *TileMap = GameState->World->TileMap;

    tile_map_position OldPlayerP = Entity->P;
    tile_map_position NewPlayerP = Entity->P;

    real32 ddPLength = LengthSq(ddP);
    if (ddPLength > 1.0f)
    {
        ddP *= (1.0f / SquareRoot(ddPLength));
    }

    if ((ddP.X != 0.0f) && (ddP.Y != 0.0f))
    {
        ddP *= 0.70710f;
    }

    real32 PlayerSpeed = 30.0f;
    ddP *= PlayerSpeed;
    ddP += -1.5f*Entity->dP;

    v2 PlayerDelta = (0.5f*ddP*Square(dt) +
                      Entity->dP*dt);
    NewPlayerP.Offset += PlayerDelta;
    Entity->dP = ddP*dt + Entity->dP;
    NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

#if 1
    tile_map_position NewPlayerLeft = NewPlayerP;
    NewPlayerLeft.Offset.X -= 0.5f*Entity->Width;
    NewPlayerLeft = RecanonicalizePosition(TileMap, NewPlayerLeft);
    tile_map_position NewPlayerRight = NewPlayerP;
    NewPlayerRight.Offset.X += 0.5f*Entity->Width;
    NewPlayerRight = RecanonicalizePosition(TileMap, NewPlayerRight);

    tile_map_position ColP = {};
    bool32 Collided = false;

    if (!IsTileMapPointEmpty(TileMap, NewPlayerP))
    {
        Collided = true;
        ColP = NewPlayerP;
    }
    if (!IsTileMapPointEmpty(TileMap, NewPlayerLeft))
    {
        Collided = true;
        ColP = NewPlayerLeft;
    }
    if (!IsTileMapPointEmpty(TileMap, NewPlayerRight))
    {
        Collided = true;
        ColP = NewPlayerRight;
    }

    if (Collided)
    {
        v2 r = {0, 0};
        if (ColP.AbsTileX > Entity->P.AbsTileX)
        {
            r = {-1, 0};
        }
        if (ColP.AbsTileX < Entity->P.AbsTileX)
        {
            r = {1, 0};
        }
        if (ColP.AbsTileY > Entity->P.AbsTileY)
        {
            r = {0, -1};
        }
        if (ColP.AbsTileY < Entity->P.AbsTileY)
        {
            r = {0, 1};
        }

        Entity->dP = Entity->dP - 2*Inner(Entity->dP, r)*r;
    }
    else
    {
        Entity->P = NewPlayerP;
    }
#else
    uint32 MinTileX = Minimum(OldPlayer.AbsTileX, NewPlayer.AbsTileX);
    uint32 MinTileY = Minimum(OldPlayer.AbsTileY, NewPlayer.AbsTileY);
    uint32 OnePastMaxTileX = Maximum(OldPlayer.AbsTileX, NewPlayer.AbsTileX) + 1;
    uint32 OnePastMaxTileY = Maximum(OldPlayer.AbsTileY, NewPlayer.AbsTileY) + 1;

    uint32 AbsTileZ = Entity->P.AbsTileZ;
    real32 tMin = 1.0f;
    for (uint32 AbsTileY = MinTileY;
         AbsTileY != OnePastMaxTileY;
         AbsTileY++)
    {
        for (uint32 AbsTileX = MinTileX;
             AbsTileX != OnePastMaxTileX;
             AbsTileX++)
        {
            tile_map_position TestTileP = CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ);
            uint32 TileValue = GetTileValue(TileMap, TestTileP);
            if (!IsTileValueEmpty(TileValue))
            {
                v2 MinCorner = -0.5f*v2{TileMap->TileSideInMeters, TileMap->TileSideInMeters};
                v2 MaxCorner = 0.5f*v2{TileMap->TileSideInMeters, TileMap->TileSideInMeters};

                tile_map_difference RelNewPlayerP = Subtract(TileMap, &TestTileP, &NewPlayerP);
                v2 Rel = RelNewPlayerP.dXY;
                real32 tResult = (WallX - RelNewPlayerP.X)/ PlayerDelta.X;
                TestWall(MinCorner.X, RelNewPlayerP.X, MinCorner.Y, RelNewPlayerP.Y);
            }
        }
    }
#endif

    if (!IsOnSameTile(OldPlayerP, NewPlayerP))
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
    Entity->P = NewPlayerP;

    if (AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y))
    {
        if (Entity->dP.X > 0)
        {
            Entity->FacingDirection = 0;
        }
        else
        {
            Entity->FacingDirection = 2;
        }
    }
    else if (AbsoluteValue(Entity->dP.X) < AbsoluteValue(Entity->dP.Y))
    {
        if (Entity->dP.Y > 0)
        {
            Entity->FacingDirection = 1;
        }
        else
        {
            Entity->FacingDirection = 3;
        }
    }
}

internal entity *
GetEntity(game_state *GameState, uint32 Index)
{
    entity *Entity = 0;

    if ((Index > 0) && (Index < ArrayCount(GameState->Entities)))
    {
        Entity = &GameState->Entities[Index];
    }

    return Entity;
}

internal uint32
AddEntity(game_state *GameState)
{
    uint32 EntityIndex = GameState->EntityCount++;
    entity *Entity = &GameState->Entities[EntityIndex];
    Entity = {};

    return EntityIndex;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        // NOTE: reserve entity slot 0 as null
        AddEntity(GameState);

        GameState->Backdrop = 
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");

        hero_bitmaps *Bitmap;

        Bitmap = GameState->HeroBitmaps;
        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;

        Bitmap++;
        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;

        Bitmap++;
        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;

        Bitmap++;
        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;

        GameState->CameraP.AbsTileX = 17/2;
        GameState->CameraP.AbsTileY = 9/2;
        
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

    for (int ControllerIndex = 0;
         ControllerIndex < ArrayCount(Input->Controllers);
         ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        entity *ControllingEntity = GetEntity(GameState,
                                              GameState->PlayerIndexForController[ControllerIndex]);
        if (ControllingEntity)
        {
            if (Controller->IsAnalog)
            {
                // NOTE: Use analog movement tuning
            }
            else
            {
                v2 ddP = {};

                if (Controller->MoveUp.EndedDown)
                {
                    ddP.Y = 1.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    ddP.Y = -1.0f;
                }
                if (Controller->MoveRight.EndedDown)
                {
                    ddP.X = 1.0f;
                }
                if (Controller->MoveLeft.EndedDown)
                {
                    ddP.X = -1.0f;
                }

                MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddP);
            }
        }
        else 
        {
            if (Controller->Start.EndedDown)
            {
                uint32 EntityIndex = AddEntity(GameState);
                ControllingEntity = GetEntity(GameState, EntityIndex);
                GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
                InitializePlayer(ControllingEntity);
                GameState->CameraFollowingEntityIndex = EntityIndex;
            }
        }
    }

    entity *CameraFollowingEntity = GetEntity(GameState, GameState->CameraFollowingEntityIndex);
    if (CameraFollowingEntity)
    {
        GameState->CameraP.AbsTileZ = CameraFollowingEntity->P.AbsTileZ;

        tile_map_difference Diff = Subtract(TileMap, &CameraFollowingEntity->P, &GameState->CameraP);
        if (Diff.dXY.X > (9.0f*TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileX += 17;
        }
        if (Diff.dXY.X < -(9.0f*TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileX -= 17;
        }
        if (Diff.dXY.Y > (5.0f*TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileY += 9;
        }
        if (Diff.dXY.Y < -(5.0f*TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileY -= 9;
        }
    }

    DrawBitmap(Buffer, &GameState->Backdrop, 0.0f, 0.0f);

    v2 ScreenCenter;
    ScreenCenter.X = 0.5f*(real32)Buffer->Width;
    ScreenCenter.Y = 0.5f*(real32)Buffer->Height;

    for (int32 RelRow = -10;
         RelRow < 10;
         RelRow++)
    {
        for (int32 RelColumn = -20;
             RelColumn < 20;
             RelColumn++)
        {
            uint32 Row = GameState->CameraP.AbsTileY + RelRow;
            uint32 Column = GameState->CameraP.AbsTileX + RelColumn;
            uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
            real32 Gray = 0.5f;
            if (TileID > 1)
            {
                if (TileID == 2)
                {
                    Gray = 1.0f;
                }
                if (TileID > 2)
                {
                    Gray = 0.25f;
                }
                if (Column == GameState->CameraP.AbsTileX && Row == GameState->CameraP.AbsTileY)
                {
                    Gray = 0.0f;
                }

                v2 TileSide = {0.5f*TileMap->TileSideInPixels, 0.5f*TileMap->TileSideInPixels};
                v2 Cen = {ScreenCenter.X - TileMap->MetersToPixels*GameState->CameraP.Offset.X + ((real32)RelColumn)*TileMap->TileSideInPixels,
                          ScreenCenter.Y + TileMap->MetersToPixels*GameState->CameraP.Offset.Y - ((real32)RelRow)*TileMap->TileSideInPixels};
                v2 Min = Cen - TileSide;
                v2 Max = Cen + TileSide;
                DrawRectangle(Buffer, Min, Max,
                              Gray, Gray, Gray);
            }
        }
    }

    entity *Entity = GameState->Entities;
    for (uint32 EntityIndex = 0;
         EntityIndex < GameState->EntityCount;
         EntityIndex++, Entity++)
    {
        if (Entity->Exists)
        {
            tile_map_difference Diff = Subtract(TileMap, &Entity->P, &GameState->CameraP);

            real32 PlayerR = 1.0f;
            real32 PlayerG = 1.0f;
            real32 PlayerB = 0.0f;
            v2 PlayerGroundPoint;
            PlayerGroundPoint.X = ScreenCenter.X + TileMap->MetersToPixels*Diff.dXY.X;
            PlayerGroundPoint.Y = ScreenCenter.Y - TileMap->MetersToPixels*Diff.dXY.Y;
            v2 PlayerLeftTop = {PlayerGroundPoint.X - TileMap->MetersToPixels*0.5f*Entity->Width,
                                    PlayerGroundPoint.Y - TileMap->MetersToPixels*Entity->Height};
            v2 PlayerWidthHeight = {Entity->Width, Entity->Height};
            DrawRectangle(Buffer, 
                          PlayerLeftTop, 
                          PlayerLeftTop + TileMap->MetersToPixels*PlayerWidthHeight,
                          PlayerR, PlayerG, PlayerB);

            hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
            DrawBitmap(Buffer, &HeroBitmaps->Torso, 
                       PlayerGroundPoint.X, PlayerGroundPoint.Y,
                       HeroBitmaps->AlignX, HeroBitmaps->AlignY);
            DrawBitmap(Buffer, &HeroBitmaps->Cape, 
                       PlayerGroundPoint.X, PlayerGroundPoint.Y,
                       HeroBitmaps->AlignX, HeroBitmaps->AlignY);
            DrawBitmap(Buffer, &HeroBitmaps->Head, 
                       PlayerGroundPoint.X, PlayerGroundPoint.Y,
                       HeroBitmaps->AlignX, HeroBitmaps->AlignY);
        }
    }
}
