#ifndef HANDMADE_H
#define HANDMADE_H

#include <math.h>
#include <stdint.h>

#include "handmade_platform.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

//
//
//

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
};

// 4 things: timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);



//
//

#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_tile.h"

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

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32 *Pixels;
};

struct hero_bitmaps
{
    uint32 AlignX;
    uint32 AlignY;

    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct entity
{
    bool32 Exists;
    tile_map_position P;
    v2 dP;
    uint32 FacingDirection;
    real32 Width, Height;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    // TODO: splitscreen??
    tile_map_position CameraP;
    uint32 CameraFollowingEntityIndex;

    uint32 PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];
    uint32 EntityCount;
    entity Entities[256];

    loaded_bitmap Backdrop;
    hero_bitmaps HeroBitmaps[4];

    uint32 HeroFacingDirection;
};

#endif
