#ifndef HANDMADE_MAP_H
#define HANDMADE_MAP_H

#include "handmade_sprite.h"

// Visual tile macros - scoped to this file only
// These map directly to overworld_tile_id enum values
// Format: T_<visual_char> where visual char represents the tile visually
#define T__ OW_Floor              // Floor (underscore for ground) - enum value T__
#define T_W OW_Wall_BotLeft       // Wall bottom left (W for wall) - enum value T_M
#define T_T OW_Wall_TopMid        // Wall top middle (T for top) - enum value T_R
#define T_L OW_Wall_TopLeft       // Wall top left (L for left) - enum value T_M
#define T_M OW_Wall_TopMid        // Wall top middle (M for middle) - enum value T_R
#define T_R OW_Wall_TopRight      // Wall top right (R for right) - enum value T_W
#define T_B OW_Wall_BotMid        // Wall bottom middle (B for bottom) - enum value T_R
#define T_K OW_Floor_Black        // Black floor (K for black) - enum value 3
#define T_E OW_Entrance           // Entrance (E for entrance) - enum value T__T__
#define T_U OW_Bush               // Bush (U for bush/underbrush) - enum value T__0

// Play area = T__T_W x T__T__

global_variable uint32 HardcodedMap[] = 
{
    T_B, T_B, T_B, T_B, T_B, T_B, T_B, T__,   T__, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
    T_B, T_B, T_B, T_B, T_E, T_B, T_B, T__,   T__, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
    T_B, T_B, T_B, T_B, T__, T__, T__, T__,   T__, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
    T_B, T_B, T_B, T__, T__, T__, T__, T__,   T__, T_B, T_B, T_B, T_B, T_B, T_B, T_B,

    T_B, T_B, T__, T__, T__, T__, T__, T__,   T__, T_W, T_B, T_B, T_B, T_B, T_B, T_B,
    T__, T__, T__, T__, T__, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T__, T__,
    T_M, T_R, T__, T__, T__, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T_B, T_B,
    T_B, T_B, T__, T__, T__, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T_B, T_B,

    T_B, T_B, T__, T__, T__, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T_B, T_B,
    T_B, T_B, T_M, T_M, T_M, T_M, T_M, T_M,   T_M, T_M, T_M, T_M, T_M, T_M, T_B, T_B,
    T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_B,   T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
};

global_variable uint32 HardcodedMap2[] = 
{
    T_B, T_B, T_U, T__, T_U, T__, T_U, T__,   T__, T_U, T__, T_U, T__, T_U, T__, T_U,
    T_B, T_B, T_U, T__, T_U, T__, T_U, T__,   T__, T_U, T__, T_U, T__, T_U, T__, T_U,
    T_B, T_B, T__, T__, T__, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T__, T__,
    T_B, T_R, T__, T__, T__, T__, T_U, T__,   T__, T_U, T__, T_U, T__, T_U, T__, T__,

    T_R, T__, T_U, T__, T_U, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T__, T__,
    T__, T__, T__, T__, T__, T__, T_U, T__,   T__, T_U, T__, T_U, T__, T_U, T__, T__,
    T_R, T__, T_U, T__, T_U, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T__, T__,
    T_B, T_R, T__, T__, T__, T__, T_U, T__,   T__, T_U, T__, T_U, T__, T_U, T__, T__,

    T_B, T_B, T__, T__, T__, T__, T__, T__,   T__, T__, T__, T__, T__, T__, T__, T__,
    T_B, T_B, T_U, T_U, T_U, T_U, T_U, T_U,   T_U, T_U, T_U, T_U, T_U, T_U, T_U, T_U,
    T_B, T_B, T_U, T_U, T_U, T_U, T_U, T_U,   T_U, T_U, T_U, T_U, T_U, T_U, T_U, T_U,
};

global_variable uint32 CaveMap[] = 
{
    T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_B,   T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
    T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_B,   T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,
    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,

    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,
    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,
    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,
    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,

    T_B, T_B, T_K, T_K, T_K, T_K, T_K, T_K,   T_K, T_K, T_K, T_K, T_K, T_K, T_B, T_B,
    T_B, T_B, T_M, T_M, T_M, T_M, T_M, T_K,   T_K, T_M, T_M, T_M, T_M, T_M, T_B, T_B,
    T_B, T_B, T_B, T_B, T_B, T_B, T_B, T_K,   T_K, T_B, T_B, T_B, T_B, T_B, T_B, T_B,
};

#endif
