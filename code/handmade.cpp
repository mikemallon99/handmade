#include "handmade.h"

#include "handmade_random.h"
#include "handmade_map.h"
#include "handmade_tile.cpp"
#include "handmade_entity.cpp"
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
    if (PlayerP.RoomIDX != HitboxP.RoomIDX ||
        PlayerP.RoomIDY != HitboxP.RoomIDY)
    {
        return false;
    }

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

internal bool32
IsEntityCollidingWithPlayer(entity *Entity, tile_map_position NewPlayerP)
{
    real32 HitboxWidth = 1.0f;
    real32 HitboxHeight = 1.0f;
    
    // Determine hitbox size and if entity should be checked
    if (Entity->Type == EntityType_Octorok || Entity->Type == EntityType_Moblin)
    {
        // Enemies - check if they have health
        if (Entity->Health == 0) return false;
        HitboxWidth = 1.0f;
        HitboxHeight = 1.0f;
    }
    else if (Entity->Type == EntityType_OctorokRock || Entity->Type == EntityType_MoblinArrow)
    {
        // Projectiles - check if active
        if (!Entity->IsActive) return false;
        HitboxWidth = 0.5f;
        HitboxHeight = 1.0f;
    }
    else
    {
        return false;
    }
    
    // Check collision
    return IsHitboxPointActive(NewPlayerP, Entity->P, HitboxWidth, HitboxHeight);
}

