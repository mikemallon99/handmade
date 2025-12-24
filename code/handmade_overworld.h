#ifndef HANDMADE_OVERWORLD_H
#define HANDMADE_OVERWORLD_H

#include "handmade_sprite.h"

// Character-based map definitions
// Character mappings:
//   '_' = Floor
//   'W' = Wall bottom left
//   'T' = Wall top middle
//   'L' = Wall top left
//   'M' = Wall bottom middle
//   'R' = Wall top right
//   'B' = Wall bottom middle (visual variant)
//   'K' = Black floor
//   'E' = Entrance
//   'U' = Bush

// Play area = 16 x 11

global_variable char HardcodedMap[11][17] = {
    "BBBBBBBB__BBBBBB",
    "BBBBEBBB__BBBBBB",
    "BBBB______BBBBBB",
    "BBB_______BBBBBB",
    "BB________WBBBBB",
    "________________",
    "MR____________BB",
    "BB____________BB",
    "BB____________BB",
    "BBMMMMMMMMMMMMBB",
    "BBBBBBBBBBBBBBBB"
};

global_variable char HardcodedMap2[11][17] = {
    "BBUU_U_U_U_U_U_U",
    "BBUU_U_U_U_U_U_U",
    "BB______________",
    "BR____U_U_U_U___",
    "R___U_U_________",
    "____U_U_U_U_____",
    "R___U_U_________",
    "BR____U_U_U_U___",
    "BB______________",
    "BBUUUUUUUUUUUUUU",
    "BBUUUUUUUUUUUUUU"
};

global_variable char CaveMap[11][17] = {
    "BBBBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBBBB",
    "BBKKKKKKKKKKKKBB",
    "BBKKKKKKKKKKKKBB",
    "BBKKKKKKKKKKKKBB",
    "BBKKKKKKKKKKKKBB",
    "BBKKKKKKKKKKKKBB",
    "BBKKKKKKKKKKKKBB",
    "BBKKKKKKKKKKKKBB",
    "BBMMMMMKKMMMMMBB",
    "BBBBBBBKKBBBBBBB"
};

#endif
