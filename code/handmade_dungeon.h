#ifndef HANDMADE_DUNGEON_H
#define HANDMADE_DUNGEON_H

#include "handmade_sprite.h"

// Character-based dungeon map definitions
// Character mappings:
//   '_' = Floor (DN_Floor)
//   'B' = Block (DN_Block)
//   'S' = Statue 1 (DN_Statue1)
//   's' = Statue 2 (DN_Statue2)
//   'K' = Black floor (DN_Black)
//   'G' = Gravel (DN_Gravel)
//   'W' = Water (DN_Water)
//   '^' = Stairs (DN_Stairs)
//   '#' = Grey wall (DN_GreyWall)
//   'H' = Grey ladder (DN_GreyLadder)

// Play area = 12 x 7 (inside the border sprite)
// Doors are part of the border, not the tile map

global_variable char DungeonRoom1[7][13] = {
    "____________",
    "____________",
    "____________",
    "____________",
    "____________",
    "____________",
    "____________"
};

#endif