internal entity *
SpawnOctorokProjectile(game_state *GameState, tile_map_position SpawnPosition)
{
    tile_room *PlayerRoom = GetTileRoom(GameState->World->TileMap, GameState->PlayerP);
    entity *Projectile = GetNewEntity(PlayerRoom->Entities);
    if (Projectile)
    {
        Projectile->Type = EntityType_OctorokRock;
        Projectile->P = SpawnPosition;
        // TODO: Make projectiles just use direction
        Projectile->VelocityX = 0.0;
        Projectile->VelocityY = -5.0;
        Projectile->Direction = {0.0f, -1.0f};
        Projectile->IsActive = true;
        Projectile->IsProjectile = true;
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
UpdateOctorok(game_state *GameState, entity *Octorok)
{
    // New algo: go forward until hit a wall, then turn a random direction
    // Octorok position update
    real32 Speed = 0.05f;
    vector2 PositionDelta = Octorok->Direction * Speed;

    tile_map_position NewPosition = Octorok->P;
    NewPosition.Pos = Octorok->P.Pos + PositionDelta;
    tile_map_position NewPositionUp = NewPosition;
    NewPositionUp.Pos.Y += Octorok->Height;
    tile_map_position NewPositionRight = NewPosition;
    NewPositionRight.Pos.X += Octorok->Width;
    tile_map_position NewPositionUpRight = NewPosition;
    NewPositionUpRight.Pos.X += Octorok->Width;
    NewPositionUpRight.Pos.Y += Octorok->Height;

    tile_map *TileMap = GameState->World->TileMap;
    if (IsTileMapPointEmpty(TileMap, NewPosition) &&
        IsTileMapPointEmpty(TileMap, NewPositionUp) &&
        IsTileMapPointEmpty(TileMap, NewPositionRight) &&
        IsTileMapPointEmpty(TileMap, NewPositionUpRight))
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

        tile_map_position UpPosition = Octorok->P;
        UpPosition.Pos = Octorok->P.Pos + UpDir * Speed;
        tile_map_position DownPosition = Octorok->P;
        DownPosition.Pos = Octorok->P.Pos + DownDir * Speed;
        tile_map_position LeftPosition = Octorok->P;
        LeftPosition.Pos = Octorok->P.Pos + LeftDir * Speed;
        tile_map_position RightPosition = Octorok->P;
        RightPosition.Pos = Octorok->P.Pos + RightDir * Speed;

        uint32 DirArraySize = 0;
        vector2 *DirectionArray[4];
        if (IsTileMapPointEmpty(GameState->World->TileMap, UpPosition))
        {
            DirectionArray[DirArraySize++] = &UpDir;
        }
        if (IsTileMapPointEmpty(GameState->World->TileMap, DownPosition))
        {
            DirectionArray[DirArraySize++] = &DownDir;
        }
        if (IsTileMapPointEmpty(GameState->World->TileMap, LeftPosition))
        {
            DirectionArray[DirArraySize++] = &LeftDir;
        }
        if (IsTileMapPointEmpty(GameState->World->TileMap, RightPosition))
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
        SpawnOctorokProjectile(GameState, Octorok->P);
    }
}

internal void
UpdateOctorokProjectile(entity *Projectile)
{
    Projectile->P.Pos.X += Projectile->VelocityX/60.0f;
    Projectile->P.Pos.Y += Projectile->VelocityY/60.0f;

    if (Projectile->P.Pos.Y < 0)
    {
        Projectile->IsActive = false;
    }
}

internal entity *
SpawnMoblinProjectile(game_state *GameState, tile_map_position SpawnPosition, vector2 ArrowDirection)
{
    tile_room *PlayerRoom = GetTileRoom(GameState->World->TileMap, GameState->PlayerP);
    entity *Projectile = GetNewEntity(PlayerRoom->Entities);
    if (Projectile)
    {
        Projectile->Type = EntityType_MoblinArrow;
        Projectile->P = SpawnPosition;
        // TODO: Make projectiles just use direction
        Projectile->VelocityX = 0.0;
        Projectile->VelocityY = -5.0;
        Projectile->Direction = ArrowDirection;
        Projectile->IsActive = true;
        Projectile->IsProjectile = true;
    }
    
    return Projectile;
}

internal void
UpdateMoblin(game_state *GameState, entity *Moblin)
{
    // Moblin position update
    local_persist int32 YDirection = -1;
    if (Moblin->P.Pos.Y > 8)
    {
        YDirection = -1;
    }
    else if (Moblin->P.Pos.Y < 3)
    {
        YDirection = 1;
    }
    Moblin->Direction.X = 0.0f;
    Moblin->Direction.Y = (real32)YDirection;
    Moblin->P.Pos.Y += (real32)YDirection * 0.05f;

    // Moblin fire projectile
    if (GameState->FrameCounter % Moblin->FireFrequency == 0)
    {
        SpawnMoblinProjectile(GameState, Moblin->P, Moblin->Direction);
    }
}

internal void
UpdateMoblinProjectile(entity *Projectile)
{
    Projectile->P.Pos.X += Projectile->VelocityX/60.0f;
    Projectile->P.Pos.Y += Projectile->VelocityY/60.0f;

    if (Projectile->P.Pos.Y < 0)
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
        // Update projectile if active
        if (Entity->IsActive)
        {
            UpdateOctorokProjectile(Entity);
        }
    }
    else if (Entity->Type == EntityType_MoblinArrow)
    {
        // Update projectile if active
        if (Entity->IsActive)
        {
            UpdateMoblinProjectile(Entity);
        }
    }
}

