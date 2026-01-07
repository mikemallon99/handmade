#ifndef HANDMADE_H
#define HANDMADE_H

#include <math.h>
#include <stdint.h>

#define internal static 
#define local_persist static
#define global_variable static

#define Pi32 3.1415926535898f

#define MAX_ENTITY_TWEENS 32

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef size_t memory_index;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

/*
  HANDMADE_INTERNAL
  0 - Build for public release
  1 - Build for developer only

  HANDMADE_SLOW
  0 - Not slow code allowed
  1 - Slow code allowed
 */

#if HANDMADE_SLOW
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression) 
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    // TODO: defines for max values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}


// NOTE: Services that the platform layer provides to the game layer

struct thread_context
{
    int Placeholder;
};

#if HANDMADE_INTERNAL
/* 
    IMPORTANT:
    these are NOT for shipping game, they are blocking
    and the write doesnt protect against lost data
*/
struct debug_read_file_result
{
    uint32 ContentsSize;
    void *Contents;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif


/*
    NOTE: Services that the game layer provides to the platform layer
    ( this may expand in the future, like sound on a different thread )
 */

struct game_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;
        };
    };
};

struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX, MouseY, MouseZ;
    real32 dtForFrame;

    // TODO: insert clock value here
    // 4 game controllers + 1 keyboard
    game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));

    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}


struct game_memory
{
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void *PermanentStorage; // REQUIRED to be cleared to 0 at startup

    uint64 TransientStorageSize;
    void *TransientStorage; // REQUIRED to be cleared to 0 at startup

    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;

    real32 GameUpdateHz;
};

// 4 things: timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);



//
//

#include "handmade_intrinsics.h"
#include "handmade_position.h"
#include "handmade_entity.h"
#include "handmade_tile.h"
#include "handmade_sprite.h"

struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;
};

internal void
InitializeArena(memory_arena *Arena, memory_index Size, uint8 *Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void *
PushSize_(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size)
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

struct world
{
    tile_map *TileMap;
};

struct game_state
{
    memory_arena WorldArena;
    uint32 FrameCounter;
    world *World;

    tile_map_position PlayerP;
    // NOTE: This is mostly for floor, may need another for hitting enemies
    real32 PlayerSpeed;
    area2d PlayerHitbox;
    area2d PlayerHitboxCache;
    int32 PlayerHealth;
    uint32 MaxHealth;
    uint32 KeyInventory;
    bool32 HasSword;
    bool32 PlayerUsingSword;
    int32 SwordUsageFrame;
    bool32 PlayerPickingUpThing;
    entity *PickUpEntity;
    int32 PickupFrame;
    int32 TotalPickupFrames;
    tile_map_position BoomerangP;
    vector2 BoomerangDirection;
    bool32 PlayerUsingBoomerang;
    int32 BoomerangUsageFrame;
    tile_map_position BoomerangStartP;
    real32 BoomerangMaxDistance;
    bool32 BoomerangReturning;
    real32 BoomerangSpeed;
    tile_map_position SwordPoint;
    vector2 PlayerDirection;
    uint32 InvincibilityTimer;
    bool32 IFramesFlicker;
    bool32 BlockPlayerInput;

    bmp_file Background;

    bmp_file LinkBMP;
    link_sprites LinkSprites;
    boomerang_sprites BoomerangSprites;
    bmp_file ItemBMP;
    item_sprites ItemSprites;
    uint32 WalkStep;

    bmp_file NPCBMP;
    npc_sprites NPCSprites;

    bmp_file OverworldBMP;
    overworld_tileset OverworldTileset;
    bmp_file DungeonBMP;
    dungeon_tileset DungeonTileset;
    text_tileset TextTileset;

    bmp_file HudBMP;
    hud_tileset HudTileset;

    bmp_file OWEnemiesBMP;

    octorok_sprites OctorokSprites;
    moblin_sprites MoblinSprites;

    // Animation stuff
    entity_tween_queue EntityTweenQueue;
    
    // Items
    entity *Sword;
    
    // Cave room text animation
    int32 CaveTextCharIndex;  // Current character index to display
};

#endif