internal void
DrawOctorok(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
            real32 PlayAreaY, uint32 CameraTileX, entity *Octorok, octorok_sprites *OctorokSprites)
{
    real32 OctoOriginX = TileMap->TileSideInPixels*(Octorok->P.Pos.X - (real32)CameraTileX);
    real32 OctoOriginY = PlayAreaY - TileMap->TileSideInPixels*(Octorok->P.Pos.Y - 11.0f);
    real32 OctoScreenX = OctoOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 OctoScreenY = OctoOriginY - TileMap->TileSideInPixels*1.0f;
    // Origin
    DrawRectangle(Buffer, OctoOriginX, OctoOriginY-2, OctoOriginX+2, OctoOriginY, 
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
DrawOctorokProjectile(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
                      real32 PlayAreaY, uint32 CameraTileX, entity *Projectile, octorok_sprites *OctorokSprites)
{
    real32 ProjOriginX = TileMap->TileSideInPixels*(Projectile->P.Pos.X - (real32)CameraTileX);
    real32 ProjOriginY = PlayAreaY - TileMap->TileSideInPixels*(Projectile->P.Pos.Y - 11.0f);
    real32 ProjScreenX = ProjOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ProjScreenY = ProjOriginY - TileMap->TileSideInPixels*1.0f;
    DrawBMPTile(&OctorokSprites->Projectile, Buffer, ProjScreenX, ProjScreenY);
}

internal void
DrawOldMan(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *OldMan, npc_sprites *NPCSprites)
{
    real32 OldManOriginX = TileMap->TileSideInPixels*(OldMan->P.Pos.X - (real32)CameraTileX);
    real32 OldManOriginY = PlayAreaY - TileMap->TileSideInPixels*(OldMan->P.Pos.Y - 11.0f);
    real32 OldManScreenX = OldManOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 OldManScreenY = OldManOriginY - TileMap->TileSideInPixels*1.0f;
    
    // NOTE: Old man is not animated
    // uint32 OldManSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&NPCSprites->OldMan[1], Buffer, OldManScreenX, OldManScreenY);
}

internal void
DrawFire(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *Fire, npc_sprites *NPCSprites)
{
    real32 FireOriginX = TileMap->TileSideInPixels*(Fire->P.Pos.X - (real32)CameraTileX);
    real32 FireOriginY = PlayAreaY - TileMap->TileSideInPixels*(Fire->P.Pos.Y - 11.0f);
    real32 FireScreenX = FireOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 FireScreenY = FireOriginY - TileMap->TileSideInPixels*1.0f;
    
    // Animate between two sprites (idle animation)
    uint32 FireSpriteIndex = (GameState->FrameCounter / 30) % 2;
    DrawBMPTile(&NPCSprites->Fire[FireSpriteIndex], Buffer, FireScreenX, FireScreenY);
}

internal void
DrawMoblin(game_state *GameState, game_offscreen_buffer *Buffer, tile_map *TileMap,
           real32 PlayAreaY, uint32 CameraTileX, entity *Moblin, moblin_sprites *MoblinSprites)
{
    real32 MoblinOriginX = TileMap->TileSideInPixels*(Moblin->P.Pos.X - (real32)CameraTileX);
    real32 MoblinOriginY = PlayAreaY - TileMap->TileSideInPixels*(Moblin->P.Pos.Y - 11.0f);
    real32 MoblinScreenX = MoblinOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 MoblinScreenY = MoblinOriginY - TileMap->TileSideInPixels*1.0f;
    // Origin
    DrawRectangle(Buffer, MoblinOriginX, MoblinOriginY-2, MoblinOriginX+2, MoblinOriginY, 
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
    real32 ProjOriginX = TileMap->TileSideInPixels*(Projectile->P.Pos.X - (real32)CameraTileX);
    real32 ProjOriginY = PlayAreaY - TileMap->TileSideInPixels*(Projectile->P.Pos.Y - 11.0f);
    real32 ProjScreenX = ProjOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 ProjScreenY = ProjOriginY - TileMap->TileSideInPixels*1.0f;

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
    real32 BoomerangOriginX = TileMap->TileSideInPixels*(GameState->BoomerangP.Pos.X - (real32)CameraTileX);
    real32 BoomerangOriginY = PlayAreaY - TileMap->TileSideInPixels*(GameState->BoomerangP.Pos.Y - 11.0f);
    real32 BoomerangScreenX = BoomerangOriginX;
    // NOTE: Dont forget that screen Y and tile Y are flipped. For screen, increasing Y goes downward. For Tiles, increasing Y goes up
    real32 BoomerangScreenY = BoomerangOriginY - TileMap->TileSideInPixels*0.5f;
    
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
        tile_room *TileRoom = LoadOverworldRoom(&GameState->WorldArena, TileMap, HardcodedMap, SpawnRoomX, SpawnRoomY);
        GameState->RoomDebug1 = TileRoom;
        GameState->RoomDebug2 = LoadOverworldRoom(&GameState->WorldArena, TileMap, HardcodedMap2, SpawnRoomX+1, SpawnRoomY);

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

        GameState->PlayerHealth = 6;
        GameState->MaxHealth = 6;
        GameState->PlayerP.RoomIDX = SpawnRoomX;
        GameState->PlayerP.RoomIDY = SpawnRoomY;
        GameState->PlayerP.Pos.X = 5.0f;
        GameState->PlayerP.Pos.Y = 5.0f;
        GameState->PlayerDirection = {0.0f, -1.0f}; // FRONT
        GameState->BoomerangMaxDistance = 5.0f;
        GameState->BoomerangSpeed = 8.0f;

        // Add Octorok1 to entity array
        entity *Entities = (entity *)GameState->RoomDebug1->Entities;
        GameState->Octorok1 = GetNewEntity(Entities);
        GameState->Octorok1->Type = EntityType_Octorok;
        GameState->Octorok1->Health = 3;
        GameState->Octorok1->P.Pos.X = 8;
        GameState->Octorok1->P.Pos.Y = 5.0f;
        GameState->Octorok1->P.RoomIDX = SpawnRoomX;
        GameState->Octorok1->P.RoomIDY = SpawnRoomY;
        GameState->Octorok1->Width = 1.0f;
        GameState->Octorok1->Height = 1.0f;
        GameState->Octorok1->Direction.X = -1.0f;
        GameState->Octorok1->Direction.Y = 0.0f;
        GameState->Octorok1->InvincibilityTimer = 0;
        GameState->Octorok1->IFramesFlicker = false;
        GameState->Octorok1->FireFrequency = 60;

        // Add Octorok2 to entity array
        GameState->Octorok2 = GetNewEntity(Entities);
        GameState->Octorok2->Type = EntityType_Octorok;
        GameState->Octorok2->Health = 3;
        GameState->Octorok2->P.Pos.X = 9;
        GameState->Octorok2->P.Pos.Y = 5.0f;
        GameState->Octorok2->P.RoomIDX = SpawnRoomX;
        GameState->Octorok2->P.RoomIDY = SpawnRoomY;
        GameState->Octorok2->Width = 1.0f;
        GameState->Octorok2->Height = 1.0f;
        GameState->Octorok2->Direction.X = -1.0f;
        GameState->Octorok2->Direction.Y = 0.0f;
        GameState->Octorok2->InvincibilityTimer = 0;
        GameState->Octorok2->IFramesFlicker = false;
        GameState->Octorok2->FireFrequency = 60;

        // Add Octorok3 to entity array
        GameState->Octorok3 = GetNewEntity(Entities);
        GameState->Octorok3->Type = EntityType_Octorok;
        GameState->Octorok3->Health = 3;
        GameState->Octorok3->P.Pos.X = 4;
        GameState->Octorok3->P.Pos.Y = 5.0f;
        GameState->Octorok3->P.RoomIDX = SpawnRoomX;
        GameState->Octorok3->P.RoomIDY = SpawnRoomY;
        GameState->Octorok3->Width = 1.0f;
        GameState->Octorok3->Height = 1.0f;
        GameState->Octorok3->Direction.X = -1.0f;

        entity *OldMan = GetNewEntity(CaveRoom->Entities);
        if (OldMan)
        {
            OldMan->Type = EntityType_OldMan;
            OldMan->IsActive = true;
            OldMan->P.Pos.X = 7.5f;
            OldMan->P.Pos.Y = 5.0f;
            OldMan->P.RoomIDX = CaveRoomX;
            OldMan->P.RoomIDY = CaveRoomY;
            OldMan->Width = 1.0f;
            OldMan->Height = 1.0f;
            OldMan->Health = 0; // NPCs don't have health
            OldMan->IsProjectile = false;
        }

        entity *Fire1 = GetNewEntity(CaveRoom->Entities);
        if (Fire1)
        {
            Fire1->Type = EntityType_Fire;
            Fire1->IsActive = true;
            Fire1->P.Pos.X = 5.5f;
            Fire1->P.Pos.Y = 5.0f;
            Fire1->P.RoomIDX = CaveRoomX;
            Fire1->P.RoomIDY = CaveRoomY;
            Fire1->Width = 1.0f;
            Fire1->Height = 1.0f;
            Fire1->Health = 0; // NPCs don't have health
            Fire1->IsProjectile = false;
        }

        entity *Fire2 = GetNewEntity(CaveRoom->Entities);
        if (Fire2)
        {
            Fire2->Type = EntityType_Fire;
            Fire2->IsActive = true;
            Fire2->P.Pos.X = 9.5f;
            Fire2->P.Pos.Y = 5.0f;
            Fire2->P.RoomIDX = CaveRoomX;
            Fire2->P.RoomIDY = CaveRoomY;
            Fire2->Width = 1.0f;
            Fire2->Height = 1.0f;
            Fire2->Health = 0; // NPCs don't have health
            Fire2->IsProjectile = false;
        }

        GameState->Octorok3->Direction.Y = 0.0f;
        GameState->Octorok3->InvincibilityTimer = 0;
        GameState->Octorok3->IFramesFlicker = false;
        GameState->Octorok3->FireFrequency = 60;

        // Add Moblin to entity array
        GameState->Moblin1 = GetNewEntity(Entities);
        GameState->Moblin1->Type = EntityType_Moblin;
        GameState->Moblin1->Health = 3;
        GameState->Moblin1->P.Pos.X = 6;
        GameState->Moblin1->P.Pos.Y = 5.0f;
        GameState->Moblin1->P.RoomIDX = SpawnRoomX;
        GameState->Moblin1->P.RoomIDY = SpawnRoomY;
        GameState->Moblin1->Width = 1.0f;
        GameState->Moblin1->Height = 1.0f;
        GameState->Moblin1->Direction.X = -1.0f;
        GameState->Moblin1->Direction.Y = 0.0f;
        GameState->Moblin1->InvincibilityTimer = 0;
        GameState->Moblin1->IFramesFlicker = false;
        GameState->Moblin1->FireFrequency = 60;

        // Add Moblin to entity array
        GameState->Moblin2 = GetNewEntity(Entities);
        GameState->Moblin2->Type = EntityType_Moblin;
        GameState->Moblin2->Health = 3;
        GameState->Moblin2->P.Pos.X = 7;
        GameState->Moblin2->P.Pos.Y = 5.0f;
        GameState->Moblin2->P.RoomIDX = SpawnRoomX;
        GameState->Moblin2->P.RoomIDY = SpawnRoomY;
        GameState->Moblin2->Width = 1.0f;
        GameState->Moblin2->Height = 1.0f;
        GameState->Moblin2->Direction.X = -1.0f;
        GameState->Moblin2->Direction.Y = 0.0f;
        GameState->Moblin2->InvincibilityTimer = 0;
        GameState->Moblin2->IFramesFlicker = false;
        GameState->Moblin2->FireFrequency = 60;

        // Add Octorok1 to entity array
        Entities = (entity *)GameState->RoomDebug2->Entities;
        GameState->Octorok4 = GetNewEntity(Entities);
        GameState->Octorok4->Type = EntityType_Octorok;
        GameState->Octorok4->Health = 3;
        GameState->Octorok4->P.Pos.X = 8;
        GameState->Octorok4->P.Pos.Y = 5.0f;
        GameState->Octorok4->P.RoomIDX = SpawnRoomX+1;
        GameState->Octorok4->P.RoomIDY = SpawnRoomY;
        GameState->Octorok4->Width = 1.0f;
        GameState->Octorok4->Height = 1.0f;
        GameState->Octorok4->Direction.X = -1.0f;
        GameState->Octorok4->Direction.Y = 0.0f;
        GameState->Octorok4->InvincibilityTimer = 0;
        GameState->Octorok4->IFramesFlicker = false;
        GameState->Octorok4->FireFrequency = 60;

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

    real32 PlayerWidth = 0.75f;
    real32 PlayerHeight = 1.0f;

    real32 PlayerSpeed = 0.0f;
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
                GameState->PlayerDirection = {0.0f, 1.0f}; // BACK
                PlayerSpeed = 5.0f;
            }
            if (Controller->MoveDown.EndedDown)
            {
                GameState->PlayerDirection = {0.0f, -1.0f}; // FRONT
                PlayerSpeed = 5.0f;
            }
            if (Controller->MoveLeft.EndedDown)
            {
                GameState->PlayerDirection = {-1.0f, 0.0f}; // LEFT
                PlayerSpeed = 5.0f;
            }
            if (Controller->MoveRight.EndedDown)
            {
                GameState->PlayerDirection = {1.0f, 0.0f}; // RIGHT
                PlayerSpeed = 5.0f;
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
        GameState->BoomerangP.RoomIDX = GameState->PlayerP.RoomIDX;
        GameState->BoomerangP.RoomIDY = GameState->PlayerP.RoomIDY;

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

    // Player movement stuff
    // NOTE: Translate origin to the center of player cuz of legacy calculations
    tile_map_position NewPlayerOrigin = GameState->PlayerP;
    tile_map_position NewPlayerP = NewPlayerOrigin;
    NewPlayerP.Pos.X += 0.5f;

    if (!GameState->PlayerUsingSword && !GameState->PlayerUsingBoomerang)
    {
        vector2 MovementDelta = Input->dtForFrame * PlayerSpeed * GameState->PlayerDirection;
        NewPlayerP.Pos.X += MovementDelta.X;
        NewPlayerP.Pos.Y += MovementDelta.Y;
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
    GameState->PlayerP = NewPlayerOrigin;

    // Update all entities
    tile_room *PlayerRoom = GetTileRoom(TileMap, GameState->PlayerP);
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
        for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
        {
            entity *Entity = &PlayerRoom->Entities[EntityIndex];
            if (Entity->IsActive && IsEntityCollidingWithPlayer(Entity, NewPlayerP))
            {
                // Skip if player blocks the projectile
                if (Entity->IsProjectile && !GameState->PlayerUsingSword &&
                    IsDirectionOpposite(Vector2ToDirectionEnum(&GameState->PlayerDirection), Vector2ToDirectionEnum(&Entity->Direction)))
                {
                    // Do nothing
                }
                else
                {
                    GameState->InvincibilityTimer = 60;
                    GameState->PlayerHealth -= 1;
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
                    bool32 IsEnemyHit = IsHitboxPointActive(GameState->SwordPoint, 
                                                            Entity->P, 
                                                            1.0f, 1.0f);
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
    direction PlayerDir = Vector2ToDirectionEnum(&GameState->PlayerDirection);
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

        if (PlayerDir == FRONT)
        {
            LinkSprite = &GameState->LinkSprites.SwordFront[SwordSpriteIndex];
        }
        else if (PlayerDir == BACK)
        {
            LinkSprite = &GameState->LinkSprites.SwordBack[SwordSpriteIndex];
        }
        else if (PlayerDir == LEFT)
        {
            LinkSprite = &GameState->LinkSprites.SwordLeft[SwordSpriteIndex];
        }
        else if (PlayerDir == RIGHT)
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
